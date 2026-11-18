#include <stdio.h>
#include <string.h> 
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "module_com.h"
#include "can.h"
#include "TB6612FNG.h"
#include <time.h>

#define IN1 GPIO_NUM_1
#define IN2 GPIO_NUM_2
#define PWM GPIO_NUM_4
#define STBY GPIO_NUM_5

void delay_seconds(unsigned int secs) {
    unsigned int end_time = time(NULL) + secs;
    while (time(NULL) < end_time) {
        // busy-wait loop
    }
}

void app_main(void)
{

    // Setting up
    tb6612fng_config_t tb6612fng_config = {0};
    tb6612fng_config.extend_pin = IN1;
    tb6612fng_config.retract_pin = IN2;
    tb6612fng_config.pwm_pin = PWM;

    tb6612fng_handle_t handle;

    tb6612fng_init(&handle, &tb6612fng_config);

    while(true) {
        tb6612fng_drive(handle, 128, true);
        vTaskDelay(pdMS_TO_TICKS(3000));
        tb6612fng_drive(handle, 128, false);
        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}


/*

static const char *TAG = "ACTUATOR";


// ESP32-S3 GPIO assignments
#define ACTUATOR_AIN1 GPIO_NUM_1
#define ACTUATOR_AIN2 GPIO_NUM_2
#define ACTUATOR_PWM  GPIO_NUM_4
#define ACTUATOR_STBY GPIO_NUM_5


void app_main(void)
{
    ESP_LOGI(TAG, "Starting linear actuator driver...");


    // Configure the TB6612FNG pins and PWM.
    tb6612fng_config_t actuator_config = {
        .ain1_pin = ACTUATOR_AIN1,
        .ain2_pin = ACTUATOR_AIN2,
        .pwm_pin = ACTUATOR_PWM,
        .stby_pin = ACTUATOR_STBY,

        .pwm_channel = LEDC_CHANNEL_0,
        .pwm_timer = LEDC_TIMER_0
    };


    // Create the driver handle.
    tb6612fng_handle_t actuator = NULL;


    // Initialize the driver.
    esp_err_t err = tb6612fng_init(
        &actuator,
        &actuator_config
    );

    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Initialization failed: %s",
                 esp_err_to_name(err));
        return;
    }

    ESP_LOGI(TAG, "Driver initialized successfully.");


    // Enable the motor driver.
    err = tb6612fng_enable(actuator, true);

    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to enable driver.");
        return;
    }


    ESP_LOGI(TAG, "Actuator ready.");


    // TEST COMMANDS
    // Leave these commented until you're ready to test.

    // Extend:
    // tb6612fng_drive(actuator, 150, true);

    // Retract:
    // tb6612fng_drive(actuator, 150, false);

    // Stop:
    // tb6612fng_stop(actuator);


    // Keep the actuator stopped.
    tb6612fng_stop(actuator);

    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

*/