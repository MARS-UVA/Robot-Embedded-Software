/*
 * main.c
 *
 * Example application showing how to configure and read the BME280.
 * Hardware settings are collected in one bme280_sensor_config_t structure,
 * so a teammate can change pins, address, I2C speed, or sampling options here.
 */

#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

// Include both the bus and sensor wrappers
#include "i2c_bus.h"
#include "BME280_Sensor.h"

static const char *TAG = "Main_App";

/* Edit these values to match the physical wiring and sensor board.
 * BME280_SENSOR_CONFIG_DEFAULT() starts with SDA=GPIO10, SCL=GPIO11,
 * address 0x76, and a 100 kHz I2C clock. */
static const bme280_sensor_config_t sensor_config = BME280_SENSOR_CONFIG_DEFAULT();

static void bme280_reading_task(void *argument)
{
    bme280_reading_t reading;

    ESP_LOGI(TAG, "Starting BME280 sensor task");

    /* 1. First, set up the shared I2C master bus configuration structure using values 
     * pulled from your team's global sensor config profile. */
    i2c_config_t bus_config = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = sensor_config.sda_gpio,
        .scl_io_num = sensor_config.scl_gpio,
        .sda_pullup_en = sensor_config.enable_internal_pullups ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE,
        .scl_pullup_en = sensor_config.enable_internal_pullups ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE,
        .master.clk_speed = sensor_config.i2c_frequency_hz,
        .clk_flags = 0,
    };

    /* Create and spin up the actual I2C master hardware port context */
    i2c_bus_handle_t shared_bus = i2c_bus_create(sensor_config.i2c_port, &bus_config);
    if (shared_bus == NULL) {
        ESP_LOGE(TAG, "Failed to initialize underlying I2C master bus hardware!");
        vTaskDelete(NULL);
        return;
    }

    /* 2. Initialize the BME280 sensor state, handing it the live, active I2C bus context */
    esp_err_t err = bme280_sensor_init(shared_bus, &sensor_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "BME280 initialization failed: %s", esp_err_to_name(err));
        // Clean up the created bus handle context if initialization completely fails
        i2c_bus_delete(&shared_bus);
        vTaskDelete(NULL);
        return;
    }

    /* 3. Begin reading loop */
    while (1) {
        err = bme280_sensor_read(&reading);

        if (err == ESP_OK && reading.is_valid) {
            ESP_LOGI(TAG, "---------------- BME280 ----------------");
            ESP_LOGI(TAG, "Temperature: %.2f C", reading.temperature_c);
            ESP_LOGI(TAG, "Humidity:    %.2f %%", reading.humidity_pct);
            ESP_LOGI(TAG, "Pressure:    %.2f hPa", reading.pressure_hpa);
        } else {
            ESP_LOGW(TAG, "Could not get a complete sensor reading: %s",
                     esp_err_to_name(err));
        }

        /* Wait two seconds before the next measurement. */
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

void app_main(void)
{
    /* Start the sensor task. */
    BaseType_t task_result = xTaskCreate(
        bme280_reading_task,  /* Function the task runs */
        "bme280_task",        /* Task name for debugging */
        4096,                 /* Stack size in bytes */
        NULL,                 /* No task argument needed */
        5,                    /* Task priority */
        NULL                  /* Task handle not needed */
    );

    if (task_result != pdPASS) {
        ESP_LOGE(TAG, "Failed to create BME280 task");
    }
}
