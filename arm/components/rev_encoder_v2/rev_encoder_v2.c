/**
 * @file rev_encoder_v2.c
 * @author Quintin Levy
 * @author Rishul Sharma
 * @author Isaiah Thornsley
 * @author Anthony Vu
 * @brief Source file for the REV Robotics Through Bore Encoder V2 driver.
 * @version 1.0
 * 
 * @details This file contains the implementation of the functions for the REV Robotics Through Bore Encoder V2 driver.
 */

#include <stdio.h>
#include "rev_encoder_v2.h"
#include "esp_err.h"
#include "esp_log.h"

static const char *TAG = "rev_encoder_v2";

struct rev_encoder_v2_t {
    mcpwm_cap_channel_handle_t chan;
    uint32_t rise_edge_time;
    bool rise_edge_exist;
    volatile uint32_t high_ticks;
    volatile bool measurement_valid;
    uint32_t res_hz;
};

static float s_pulse_to_angle(float pulse_us)
{
    float angle = (pulse_us - PULSE_MIN_US) * 360.0 / (PULSE_MAX_US - PULSE_MIN_US);
    if (angle < 0.0) {
        angle = 0.0;
    }   
    if (angle > 360.0) {
       angle = 360.0; 
    } 
    return angle;
}

static bool IRAM_ATTR s_on_capture(mcpwm_cap_channel_handle_t chan, const mcpwm_capture_event_data_t *edata, void *user_data)
{
    rev_encoder_v2_handle_t dev = user_data;

    if (edata->cap_edge == MCPWM_CAP_EDGE_POS) {
        dev->rise_edge_time = edata->cap_value;
        dev->rise_edge_exist = true;
    } else if (dev->rise_edge_exist) {
        dev->high_ticks = edata->cap_value - dev->rise_edge_time;
        dev->measurement_valid = true;
        dev->rise_edge_exist = false;
    }
    return false;
} 


esp_err_t rev_encoder_v2_init(rev_encoder_v2_handle_t *handle, rev_encoder_v2_config_t *config)
{
    if (handle == NULL || config == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    *handle = NULL;
    
    rev_encoder_v2_handle_t dev = calloc(1, sizeof(*dev));
    if (dev == NULL) {
        return ESP_ERR_NO_MEM;
    }

    // Retrieve the resolution of the capture timer in Hz and store it in out_res_hz(TBD)
    esp_err_t err = mcpwm_capture_timer_get_resolution(config->timer, &dev->res_hz);
    if (err != ESP_OK) {
        goto fail;
    }
    
    mcpwm_capture_channel_config_t chan_config = {
        .gpio_num = config->input_pin, // GPIO pin for the input signal
        .prescale = 1, // No prescaling
        .flags = {
            .pos_edge = 1, // Capture on positive edge
            .neg_edge = 1, // Capture on negative edge
            .invert_cap_signal = 0, // Do not invert the input signal
        },
    };

    // Allocate and initialize a new MCPWM capture channel using the specified configuration(TBD)
    err = mcpwm_new_capture_channel(config->timer, &chan_config, &dev->chan);
    if (err != ESP_OK) {
        goto fail;
    }

    // Define the callback structure, assigning 'on_capture' to run when a signal event occurs(TBD)
    mcpwm_capture_event_callbacks_t cbs = {
        .on_cap = s_on_capture
    };
    
    // Register the event callback function with the capture channel(TBD)
    err = mcpwm_capture_channel_register_event_callbacks(dev->chan, &cbs, dev);
    //err = mcpwm_capture_channel_register_event_callbacks(chan, &cbs, (void *)handle);
    if (err != ESP_OK) {
        goto fail_channel;
    }

    //Register the capture channel with the timer and enable it to start capturing events(TBD)
    err = mcpwm_capture_channel_enable(dev->chan);
    if (err != ESP_OK) {
        goto fail_channel;
    }

    *handle = dev;
    return ESP_OK;

    fail_channel:
        mcpwm_del_capture_channel(dev->chan);

    fail:
        free(dev);
        *handle = NULL;
        return err;
}

esp_err_t rev_encoder_v2_deinit(rev_encoder_v2_handle_t handle) {
    if (handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    mcpwm_capture_event_callbacks_t cbs = {
        .on_cap = NULL
    };
    esp_err_t err = mcpwm_capture_channel_register_event_callbacks(handle->chan, &cbs, NULL);
    if (err != ESP_OK) {
        return err;
    }

    free(handle);
    return ESP_OK;
}

esp_err_t rev_encoder_v2_read(rev_encoder_v2_handle_t handle, float *angle) {
    if (handle == NULL || angle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (handle->high_ticks > 0 && handle->measurement_valid) {
            float pulse_us = (float)handle->high_ticks * 1e6 / handle->res_hz;
            *angle = s_pulse_to_angle(pulse_us);
            return ESP_OK;
                
    }
    return ESP_ERR_INVALID_STATE;
}
