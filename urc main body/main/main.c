#include <stdio.h>
#include "DRV8711.h"
#include "hal/gpio_types.h"
#include "freertos/FreeRTOS.h"

void app_main(void)
{
    spi_bus_config_t buscfg = {
        .miso_io_num = GPIO_NUM_17,
        .mosi_io_num = GPIO_NUM_16,
        .sclk_io_num = GPIO_NUM_10,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
    };
    spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO);

    drv8711_config_t config = {
        .spi_cs_pin = GPIO_NUM_9,
        .step_pin = GPIO_NUM_12,
        .dir_pin = GPIO_NUM_11,
        .sleep_pin = GPIO_NUM_8,
        .fault_pin = GPIO_NUM_15,
        .reset_pin = GPIO_NUM_5,
        .spi_host = SPI2_HOST,
    };
    drv8711_handle_t handle;
    drv8711_init(&handle, &config);

    while(true) {
        printf("running task \n");
        drv8711_step(handle);
        esp_rom_delay_us(100);
        vTaskDelay(1);
    }
}
