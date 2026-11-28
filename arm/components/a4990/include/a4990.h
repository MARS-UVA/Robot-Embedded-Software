#pragma once

#include "driver/gpio.h"
#include "driver/ledc.h"

typedef struct a4990_t *a4990_handle_t; /**< Handle for the A4990 instance */

/** @brief Structure for configuring the A4990 input channels */
typedef struct {
    gpio_num_t gpio;
    ledc_channel_t channel;
} a4990_input_config_t;

/** @brief Structure for configuring the A4990 driver */
typedef struct {
    a4990_input_config_t in1;
    a4990_input_config_t in2;
    a4990_input_config_t in3;
    a4990_input_config_t in4;

    gpio_num_t inh;
    gpio_num_t ef1;
    gpio_num_t ef2;
    
    ledc_mode_t speed_mode; /**< LEDC channel mode, LEDC_HIGH_SPEED_MODE only for ESP32 */
    ledc_timer_t timer;
    ledc_timer_bit_t duty_resolution;
    uint32_t freq_hz;
} a4990_config_t;

/** @brief Enumeration of A4990 phases */
typedef enum {
    A4990_PHASE_A,
    A4990_PHASE_B
} a4990_phase_t;

/**
 * @brief Initialize the A4990 driver
 * @param handle[out] Pointer to the handle for the A4990 instance
 * @param config[in] Pointer to the configuration structure for the A4990
 * @return ESP_OK on success, error code on failure
 */
esp_err_t a4990_init(a4990_handle_t *handle, const a4990_config_t *config);

/** 
 * @brief Deinitialize the A4990 driver
 * @param handle[in] Pointer to the handle for the A4990 instance
 * @return ESP_OK on success, error code on failure
 */
esp_err_t a4990_deinit(a4990_handle_t handle);

/** 
 * @brief Brings A4990 out of sleep mode
 * @param handle[in] Pointer to the handle for the A4990 instance
 * @return ESP_OK on success, error code on failure
 */
esp_err_t a4990_enable(a4990_handle_t handle);

/** 
 * @brief Puts the A4990 driver to sleep
 * @param handle[in] Pointer to the handle for the A4990 instance
 * @return ESP_OK on success, error code on failure
 */
esp_err_t a4990_disable(a4990_handle_t handle);

/** 
 * @brief Set the output of the A4990 driver
 * @param handle[in] Pointer to the handle for the A4990 instance
 * @param phase[in] The phase for which to set the output
 * @param duty_cycle[in] The duty cycle for the output
 * @return ESP_OK on success, error code on failure
 */
esp_err_t a4990_set_output(a4990_handle_t handle, a4990_phase_t phase, float duty_cycle);
