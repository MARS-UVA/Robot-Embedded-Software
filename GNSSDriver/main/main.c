#include "maxm10s.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"


maxm10s_config_t config = {
    .sda_pin = GPIO_NUM_4,
    .scl_pin = GPIO_NUM_5,
};

maxm10s_dev_handle_t dev;

maxm10s_data_t data;


void app_main(void) {

    maxm10s_init(&config, &dev);

    while(1){
        maxm10s_read(&dev, &data);
        vTaskDelay(1000/portTICK_PERIOD_MS);
        printMax10sData(data);
    }


}
