#include "can.h"
#include "esp_check.h"          // ESP_RETURN_ON_FALSE / ESP_RETURN_ON_ERROR
#include "esp_log.h"            // ESP_LOGI / ESP_LOGE for printing
#include "esp_twai.h"           // twai_node_enable, _disable, _delete, ..
#include "esp_twai_onchip.h"    // twai_new_node_onchip + its config struct

// Used with ESP log (DEBUG)
static const char *TAG = "CAN";

// Handle for CAN/twai. shared between function calls
static twai_node_handle_t s_node = NULL;
static bool s_started = false; // node enabled status


//static IRAM_ATTR bool twai_sender_tx_done_callback(twai_node_handle_t handle, const twai_tx_done_event_data_t *edata, void *user_ctx);
//static IRAM_ATTR bool twai_sender_on_error_callback(twai_node_handle_t handle, const twai_error_event_data_t *edata, void *user_ctx);

esp_err_t CAN_init(const CAN_config_t *config)
{
    ESP_RETURN_ON_FALSE(config != NULL, ESP_ERR_INVALID_ARG, TAG, "config is NULL");
    ESP_RETURN_ON_FALSE(config->bitrate > 0, ESP_ERR_INVALID_ARG, TAG, "bitrate must be > 0");
    ESP_RETURN_ON_FALSE(s_node == NULL,  ESP_ERR_INVALID_STATE, TAG, "node already initialized");

    twai_onchip_node_config_t node_cfg =
    {
        .io_cfg.tx = config->tx_gpio,
        .io_cfg.rx = config->rx_gpio,
        .io_cfg.quanta_clk_out = -1,
        .io_cfg.bus_off_indicator = -1,
        .bit_timing.bitrate = config->bitrate,
        .tx_queue_depth = config->tx_queue_depth,
        .flags.enable_self_test = config->self_test,
        .flags.enable_loopback = config->self_test,
        .fail_retry_cnt = -1                    // retry a failed frame forever
    };

    // driver copies node_cfg and writes the new handle into s_node
    ESP_RETURN_ON_ERROR(twai_new_node_onchip(&node_cfg, &s_node), TAG, "failed to create node");
    
    ESP_LOGI(TAG, "initialized: tx=%d rx=%d bitrate=%lu%s",
             config->tx_gpio, config->rx_gpio, config->bitrate,
             config->self_test ? " (self-test)" : "");
    return ESP_OK;
}

esp_err_t CAN_deinit(void)
{
    ESP_RETURN_ON_FALSE(s_node != NULL, ESP_ERR_INVALID_STATE, TAG, "cannot deinitialize null CAN node");
    ESP_RETURN_ON_FALSE(!s_started, ESP_ERR_INVALID_STATE, TAG, "cannot deinitialize running CAN node");

    ESP_RETURN_ON_ERROR(twai_node_delete(s_node), TAG, "delete failed");
    s_node = NULL;

    ESP_LOGI(TAG, "deinitialized");
    return ESP_OK;
}

esp_err_t CAN_start(void)
{
    // check for valid state before starting CAN
    ESP_RETURN_ON_FALSE(s_node != NULL, ESP_ERR_INVALID_STATE, TAG, "not initialized");
    ESP_RETURN_ON_FALSE(!s_started, ESP_ERR_INVALID_STATE, TAG, "already started");
    
    ESP_RETURN_ON_ERROR(twai_node_enable(s_node), TAG, "enable failed");

    s_started = true;
    ESP_LOGI(TAG, "started");
    return ESP_OK;
}

esp_err_t CAN_stop(void)
{
    // check for valid state before stopping CAN
    ESP_RETURN_ON_FALSE(s_node != NULL, ESP_ERR_INVALID_STATE, TAG, "not initialized");
    ESP_RETURN_ON_FALSE(s_started, ESP_ERR_INVALID_STATE, TAG, "not started");

    ESP_RETURN_ON_ERROR(twai_node_disable(s_node), TAG, "disable failed");

    s_started = false;
    ESP_LOGI(TAG, "stopped");
    return ESP_OK;
}