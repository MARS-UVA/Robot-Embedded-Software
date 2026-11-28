#include "arm_controller.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void app_main(void)
{
    xTaskCreate(
        arm_controller_task,    // Task function
        "arm_controller_task",  // Name for debugging
        2048,                   // Stack size in BYTES (ESP-IDF specific)
        NULL,                   // Parameter passed to task
        5,                      // Priority (higher number = higher priority)
        NULL                    // Optional task handle
    );
}
