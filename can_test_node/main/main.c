#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "can.h"

// Placeholder pins until the transceiver wiring is decided
#define CAN_TX_GPIO     4
#define CAN_RX_GPIO     5
#define CAN_BITRATE     500000      // must match every other node on the bus

// true = single board loopback, false = real bus with another node
#define CAN_SELF_TEST   false

// each board sends on its own ID, two nodes must never send the same ID
#define TEST_FRAME_ID   0x101       // urc_main_body uses 0x100

static const char *TAG = "test_node";

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

    ESP_ERROR_CHECK(CAN_init(&can_cfg));
    ESP_ERROR_CHECK(CAN_start());

    // send a counter once a second, frames from the other board are printed by the CAN rx task
    CAN_frame_t frame = {
        .id = TEST_FRAME_ID,
        .extended = false,
        .len = sizeof(uint32_t),
    };
    uint32_t counter = 0;

    while (1) {
        memcpy(frame.data, &counter, sizeof(counter));     // 4 bytes, little-endian
        esp_err_t err = CAN_send(&frame, 100);

        // wait 1000 ms = 1 second
        vTaskDelay(pdMS_TO_TICKS(1000));

        // Log results for debug
        CAN_stats_t stats;
        CAN_get_stats(&stats);
        ESP_LOGI(TAG, "sent #%lu: %s | tx ok=%lu failed=%lu | rx=%lu dropped=%lu",
                 counter, esp_err_to_name(err), stats.tx_success, stats.tx_failed,
                 stats.rx_received, stats.rx_dropped);
        counter+=2;
    }
}
