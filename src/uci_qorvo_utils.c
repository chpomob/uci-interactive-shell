#include "../include/uci_qorvo_utils.h"
#include "../include/uci_packet_utils.h"
#include <string.h>

int uci_qorvo_decode_device_boot(const unsigned char* payload,
                                 size_t payload_len,
                                 uci_qorvo_device_boot_t* out) {
    if (!payload || !out || payload_len == 0) {
        return -1;
    }

    out->reason = payload[0];
    return 0;
}

int uci_qorvo_decode_psdu_report(const unsigned char* payload,
                                 size_t payload_len,
                                 uci_qorvo_psdu_report_t* out) {
    if (!payload || !out || payload_len < 4) {
        return -1;
    }

    out->session_handle = read_u32_le(payload);
    out->frame_count = 0;

    size_t cursor = 4;
    while (cursor + 3 <= payload_len) {
        if (out->frame_count >= UCI_QORVO_MAX_PSDU_FRAMES) {
            return -1;
        }

        uint8_t index = payload[cursor++];
        uint16_t length = read_u16_le(&payload[cursor]);
        cursor += 2;

        if (cursor + length > payload_len || length > UCI_QORVO_MAX_PSDU_LENGTH) {
            return -1;
        }

        uci_qorvo_psdu_frame_t* frame = &out->frames[out->frame_count++];
        frame->index = index;
        frame->length = length;
        if (length > 0) {
            memcpy(frame->data, &payload[cursor], length);
        }
        cursor += length;
    }

    if (cursor != payload_len) {
        return -1;
    }

    return 0;
}
