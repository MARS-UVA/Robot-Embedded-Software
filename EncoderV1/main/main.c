#include <stdio.h>
#include <stddef.h>
#include "../../components/rev_v2_encoder/include/rev_v2_encoder.h"
#include "driver/gpio.h"
#include "driver/mcpwm_cap.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"

void app_main(void)
{
    rev_v2_encoder_handle_t handle;
    rev_v2_encoder_init(&handle, GPIO_NUM_6);
    double angle;

    while (1) {
        rev_v2_encoder_read(handle, &angle);
        printf("Angle: %.2f deg\n", angle);
        vTaskDelay(pdMS_TO_TICKS(100));
    }

}
