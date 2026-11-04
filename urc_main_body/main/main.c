#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "BME280_Sensor.h"

// Define your ESP32 hardware pins connecting to the Adafruit board
#define I2C_SDA_GPIO    10  // Change these to match your actual wiring
#define I2C_SCL_GPIO    11  // Change these to match your actual wiring

static const char *MAIN_TAG = "Main_App";

void bme280_reading_task(void *pvParameters) {
    bme280_telemetry_t sensor_data;

    ESP_LOGI(MAIN_TAG, "Initializing BME280 sensor via pins SDA: %d, SCL: %d", I2C_SDA_GPIO, I2C_SCL_GPIO);

    // Call your initialization function using the primary address defined in your header file
    if (!network_bme280_init(I2C_SDA_GPIO, I2C_SCL_GPIO, BME280_I2C_ADDR_PRIMARY)) {
        ESP_LOGE(MAIN_TAG, "Sensor initialization failed! Terminating task loop.");
        vTaskDelete(NULL);
        return;
    }

    // Main telemetry logging loop
    while (1) {
        // Query the hardware registers
        network_bme280_get_readings(&sensor_data);

        // Verify the status flag before logging
        if (sensor_data.is_valid) {
            ESP_LOGI(MAIN_TAG, "--- Telemetry Report ---");
            ESP_LOGI(MAIN_TAG, "Temperature: %.2f °C", sensor_data.temperature);
            ESP_LOGI(MAIN_TAG, "Humidity:    %.2f %%", sensor_data.humidity);
            ESP_LOGI(MAIN_TAG, "Pressure:    %.2f hPa", sensor_data.pressure);
        } else {
            ESP_LOGW(MAIN_TAG, "Telemetry data invalid or I2C bus failed to respond.");
        }

        // Delay for 2000 milliseconds (2 seconds) before polling again
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

void app_main(void) {
    // Spawn the sensor reading thread into the FreeRTOS scheduler
    xTaskCreate(
        bme280_reading_task,    // Task function pointer
        "bme280_task",          // Structural task identifier name
        4096,                   // Stack size safely handling float calculations & ESP_LOG
        NULL,                   // Task input parameters
        5,                      // Task Execution Priority
        NULL                    // Task handler tracking reference
    );
}