#include "can.h"
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "esp_check.h"          // ESP_RETURN_ON_FALSE / ESP_RETURN_ON_ERROR
#include "esp_heap_caps.h"      // heap_caps_calloc
#include "esp_log.h"            // ESP_LOGI / ESP_LOGE for printing
#include "esp_twai.h"           // twai_node_enable, _disable, _delete, ..
#include "esp_twai_onchip.h"    // twai_new_node_onchip + its config struct

// Used with ESP log (DEBUG)
static const char *TAG = "CAN";

// Handle for CAN/twai. shared between function calls
static twai_node_handle_t s_node = NULL;
static bool s_started = false; // node enabled status

// One transmit buffer. The driver keeps a pointer to the frame until it is sent,
// so the frame must live here instead of on the caller's stack.
typedef struct {
    twai_frame_t frame;                 // must stay first: on_tx_done casts the frame pointer back to its slot
    uint8_t      data[CAN_MAX_DATA_LEN];
} tx_slot_t;

static tx_slot_t *s_tx_slots = NULL;        // pool of transmit buffers
static uint32_t s_tx_slot_count = 0;
static QueueHandle_t s_tx_free = NULL;      // pointers to slots not currently owned by the driver

// written from the TX done interrupt, read from tasks
static volatile uint32_t s_tx_success = 0;
static volatile uint32_t s_tx_failed = 0;

// Runs in interrupt context when the hardware finishes with a frame (sent or given up).
// Must be short and non-blocking: count the result and hand the slot back to the free list.
static bool IRAM_ATTR on_tx_done(twai_node_handle_t handle, const twai_tx_done_event_data_t *edata, void *user_ctx)
{
    if (edata->is_tx_success) {
        s_tx_success++;
    } else {
        s_tx_failed++;
    }

    tx_slot_t *slot = (tx_slot_t *)edata->done_tx_frame;
    BaseType_t task_woken = pdFALSE;
    xQueueSendFromISR(s_tx_free, &slot, &task_woken);
    return task_woken == pdTRUE;    // tells the driver to switch to a task waiting for a free slot
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

static void tx_pool_free(void)
{
    if (s_tx_free != NULL) {
        vQueueDelete(s_tx_free);
        s_tx_free = NULL;
    }
    free(s_tx_slots);
    s_tx_slots = NULL;
    s_tx_slot_count = 0;
}

esp_err_t CAN_init(const CAN_config_t *config)
{
    ESP_RETURN_ON_FALSE(config != NULL, ESP_ERR_INVALID_ARG, TAG, "config is NULL");
    ESP_RETURN_ON_FALSE(config->bitrate > 0, ESP_ERR_INVALID_ARG, TAG, "bitrate must be > 0");
    ESP_RETURN_ON_FALSE(config->tx_queue_depth > 0, ESP_ERR_INVALID_ARG, TAG, "tx_queue_depth must be > 0");
    ESP_RETURN_ON_FALSE(s_node == NULL,  ESP_ERR_INVALID_STATE, TAG, "node already initialized");

    esp_err_t ret = ESP_OK;     // ESP_GOTO_ON_* macros store the error here before jumping to cleanup

    twai_onchip_node_config_t node_cfg =
    {
        .io_cfg.tx = config->tx_gpio,
        .io_cfg.rx = config->rx_gpio,
        .io_cfg.quanta_clk_out = -1,
        .io_cfg.bus_off_indicator = -1,
        .bit_timing.bitrate = config->bitrate,
        .tx_queue_depth = config->tx_queue_depth,
        .flags.enable_self_test = config->self_test,
        .flags.enable_loopback = config->self_test,
        .fail_retry_cnt = -1                    // retry a failed frame forever
    };

    // one slot per driver queue entry, plus one for the frame currently in the hardware
    s_tx_slot_count = config->tx_queue_depth + 1;
    // internal RAM, since the interrupt reads these buffers
    s_tx_slots = heap_caps_calloc(s_tx_slot_count, sizeof(tx_slot_t), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    ESP_GOTO_ON_FALSE(s_tx_slots != NULL, ESP_ERR_NO_MEM, cleanup, TAG, "no memory for tx slots");
    s_tx_free = xQueueCreate(s_tx_slot_count, sizeof(tx_slot_t *));
    ESP_GOTO_ON_FALSE(s_tx_free != NULL, ESP_ERR_NO_MEM, cleanup, TAG, "no memory for tx free list");
    tx_pool_reset();

    // driver copies node_cfg and writes the new handle into s_node
    ESP_GOTO_ON_ERROR(twai_new_node_onchip(&node_cfg, &s_node), cleanup, TAG, "failed to create node");

    // callbacks can only be registered while the node is disabled
    twai_event_callbacks_t cbs = {
        .on_tx_done = on_tx_done,
    };
    ESP_GOTO_ON_ERROR(twai_node_register_event_callbacks(s_node, &cbs, NULL), cleanup, TAG, "failed to register callbacks");

    s_tx_success = 0;
    s_tx_failed = 0;

    ESP_LOGI(TAG, "initialized: tx=%d rx=%d bitrate=%lu%s",
             config->tx_gpio, config->rx_gpio, config->bitrate,
             config->self_test ? " (self-test)" : "");
    return ESP_OK;

cleanup:
    // undo whatever succeeded before the failure
    if (s_node != NULL) {
        twai_node_delete(s_node);
        s_node = NULL;
    }
    tx_pool_free();
    return ret;
}

esp_err_t CAN_deinit(void)
{
    ESP_RETURN_ON_FALSE(s_node != NULL, ESP_ERR_INVALID_STATE, TAG, "cannot deinitialize null CAN node");
    ESP_RETURN_ON_FALSE(!s_started, ESP_ERR_INVALID_STATE, TAG, "cannot deinitialize running CAN node");

    ESP_RETURN_ON_ERROR(twai_node_delete(s_node), TAG, "delete failed");
    s_node = NULL;
    tx_pool_free();

    ESP_LOGI(TAG, "deinitialized");
    return ESP_OK;
}

esp_err_t CAN_start(void)
{
    // check for valid state before starting CAN
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
    // check for valid state before stopping CAN
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
    ESP_RETURN_ON_FALSE(frame->id <= (frame->extended ? CAN_EXT_ID_MAX : CAN_STD_ID_MAX), ESP_ERR_INVALID_ARG,
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

esp_err_t CAN_get_stats(CAN_stats_t *stats)
{
    ESP_RETURN_ON_FALSE(stats != NULL, ESP_ERR_INVALID_ARG, TAG, "stats is NULL");

    stats->tx_success = s_tx_success;
    stats->tx_failed = s_tx_failed;
    return ESP_OK;
}
