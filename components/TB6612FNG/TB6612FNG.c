/**
 * @file TB6612FNG.c
 * @author Randy Luong
 * @author Michael Petty
 * @author Anthony Vu
 * @author Josalin Wang
 * @brief Source file for TB6612FNG driver
 */

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h> 
#include "TB6612FNG.h"
#include "driver/gpio.h"
#include "driver/ledc.h"

/**
 * @brief Internal representation of a TB6612FNG driver instance.
 */

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
#define MOTOR_PWM_TIMER        LEDC_TIMER_0
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

    gpio_reset_pin(config->standby_pin);
    gpio_set_direction(config->standby_pin, GPIO_MODE_OUTPUT);
    gpio_set_level(config->standby_pin, 1);

    ledc_timer_config_t ledc_timer = {
        .speed_mode       = MOTOR_PWM_SPEED_MODE,
        .timer_num        = MOTOR_PWM_TIMER,
        .duty_resolution  = MOTOR_PWM_DUTY_RES, // 0-255 range
        .freq_hz          = MOTOR_PWM_FREQ,             // 5 kHz frequency
        .clk_cfg          = LEDC_AUTO_CLK
    };
    
    ledc_channel_config_t ledc_channel = {
        .gpio_num = config->pwm_pin,
        .speed_mode = MOTOR_PWM_SPEED_MODE,
        .channel = config->channel,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = MOTOR_PWM_TIMER,
        .duty = 0,
        .hpoint = 0
    };

    esp_err_t err = ledc_timer_config(&ledc_timer);
    if (err != ESP_OK) {
        goto fail;
    }


    err = ledc_channel_config(&ledc_channel);
    if (err != ESP_OK) {
        goto fail;
    }

    dev->extend_pin = config->extend_pin;
    dev->retract_pin = config->retract_pin;
    dev->pwm_pin = config->pwm_pin;
    dev->standby_pin = config->standby_pin;
    dev->timer = ledc_timer.timer_num; 
    dev->channel = ledc_channel.channel;

    *handle = dev;
    return ESP_OK;

    fail:
        free(dev);
        *handle = NULL;
        return err;
}

esp_err_t tb6612fng_deinit(tb6612fng_handle_t *handle){
    if (handle == NULL || *handle == NULL){
        return ESP_ERR_INVALID_ARG;
    }

    tb6612fng_handle_t dev = *handle;

    esp_err_t err = gpio_set_level(dev->standby_pin, 0);
    if (err != ESP_OK) {
        return err;
    }

    err = gpio_set_level(dev->retract_pin, 0);
    if (err != ESP_OK) {
        return err;
    }

    err = gpio_set_level(dev->extend_pin, 0);
    if (err != ESP_OK) {
        return err;
    }

    err = ledc_stop(MOTOR_PWM_SPEED_MODE, dev->channel, 0);
    if (err != ESP_OK) {
        return err;
    }

    gpio_reset_pin(dev->extend_pin);
    gpio_reset_pin(dev->retract_pin);
    gpio_reset_pin(dev->standby_pin);
    gpio_reset_pin(dev->pwm_pin);

    free(dev);
    *handle = NULL;

    return ESP_OK;
}


esp_err_t tb6612fng_drive(tb6612fng_handle_t handle, uint8_t duty_cycle, bool forward)
{
    if (handle == NULL){
        return ESP_ERR_INVALID_ARG;
    }
    // set direction pins opposite of each other
    esp_err_t err = gpio_set_level(handle->extend_pin, forward);
    if (err != ESP_OK) {
        return err;
    }
    err = gpio_set_level(handle->retract_pin, !forward);
    if (err != ESP_OK) {
        return err;
    }
    
    // Set new target duty cycle
    err = ledc_set_duty(MOTOR_PWM_SPEED_MODE, handle->channel, duty_cycle);
    if (err != ESP_OK) {
        return err;
    }
    // commit new duty cycle to hardware register
    return ledc_update_duty(MOTOR_PWM_SPEED_MODE, handle->channel);
}