/**
 * @file rev_encoder_v2.h
 * @author Quintin Levy
 * @author Rishul Sharma
 * @author Isaiah Thornsley
 * @author Anthony Vu
 * @brief Header file for the REV Robotics Through Bore Encoder V2 driver.
 * @version 1.0
 * 
 * @details This file contains the function declarations and data structures for the REV Robotics Through Bore Encoder V2 driver.
 */

#pragma once

#include "esp_err.h"
#include "driver/gpio.h"
#include "driver/mcpwm_cap.h"

#define PUSLE_RESOLUTION    4096    // 12-bit resolution for the encoder
#define PULSE_MIN_US        3.884   // min pulse time for each cycle, corresponds to 0 degrees
#define PULSE_MAX_US        998.06  // max pulse time for each cycle, corresponds to 360 degrees
#define PERIOD_HZ           1000    // 1 kHz period for the encoder signal

typedef struct rev_encoder_v2_t *rev_encoder_v2_handle_t;

typedef struct {
    mcpwm_cap_timer_handle_t timer; /**< Handle to the MCPWM capture timer. */
    gpio_num_t input_pin; /**< GPIO pin number for the input signal. */
} rev_encoder_v2_config_t;

/**
 * @brief Initialize the REV Robotics Through Bore Encoder V2 driver.
 * 
 * @param handle[out] Pointer to the encoder handle.
 * @param config[in] Pointer to the configuration structure containing the input pin.
 * @return ESP_OK if successful, otherwise an error code.
 */
esp_err_t rev_encoder_v2_init(rev_encoder_v2_handle_t *handle, rev_encoder_v2_config_t *config);

/**
 * @brief Deinitialize the REV Robotics Through Bore Encoder V2 driver.
 * 
 * @param handle[in] Encoder handle to deinitialize.
 * @return ESP_OK if successful, otherwise an error code.
 */
esp_err_t rev_encoder_v2_destroy(rev_encoder_v2_handle_t handle);

/**
 * @brief Read the angle from the REV Robotics Through Bore Encoder V2.
 * 
 * @param handle[in] Encoder handle to read from.
 * @param angle[out] Pointer to store the read angle in degrees.
 * @return ESP_OK if successful, otherwise an error code.
 */
esp_err_t rev_encoder_v2_read(rev_encoder_v2_handle_t handle, float *angle);
