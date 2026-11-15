#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "can.h"
#include "module_com.h"

#define CAN_TX_GPIO     4
#define CAN_RX_GPIO     5
#define CAN_BITRATE     500000      // must match every other node on the bus
#define CAN_SELF_TEST   false       // true = single board loopback, false = real bus with another node

#define MY_NODE         MC_NODE_TEST

static const char *TAG = "test_node";

static void on_heartbeat(mc_node_t sender, const uint8_t *data, uint8_t len, void *ctx)
{
    mc_heartbeat_t hb;
    if (len != sizeof(hb)) {return;}
    memcpy(&hb, data, sizeof(hb));
    ESP_LOGI(TAG, "heartbeat from node %d: up %lu ms", sender, hb.uptime_ms);
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
    ESP_ERROR_CHECK(module_com_init(MY_NODE));      // also answers pings automatically
    ESP_ERROR_CHECK(module_com_on(MC_MSG_HEARTBEAT, on_heartbeat, NULL));
    ESP_ERROR_CHECK(CAN_start());

    while (1) {
        mc_heartbeat_t hb = { .uptime_ms = (uint32_t)(esp_timer_get_time() / 1000) };
        module_com_send(MC_MSG_HEARTBEAT, &hb, sizeof(hb));
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
