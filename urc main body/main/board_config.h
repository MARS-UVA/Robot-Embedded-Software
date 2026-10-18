#pragma once
#include "sdkconfig.h"
#include "driver/gpio.h"

#define BOARD_CAN_TX_PIN GPIO_NUM_4
#define BOARD_CAN_RX_PIN GPIO_NUM_5
#define BOARD_HELLO_INTERVAL_MS 2000
#define BOARD_PEER_TIMEOUT_MS 6000
#define BOARD_POLL_INTERVAL_MS 50

#ifdef CONFIG_URC_BOARD_B
#define BOARD_NODE_ID 0x102U
#define BOARD_PEER_ID 0x101U
#else
#define BOARD_NODE_ID 0x101U
#define BOARD_PEER_ID 0x102U
#endif
