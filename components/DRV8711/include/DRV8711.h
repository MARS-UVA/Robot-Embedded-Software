/**
 * @file DRV8711.h
 * @author Cole Luba
 * @author Rishul Sharma
 * @author Josalin Wang
 * @brief Header file for DRV8711 driver.
 * @version 0.1
 * 
 * @details 
 */

 #pragma once

 #include <stdbool.h>
 #include "esp_err.h"
 #include "hal/gpio_types.h"

 typedef struct {
    gpio_num_t spi_cs_pin;
    gpio_num_t spi_clk_pin;
    gpio_num_t spi_mosi_pin;
    gpio_num_t spi_miso_pin;
    gpio_num_t step_pin;
    gpio_num_t dir_pin;
    gpio_num_t enable_pin;
    gpio_num_t fault_pin;
    gpio_num_t reset_pin;
    gpio_num_t sleep_pin;
 } drv8711_config_t;

 /**
 * @brief Opaque handle for the DRV8711 driver.
 */
 typedef struct drv8711_t *drv8711_handle_t;

 /**
 * @brief DRV8711 register addresses.
 */
 typedef enum {
    HPSD_REG_CTRL   = 0x00,
    HPSD_REG_TORQUE = 0x01,
    HPSD_REG_OFF    = 0x02,
    HPSD_REG_BLANK  = 0x03,
    HPSD_REG_DECAY  = 0x04,
    HPSD_REG_STALL  = 0x05,
    HPSD_REG_DRIVE  = 0x06,
    HPSD_REG_STATUS = 0x07
} drv8711_reg_addr_t;

/**
 * @brief DRV8711 step modes.
 */
typedef enum {
    MICROSTEP_1   = 0b0000, // Full-step (71% current)
    MICROSTEP_2   = 0b0001, // 1/2 step
    MICROSTEP_4   = 0b0010, // 1/4 step
    MICROSTEP_8   = 0b0011, // 1/8 step
    MICROSTEP_16  = 0b0100, // 1/16 step
    MICROSTEP_32  = 0b0101, // 1/32 step
    MICROSTEP_64  = 0b0110, // 1/64 step
    MICROSTEP_128 = 0b0111, // 1/128 step
    MICROSTEP_256 = 0b1000  // 1/256 step
} drv8711_step_mode_t;

/**
 * @brief Initializes the DRV8711 driver.
 * 
 * @param handle Pointer to a handle that will be initialized.
 * @param config Pointer to a configuration structure.
 * @return esp_err_t ESP_OK on success, or an error code on failure.
 */
esp_err_t drv8711_init(drv8711_handle_t *handle, const drv8711_config_t *config);

/**
 * @brief Steps the motor once.
 * 
 * @param handle Driver handle returned by drv8711_init().
 * @return esp_err_t ESP_OK on success, or an error code on failure.
 */
esp_err_t step(drv8711_handle_t handle);

/**
 * @brief Sets the direction of the motor.
 * 
 * @param handle Driver handle returned by drv8711_init().
 * @param dir Direction to set (true for one direction, false for the other).
 * @return esp_err_t ESP_OK on success, or an error code on failure.
 */
esp_err_t setDirection(drv8711_handle_t handle, bool dir);

