//allows file to be included only once
#pragma once
#include <stdint.h>
#include <stdbool.h>    // bool types
#include <esp_err.h>    // provides error types

typedef struct {
    int         tx_gpio;
    int         rx_gpio;
    uint32_t    bitrate;
    uint32_t    tx_queue_depth;
    bool        self_test;          // test without ACK
} CAN_config_t;


#define CAN_MAX_DATA_LEN 8
typedef struct {
    uint32_t    id;
    bool        extended;
    uint8_t     len;
    uint8_t     data[CAN_MAX_DATA_LEN];
} CAN_frame_t;

esp_err_t CAN_init(const config *);
esp_err_t CAN_deinit();
esp_err_t CAN_start();
esp_err_t CAN_stop();


//static IRAM_ATTR bool twai_sender_tx_done_callback(twai_node_handle_t handle, const twai_tx_done_event_data_t *edata, void *user_ctx);
//static IRAM_ATTR bool twai_sender_on_error_callback(twai_node_handle_t handle, const twai_error_event_data_t *edata, void *user_ctx);
