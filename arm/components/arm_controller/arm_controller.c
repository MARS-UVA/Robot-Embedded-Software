#include "arm_controller.h"

#include "a4990.h"
#include "driver/ledc.h"
#include "driver/gpio.h"

void arm_controller_task(void *pvParameters)
{
    ledc_timer_config_t motors_timer_config = {
        .speed_mode         = LEDC_LOW_SPEED_MODE,
        .timer_num          = LEDC_TIMER_0,
        .duty_resolution    = LEDC_TIMER_8_BIT,
        .freq_hz            = 5000,
        .clk_cfg            = LEDC_AUTO_CLK
    };

    ESP_ERROR_CHECK_WITHOUT_ABORT(ledc_timer_config(&motors_timer_config));

    a4990_config_t motors_config = {
        .in1 = { .gpio = GPIO_NUM_0, .channel = LEDC_CHANNEL_0 },
        .in2 = { .gpio = GPIO_NUM_1, .channel = LEDC_CHANNEL_1 },
        .in3 = { .gpio = GPIO_NUM_2, .channel = LEDC_CHANNEL_2 },
        .in4 = { .gpio = GPIO_NUM_3, .channel = LEDC_CHANNEL_4 },
        .inh = GPIO_NUM_4,
        .ef1 = GPIO_NUM_5,
        .ef2 = GPIO_NUM_6,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer = motors_timer_config.timer_num,
        .duty_resolution = motors_timer_config.duty_resolution,
        .freq_hz = motors_timer_config.freq_hz
    };

    a4990_handle_t motors;

    a4990_init(&motors, &motors_config);
    a4990_enable(motors);

    a4990_set_output(motors, A4990_PHASE_A, 1.0f);

    while (true);
}
