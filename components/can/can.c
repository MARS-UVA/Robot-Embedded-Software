#include "can.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "esp_check.h"          // ESP_RETURN_ON_FALSE / ESP_RETURN_ON_ERROR / ESP_GOTO_ON_*
#include "esp_log.h"
#include "esp_twai.h"           // twai_node_enable, _disable, _delete, _transmit, ...
#include "esp_twai_onchip.h"    // twai_new_node_onchip + its config struct

// ********** constants **********

static const char *TAG = "CAN";

#define CAN_RX_TASK_STACK       4096    // bytes; handlers run on this stack
#define CAN_RX_TASK_PRIORITY    10      // above app_main (1) so incoming frames are handled promptly
#define CAN_TX_SLOT_MAX         (CAN_TX_QUEUE_MAX + 1)  // +1: one frame can be in the hardware while the queue is full

// ********** types **********

// One transmit buffer. The driver keeps a pointer to the frame until it is sent,
// so the frame must live here instead of on the caller's stack.
typedef struct {
    twai_frame_t frame;                     // must stay first: on_tx_done casts the frame pointer back to its slot
    uint8_t      data[CAN_MAX_DATA_LEN];    // payload bytes, frame.buffer points here
} tx_slot_t;

// One registration from CAN_register_handler()
typedef struct {
    uint32_t      id;
    uint32_t      mask;
    bool          extended;
    CAN_handler_t handler;
    void         *ctx;
} handler_entry_t;

// ********** state **********

// Node
static twai_node_handle_t s_node = NULL;
static bool s_started = false;

// TX slot pool, statically allocated. s_tx_free holds pointers to slots not owned by the driver.
static tx_slot_t s_tx_slots[CAN_TX_SLOT_MAX];
static StaticQueue_t s_tx_free_buf;                                 // queue's bookkeeping
static uint8_t s_tx_free_storage[CAN_TX_SLOT_MAX * sizeof(tx_slot_t *)];   // queue's item storage
static QueueHandle_t s_tx_free = NULL;
static uint32_t s_tx_slot_count = 0;        // slots in use: tx_queue_depth + 1

// RX: the ISR copies frames into the queue, the rx task dispatches them
static QueueHandle_t s_rx_queue = NULL;
static TaskHandle_t s_rx_task = NULL;

// Handlers. Only modified before CAN_start(), so rx_task can read it without locking.
static handler_entry_t s_handlers[CAN_MAX_HANDLERS];
static uint32_t s_handler_count = 0;

// Counters, written from interrupts and the rx task, read by CAN_get_stats()
static volatile CAN_stats_t s_stats;

// ********** interrupt callbacks **********

// Runs in interrupt context when the hardware finishes with a frame (sent or given up).
// Must be short and non-blocking: count the result and hand the slot back to the free list.
static bool on_tx_done(twai_node_handle_t handle, const twai_tx_done_event_data_t *edata, void *user_ctx)
{
    if (edata->is_tx_success) {
        s_stats.tx_success++;
    } else {
        s_stats.tx_failed++;
    }

    tx_slot_t *slot = (tx_slot_t *)edata->done_tx_frame;
    BaseType_t task_woken = pdFALSE;
    xQueueSendFromISR(s_tx_free, &slot, &task_woken);
    return task_woken == pdTRUE;    // tells the driver to switch to a task waiting for a free slot
}

// Runs in interrupt context when the hardware has received a frame.
// Must be short and non-blocking: copy the frame out of the driver and queue it for the rx task.
// Returning early is safe: the driver already removed the frame from the hardware before calling us.
static bool on_rx_done(twai_node_handle_t handle, const twai_rx_done_event_data_t *edata, void *user_ctx)
{
    CAN_frame_t frame = {0};

    // the driver writes the payload straight into frame.data
    twai_frame_t rx = {
        .buffer = frame.data,
        .buffer_len = sizeof(frame.data),
    };
    if (twai_node_receive_from_isr(handle, &rx) != ESP_OK) {
        return false;
    }

    // remote frames carry no data and aren't supported (the S3 can't filter them in hardware)
    if (rx.header.rtr) {
        return false;
    }

    frame.id = rx.header.id;
    frame.extended = rx.header.ide;
    // classic CAN: a DLC of 9-15 still means 8 bytes
    frame.len = (rx.header.dlc > CAN_MAX_DATA_LEN) ? CAN_MAX_DATA_LEN : rx.header.dlc;

    // the queue copies the whole frame, so the local variable can go away after this
    BaseType_t task_woken = pdFALSE;
    if (xQueueSendFromISR(s_rx_queue, &frame, &task_woken) == pdTRUE) {
        s_stats.rx_received++;
    } else {
        s_stats.rx_dropped++;       // rx task is falling behind
    }
    return task_woken == pdTRUE;    // tells the driver to switch to the rx task right away
}

// ********** internal helper functions **********

static uint32_t id_max(bool extended)
{
    return extended ? CAN_EXT_ID_MAX : CAN_STD_ID_MAX;
}

static bool handler_matches(const handler_entry_t *h, const CAN_frame_t *frame)
{
    return h->extended == frame->extended && (frame->id & h->mask) == h->id;
}

// Waits for received frames and delivers each one to every matching handler.
// Runs in task context, so handlers may block, log or send.
static void rx_task(void *arg)
{
    CAN_frame_t frame;
    while (1) {
        xQueueReceive(s_rx_queue, &frame, portMAX_DELAY);  // blocks until a frame arrives

        bool handled = false;
        for (uint32_t i = 0; i < s_handler_count; i++) {
            const handler_entry_t *h = &s_handlers[i];
            if (handler_matches(h, &frame)) {
                h->handler(&frame, h->ctx);
                handled = true;
            }
        }

        if (!handled) {
            s_stats.rx_unhandled++;
            ESP_LOGD(TAG, "unhandled rx id=0x%03lx", frame.id);
        }
    }
}

// Mark every slot as free. Only safe while the node is disabled (no interrupts using the slots).
static void tx_pool_reset(void)
{
    xQueueReset(s_tx_free);
    for (uint32_t i = 0; i < s_tx_slot_count; i++) {
        tx_slot_t *slot = &s_tx_slots[i];
        xQueueSend(s_tx_free, &slot, 0);
    }
}

// Release everything CAN_init() created. Safe to call when only some of it was created.
static void release_all(void)
{
    if (s_node != NULL) {
        twai_node_delete(s_node);
        s_node = NULL;
    }
    if (s_rx_task != NULL) {
        vTaskDelete(s_rx_task);
        s_rx_task = NULL;
    }
    if (s_rx_queue != NULL) {
        vQueueDelete(s_rx_queue);
        s_rx_queue = NULL;
    }
    if (s_tx_free != NULL) {
        vQueueDelete(s_tx_free);    // static queue: releases it, frees no memory
        s_tx_free = NULL;
    }
    s_tx_slot_count = 0;
    s_handler_count = 0;
}

// ********** public functions **********

esp_err_t CAN_init(const CAN_config_t *config)
{
    ESP_RETURN_ON_FALSE(config != NULL, ESP_ERR_INVALID_ARG, TAG, "config is NULL");
    ESP_RETURN_ON_FALSE(config->bitrate > 0, ESP_ERR_INVALID_ARG, TAG, "bitrate must be > 0");
    ESP_RETURN_ON_FALSE(config->tx_queue_depth > 0, ESP_ERR_INVALID_ARG, TAG, "tx_queue_depth must be > 0");
    ESP_RETURN_ON_FALSE(config->tx_queue_depth <= CAN_TX_QUEUE_MAX, ESP_ERR_INVALID_ARG, TAG, "tx_queue_depth must be <= %d", CAN_TX_QUEUE_MAX);
    ESP_RETURN_ON_FALSE(config->rx_queue_depth > 0, ESP_ERR_INVALID_ARG, TAG, "rx_queue_depth must be > 0");
    ESP_RETURN_ON_FALSE(s_node == NULL, ESP_ERR_INVALID_STATE, TAG, "node already initialized");

    esp_err_t ret = ESP_OK;     // ESP_GOTO_ON_* macros store the error here before jumping to cleanup

    twai_onchip_node_config_t node_cfg = {
        .io_cfg.tx = config->tx_gpio,
        .io_cfg.rx = config->rx_gpio,
        .io_cfg.quanta_clk_out = -1,
        .io_cfg.bus_off_indicator = -1,
        .bit_timing.bitrate = config->bitrate,
        .tx_queue_depth = config->tx_queue_depth,
        .flags.enable_self_test = config->self_test,
        .flags.enable_loopback = config->self_test,
        .fail_retry_cnt = -1,                   // retry a failed frame forever
    };

    // TX: one slot per driver queue entry, plus one for the frame currently in the hardware
    s_tx_slot_count = config->tx_queue_depth + 1;
    s_tx_free = xQueueCreateStatic(s_tx_slot_count, sizeof(tx_slot_t *), s_tx_free_storage, &s_tx_free_buf);
    ESP_GOTO_ON_FALSE(s_tx_free != NULL, ESP_ERR_NO_MEM, cleanup, TAG, "failed to create tx free list");
    tx_pool_reset();

    // RX: frames are copied whole into this queue, so it holds CAN_frame_t, not pointers
    s_rx_queue = xQueueCreate(config->rx_queue_depth, sizeof(CAN_frame_t));
    ESP_GOTO_ON_FALSE(s_rx_queue != NULL, ESP_ERR_NO_MEM, cleanup, TAG, "no memory for rx queue");
    ESP_GOTO_ON_FALSE(xTaskCreate(rx_task, "can_rx", CAN_RX_TASK_STACK, NULL, CAN_RX_TASK_PRIORITY, &s_rx_task) == pdPASS,
                      ESP_ERR_NO_MEM, cleanup, TAG, "failed to create rx task");

    // Node: the driver copies node_cfg and writes the new handle into s_node
    ESP_GOTO_ON_ERROR(twai_new_node_onchip(&node_cfg, &s_node), cleanup, TAG, "failed to create node");

    // callbacks can only be registered while the node is disabled
    twai_event_callbacks_t cbs = {
        .on_tx_done = on_tx_done,
        .on_rx_done = on_rx_done,
    };
    ESP_GOTO_ON_ERROR(twai_node_register_event_callbacks(s_node, &cbs, NULL), cleanup, TAG, "failed to register callbacks");

    s_stats = (CAN_stats_t){0};

    ESP_LOGI(TAG, "initialized: tx=%d rx=%d bitrate=%lu%s",
             config->tx_gpio, config->rx_gpio, config->bitrate,
             config->self_test ? " (self-test)" : "");
    return ESP_OK;

cleanup:
    release_all();
    return ret;
}

esp_err_t CAN_deinit(void)
{
    ESP_RETURN_ON_FALSE(s_node != NULL, ESP_ERR_INVALID_STATE, TAG, "not initialized");
    ESP_RETURN_ON_FALSE(!s_started, ESP_ERR_INVALID_STATE, TAG, "stop before deinit");

    // delete the node first and report failure: once it's gone, no interrupts can touch the queues
    ESP_RETURN_ON_ERROR(twai_node_delete(s_node), TAG, "delete failed");
    s_node = NULL;
    release_all();

    ESP_LOGI(TAG, "deinitialized");
    return ESP_OK;
}

esp_err_t CAN_start(void)
{
    ESP_RETURN_ON_FALSE(s_node != NULL, ESP_ERR_INVALID_STATE, TAG, "not initialized");
    ESP_RETURN_ON_FALSE(!s_started, ESP_ERR_INVALID_STATE, TAG, "already started");

    // frames left queued by a previous CAN_stop() are dropped, so reclaim their slots
    tx_pool_reset();
    ESP_RETURN_ON_ERROR(twai_node_enable(s_node), TAG, "enable failed");

    s_started = true;
    ESP_LOGI(TAG, "started");
    return ESP_OK;
}

esp_err_t CAN_stop(void)
{
    ESP_RETURN_ON_FALSE(s_node != NULL, ESP_ERR_INVALID_STATE, TAG, "not initialized");
    ESP_RETURN_ON_FALSE(s_started, ESP_ERR_INVALID_STATE, TAG, "not started");

    ESP_RETURN_ON_ERROR(twai_node_disable(s_node), TAG, "disable failed");

    s_started = false;
    ESP_LOGI(TAG, "stopped");
    return ESP_OK;
}

esp_err_t CAN_send(const CAN_frame_t *frame, int timeout_ms)
{
    ESP_RETURN_ON_FALSE(frame != NULL, ESP_ERR_INVALID_ARG, TAG, "frame is NULL");
    ESP_RETURN_ON_FALSE(frame->len <= CAN_MAX_DATA_LEN, ESP_ERR_INVALID_ARG, TAG, "len %u > %d", frame->len, CAN_MAX_DATA_LEN);
    ESP_RETURN_ON_FALSE(frame->id <= id_max(frame->extended), ESP_ERR_INVALID_ARG,
                        TAG, "id 0x%lx too large for %s ID", frame->id, frame->extended ? "extended" : "standard");
    ESP_RETURN_ON_FALSE(s_started, ESP_ERR_INVALID_STATE, TAG, "not started");

    // wait for a free slot (one frees up each time the hardware finishes a frame)
    TickType_t ticks = (timeout_ms < 0) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
    tx_slot_t *slot = NULL;
    ESP_RETURN_ON_FALSE(xQueueReceive(s_tx_free, &slot, ticks) == pdTRUE, ESP_ERR_TIMEOUT, TAG, "tx queue full");

    // copy the caller's frame into the slot, translating to the driver's format
    memcpy(slot->data, frame->data, frame->len);
    slot->frame = (twai_frame_t) {
        .header = {
            .id = frame->id,
            .ide = frame->extended,
            .dlc = frame->len,
        },
        .buffer = slot->data,
        .buffer_len = frame->len,
    };

    // timeout 0: there are never more slots than the driver queue holds, so it can't be full
    esp_err_t ret = twai_node_transmit(s_node, &slot->frame, 0);
    if (ret != ESP_OK) {
        // driver didn't take the frame, so the slot is still ours to give back
        xQueueSend(s_tx_free, &slot, 0);
        ESP_LOGE(TAG, "transmit failed: %s", esp_err_to_name(ret));
    }
    return ret;
}

esp_err_t CAN_register_handler(uint32_t id, uint32_t mask, bool extended, CAN_handler_t handler, void *ctx)
{
    ESP_RETURN_ON_FALSE(handler != NULL, ESP_ERR_INVALID_ARG, TAG, "handler is NULL");
    ESP_RETURN_ON_FALSE(id <= id_max(extended) && mask <= id_max(extended), ESP_ERR_INVALID_ARG, TAG, "id/mask too large");
    // a bit set in id but not in mask can never match, almost certainly a mistake
    ESP_RETURN_ON_FALSE((id & ~mask) == 0, ESP_ERR_INVALID_ARG, TAG, "id 0x%lx has bits outside mask 0x%lx", id, mask);
    ESP_RETURN_ON_FALSE(s_node != NULL, ESP_ERR_INVALID_STATE, TAG, "not initialized");
    ESP_RETURN_ON_FALSE(!s_started, ESP_ERR_INVALID_STATE, TAG, "register handlers before CAN_start()");
    ESP_RETURN_ON_FALSE(s_handler_count < CAN_MAX_HANDLERS, ESP_ERR_NO_MEM, TAG, "handler table full");

    s_handlers[s_handler_count++] = (handler_entry_t) {
        .id = id,
        .mask = mask,
        .extended = extended,
        .handler = handler,
        .ctx = ctx,
    };
    return ESP_OK;
}

esp_err_t CAN_get_stats(CAN_stats_t *stats)
{
    ESP_RETURN_ON_FALSE(stats != NULL, ESP_ERR_INVALID_ARG, TAG, "stats is NULL");

    *stats = s_stats;
    return ESP_OK;
}
