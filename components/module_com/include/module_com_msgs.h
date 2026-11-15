#pragma once
#include <stdint.h>

// this file holds a bunch of enums that will hopefully make use and readbility better

// --- [NODES - who is sending (4 bits, 1-15)] ---
typedef enum {
    MC_NODE_NONE        = 0,    // not a real board
    MC_NODE_MAIN_BODY   = 1,
    MC_NODE_TEST        = 2,
    MC_NODE_MAX         = 15,
    // add more node types here as needed
} mc_node_t;


// --- [Message Types - what is being sent (7 bits, 0-127)] ---
// lower number = higher priority on the bus
typedef enum {
    MC_MSG_ESTOP        = 0,
    MC_MSG_HEARTBEAT    = 10,
    MC_MSG_PING         = 20,
    MC_MSG_PONG         = 21,
    MC_MSG_MAX          = 127,
    // add more message types here as needed
} mc_msg_t;

// --- [Payloads (< 8 bytes)] ---
// included here are some very basic payload types one could use
// NOTE: packed attribute just makes sure compiler does not add extra padding to struct
typedef struct __attribute__((packed)) { 
    uint32_t uptime_ms; // the sender's time since they booted
} mc_heartbeat_t;

typedef struct __attribute__((packed)) {
    uint32_t seq;   // sequence number, match a pong to same ping
    uint32_t sent_us; // sender's timestamp (echoed in pong)
} mc_ping_t;    // PONG uses the same layout

// compile-time checks: build fails if a payload is too big for a frame (i.e. check to make sure payloads are valid)
_Static_assert(sizeof(mc_heartbeat_t) <= 8, "mc_heartbeat_t too big");
_Static_assert(sizeof(mc_ping_t) <= 8, "mc_ping_t too big");
// add more asserts here as more payload types are created