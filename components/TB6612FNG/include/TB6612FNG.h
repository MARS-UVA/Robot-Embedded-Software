/**
 * @file TB6612FNG.h
 * @author Randy Luong
 * @author Michael Petty
 * @author Anthony Vu
 * @author Josalin Wang
 * @brief Include file for TB6612FNG driver
 */

#pragma once
#include <stdint.h>
#include <stdbool.h>

#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_err.h"

typedef struct {
    gpio_num_t extend_pin; // AO1
    gpio_num_t retract_pin; // AO2
    gpio_num_t pwm_pin;
    gpio_num_t standby_pin;
    ledc_channel_t channel;
} tb6612fng_config_t;

typedef struct tb6612fng_t *tb6612fng_handle_t;

/** 
 * @brief Initialize a TB6612FNG motor/actuator driver instance
 * 
 * Allocates memory for the device handle, configures the direction
 * and the standby GPIOs, intializes the LEDC timer and PWM channel, 
 * and takes the driver out of standby mode.
 * 
 * @param[out] handle Pointer to a handle where the newly allocated
 * instance will be stored... Set to NULL if initialization fails
 * 
 * @param[in] config POinter to the configuration structure specifying pins and LEDC channel
 * 
 * @return 
 *      - ESP_OK: initialization successful
 *      - ESP_ERR_INVALID_ARG: 'handle' or 'config' is NULL, or invalid GPIO/LEDC params
 *      - ESP_ERR_NO_ERR: failed to allocate memory for the driver instance
 *      - other esp_err_t codes from LEDC config functions
 **/

esp_err_t tb6612fng_init(tb6612fng_handle_t *handle, tb6612fng_config_t *config);

/** 
 * @brief Drive the actuator in a specified direction and speed
 * 
 * @param[in] handle    Active TB6612FNG driver handle
 * @param[in] duty_cycle PWM duty cycle value (0=stopped, 255= full speed)
 * @param[in] forward   Direction flag. True to extend, false to retract
 * 
 * @return 
 *      - ESP_OK: speed and direction updated successfully
 *      - ESP_ERR_INVALID_ARG: handle is NULL
 *      - other esp_err_t: codes from GPIO or LEDC drivers. 
 **/
esp_err_t tb6612fng_drive(tb6612fng_handle_t handle, uint8_t duty_cycle, bool forward);

/** 
 * @brief de-initialize a TB6612FNG driver instance and release the memory 
 * 
 * Places the driver into standby, sets direction pins low, stops the assigned PWM channel
 * resets all associated GPIOs, frees internal memory, nullifies the caller's handle
 * 
 * @note Does not deconfigure the shared LEDC timer so other driver instances using the same timer
 * remain unaffected
 * 
 * @param[in, out] handle Pointer to the driver handle to de-init. 
 *                      Set to NULL upon successful completion
 * @return 
 *      - ESP_OK: successful deinitialization
 *      - ESP_ERR_INVALID_ARG: 'handle' or '*handle' is NULL
 *      - other esp_err_t from LEDC or GPIO driver functions
 **/
esp_err_t tb6612fng_deinit(tb6612fng_handle_t *handle);
