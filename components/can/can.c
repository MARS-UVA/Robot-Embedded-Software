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
    ESP_RETURN_ON_FALSE(s_node == NULL,  ESP_ERR_INVALID_STATE, TAG, "node already initialized");
    ESP_RETURN_ON_FALSE(!s_started, ESP_ERR_INVALID_STATE, TAG, "cannot initialize an already running node");

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
        .fail_retry_cnt = -1                    // dont retry on filed frame
    };
}

esp_err_t CAN_deinit(void) 
{
    ESP_RETURN_ON_FALSE(s_node != NULL, ESP_ERR_INVALID_STATE, TAG, "cannot deinitialize null CAN node");
    ESP_RETURN_ON_FALSE(!s_started, ESP_ERR_INVALID_STATE, TAG, "cannot deinitialize running CAN node");

    twai_node_delete(s_node);
    s_node = NULL;

    return ESP_OK; // change this
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

// Refactor to match other code
esp_err_t CAN_stop(void) {
    twai_status_info_t status_info;  //Defines status & then we will pour info into it
    
    // 1. Get the current status of the TWAI driver
    esp_err_t err = twai_get_status_info(&status_info);
    if (err != ESP_OK) {
        printf("Error getting TWAI status. Driver might not be installed.\n");
        return err;
    }

    // 2. Check if the driver is already stopped
    if (status_info.state == TWAI_STATE_STOPPED) {
        printf("TWAI driver is already stopped.\n");
        return ESP_OK; // Safe exit, no action needed
    }

    // 3. Stop the driver safely if it is actively running
    if (status_info.state == TWAI_STATE_RUNNING) {
        printf("TWAI driver is running. Stopping it now...\n");
        return twai_stop();
    }

    // 4. Handle edge cases (e.g., if the driver is stuck in Bus-Off or Recovery)
    if (status_info.state == TWAI_STATE_BUS_OFF || status_info.state == TWAI_STATE_RECOVERY) {
        printf("TWAI driver is in a faulted/recovery state (%d). Stopping driver.\n", status_info.state);
        return twai_stop(); 
    }

    return ESP_ERR_INVALID_STATE;
}