/**
 * @file rev_encoder_v2.h
 * @author Quintin Levy
 * @author Rishul Sharma
 * @author Isaiah Thornsley
 * @author Anthony Vu
 * @brief Header file for the REV Robotics V2 Encoder driver.
 * @version 1.0
 * 
 * @details This file contains the function declarations and data structures for the REV Robotics Encoder driver.
 */

#pragma once

//C Libraries
#include <stdbool.h>
#include <stdint.h>

//Special Libraries
#include "esp_err.h"
#include "driver/gpio.h"
#include "driver/mcpwm_cap.h"
#include "freertos/FreeRTOS.h"

//pulse resolution size for input pwm signal
#define PUSLE_RESOLUTION 4096
//min pulse time for each cycle, corresponds to 0 degrees
#define PULSE_MIN_US   3.884
//max pulse time for each cycle, corresponds to 360 degrees
#define PULSE_MAX_US   998.06
#define PERIOD_HZ 1000
//state variable for tracking 
static bool rise_edge_exist = false;

typedef struct rev_encoder_v2_t *rev_encoder_v2_handle_t;

/**
 * @brief Initialize the REV Robotics V2 Encoder driver.
 * 
 * @param handle[out] Pointer to the encoder handle.
 * @param input_pin[in] GPIO pin number for the input signal.
 * @return ESP_OK if successful, otherwise an error code.
 */
esp_err_t rev_encoder_v2_init(rev_encoder_v2_handle_t *handle, gpio_num_t input_pin);

/**
 * @brief Deinitialize the REV Robotics V2 Encoder driver.
 * 
 * @param handle[in] Encoder handle to deinitialize.
 * @return ESP_OK if successful, otherwise an error code.
 */
esp_err_t rev_encoder_v2_deinit(rev_encoder_v2_handle_t handle);

/**
 * @brief Read the angle from the REV Robotics V2 Encoder.
 * 
 * @param handle[in] Encoder handle to read from.
 * @param ret_angle[out] Pointer to store the read angle in degrees.
 * @return ESP_OK if successful, otherwise an error code.
 */
esp_err_t rev_encoder_v2_read(rev_encoder_v2_handle_t handle, double *ret_angle);
