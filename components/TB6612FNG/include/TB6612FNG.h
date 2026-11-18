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
} tb6612fng_config_t;

typedef struct tb6612fng_t *tb6612fng_handle_t;

esp_err_t tb6612fng_init(tb6612fng_handle_t *handle, tb6612fng_config_t *config);

esp_err_t tb6612fng_drive(tb6612fng_handle_t handle, uint8_t duty_cycle, bool forward);

void init_hardware_pwm_timer(void);
/*Wheel init_actuator(uint8_t pinEn, uint8_t pinF, uint8_t pinB, ledc_channel_t pwmChannel, ledc_timer_t pwmTimer);
void drive(Actuator *actuator, uint8_t duty, bool isForward);*/
