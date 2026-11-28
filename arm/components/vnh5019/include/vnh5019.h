#pragma once

#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_err.h"

typedef struct vnh5019_t *vnh5019_handle_t; /**< Handle for the VNH5019 instance */

/** Configuration structure for the VNH5019 instance */
typedef struct {
    gpio_num_t ina;
    gpio_num_t inb;
    gpio_num_t pwm;
    gpio_num_t cs; /**< Current sense pin, optional, set to GPIO_NUM_NC if not used */
    gpio_num_t enadiaga;
    gpio_num_t enbdiagb;

    ledc_channel_t channel;
    ledc_mode_t speed_mode; /**< LEDC channel mode, LEDC_HIGH_SPEED_MODE is only available for the ESP32 */
    ledc_timer_t timer;
    ledc_timer_bit_t duty_resolution;
    uint32_t freq_hz;
} vnh5019_config_t;

/**
 * @brief Initialize the VNH5019 driver
 * @param handle[out] Pointer to the handle for the VNH5019 instance
 * @param config[in] Pointer to the configuration structure for the VNH5019
 * @return ESP_OK on success, error code on failure
 */
esp_err_t vnh5019_init(vnh5019_handle_t *handle, const vnh5019_config_t *config);

/**
 * @brief Deinitialize the VNH5019 driver
 * @param handle[in] Handle for the VNH5019 instance
 * @return ESP_OK on success, error code on failure
 */
esp_err_t vnh5019_deinit(vnh5019_handle_t handle);

/**
 * @brief Set the output of the VNH5019 driver
 * @param handle[in] Handle for the VNH5019 instance
 * @param duty_cycle[in] Duty cycle for the output (-1.0 to 1.0)
 * @return ESP_OK on success, error code on failure
 */
esp_err_t vnh5019_set_output(vnh5019_handle_t handle, float duty_cycle);
