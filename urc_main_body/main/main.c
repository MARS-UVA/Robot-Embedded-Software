#include "can.h"

// Placeholder pins until the transceiver wiring is decided
#define CAN_TX_GPIO     4
#define CAN_RX_GPIO     5
#define CAN_BITRATE     500000

void app_main(void)
{
    CAN_config_t can_cfg = {
        .tx_gpio = CAN_TX_GPIO,
        .rx_gpio = CAN_RX_GPIO,
        .bitrate = CAN_BITRATE,
        .tx_queue_depth = 10,
        .self_test = true,      // single board, no transceiver needed
    };

    ESP_ERROR_CHECK(CAN_init(&can_cfg));
    ESP_ERROR_CHECK(CAN_start());
}
