/**
 * @file DRV8711.c
 * @author Rishul Sharma
 * @author Josalin Wang
 * @brief Source file for DRV8711 driver.
 * @version 0.1
 * 
 * @details
 */

#include "DRV8711.h"

#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_rom_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

typedef struct {
    gpio_num_t spi_cs_pin;
    gpio_num_t spi_clk_pin;
    gpio_num_t spi_mosi_pin;
    gpio_num_t spi_miso_pin;
    gpio_num_t step_pin;
    gpio_num_t dir_pin;
    gpio_num_t enable_pin;
    gpio_num_t fault_pin;
    gpio_num_t reset_pin;
    spi_device_handle_t spi_handle;
} drv8711_handle_t;

esp_err_t drv8711_init(drv8711_handle_t *handle, const drv8711_config_t *config)
{
    *handle = (drv8711_handle_t)calloc(1, sizeof(struct drv8711_t));

    gpio_config_t io_conf = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL << config->step_pin) 
                      | (1ULL << config->dir_pin) 
                      | (1ULL << config->enable_pin) 
                      | (1ULL << config->reset_pin),
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_config);
    gpio_set_level(config->enable_pin, 1); // Disable the driver initially
    vTaskDelay(pdMS_TO_TICKS(10)); // Wait for the driver to stabilize
    gpio_set_level(config->reset_pin, 0); // Reset the driver

    spi_bus_config_t buscfg = {
        .miso_io_num = config->spi_miso_pin,
        .mosi_io_num = config->spi_mosi_pin,
        .sclk_io_num = config->spi_clk_pin,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 0
    };
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 1000000, // 1 MHz
        .mode = 0, // SPI mode 0
        .spics_io_num = config->spi_cs_pin,
        .queue_size = 1,
    };

    esp_err_t err = spi_bus_initialize(HSPI_HOST, &buscfg, 1);
    if (err != ESP_OK) {
        return err;
    }

    err = spi_bus_add_device(HSPI_HOST, &devcfg, &handle->spi_handle);
    if (err != ESP_OK) {
        return err;
    }

    gpio_set_level(config->enable_pin, 0); // Enable the driver
    

}
