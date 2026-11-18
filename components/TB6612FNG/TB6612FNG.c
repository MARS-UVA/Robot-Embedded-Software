/**
 * @file TB6612FNG.c
 * @author Randy Luong
 * @author Michael Petty
 * @author Anthony Vu
 * @brief Source file for TB6612FNG driver
 */

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h> 
#include "TB6612FNG.h"
#include "driver/gpio.h"
#include "driver/ledc.h"

struct tb6612fng_t {
    gpio_num_t extend_pin; // AO1
    gpio_num_t retract_pin; // AO2
    gpio_num_t pwm_pin;
    gpio_num_t standby_pin;
    ledc_channel_t channel;
    ledc_timer_t timer;
};

// Define the PWM properties (adjust speed/resolution as needed)
#define MOTOR_PWM_SPEED_MODE   LEDC_LOW_SPEED_MODE
#define MOTOR_PWM_DUTY_RES     LEDC_TIMER_8_BIT      // 0 to 255 range to match your uint8_t duty
#define MOTOR_PWM_FREQ         5000                  // 5kHz frequency is standard for DC motors

esp_err_t tb6612fng_init(tb6612fng_handle_t *handle, tb6612fng_config_t *config)
{
    if (handle == NULL || config == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    tb6612fng_handle_t dev = calloc(1, sizeof(struct tb6612fng_t));
    if (dev == NULL) {
        return ESP_ERR_NO_MEM;
    }
    
    gpio_reset_pin(config->extend_pin);
    gpio_set_direction(config->extend_pin, GPIO_MODE_OUTPUT);
    gpio_set_level(config->extend_pin, 0);

    gpio_reset_pin(config->retract_pin);
    gpio_set_direction(config->retract_pin, GPIO_MODE_OUTPUT);
    gpio_set_level(config->retract_pin, 0);

    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_LOW_SPEED_MODE,
        .timer_num        = LEDC_TIMER_0,
        .duty_resolution  = LEDC_TIMER_8_BIT, // 0-255 range
        .freq_hz          = 5000,             // 5 kHz frequency
        .clk_cfg          = LEDC_AUTO_CLK
    };
    esp_err_t err = ledc_timer_config(&ledc_timer);
    if (err != ESP_OK) {
        goto fail;
    }

    dev->extend_pin = config->extend_pin;
    dev->retract_pin = config->retract_pin;
    dev->pwm_pin = config->pwm_pin;
    dev->timer = ledc_timer.timer_num; 

    *handle = dev;
    return ESP_OK;

    fail:
        free(dev);
        *handle = NULL;
        return err;
}

esp_err_t tb6612fng_drive(tb6612fng_handle_t handle, uint8_t duty_cycle, bool forward)
{
    // set direction pins
    gpio_set_level(handle->extend_pin, forward);
    gpio_set_level(handle->retract_pin, !forward);
    
    // Set new target duty cycle
    ledc_set_duty(MOTOR_PWM_SPEED_MODE, handle->pwm_pin, duty_cycle);
    // commit new duty cycle to hardware register
    ledc_update_duty(MOTOR_PWM_SPEED_MODE, handle->pwm_pin);

    return ESP_OK;
}