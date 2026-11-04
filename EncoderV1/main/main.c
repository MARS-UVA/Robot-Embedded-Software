#include <stdio.h>
#include <stddef.h>
#include "driver/gpio.h"
#include "driver/mcpwm_cap.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"


#define PUSLE_RESOLUTION 4096
#define PWM_IN 6
#define PULSE_MIN_US   3.884
#define PULSE_MAX_US   998.06

mcpwm_cap_channel_handle_t chan;
mcpwm_cap_timer_handle_t timer;
uint32_t out_res_hz;
uint32_t cur;
uint32_t rise_edge_time;
bool rise_edge_exist = false;
uint32_t period_ticks;
uint32_t high_ticks;


double pulse_to_angle(double pulse_us)
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
    cur = edata->cap_value;

    if (edata->cap_edge == MCPWM_CAP_EDGE_POS) {
        if (rise_edge_exist) {
            period_ticks = cur - rise_edge_time;
        }
        rise_edge_time = cur;
        rise_edge_exist = true;
    } else if (rise_edge_exist) {
        high_ticks = cur - rise_edge_time;
    }
    return false;;
}
void mcpwm_init(void) {
    mcpwm_capture_timer_config_t config_timer = {
        .group_id = 0,
        .clk_src = MCPWM_CAPTURE_CLK_SRC_DEFAULT,
    };

    mcpwm_capture_channel_config_t config_chan = {
        .gpio_num = PWM_IN,
        .prescale = 1,
        .flags = {
            .pos_edge = true,    
            .neg_edge = true,
        }
    };

    ESP_ERROR_CHECK(mcpwm_new_capture_timer(&config_timer, &timer));
    ESP_ERROR_CHECK(mcpwm_capture_timer_get_resolution(timer, &out_res_hz));
    ESP_ERROR_CHECK(mcpwm_new_capture_channel(timer, &config_chan, &chan));
    mcpwm_capture_event_callbacks_t cbs = { .on_cap = on_capture };
    ESP_ERROR_CHECK(mcpwm_capture_channel_register_event_callbacks(chan, &cbs, NULL));
    ESP_ERROR_CHECK(mcpwm_capture_channel_enable(chan));
    ESP_ERROR_CHECK(mcpwm_capture_timer_enable(timer));
    ESP_ERROR_CHECK(mcpwm_capture_timer_start(timer));
}

void app_main(void)
{
    mcpwm_init();


    while (1) {
        uint32_t period = period_ticks;
        uint32_t high = high_ticks;

        if (period > 0) {
            double pulse_us = (double)high * 1e6 / out_res_hz;
            double angle = pulse_to_angle(pulse_us);
            printf("Freq: %.2f Hz, Pulse: %.3f us, Angle: %.2f deg\n",
                (double)out_res_hz / period, pulse_us, angle);
        } else {
            printf("No signal\n");
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }

}
