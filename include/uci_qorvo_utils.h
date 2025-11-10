#ifndef UCI_QORVO_UTILS_H
#define UCI_QORVO_UTILS_H

#include <stddef.h>
#include <stdint.h>

#define UCI_QORVO_MAX_PSDU_FRAMES 8
#define UCI_QORVO_MAX_PSDU_LENGTH 256

typedef struct {
    uint8_t reason;
} uci_qorvo_device_boot_t;

typedef struct {
    uint8_t index;
    uint16_t length;
    uint8_t data[UCI_QORVO_MAX_PSDU_LENGTH];
} uci_qorvo_psdu_frame_t;

typedef struct {
    uint32_t session_handle;
    uint8_t frame_count;
    uci_qorvo_psdu_frame_t frames[UCI_QORVO_MAX_PSDU_FRAMES];
} uci_qorvo_psdu_report_t;

int uci_qorvo_decode_device_boot(const unsigned char* payload,
                                 size_t payload_len,
                                 uci_qorvo_device_boot_t* out);

int uci_qorvo_decode_psdu_report(const unsigned char* payload,
                                 size_t payload_len,
                                 uci_qorvo_psdu_report_t* out);

#endif // UCI_QORVO_UTILS_H
