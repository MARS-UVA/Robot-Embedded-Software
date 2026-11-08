/**
 * @file DRV8711.c
 * @author Cole Luba
 * @author Rishul Sharma
 * @author Josalin Wang
 * @brief Source file for DRV8711 driver.
 * @version 0.1
 * 
 * @details
 */

#include "DRV8711.h"

#include <stdlib.h>

#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_check.h"
#include "esp_log.h"
#include "esp_rom_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/* Full definition of the opaque type declared in DRV8711.h */
struct drv8711_t {
    gpio_num_t step_pin;
    gpio_num_t dir_pin;
    gpio_num_t enable_pin;
    gpio_num_t fault_pin;
    gpio_num_t reset_pin;
    gpio_num_t sleep_pin;
    spi_device_handle_t spi_handle;
};


static esp_err_t drv8711_write_register(drv8711_handle_t handle, uint8_t address, uint16_t data) {
    uint16_t packet = ((uint16_t)(address & 0x07) << 12) | (data & 0x0FFF);
    packet = (packet >> 8) | (packet << 8); // Swap to Big-Endian

    spi_transaction_t t = {
        .flags = 0,
        .length = 16,
        .tx_buffer = &packet,
        .rx_buffer = NULL
    };
    return spi_device_transmit(handle->spi_handle, &t);
}

esp_err_t drv8711_init(drv8711_handle_t *handle, const drv8711_config_t *config)
{
    if (handle == NULL || config == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    drv8711_handle_t dev = calloc(1, sizeof(struct drv8711_t));
    if (dev == NULL) {
        return ESP_ERR_NO_MEM;
    }
    dev->step_pin   = config->step_pin;
    dev->dir_pin    = config->dir_pin;
    dev->enable_pin = config->enable_pin;
    dev->fault_pin  = config->fault_pin;
    dev->reset_pin  = config->reset_pin;
    dev->sleep_pin  = config->sleep_pin;

    // RESET, nSLEEP, STEP and DIR are all outputs
    gpio_config_t io_conf = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL << config->reset_pin) | (1ULL << config->sleep_pin) |
                        (1ULL << config->step_pin)  | (1ULL << config->dir_pin),
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    esp_err_t err = gpio_config(&io_conf);
    if (err != ESP_OK) {
        goto fail;
    }
    gpio_set_level(dev->step_pin, 0);
    gpio_set_level(dev->dir_pin, 0);

    // Pulse RESET (active high)
    gpio_set_level(dev->reset_pin, 1);
    vTaskDelay(pdMS_TO_TICKS(1));
    gpio_set_level(dev->reset_pin, 0);

    spi_bus_config_t buscfg = {
        .miso_io_num = config->spi_miso_pin,
        .mosi_io_num = config->spi_mosi_pin,
        .sclk_io_num = config->spi_clk_pin,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
    };
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 2*1000*1000,
        .mode = 0, // SPI mode 0
        .spics_io_num = config->spi_cs_pin,
        .flags = SPI_DEVICE_POSITIVE_CS,
        .queue_size = 1,
    };
    err = spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO);
    if (err != ESP_OK) {
        goto fail;
    }
    err = spi_bus_add_device(SPI2_HOST, &devcfg, &dev->spi_handle);
    if (err != ESP_OK) {
        spi_bus_free(SPI2_HOST);
        goto fail;
    }

    // Wake the driver (nSLEEP high)
    gpio_set_level(dev->sleep_pin, 1);
    vTaskDelay(pdMS_TO_TICKS(5));

    err = drv8711_write_register(dev, HPSD_REG_CTRL, 0x0C11);
    if (err != ESP_OK) {
        spi_bus_remove_device(dev->spi_handle);
        spi_bus_free(SPI2_HOST);
        goto fail;
    }

    *handle = dev;
    return ESP_OK;

fail:
    free(dev);
    *handle = NULL;
    return err;
}

esp_err_t step(drv8711_handle_t handle)
{
    esp_err_t err = gpio_set_level(handle->step_pin, 1);
    esp_rom_delay_us(3);

    gpio_set_level(handle->step_pin, 0);
    esp_rom_delay_us(3);
    return err;
}

esp_err_t setDirection(drv8711_handle_t handle, bool dir)
{
    esp_rom_delay_us(1);
    esp_err_t err = gpio_set_level(handle->dir_pin, dir);
    esp_rom_delay_us(1);
    return err;
}
