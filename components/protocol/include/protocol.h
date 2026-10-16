#pragma once
#include "can.h"

// IDs identify CAN message streams, not destination addresses.
// HELLO retains its original five-byte wire format.
esp_err_t protocol_make_hello(uint32_t sender_id, can_frame_t *frame);
bool protocol_is_peer_hello(const can_frame_t *frame, uint32_t peer_id);
