#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "driver/gpio.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"

typedef struct {
    uint32_t id;
    uint8_t length;
    uint8_t data[8];
    bool extended;
    bool remote;
} can_frame_t;

// Single controller, owned by one task. Call can_service regularly.
// Classical CAN at 500 kbit/s. ESP_OK from can_send only confirms queuing.
esp_err_t can_init(gpio_num_t tx_pin, gpio_num_t rx_pin);
esp_err_t can_send(const can_frame_t *frame, TickType_t timeout);
esp_err_t can_receive(can_frame_t *frame, TickType_t timeout);
esp_err_t can_service(void);
bool can_is_running(void);
