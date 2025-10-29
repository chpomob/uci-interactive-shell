#include "test_helpers.h"
#include "../include/uci_ui_packet_decoder.h"

// Decodes a SESSION_INIT command payload for testing
int test_decode_session_init_cmd(const unsigned char* payload, int payload_len, decoded_session_init_cmd_t* decoded_cmd) {
    if (payload_len < 5) {
        return -1; // Error: payload too short
    }

    decoded_cmd->session_id = ui_read_u32_le(payload);
    decoded_cmd->session_type = payload[4];

    return 0; // Success
}

// Decodes a SESSION_DEINIT command payload for testing
int test_decode_session_deinit_cmd(const unsigned char* payload, int payload_len, decoded_session_deinit_cmd_t* decoded_cmd) {
    if (payload_len < 4) {
        return -1; // Error: payload too short
    }

    decoded_cmd->session_id = ui_read_u32_le(payload);

    return 0; // Success
}

// Decodes a SESSION_START command payload for testing
int test_decode_session_start_cmd(const unsigned char* payload, int payload_len, decoded_session_start_cmd_t* decoded_cmd) {
    if (payload_len < 4) {
        return -1; // Error: payload too short
    }

    decoded_cmd->session_id = ui_read_u32_le(payload);

    return 0; // Success
}

// Decodes a SESSION_STOP command payload for testing
int test_decode_session_stop_cmd(const unsigned char* payload, int payload_len, decoded_session_stop_cmd_t* decoded_cmd) {
    if (payload_len < 4) {
        return -1; // Error: payload too short
    }

    decoded_cmd->session_id = ui_read_u32_le(payload);

    return 0; // Success
}

// Decodes a GET_SESSION_STATE command payload for testing
int test_decode_get_session_state_cmd(const unsigned char* payload, int payload_len, decoded_get_session_state_cmd_t* decoded_cmd) {
    if (payload_len < 4) {
        return -1; // Error: payload too short
    }

    decoded_cmd->session_id = ui_read_u32_le(payload);

    return 0; // Success
}
