#include <stdio.h>
#include "DRV8711.h"
#include "hal/gpio_types.h"
#include "freertos/FreeRTOS.h"

void app_main(void)
{
    drv8711_config_t config = {
        .spi_miso_pin = GPIO_NUM_17,
        .spi_mosi_pin = GPIO_NUM_16,
        .spi_clk_pin = GPIO_NUM_10,
        .spi_cs_pin = GPIO_NUM_9,
        .step_pin = GPIO_NUM_12,
        .dir_pin = GPIO_NUM_11,
        .sleep_pin = GPIO_NUM_8,
        .fault_pin = GPIO_NUM_15,
        .reset_pin = GPIO_NUM_5,
    };
    drv8711_handle_t handle;
    drv8711_init(&handle, &config);

    while(true){
        printf("running task \n");
        step(handle);
        esp_rom_delay_us(100 - 6);
        vTaskDelay(1);
    }
}
