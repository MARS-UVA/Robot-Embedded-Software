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
#define CAN_STD_ID_MAX   0x7FF          // largest 11-bit standard ID
#define CAN_EXT_ID_MAX   0x1FFFFFFF     // largest 29-bit extended ID

typedef struct {
    uint32_t    id;
    bool        extended;
    uint8_t     len;
    uint8_t     data[CAN_MAX_DATA_LEN];
} CAN_frame_t;

typedef struct {
    uint32_t    tx_success;         // frames sent and acknowledged
    uint32_t    tx_failed;          // frames the hardware gave up on
} CAN_stats_t;

/**
 * @brief Create the CAN node with the given settings
 *
 * @note The node stays idle (not on the bus) until CAN_start() is called
 *
 * @param config Pins, bitrate, TX queue depth and self-test flag for the node
 *
 * @return
 *      - ESP_OK: Success
 *      - ESP_ERR_INVALID_ARG: config is NULL, bitrate or tx_queue_depth is 0, or the driver rejected a setting (e.g. invalid pin)
 *      - ESP_ERR_INVALID_STATE: Node already initialized
 *      - ESP_ERR_NO_MEM: Not enough memory to create the node
 *      - ESP_ERR_NOT_FOUND: No free TWAI controller
 */
esp_err_t CAN_init(const CAN_config_t *config);

/**
 * @brief Delete the CAN node and free its resources
 *
 * @note The node must be stopped with CAN_stop() first. CAN_init() can be called again afterwards.
 *
 * @return
 *      - ESP_OK: Success
 *      - ESP_ERR_INVALID_STATE: Node not initialized, or still started
 */
esp_err_t CAN_deinit(void);

/**
 * @brief Start the node so it can send, receive and acknowledge frames on the bus
 *
 * @return
 *      - ESP_OK: Success
 *      - ESP_ERR_INVALID_STATE: Node not initialized, or already started
 */
esp_err_t CAN_start(void);

/**
 * @brief Stop the node. It leaves the bus but keeps its configuration.
 *
 * @note CAN_start() can be called again to rejoin the bus
 *
 * @return
 *      - ESP_OK: Success
 *      - ESP_ERR_INVALID_STATE: Node not initialized, or not started
 */
esp_err_t CAN_stop(void);

/**
 * @brief Queue a frame to be sent on the bus
 *
 * @note The frame is copied, so the caller's frame can be reused as soon as this returns.
 *       ESP_OK means the frame was queued, not that it was delivered. See CAN_get_stats().
 *
 * @param frame      Frame to send
 * @param timeout_ms How long to wait for space if the transmit queue is full. 0 = don't wait, -1 = wait forever
 *
 * @return
 *      - ESP_OK: Frame queued
 *      - ESP_ERR_INVALID_ARG: frame is NULL, len > CAN_MAX_DATA_LEN, or id too large for its format
 *      - ESP_ERR_INVALID_STATE: Node not started, or in bus-off
 *      - ESP_ERR_TIMEOUT: Transmit queue still full after timeout_ms
 */
esp_err_t CAN_send(const CAN_frame_t *frame, int timeout_ms);

/**
 * @brief Read the transmit counters since CAN_init()
 *
 * @param stats Filled with the current counts
 *
 * @return
 *      - ESP_OK: Success
 *      - ESP_ERR_INVALID_ARG: stats is NULL
 */
esp_err_t CAN_get_stats(CAN_stats_t *stats);


//static IRAM_ATTR bool twai_sender_tx_done_callback(twai_node_handle_t handle, const twai_tx_done_event_data_t *edata, void *user_ctx);
//static IRAM_ATTR bool twai_sender_on_error_callback(twai_node_handle_t handle, const twai_error_event_data_t *edata, void *user_ctx);
