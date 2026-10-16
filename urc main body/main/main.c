#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "board_config.h"
#include "can.h"
#include "protocol.h"

static const char *TAG = "HELLO_APP";

void app_main(void)
{
    esp_err_t err = can_init(BOARD_CAN_TX_PIN, BOARD_CAN_RX_PIN);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "CAN initialization failed: %s", esp_err_to_name(err));
        return;
    }
    can_frame_t hello;
    ESP_ERROR_CHECK(protocol_make_hello(BOARD_NODE_ID, &hello));
    ESP_LOGI(TAG, "Node 0x%03X, peer 0x%03X; CAN at 500 kbit/s",
             (unsigned)BOARD_NODE_ID, (unsigned)BOARD_PEER_ID);

    const TickType_t interval = pdMS_TO_TICKS(BOARD_HELLO_INTERVAL_MS);
    TickType_t last_send = xTaskGetTickCount() - interval;
    TickType_t last_hello = xTaskGetTickCount();
    bool peer_online = false;
    bool timeout_reported = false;
    uint32_t queued = 0;

    // One task owns communication and recovery; no core pinning is needed.
    while (true) {
        err = can_service();
        if (err != ESP_OK) ESP_LOGE(TAG, "CAN service failed: %s", esp_err_to_name(err));

        TickType_t now = xTaskGetTickCount();
        if (can_is_running() && (TickType_t)(now - last_send) >= interval) {
            err = can_send(&hello, 0);
            last_send = now;
            if (err == ESP_OK) {
                ESP_LOGI(TAG, "Queued HELLO #%" PRIu32, ++queued);
            } else {
                ESP_LOGW(TAG, "Could not queue HELLO: %s", esp_err_to_name(err));
            }
        }

        if (can_is_running()) {
            can_frame_t received;
            err = can_receive(&received, pdMS_TO_TICKS(BOARD_POLL_INTERVAL_MS));
            if (err == ESP_OK && protocol_is_peer_hello(&received, BOARD_PEER_ID)) {
                last_hello = xTaskGetTickCount();
                if (!peer_online) ESP_LOGI(TAG, "Peer online");
                peer_online = true;
                timeout_reported = false;
                ESP_LOGI(TAG, "Received HELLO from 0x%03X | 48 45 4C 4C 4F", (unsigned)received.id);
            } else if (err != ESP_OK && err != ESP_ERR_TIMEOUT) {
                ESP_LOGW(TAG, "Receive failed: %s", esp_err_to_name(err));
            }
        }

        now = xTaskGetTickCount();
        if (!timeout_reported && (TickType_t)(now - last_hello) >= pdMS_TO_TICKS(BOARD_PEER_TIMEOUT_MS)) {
            ESP_LOGW(TAG, "No valid peer HELLO for %d ms", BOARD_PEER_TIMEOUT_MS);
            peer_online = false;
            timeout_reported = true;
        }
        // Yield under heavy traffic and while the controller is recovering.
        vTaskDelay(1);
    }
}
