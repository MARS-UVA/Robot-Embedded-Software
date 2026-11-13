#include "module_com.h"
#include "can.h"
#include "esp_check.h"
#include "esp_log.h"
#include <string.h>

// name used in ESP error message logging
static const char *TAG = "module_com";

// just an enum
static mc_node_t s_my_node = MC_NODE_NONE; // NONE = not initialized

typedef struct {
    mc_handler_t handler;
    void *ctx;
} mc_entry_t;

static mc_entry_t s_entries[MC_MAX_HANDLERS];
static uint32_t s_entry_count = 0;

// regsitered with CAN for every message type. 
// Translates a raw CAN frame into the module_com handler call,
// then forward to the user's handler.
static void mc_can_handler(const CAN_frame_t *frame, void *ctx)
{
    const mc_entry_t *entry = ctx;  // which registeration this is
    entry->handler(
        MC_ID_NODE(frame->id), 
        frame->data, 
        frame->len, 
        entry->ctx
    );
}

// // reply to every PING with a PONG carrying the same payload
// static void mc_on_ping(mc_node_t sender, const uint8_t *data, uint8_t len, void *ctx)
// {
//     // echo seq + sent_us back unchanged, so the pinger can match it and time the round trip
//     esp_err_t err = module_com_send(MC_MSG_PONG, data, len);
//     if (err != ESP_OK) {
//         ESP_LOGW(TAG, "pong to node %d failed: %s", sender, esp_err_to_name(err));
//     }
// }
static void mc_on_ping(mc_node_t sender, const uint8_t *data, uint8_t len, void *ctx);

esp_err_t module_com_init(mc_node_t my_node)
{
    // check for valid init call
    ESP_RETURN_ON_FALSE(my_node > MC_NODE_NONE && my_node <= MC_NODE_MAX, ESP_ERR_INVALID_ARG, TAG, "the node must be 1-15");
    ESP_RETURN_ON_FALSE(s_my_node == MC_NODE_NONE, ESP_ERR_INVALID_STATE, TAG, "already initialized");

    s_my_node = my_node;
    // built-in: answer every ping so any board can check that theyre alive
    esp_err_t ret = module_com_on(MC_MSG_PING, mc_on_ping, NULL);
    if (ret != ESP_OK) {    // somethin went wrong :(
        s_my_node = MC_NODE_NONE;
        return ret;
    }

    ESP_LOGI(TAG, "intitialized as node %d", my_node);
    return ESP_OK;
}

esp_err_t module_com_send(mc_msg_t type, const void *data, uint8_t len)
{
    // preliminary checks
    ESP_RETURN_ON_FALSE(s_my_node != MC_NODE_NONE, ESP_ERR_INVALID_STATE, TAG, "not initialized");
    ESP_RETURN_ON_FALSE(type <= MC_MSG_MAX, ESP_ERR_INVALID_ARG, TAG, "bad type %d", type);
    ESP_RETURN_ON_FALSE(len <= CAN_MAX_DATA_LEN, ESP_ERR_INVALID_ARG, TAG, "len %u > 8", len);
    ESP_RETURN_ON_FALSE(len == 0 || data != NULL, ESP_ERR_INVALID_ARG, TAG, "data is NULL");

    CAN_frame_t frame = {
        .id = MC_MAKE_ID(type, s_my_node),
        .extended = false,
        .len = len,
    };
    
    if (len > 0) {
        memcpy(frame.data, data, len);
    }
    return CAN_send(&frame, 0);
}

esp_err_t module_com_on(mc_msg_t type, mc_handler_t handler, void *ctx)
{
    // preliminary check
    ESP_RETURN_ON_FALSE(handler != NULL, ESP_ERR_INVALID_ARG, TAG, "handler is NULL");
    ESP_RETURN_ON_FALSE(type <= MC_MSG_MAX, ESP_ERR_INVALID_ARG, TAG, "bad type %d", type);
    ESP_RETURN_ON_FALSE(s_entry_count < MC_MAX_HANDLERS, ESP_ERR_NO_MEM, TAG, "handler table full");

    mc_entry_t *entry = &s_entries[s_entry_count];
    entry->handler = handler;
    entry->ctx = ctx;

    ESP_RETURN_ON_ERROR(CAN_register_handler(MC_MAKE_ID(type, 0), MC_TYPE_MASK, false, mc_can_handler, entry), TAG, "CAN registration failed");

    s_entry_count++; // CAN accept so increment
    return ESP_OK;
}

static void mc_on_ping(mc_node_t sender, const uint8_t *data, uint8_t len, void *ctx)
{
    // echo seq + sent us back unchanged, so the pinger can match it and time the round trip
    esp_err_t err = module_com_send(MC_MSG_PONG, data, len);
    // check if response was recieved
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "pong to node %d failed: %s", sender, esp_err_to_name(err));
    }
}