#include <stdio.h>
#include "rev_v2_encoder.h"
#include "esp_err.h"
#include "esp_log.h"

static const char *TAG = "rev_v2_encoder";

struct rev_v2_encoder_t {
    mcpwm_cap_channel_handle_t chan;
    mcpwm_cap_timer_handle_t timer;
    uint32_t rise_edge_time;
    bool rise_edge_exist;
    uint32_t high_ticks;
    double angle;
    uint32_t res_hz;
};

static double s_rev_v2_encoder_pulse_to_angle(double pulse_us)
{
    double angle = (pulse_us - PULSE_MIN_US) * 360.0 / (PULSE_MAX_US - PULSE_MIN_US);
    if (angle < 0.0) {
        angle = 0.0;
    }   
    if (angle > 360.0) {
       angle = 360.0; 
    } 
    return angle;
}

bool IRAM_ATTR on_capture(mcpwm_cap_channel_handle_t chan, const mcpwm_capture_event_data_t *edata, void *user_data) {
    uint32_t cur = edata->cap_value;
    rev_v2_encoder_handle_t handle = (rev_v2_encoder_handle_t) user_data;

    if (edata->cap_edge == MCPWM_CAP_EDGE_POS) {
        handle->rise_edge_time = cur;
        handle->rise_edge_exist = true;
    } else if (handle->rise_edge_exist) {
        handle->high_ticks = - cur - handle->rise_edge_time;
        handle->rise_edge_exist = false;
    }
    return false;
} 


esp_err_t rev_v2_encoder_init(rev_v2_encoder_handle_t *handle, gpio_num_t input_pin) {
    if (handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    rev_v2_encoder_handle_t dev = calloc(1, sizeof(struct rev_v2_encoder_t));
    if (dev == NULL) {
        return ESP_ERR_NO_MEM;
    }
    
    mcpwm_capture_timer_config_t config_timer = {
        .group_id = 0,
        .clk_src = MCPWM_CAPTURE_CLK_SRC_DEFAULT,
    };

    mcpwm_capture_channel_config_t config_chan = {
        .gpio_num = input_pin,
        .prescale = 1,
        .flags = {
            .pos_edge = true,    
            .neg_edge = true,
        }
    };
    mcpwm_cap_channel_handle_t chan;
    mcpwm_cap_timer_handle_t timer;
    // Allocate and initialize a new MCPWM capture timer using the specified configuration(TBD)
    esp_err_t err = mcpwm_new_capture_timer(&config_timer, &timer);
    if (err != ESP_OK) {
        goto fail;
    }
    uint32_t out_res_hz;

    //Retrieve the resolution of the capture timer in Hz and store it in out_res_hz(TBD)
    err = mcpwm_capture_timer_get_resolution(timer, &out_res_hz);
    if (err != ESP_OK) {
        goto fail;
    }
    
    // Allocate and initialize a new MCPWM capture channel using the specified configuration(TBD)
    err = mcpwm_new_capture_channel(timer, &config_chan, &chan);
    if (err != ESP_OK) {
        goto fail;
    }
    dev->rise_edge_exist = false;
    // Define the callback structure, assigning 'on_capture' to run when a signal event occurs(TBD)
    mcpwm_capture_event_callbacks_t cbs = { .on_cap = on_capture };
    
    // Register the event callback function with the capture channel(TBD)
    err = mcpwm_capture_channel_register_event_callbacks(chan, &cbs, (void *)handle);
    if (err != ESP_OK) {
        goto fail;
    }

    //Register the capture channel with the timer and enable it to start capturing events(TBD)
    err = mcpwm_capture_channel_enable(chan);
    if (err != ESP_OK) {
        goto fail;
    }

    //Register the capture timer with the MCPWM driver and enable it to start capturing events(TBD)
    err = mcpwm_capture_timer_enable(timer);
    if (err != ESP_OK) {
        goto fail;
    }

    //Register the capture timer with the MCPWM driver and start it to begin capturing events(TBD)
    err = mcpwm_capture_timer_start(timer);
    if (err != ESP_OK) {
        goto fail;
    }

    dev->chan = chan;
    dev->timer = timer;
    dev->res_hz = out_res_hz;
    *handle = dev;
    return ESP_OK;

    //Free memory on driver & return error code if any operations fail
    fail:
        free(dev);
        *handle = NULL;
        return err;
}

esp_err_t rev_v2_encoder_deinit(rev_v2_encoder_handle_t handle) {
    if (handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t err = mcpwm_capture_channel_register_event_callbacks(handle->chan, NULL, handle);
    if (err != ESP_OK) {
        return err;
    }

    free(handle);
    return ESP_OK;
}

esp_err_t rev_v2_encoder_read(rev_v2_encoder_handle_t handle) {
    if (handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    
    if (handle->high_ticks > 0) {
            double pulse_us = (double)handle->high_ticks * 1e6 / handle->res_hz;

            handle->angle = s_rev_v2_encoder_pulse_to_angle(pulse_us);

            printf("Freq: %.2f Hz, Pulse: %.3f \u00B5s, Angle: %.2f deg\n",(double)handle->res_hz / PERIOD_HZ, pulse_us, handle->angle);
                
    } else {
            ESP_LOGI(TAG, "No Input Detected");
            //printf("No signal\n");
            
    }
    return ESP_OK;
        
}
