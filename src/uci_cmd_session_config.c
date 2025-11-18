/**
 * @file uci_cmd_session_config.c
 * @brief Session configuration command handlers implementation
 */

#include "uci_cmd_session_config.h"
#include "uci.h"
#include "uci_pdl.h"
#include "uci_functions.h"
#include "uci_config_manager.h"
#include "uci_command_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_PAYLOAD_LENGTH 255

int handle_set_app_config_command(char* session_id_str, char* config_name, char* value_str) {
    if (!session_id_str || !config_name || !value_str) {
        printf("Usage: set_app_config <session_id> <config_name> <value>\n");
        printf("  Example: set_app_config 1 device_type responder\n");
        printf("  Examples: set_app_config 1 ranging_usage ranging\n");
        printf("            set_app_config 1 sts_config dynamic\n");
        printf("            set_app_config 1 multi_node_mode unicast\n");
        printf("            set_app_config 1 channel 5\n");
        printf("            set_app_config 1 device_role controller\n");
        return -1;
    }

    unsigned int session_id = 0;
    if (!validate_session_id(session_id_str, &session_id)) {
        return -1;
    }

    return handle_set_app_config_command_value(session_id, config_name, value_str);
}

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

int handle_get_app_config_command(char* session_id_str, char** config_names, int config_count) {
    if (!session_id_str || !config_names || config_count <= 0) {
        printf("Usage: get_app_config <session_id> <config_name_1> [config_name_2]...\n");
        return -1;
    }

    unsigned int session_id = 0;
    if (!validate_session_id(session_id_str, &session_id)) {
        return -1;
    }

    if (config_count == 1) {
        const char* single_name = config_names ? config_names[0] : NULL;
        return handle_get_app_config_command_value(session_id, single_name);
    }

    unsigned char payload[MAX_PAYLOAD_LENGTH];
    size_t offset = 0;
    payload[offset++] = session_id & 0xFF;
    payload[offset++] = (session_id >> 8) & 0xFF;
    payload[offset++] = (session_id >> 16) & 0xFF;
    payload[offset++] = (session_id >> 24) & 0xFF;

    size_t count_index = offset;
    payload[offset++] = 0; // Placeholder for number of configs

    int num_configs = 0;
    for (int i = 0; i < config_count; i++) {
        const char* config_name = config_names[i];
        if (!config_name) {
            continue;
        }

        AppConfigTlvType cfg_id = 0;
        if (uci_config_lookup_app_param(config_name, &cfg_id, NULL) != 0) {
            printf("Unknown config_name: %s. Use 'show_app_configs' to list supported parameters.\n", config_name);
            return -1;
        }

        if (offset >= MAX_PAYLOAD_LENGTH) {
            printf("Too many config names provided.\n");
            return -1;
        }

        payload[offset++] = (unsigned char)cfg_id;
        num_configs++;
    }

    if (num_configs == 0) {
        printf("No valid config names provided.\n");
        return -1;
    }

    payload[count_index] = (unsigned char)num_configs;
    send_uci_command(COMMAND, 0, SESSION_CONFIG, SESSION_GET_APP_CONFIG, payload, (int)offset);
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

int handle_update_multicast_list_command(char* session_id_str,
                                         char* action_str,
                                         char* short_address_str,
                                         char* subsession_id_str) {
    if (!session_id_str || !action_str || !short_address_str || !subsession_id_str) {
        printf("Usage: session_update_multicast_list <session_id> <action> <short_address> <subsession_id>\n");
        printf("  Examples:\n");
        printf("    session_update_multicast_list 1 add 0x1234 0x00000001\n");
        printf("    session_update_multicast_list 1 remove 0x5678 0x00000002\n");
        printf("    session_update_multicast_list 1 add_short_key 0xABCD 0x00000003\n");
        printf("    session_update_multicast_list 1 add_long_key 0xEF01 0x00000004\n");
        printf("  Actions:\n");
        printf("    - add / add_short_key\n");
        printf("    - remove\n");
        printf("    - add_long_key\n");
        return -1;
    }

    unsigned int session_id = (unsigned int)strtoul(session_id_str, NULL, 10);
    unsigned short short_address = (unsigned short)strtoul(short_address_str, NULL, 0);
    unsigned int subsession_id = (unsigned int)strtoul(subsession_id_str, NULL, 0);

    return handle_update_multicast_list_command_values(session_id,
                                                       action_str,
                                                       short_address,
                                                       subsession_id);
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

int handle_session_update_dt_tag_rounds_command(char* session_id_str,
                                               char** round_values,
                                               int round_count) {
    if (!session_id_str) {
        printf("Usage: session_update_dt_tag_rounds <session_id> [round_index ...]\n");
        printf("  Examples:\n");
        printf("    session_update_dt_tag_rounds 1 0 1 2\n");
        printf("    session_update_dt_tag_rounds 1 0x00 0x01\n");
        return -1;
    }

    unsigned long session_id_ul = strtoul(session_id_str, NULL, 10);
    if (session_id_ul > 0xFFFFFFFFUL) {
        printf("Error: session_id out of range (must be 0-4294967295).\n");
        return -1;
    }

    if (round_count < 0) {
        round_count = 0;
    }
    if (round_count > 255) {
        printf("Error: Too many round indices provided (maximum 255).\n");
        return -1;
    }

    unsigned char payload[5 + 255];
    unsigned int session_id = (unsigned int)session_id_ul;
    payload[0] = session_id & 0xFF;
    payload[1] = (session_id >> 8) & 0xFF;
    payload[2] = (session_id >> 16) & 0xFF;
    payload[3] = (session_id >> 24) & 0xFF;
    payload[4] = (unsigned char)round_count;

    for (int i = 0; i < round_count; i++) {
        char* arg = round_values[i];
        if (!arg) {
            printf("Error: Missing round index at position %d.\n", i + 1);
            return -1;
        }
        char* endptr = NULL;
        long value = strtol(arg, &endptr, 0);
        if (endptr == arg || *endptr != '\0' || value < 0 || value > 0xFF) {
            printf("Error: Invalid round index '%s'. Provide values between 0 and 255 (decimal or hex).\n", arg);
            return -1;
        }
        payload[5 + i] = (unsigned char)value;
    }

    int payload_len = 5 + round_count;
    send_uci_command(COMMAND, 0, SESSION_CONFIG, SESSION_UPDATE_ACTIVE_ROUNDS_DT_TAG, payload, payload_len);
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

int handle_session_data_transfer_phase_config_command(char* session_id_str,
                                                      char* repetition_str,
                                                      char* control_str,
                                                      char* size_str,
                                                      char** payload_values,
                                                      int payload_count) {
    if (!session_id_str || !repetition_str || !control_str || !size_str) {
        printf("Usage: session_data_transfer_phase_config <session_id> <repetition> <control> <size> [payload_byte ...]\n");
        printf("  Example: session_data_transfer_phase_config 1 1 0x02 4 AA BB CC DD\n");
        return -1;
    }

    unsigned long session_id_ul = strtoul(session_id_str, NULL, 10);
    if (session_id_ul > 0xFFFFFFFFUL) {
        printf("Error: session_id out of range (must be 0-4294967295).\n");
        return -1;
    }

    char* endptr = NULL;
    long repetition_val = strtol(repetition_str, &endptr, 0);
    if (endptr == repetition_str || *endptr != '\0' || repetition_val < 0 || repetition_val > 0xFF) {
        printf("Error: Invalid repetition value '%s'. Provide 0-255.\n", repetition_str);
        return -1;
    }

    endptr = NULL;
    long control_val = strtol(control_str, &endptr, 0);
    if (endptr == control_str || *endptr != '\0' || control_val < 0 || control_val > 0xFF) {
        printf("Error: Invalid control value '%s'. Provide 0-255.\n", control_str);
        return -1;
    }

    endptr = NULL;
    long size_val = strtol(size_str, &endptr, 0);
    if (endptr == size_str || *endptr != '\0' || size_val < 0 || size_val > 0xFF) {
        printf("Error: Invalid size value '%s'. Provide 0-255.\n", size_str);
        return -1;
    }

    if (payload_count < 0) {
        payload_count = 0;
    }

    if (payload_count != size_val) {
        if (payload_count == 0 && size_val == 0) {
            // OK
        } else {
            printf("Error: Provided payload byte count (%d) does not match declared size (%ld).\n",
                   payload_count, size_val);
            return -1;
        }
    }

    if (payload_count > 64) {
        printf("Error: Payload byte count exceeds maximum supported length (64).\n");
        return -1;
    }

    unsigned char payload_bytes[64];
    for (int i = 0; i < payload_count; i++) {
        char* arg = payload_values[i];
        char* local_endptr = NULL;
        long value = strtol(arg, &local_endptr, 0);
        if (local_endptr == arg || *local_endptr != '\0' || value < 0 || value > 0xFF) {
            printf("Error: Invalid payload byte '%s'. Provide values between 0 and 255 (decimal or hex).\n", arg);
            return -1;
        }
        payload_bytes[i] = (unsigned char)value;
    }

    return handle_session_data_transfer_phase_config_command_values(
        (uint32_t)session_id_ul,
        (unsigned char)repetition_val,
        (unsigned char)control_val,
        (unsigned char)size_val,
        payload_count > 0 ? payload_bytes : NULL,
        (size_t)payload_count);
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
