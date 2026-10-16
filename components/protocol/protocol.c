#include "protocol.h"
#include <string.h>

static const uint8_t hello[] = {'H', 'E', 'L', 'L', 'O'};

esp_err_t protocol_make_hello(uint32_t sender_id, can_frame_t *frame)
{
    if (!frame || sender_id > 0x7FFU) return ESP_ERR_INVALID_ARG;
    *frame = (can_frame_t) {.id = sender_id, .length = sizeof(hello)};
    memcpy(frame->data, hello, sizeof(hello));
    return ESP_OK;
}

bool protocol_is_peer_hello(const can_frame_t *frame, uint32_t peer_id)
{
    return frame && peer_id <= 0x7FFU && !frame->extended && !frame->remote &&
        frame->id == peer_id && frame->length == sizeof(hello) &&
        memcmp(frame->data, hello, sizeof(hello)) == 0;
}
