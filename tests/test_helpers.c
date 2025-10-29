#include "test_helpers.h"
#include "../include/uci_ui_packet_decoder.h"
#include <string.h>

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

// Decodes a DEVICE_RESET command payload for testing
int test_decode_device_reset_cmd(const unsigned char* payload, int payload_len, decoded_device_reset_cmd_t* decoded_cmd) {
    if (payload_len < 1) {
        return -1; // Error: payload too short
    }

    decoded_cmd->reset_config = payload[0];

    return 0; // Success
}

// Decodes a SET_CONFIG command payload for testing
int test_decode_set_config_cmd(const unsigned char* payload, int payload_len, decoded_set_config_cmd_t* decoded_cmd) {
    if (payload_len < 1) {
        return -1; // Error: payload too short
    }

    decoded_cmd->num_configs = payload[0];
    if (payload_len < 1 + decoded_cmd->num_configs) {
        return -1; // Error: payload too short
    }

    memcpy(decoded_cmd->configs, payload + 1, payload_len - 1);

    return 0; // Success
}

// Decodes a GET_CONFIG command payload for testing
int test_decode_get_config_cmd(const unsigned char* payload, int payload_len, decoded_get_config_cmd_t* decoded_cmd) {
    if (payload_len < 1) {
        return -1; // Error: payload too short
    }

    decoded_cmd->num_configs = payload[0];
    int ids_available = payload_len - 1;

    if (ids_available < decoded_cmd->num_configs) {
        return -1; // Error: payload too short for advertised config IDs
    }

    if (ids_available > 255) {
        return -1; // Error: exceeds buffer capacity
    }

    decoded_cmd->config_ids_len = (uint8_t)ids_available;
    memcpy(decoded_cmd->config_ids, payload + 1, ids_available);

    return 0; // Success
}
