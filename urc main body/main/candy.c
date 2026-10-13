#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/twai.h"
#include "esp_log.h"

static const char *TAG = "TWAI_NODE";

// --- Hardware & Configuration Pins ---
#define TX_PIN          GPIO_NUM_4
#define RX_PIN          GPIO_NUM_5

// --- Node Swapping Identification ---
// Board A config: NODE_ID 0x101, TARGET_ID 0x102
// Board B config: NODE_ID 0x102, TARGET_ID 0x101
#define NODE_ID         0x101   
#define TARGET_ID       0x102   

// FreeRTOS Task Handles
TaskHandle_t rx_task_handle = NULL;
TaskHandle_t tx_task_handle = NULL;

/**
 * @brief FreeRTOS Task dedicated to continuously listening for incoming frames.
 */
void twai_rx_task(void *pvParameters) {
    ESP_LOGI(TAG, "Receiver Task Started.");
    twai_message_t rx_msg;

    while (1) {
        // Non-blocking wait: blocks the task until a message arrives (portMAX_DELAY)
        if (twai_receive(&rx_msg, portMAX_DELAY) == ESP_OK) {
            
            // Only process standard data frames that match our targeted peer ID
            if (!(rx_msg.rtr) && (rx_msg.identifier == TARGET_ID)) {
                ESP_LOGI(TAG, "Received frame from ID: 0x%03X | Data length: %d", (unsigned int)rx_msg.identifier, rx_msg.data_length_code);
                
                // Print payload data in hex and characters
                printf("Payload: ");
                for (int i = 0; i < rx_msg.data_length_code; i++) {
                    printf("0x%02X ", rx_msg.data[i]);
                }
                printf("\n");
            }
        }
    }
}

/**
 * @brief FreeRTOS Task dedicated to periodically packing and transmitting frames.
 */
void twai_tx_task(void *pvParameters) {
    ESP_LOGI(TAG, "Transmitter Task Started.");
    uint32_t counter = 0;

    while (1) {
        twai_message_t tx_msg;
        
        // Configure standard frame details
        tx_msg.identifier = NODE_ID;
        tx_msg.extd = 0;             // Standard 11-bit ID frame
        tx_msg.rtr = 0;              // Data frame (not a remote transmission request)
        tx_msg.data_length_code = 5; // Sending 5 bytes of data
        
        // Pack data bytes (CAN 2.0B max limit is 8 bytes total)
        tx_msg.data[0] = 'H';
        tx_msg.data[1] = 'E';
        tx_msg.data[2] = 'L';
        tx_msg.data[3] = 'L';
        tx_msg.data[4] = 'O';

        // Queue packet execution to the TWAI Controller
        esp_err_t res = twai_transmit(&tx_msg, pdMS_TO_TICKS(1000));
        if (res == ESP_OK) {
            ESP_LOGI(TAG, "Successfully queued packet #%u for transmission", (unsigned int)counter++);
        } else {
            ESP_LOGE(TAG, "Failed to transmit packet: %s", esp_err_to_name(res));
        }

        // Send a packet every 2000 milliseconds (2 seconds)
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

void app_main(void) {
    // 1. Structural Configuration Setup
    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(TX_PIN, RX_PIN, TWAI_MODE_NORMAL);
    twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS(); // Set bus speed to 500 kbps
    twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

    // 2. Install and Start TWAI Driver Layer
    if (twai_driver_install(&g_config, &t_config, &f_config) == ESP_OK) {
        ESP_LOGI(TAG, "TWAI Driver installed successfully.");
    } else {
        ESP_LOGE(TAG, "Failed to install TWAI Driver.");
        return;
    }

    if (twai_driver_start() == ESP_OK) {
        ESP_LOGI(TAG, "TWAI Driver started.");
    } else {
        ESP_LOGE(TAG, "Failed to start TWAI Driver.");
        return;
    }

    // 3. Spawn Independent FreeRTOS Worker Tasks
    // Split execution priorities: Processing incoming reads (RX) gets priority over TX pacing
    xTaskCreatePinnedToCore(twai_rx_task, "twai_rx_task", 4096, NULL, 6, &rx_task_handle, 1);
    xTaskCreatePinnedToCore(twai_tx_task, "twai_tx_task", 4096, NULL, 5, &tx_task_handle, 1);
}
