#include <stdio.h>
#include "SEN0114.h"
#include "freertos/FreeRTOS.h"

void app_main(void)
{
    SEN0114_init();
    while (true)
    {
        int reading = SEN0114_read();
        printf("Reading: %d\n", reading);
        vTaskDelay(pdMS_TO_TICKS(1000)); // Delay for 1 second
    }
}
