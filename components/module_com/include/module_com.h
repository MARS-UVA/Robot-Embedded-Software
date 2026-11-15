#pragma once // makes sure only compiled once
#include <stdint.h>
#include "esp_err.h"
#include "module_com_msgs.h"

// --- [ ID scheme --> ID = (type << 4) | node ] ---
#define MC_NODE_BITS    4
#define MC_NODE_MASK    0x00F   // bottom 4 bits = sender
#define MC_TYPE_MASK    0x7F0   // top 7 bits = message type
#define MC_MAKE_ID(type, node) ((((uint32_t)(type)) << MC_NODE_BITS) | ((node) & MC_NODE_MASK))
#define MC_ID_TYPE(id) ((mc_msg_t)((id) >> MC_NODE_BITS))
#define MC_ID_NODE(id) ((mc_node_t) ((id) & MC_NODE_MASK))

// the max module_com_on() registrations
#define MC_MAX_HANDLERS 8 


// called when a message of a registered type arrives
typedef void (*mc_handler_t)(mc_node_t sender, const uint8_t *data, uint8_t len, void *ctx);

// set the boards node ID.
// call after CAN_init() and before CAN_start().
esp_err_t module_com_init(mc_node_t my_node);

// send a message of the given type from the node
esp_err_t module_com_send(mc_msg_t type, const void *data, uint8_t len);

// receive every message of this type from any node
// should be called before CAN_start
esp_err_t module_com_on(mc_msg_t type, mc_handler_t handler, void *ctx);
