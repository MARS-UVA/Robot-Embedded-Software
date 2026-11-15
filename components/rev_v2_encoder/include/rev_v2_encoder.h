/**
  *
  *
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

typedef struct rev_v2_encoder_t *rev_v2_encoder_handle_t;

esp_err_t rev_v2_encoder_init(rev_v2_encoder_handle_t *handle, gpio_num_t input_pin);

esp_err_t rev_v2_encoder_read(rev_v2_encoder_handle_t handle);

esp_err_t rev_v2_encoder_deinit(rev_v2_encoder_handle_t handle);

