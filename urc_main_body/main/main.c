#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "module_com.h"
#include "can.h"

// use this to toggle on or off the logg messeges (TODO: not implemented)
// plan to use if def or possibly levels of debug modes
#define DEBUG_MODE true

// Placeholder pins until the transceiver wiring is decided
#define CAN_TX_GPIO     4
#define CAN_RX_GPIO     5
#define CAN_BITRATE     500000      // must match every other node on the bus
#define CAN_SELF_TEST   false       // true = single board loopback, false = real bus with another node

#define MY_NODE MC_NODE_MAIN_BODY

static const char *TAG = "main";

// static void on_test_node_counter(const CAN_frame_t *frame, void *ctx)
// {
//     if(frame->len < sizeof(uint32_t)) {
//         return; // not the 4-byte this function expect (something is wrong) Investigate later if not working
//     }

//     uint32_t value = 0;
//     // use memcpy with size included or else dangerous :)
//     memcpy(&value, frame->data, sizeof(value));
//     ESP_LOGI(TAG, "counter from 0x%03lx: %lu", frame->id, value);
// }


static void on_heartbeat(mc_node_t sender, const uint8_t *data, uint8_t len, void *ctx)
{
    mc_heartbeat_t hb;
    if (len != sizeof(hb)) {return;} // lengths do not match --> INVALID!!
    memcpy(&hb, data, sizeof(hb));
    ESP_LOGI(TAG, "heartbeat from node %d: up %lu ms", sender, hb.uptime_ms);
}

// acknowledge event
static void on_pong(mc_node_t sender, const uint8_t *data, uint8_t len, void *ctx)
{
    mc_ping_t pong;
    if (len != sizeof(pong)) {return;} // double check for correct length (verify this wont possibly cause silent failure)
    memcpy(&pong, data, sizeof(pong));
    uint32_t rtt_us = (uint32_t)esp_timer_get_time() - pong.sent_us; 
    ESP_LOGI(TAG, "pong #%lu from node %d: round trip %lu us", pong.seq, sender, rtt_us);
}

void app_main(void)
{
    CAN_config_t can_cfg = {
        .tx_gpio = CAN_TX_GPIO,
        .rx_gpio = CAN_SELF_TEST ? CAN_TX_GPIO : CAN_RX_GPIO,
        .bitrate = CAN_BITRATE,
        .tx_queue_depth = 10,
        .rx_queue_depth = 10,
        .self_test = CAN_SELF_TEST,
    };

    // order: CAN init --> module_com init + registrations --> CAN start
    ESP_ERROR_CHECK(CAN_init(&can_cfg));
    ESP_ERROR_CHECK(module_com_init(MY_NODE));
    ESP_ERROR_CHECK(module_com_on(MC_MSG_HEARTBEAT, on_heartbeat, NULL));
    ESP_ERROR_CHECK(module_com_on(MC_MSG_PONG, on_pong, NULL));
    ESP_ERROR_CHECK(CAN_start());

    uint32_t seq = 0;
    // send a counter once a second, frames from the other board are printed by the CAN rx task
    // CAN_frame_t frame = {
    //     .id = TEST_FRAME_ID,
    //     .extended = false,
    //     .len = sizeof(uint32_t),
    // };
    // uint32_t counter = 0;

    while (1) {
        mc_heartbeat_t hb = { .uptime_ms = (uint32_t)(esp_timer_get_time() / 1000) };
        module_com_send(MC_MSG_HEARTBEAT, &hb, sizeof(hb));

        mc_ping_t ping = { .seq = seq++, .sent_us = (uint32_t)esp_timer_get_time() };
        module_com_send(MC_MSG_PING, &ping, sizeof(ping));

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
