#include "can.h"
#include <string.h>
#include "driver/twai.h"
#include "esp_log.h"

static const char *TAG = "CAN";
static bool installed;
static bool running;

esp_err_t can_init(gpio_num_t tx_pin, gpio_num_t rx_pin)
{
    if (installed) return ESP_ERR_INVALID_STATE;
    twai_general_config_t general = TWAI_GENERAL_CONFIG_DEFAULT(tx_pin, rx_pin, TWAI_MODE_NORMAL);
    twai_timing_config_t timing = TWAI_TIMING_CONFIG_500KBITS();
    twai_filter_config_t filter = TWAI_FILTER_CONFIG_ACCEPT_ALL();
    general.alerts_enabled = TWAI_ALERT_TX_SUCCESS | TWAI_ALERT_TX_FAILED |
        TWAI_ALERT_BUS_ERROR | TWAI_ALERT_RX_QUEUE_FULL | TWAI_ALERT_BUS_OFF |
        TWAI_ALERT_BUS_RECOVERED | TWAI_ALERT_ERR_PASS;
    esp_err_t err = twai_driver_install(&general, &timing, &filter);
    if (err != ESP_OK) return err;
    installed = true;
    err = twai_start();
    if (err != ESP_OK) {
        esp_err_t cleanup = twai_driver_uninstall();
        installed = cleanup != ESP_OK;
        return err;
    }
    running = true;
    return ESP_OK;
}

bool can_is_running(void) { return running; }

esp_err_t can_send(const can_frame_t *frame, TickType_t timeout)
{
    if (!frame || frame->length > 8 ||
        frame->id > (frame->extended ? 0x1FFFFFFFU : 0x7FFU)) return ESP_ERR_INVALID_ARG;
    if (!running) return ESP_ERR_INVALID_STATE;
    twai_message_t message = {0};
    message.identifier = frame->id;
    message.extd = frame->extended;
    message.rtr = frame->remote;
    message.data_length_code = frame->length;
    memcpy(message.data, frame->data, frame->length);
    return twai_transmit(&message, timeout);
}

esp_err_t can_receive(can_frame_t *frame, TickType_t timeout)
{
    if (!frame) return ESP_ERR_INVALID_ARG;
    if (!running) return ESP_ERR_INVALID_STATE;
    twai_message_t message = {0};
    esp_err_t err = twai_receive(&message, timeout);
    if (err != ESP_OK) return err;
    if (message.data_length_code > 8) return ESP_ERR_INVALID_SIZE;
    *frame = (can_frame_t) {
        .id = message.identifier, .length = message.data_length_code,
        .extended = message.extd, .remote = message.rtr,
    };
    memcpy(frame->data, message.data, frame->length);
    return ESP_OK;
}

esp_err_t can_service(void)
{
    if (!installed) return ESP_ERR_INVALID_STATE;
    uint32_t alerts = 0;
    esp_err_t err = twai_read_alerts(&alerts, 0);
    if (err != ESP_OK && err != ESP_ERR_TIMEOUT) return err;
    if (alerts & TWAI_ALERT_TX_SUCCESS) ESP_LOGI(TAG, "Frame transmitted and acknowledged on CAN bus");
    if (alerts & TWAI_ALERT_TX_FAILED) ESP_LOGW(TAG, "Transmission failed");
    if (alerts & TWAI_ALERT_BUS_ERROR) ESP_LOGW(TAG, "Bus error: check peer, wiring, termination and bitrate");
    if (alerts & TWAI_ALERT_RX_QUEUE_FULL) ESP_LOGW(TAG, "Receive queue overflow: frames lost");
    if (alerts & TWAI_ALERT_ERR_PASS) ESP_LOGW(TAG, "Controller entered error-passive state");

    // Inspect state too, so a failed recovery or restart is retried next call.
    twai_status_info_t status;
    err = twai_get_status_info(&status);
    if (err != ESP_OK) return err;
    running = status.state == TWAI_STATE_RUNNING;
    if (status.state == TWAI_STATE_BUS_OFF) {
        ESP_LOGW(TAG, "Bus off; starting recovery (pending transmissions discarded)");
        return twai_initiate_recovery();
    }
    if (status.state == TWAI_STATE_STOPPED) {
        err = twai_start();
        running = err == ESP_OK;
        if (running) ESP_LOGI(TAG, "CAN restarted after recovery");
        return err;
    }
    return ESP_OK;
}
