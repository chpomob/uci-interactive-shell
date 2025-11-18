#include <stdio.h>
#include <stdbool.h>
#include "../include/uci.h"
#include "../include/uci_functions.h"
#include "../include/uci_pdl.h"
#include "uci_command_utils.h"
#include "../include/uci_cmd_session.h"

// Helper function to encode session_id in little-endian format
static void encode_session_id_le(unsigned char* payload, uint32_t session_id) {
    payload[0] = (unsigned char)(session_id & 0xFF);
    payload[1] = (unsigned char)((session_id >> 8) & 0xFF);
    payload[2] = (unsigned char)((session_id >> 16) & 0xFF);
    payload[3] = (unsigned char)((session_id >> 24) & 0xFF);
}

static bool session_type_is_valid(SessionType session_type) {
    switch (session_type) {
        case FIRA_RANGING_SESSION:
        case FIRA_RANGING_AND_IN_BAND_DATA_SESSION:
        case FIRA_DATA_TRANSFER_SESSION:
        case FIRA_RANGING_ONLY_PHASE:
        case FIRA_IN_BAND_DATA_PHASE:
        case FIRA_RANGING_WITH_DATA_PHASE:
            return true;
        default:
            return false;
    }
}

int handle_session_init_command_values(uint32_t session_id, SessionType session_type) {
    if (!session_type_is_valid(session_type)) {
        printf("Invalid session_type: 0x%02X\n", session_type);
        return -1;
    }

    unsigned char payload[5];
    encode_session_id_le(payload, session_id);
    payload[4] = (unsigned char)session_type;
    send_uci_command(COMMAND, 0, SESSION_CONFIG, SESSION_INIT, payload, sizeof(payload));
    return 0;
}

int handle_session_deinit_command_value(uint32_t session_id) {
    unsigned char payload[4];
    encode_session_id_le(payload, session_id);
    send_uci_command(COMMAND, 0, SESSION_CONFIG, SESSION_DEINIT, payload, sizeof(payload));
    return 0;
}

int handle_session_start_command_value(uint32_t session_id) {
    unsigned char payload[4];
    encode_session_id_le(payload, session_id);
    send_uci_command(COMMAND, 0, SESSION_CONTROL, SESSION_START, payload, sizeof(payload));
    return 0;
}

int handle_session_stop_command_value(uint32_t session_id) {
    unsigned char payload[4];
    encode_session_id_le(payload, session_id);
    send_uci_command(COMMAND, 0, SESSION_CONTROL, SESSION_STOP, payload, sizeof(payload));
    return 0;
}

int handle_get_session_state_command_value(uint32_t session_id) {
    unsigned char payload[4];
    encode_session_id_le(payload, session_id);
    send_uci_command(COMMAND, 0, SESSION_CONFIG, SESSION_GET_STATE, payload, sizeof(payload));
    return 0;
}

int handle_session_send_data_command_values(uint32_t session_id,
                                            uint64_t destination,
                                            uint16_t sequence,
                                            const unsigned char* payload,
                                            size_t payload_len) {
    if (!payload || payload_len == 0) {
        printf("Error: payload must contain at least one byte.\n");
        return -1;
    }
    if (payload_len > 512) {
        printf("Error: payload length %zu exceeds maximum supported size (512 bytes).\n",
               payload_len);
        return -1;
    }

    uci_send_data_message(session_id, destination, sequence, payload, payload_len);
    return 0;
}

int handle_session_logical_link_create_command_values(uint32_t session_id,
                                                      unsigned char link_id,
                                                      bool mode_present,
                                                      unsigned char mode,
                                                      bool credit_present,
                                                      unsigned char credit) {
    unsigned char payload[7];
    encode_session_id_le(payload, session_id);

    size_t payload_len = 5;
    payload[4] = link_id;

    if (mode_present) {
        payload[5] = mode;
        payload_len = 6;
    }

    if (credit_present) {
        payload[6] = credit;
        payload_len = 7;
    } else if (mode_present) {
        payload[6] = 1;
        payload_len = 7;
    }

    send_uci_command(COMMAND, 0, SESSION_CONTROL, SESSION_LOGICAL_LINK_CREATE,
                     payload, (int)payload_len);
    return 0;
}

int handle_session_logical_link_close_command_value(uint32_t session_id,
                                                    unsigned char link_id) {
    unsigned char payload[5];
    encode_session_id_le(payload, session_id);
    payload[4] = link_id;

    send_uci_command(COMMAND, 0, SESSION_CONTROL, SESSION_LOGICAL_LINK_CLOSE,
                     payload, sizeof(payload));
    return 0;
}

int handle_session_logical_link_get_param_command_value(uint32_t session_id,
                                                        unsigned char link_id) {
    unsigned char payload[5];
    encode_session_id_le(payload, session_id);
    payload[4] = link_id;

    send_uci_command(COMMAND, 0, SESSION_CONTROL, SESSION_LOGICAL_LINK_GET_PARAM,
                     payload, sizeof(payload));
    return 0;
}
