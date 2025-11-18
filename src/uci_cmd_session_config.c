/**
 * @file uci_cmd_session_config.c
 * @brief Session configuration command handlers implementation
 */

#include "uci_cmd_session_config.h"
#include "uci.h"
#include "uci_pdl.h"
#include "uci_functions.h"
#include "uci_config_manager.h"
#include <stdio.h>
#include <string.h>

#define MAX_PAYLOAD_LENGTH 255

int handle_set_app_config_command_value(uint32_t session_id,
                                        const char* config_name,
                                        const char* value_str) {
    if (!config_name || !value_str) {
        printf("Usage: set_app_config <session_id> <config_name> <value>\n");
        return -1;
    }

    AppConfigTlvType cfg_id = 0;
    const config_param_info_t* info = NULL;
    if (uci_config_lookup_app_param(config_name, &cfg_id, &info) != 0) {
        printf("Unknown config_name: %s. Use 'show_app_configs' to list supported parameters.\n", config_name);
        return -1;
    }

    unsigned char value[256];
    size_t value_len = sizeof(value);
    if (uci_config_parse_app_value(cfg_id, value_str, value, &value_len) != 0) {
        const char* name = (info && info->name) ? info->name : "app config";
        printf("Invalid value '%s' for %s (ID 0x%02X).\n", value_str, name, cfg_id);
        uci_config_show_app_param_help(cfg_id);
        return -1;
    }

    if (value_len > 255) {
        printf("Error: Parsed value length %zu exceeds TLV limit (255 bytes).\n", value_len);
        return -1;
    }

    unsigned char payload[4 + 1 + 1 + 1 + 255];
    size_t offset = 0;
    payload[offset++] = session_id & 0xFF;
    payload[offset++] = (session_id >> 8) & 0xFF;
    payload[offset++] = (session_id >> 16) & 0xFF;
    payload[offset++] = (session_id >> 24) & 0xFF;
    payload[offset++] = 1; // Number of TLVs
    payload[offset++] = (unsigned char)cfg_id;
    payload[offset++] = (unsigned char)value_len;
    memcpy(&payload[offset], value, value_len);
    offset += value_len;

    send_uci_command(COMMAND, 0, SESSION_CONFIG, SESSION_SET_APP_CONFIG, payload, (int)offset);
    return 0;
}

int handle_get_app_config_command_value(uint32_t session_id, const char* config_name) {
    if (!config_name) {
        printf("Usage: get_app_config <session_id> <config_name>\n");
        return -1;
    }

    AppConfigTlvType cfg_id = 0;
    if (uci_config_lookup_app_param(config_name, &cfg_id, NULL) != 0) {
        printf("Unknown config_name: %s. Use 'show_app_configs' to list supported parameters.\n", config_name);
        return -1;
    }

    unsigned char payload[6];
    payload[0] = session_id & 0xFF;
    payload[1] = (session_id >> 8) & 0xFF;
    payload[2] = (session_id >> 16) & 0xFF;
    payload[3] = (session_id >> 24) & 0xFF;
    payload[4] = 1; // Number of configs
    payload[5] = (unsigned char)cfg_id;

    send_uci_command(COMMAND, 0, SESSION_CONFIG, SESSION_GET_APP_CONFIG, payload, sizeof(payload));
    return 0;
}

int handle_update_multicast_list_command_values(uint32_t session_id,
                                                const char* action_str,
                                                unsigned short short_address,
                                                uint32_t subsession_id) {
    if (!action_str) {
        printf("Error: action string is required for multicast updates.\n");
        return -1;
    }

    UpdateMulticastListAction action;
    if (strcmp(action_str, "add") == 0 || strcmp(action_str, "add_short_key") == 0) {
        action = MULTICAST_ACTION_ADD_SHORT_KEY;
    } else if (strcmp(action_str, "remove") == 0) {
        action = MULTICAST_ACTION_REMOVE;
    } else if (strcmp(action_str, "add_long_key") == 0) {
        action = MULTICAST_ACTION_ADD_LONG_KEY;
    } else {
        printf("Unknown action: %s\n", action_str);
        printf("  Supported actions:\n");
        printf("    - add / add_short_key (0x00)\n");
        printf("    - remove (0x01)\n");
        printf("    - add_long_key (0x03)\n");
        return -1;
    }

    unsigned char payload[12];
    payload[0] = session_id & 0xFF;
    payload[1] = (session_id >> 8) & 0xFF;
    payload[2] = (session_id >> 16) & 0xFF;
    payload[3] = (session_id >> 24) & 0xFF;
    payload[4] = 1; // Number of entries
    payload[5] = action;
    payload[6] = short_address & 0xFF;
    payload[7] = (short_address >> 8) & 0xFF;
    payload[8] = subsession_id & 0xFF;
    payload[9] = (subsession_id >> 8) & 0xFF;
    payload[10] = (subsession_id >> 16) & 0xFF;
    payload[11] = (subsession_id >> 24) & 0xFF;

    send_uci_command(COMMAND, 0, SESSION_CONFIG, SESSION_UPDATE_CONTROLLER_MULTICAST_LIST, payload, sizeof(payload));
    return 0;
}

int handle_session_update_dt_tag_rounds_command_values(uint32_t session_id,
                                                       const unsigned char* round_bytes,
                                                       size_t round_count) {
    if (round_count > 255) {
        printf("Error: Too many round indexes (%zu). Maximum is 255.\n", round_count);
        return -1;
    }
    if (round_count > 0 && !round_bytes) {
        printf("Error: Round byte buffer is NULL.\n");
        return -1;
    }

    unsigned char payload[5 + 255];
    payload[0] = session_id & 0xFF;
    payload[1] = (session_id >> 8) & 0xFF;
    payload[2] = (session_id >> 16) & 0xFF;
    payload[3] = (session_id >> 24) & 0xFF;
    payload[4] = (unsigned char)round_count;

    if (round_count > 0) {
        memcpy(&payload[5], round_bytes, round_count);
    }

    int payload_len = 5 + (int)round_count;
    send_uci_command(COMMAND, 0, SESSION_CONFIG, SESSION_UPDATE_ACTIVE_ROUNDS_DT_TAG, payload, payload_len);
    return 0;
}

int handle_session_data_transfer_phase_config_command_values(uint32_t session_id,
                                                             unsigned char repetition,
                                                             unsigned char control,
                                                             unsigned char size,
                                                             const unsigned char* payload,
                                                             size_t payload_len) {
    if (size > 64) {
        printf("Error: Declared payload size exceeds maximum supported length (64 bytes).\n");
        return -1;
    }

    if (size == 0) {
        if (payload && payload_len > 0) {
            printf("Error: Payload bytes provided but size is zero.\n");
            return -1;
        }
    } else {
        if (!payload || payload_len != size) {
            printf("Error: Provided payload length (%zu) does not match declared size (%u).\n",
                   payload_len, size);
            return -1;
        }
    }

    unsigned char buffer[7 + 64];
    buffer[0] = session_id & 0xFF;
    buffer[1] = (session_id >> 8) & 0xFF;
    buffer[2] = (session_id >> 16) & 0xFF;
    buffer[3] = (session_id >> 24) & 0xFF;
    buffer[4] = repetition;
    buffer[5] = control;
    buffer[6] = size;

    if (payload && payload_len > 0) {
        memcpy(&buffer[7], payload, payload_len);
    }

    int payload_size = 7 + (int)((payload && payload_len > 0) ? payload_len : 0);
    send_uci_command(COMMAND, 0, SESSION_CONFIG, SESSION_DATA_TRANSFER_PHASE_CONFIG, buffer, payload_size);
    return 0;
}
