#include <stdio.h>

#include "rev_encoder_v2.h"

#define NUM_ENCODERS 3

static rev_encoder_v2_handle_t encoders[NUM_ENCODERS];

static mcpwm_cap_timer_handle_t capture_timer;

void app_main(void)
{
    mcpwm_capture_timer_config_t timer_config = {
        .group_id = 0,
        .clk_src = MCPWM_CAPTURE_CLK_SRC_DEFAULT,
    };

    ESP_ERROR_CHECK(mcpwm_new_capture_timer(&timer_config, &capture_timer));

    rev_encoder_v2_config_t encoder_configs[NUM_ENCODERS] = {
        { .input_pin = GPIO_NUM_4, .timer = capture_timer },
        { .input_pin = GPIO_NUM_5, .timer = capture_timer },
        { .input_pin = GPIO_NUM_6, .timer = capture_timer }
    };

    for (int i = 0; i < NUM_ENCODERS; i++) {
        ESP_ERROR_CHECK(rev_encoder_v2_init(&encoders[i], &encoder_configs[i]));
    }

    ESP_ERROR_CHECK(mcpwm_capture_timer_enable(capture_timer));

    ESP_ERROR_CHECK(mcpwm_capture_timer_start(capture_timer));

    while (true) {
        for (int i = 0; i < NUM_ENCODERS; i++) {
            float angle;

            if (rev_encoder_v2_read(encoders[i], &angle) == ESP_OK) {
                printf("Encoder %d: %.2f degrees\n", i, angle);
            } else {
                printf("Encoder %d: Error reading angle\n", i);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }

    for (int i = 0; i < NUM_ENCODERS; i++) {
        rev_encoder_v2_destroy(encoders[i]);
    }
    mcpwm_capture_timer_stop(capture_timer);
    mcpwm_capture_timer_disable(capture_timer);
    mcpwm_del_capture_timer(capture_timer);
}
