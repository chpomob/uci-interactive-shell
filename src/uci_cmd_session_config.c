/**
 * @file uci_cmd_session_config.c
 * @brief Session configuration command handlers implementation
 */

#include "uci_cmd_session_config.h"
#include "uci.h"
#include "uci_pdl.h"
#include "uci_functions.h"
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

    unsigned int session_id = (unsigned int)strtoul(session_id_str, NULL, 10);
    AppConfigTlvType cfg_id;
    unsigned char value;
    int found = 0; // Flag to track if config name was found

    if (strcmp(config_name, "device_type") == 0) {
        cfg_id = DEVICE_TYPE;
        if (strcmp(value_str, "responder") == 0) {
            value = 0x01;
        } else if (strcmp(value_str, "initiator") == 0) {
            value = 0x02;
        } else {
            printf("Invalid value for device_type. Use 'responder' or 'initiator'.\n");
            return -1;
        }
        found = 1;
    } else if (strcmp(config_name, "ranging_usage") == 0 || strcmp(config_name, "ranging_round_usage") == 0) {
        cfg_id = RANGING_ROUND_USAGE;
        if (strcmp(value_str, "ranging") == 0) {
            value = 0x00;
        } else if (strcmp(value_str, "data") == 0) {
            value = 0x01;
        } else {
            printf("Invalid value for ranging_usage. Use 'ranging' or 'data'.\n");
            return -1;
        }
        found = 1;
    } else if (strcmp(config_name, "sts_config") == 0) {
        cfg_id = STS_CONFIG;
        if (strcmp(value_str, "static") == 0) {
            value = 0x00;
        } else if (strcmp(value_str, "dynamic") == 0) {
            value = 0x01;
        } else {
            printf("Invalid value for sts_config. Use 'static' or 'dynamic'.\n");
            return -1;
        }
        found = 1;
    } else if (strcmp(config_name, "multi_node_mode") == 0) {
        cfg_id = MULTI_NODE_MODE;
        if (strcmp(value_str, "unicast") == 0) {
            value = 0x00;
        } else if (strcmp(value_str, "anycast") == 0) {
            value = 0x01;
        } else if (strcmp(value_str, "multicast") == 0) {
            value = 0x02;
        } else {
            printf("Invalid value for multi_node_mode. Use 'unicast', 'anycast', or 'multicast'.\n");
            return -1;
        }
        found = 1;
    } else if (strcmp(config_name, "channel") == 0 || strcmp(config_name, "channel_number") == 0) {
        cfg_id = CHANNEL_NUMBER;
        // Allow numeric values for channel number
        char *endptr;
        long channel_val = strtol(value_str, &endptr, 10);
        if (*endptr != '\0') {
            printf("Invalid value for channel. Use a numeric value.\n");
            return -1;
        }
        if (channel_val < 0 || channel_val > 255) {
            printf("Invalid value for channel. Use 0-255.\n");
            return -1;
        }
        value = (unsigned char)channel_val;
        found = 1;
    } else if (strcmp(config_name, "device_role") == 0) {
        cfg_id = DEVICE_ROLE;
        if (strcmp(value_str, "controller") == 0) {
            value = 0x00;
        } else if (strcmp(value_str, "controlee") == 0) {
            value = 0x01;
        } else {
            printf("Invalid value for device_role. Use 'controller' or 'controlee'.\n");
            return -1;
        }
        found = 1;
    } else if (strcmp(config_name, "aoa_request") == 0 || strcmp(config_name, "aoa_result_req") == 0) {
        cfg_id = AOA_RESULT_REQ;
        if (strcmp(value_str, "disable") == 0 || strcmp(value_str, "off") == 0) {
            value = 0x00;
        } else if (strcmp(value_str, "enable") == 0 || strcmp(value_str, "on") == 0) {
            value = 0x01;
        } else {
            printf("Invalid value for aoa_request. Use 'enable'/'on' or 'disable'/'off'.\n");
            return -1;
        }
        found = 1;
    } else if (strcmp(config_name, "scheduled_mode") == 0) {
        cfg_id = SCHEDULED_MODE;
        if (strcmp(value_str, "cont") == 0 || strcmp(value_str, "continuous") == 0) {
            value = 0x00;
        } else if (strcmp(value_str, "scheduled") == 0) {
            value = 0x01;
        } else {
            printf("Invalid value for scheduled_mode. Use 'cont'/'continuous' or 'scheduled'.\n");
            return -1;
        }
        found = 1;
    } else if (strcmp(config_name, "slot_duration") == 0) {
        cfg_id = SLOT_DURATION;
        found = 1;
    } else if (strcmp(config_name, "ranging_duration") == 0) {
        cfg_id = RANGING_DURATION;
        found = 1;
    } else if (strcmp(config_name, "sts_index") == 0) {
        cfg_id = STS_INDEX;
        found = 1;
    } else if (strcmp(config_name, "preamble_code_index") == 0) {
        cfg_id = PREAMBLE_CODE_INDEX;
        found = 1;
    } else if (strcmp(config_name, "sfd_id") == 0) {
        cfg_id = SFD_ID;
        found = 1;
    } else if (strcmp(config_name, "psdu_data_rate") == 0) {
        cfg_id = PSDU_DATA_RATE;
        found = 1;
    } else if (strcmp(config_name, "prf_mode") == 0) {
        cfg_id = PRF_MODE;
        found = 1;
    } else if (strcmp(config_name, "hopping_mode") == 0) {
        cfg_id = HOPPING_MODE;
        found = 1;
    } else if (strcmp(config_name, "result_report_config") == 0) {
        cfg_id = RESULT_REPORT_CONFIG;
        found = 1;
    } else if (strcmp(config_name, "max_rr_retry") == 0) {
        cfg_id = MAX_RR_RETRY;
        found = 1;
    } else if (strcmp(config_name, "uwb_initiation_time") == 0 || strcmp(config_name, "initiation_time") == 0) {
        cfg_id = UWB_INITIATION_TIME;
        found = 1;
    } else if (strcmp(config_name, "sub_session_id") == 0) {
        cfg_id = SUB_SESSION_ID;
        found = 1;
    }

    if (!found) {
        printf("Unknown config_name: %s\n", config_name);
        printf("Supported config names: device_type, ranging_usage, sts_config, multi_node_mode, channel, device_role, aoa_request, scheduled_mode, slot_duration, ranging_duration, sts_index, preamble_code_index, sfd_id, psdu_data_rate, prf_mode, hopping_mode, result_report_config, max_rr_retry, uwb_initiation_time, sub_session_id\n");
        return -1;
    }

    unsigned char payload[20]; // Increased size to handle more complex values
    // Send session_id in little-endian format to match UCI spec and read_u32_le parsing
    payload[0] = session_id & 0xFF;           // LSB first
    payload[1] = (session_id >> 8) & 0xFF;
    payload[2] = (session_id >> 16) & 0xFF;
    payload[3] = (session_id >> 24) & 0xFF;   // MSB last
    payload[4] = 1; // Number of TLVs
    payload[5] = cfg_id;
    payload[6] = 1; // Length (will be updated if needed)
    payload[7] = value;

    // Update payload length based on the actual value size for multi-byte parameters
    int payload_len = 8;
    switch(cfg_id) {
        case RANGING_DURATION:
        case SLOT_DURATION:
        case STS_INDEX:
        case RNG_DATA_NTF_PROXIMITY_NEAR:
        case RNG_DATA_NTF_PROXIMITY_FAR:
        case RNG_DATA_NTF_AOA_BOUND:
        case TX_JITTER_WINDOW_SIZE:
        case MAX_RR_RETRY:
        case UWB_INITIATION_TIME:
        case BLOCK_STRIDE_LENGTH:
        case IN_BAND_TERMINATION_ATTEMPT_COUNT:
        case SUB_SESSION_ID:
            // These parameters use 4-byte values
            {
                char *endptr;
                unsigned long val = strtoul(value_str, &endptr, 10);
                if (*endptr != '\0') {
                    printf("Invalid numeric value for %s\n", config_name);
                    return -1;
                }
                payload[6] = 4; // Length
                payload[7] = val & 0xFF;           // LSB first
                payload[8] = (val >> 8) & 0xFF;
                payload[9] = (val >> 16) & 0xFF;
                payload[10] = (val >> 24) & 0xFF;  // MSB last
                payload_len = 11;
            }
            break;
        case PREAMBLE_CODE_INDEX:
        case SFD_ID:
        case PSDU_DATA_RATE:
        case PRF_MODE:
        case NUMBER_OF_STS_SEGMENTS:
        case HOPPING_MODE:
        case RESULT_REPORT_CONFIG:
            // These parameters use 2-byte values
            {
                char *endptr;
                unsigned long val = strtoul(value_str, &endptr, 10);
                if (*endptr != '\0') {
                    printf("Invalid numeric value for %s\n", config_name);
                    return -1;
                }
                payload[6] = 2; // Length
                payload[7] = val & 0xFF;           // LSB first
                payload[8] = (val >> 8) & 0xFF;    // MSB last
                payload_len = 9;
            }
            break;
        default:
            // Single byte value - already handled above
            break;
    }

    send_uci_command(COMMAND, 0, SESSION_CONFIG, SESSION_SET_APP_CONFIG, payload, payload_len);
    return 0;
}

int handle_get_app_config_command(char* session_id_str, char* config_name) {
    if (!session_id_str) {
        printf("Usage: get_app_config <session_id> <config_name_1> [config_name_2]...\n");
        return -1;
    }
    unsigned int session_id = (unsigned int)strtoul(session_id_str, NULL, 10);

    unsigned char payload[MAX_PAYLOAD_LENGTH];
    // Send session_id in little-endian format to match UCI spec and read_u32_le parsing
    payload[0] = session_id & 0xFF;           // LSB first
    payload[1] = (session_id >> 8) & 0xFF;
    payload[2] = (session_id >> 16) & 0xFF;
    payload[3] = (session_id >> 24) & 0xFF;   // MSB last

    int num_configs = 0;
    while (config_name != NULL) {
        int valid_config = 0;
        if (strcmp(config_name, "device_type") == 0) {
            payload[5 + num_configs] = DEVICE_TYPE;
            num_configs++;
            valid_config = 1;
        } else if (strcmp(config_name, "ranging_usage") == 0 || strcmp(config_name, "ranging_round_usage") == 0) {
            payload[5 + num_configs] = RANGING_ROUND_USAGE;
            num_configs++;
            valid_config = 1;
        } else if (strcmp(config_name, "sts_config") == 0) {
            payload[5 + num_configs] = STS_CONFIG;
            num_configs++;
            valid_config = 1;
        } else if (strcmp(config_name, "multi_node_mode") == 0) {
            payload[5 + num_configs] = MULTI_NODE_MODE;
            num_configs++;
            valid_config = 1;
        } else if (strcmp(config_name, "channel") == 0 || strcmp(config_name, "channel_number") == 0) {
            payload[5 + num_configs] = CHANNEL_NUMBER;
            num_configs++;
            valid_config = 1;
        } else if (strcmp(config_name, "device_role") == 0) {
            payload[5 + num_configs] = DEVICE_ROLE;
            num_configs++;
            valid_config = 1;
        } else if (strcmp(config_name, "aoa_request") == 0 || strcmp(config_name, "aoa_result_req") == 0) {
            payload[5 + num_configs] = AOA_RESULT_REQ;
            num_configs++;
            valid_config = 1;
        } else if (strcmp(config_name, "scheduled_mode") == 0) {
            payload[5 + num_configs] = SCHEDULED_MODE;
            num_configs++;
            valid_config = 1;
        } else if (strcmp(config_name, "slot_duration") == 0) {
            payload[5 + num_configs] = SLOT_DURATION;
            num_configs++;
            valid_config = 1;
        } else if (strcmp(config_name, "ranging_duration") == 0) {
            payload[5 + num_configs] = RANGING_DURATION;
            num_configs++;
            valid_config = 1;
        } else if (strcmp(config_name, "sts_index") == 0) {
            payload[5 + num_configs] = STS_INDEX;
            num_configs++;
            valid_config = 1;
        } else if (strcmp(config_name, "preamble_code_index") == 0) {
            payload[5 + num_configs] = PREAMBLE_CODE_INDEX;
            num_configs++;
            valid_config = 1;
        } else if (strcmp(config_name, "sfd_id") == 0) {
            payload[5 + num_configs] = SFD_ID;
            num_configs++;
            valid_config = 1;
        } else if (strcmp(config_name, "psdu_data_rate") == 0) {
            payload[5 + num_configs] = PSDU_DATA_RATE;
            num_configs++;
            valid_config = 1;
        } else if (strcmp(config_name, "prf_mode") == 0) {
            payload[5 + num_configs] = PRF_MODE;
            num_configs++;
            valid_config = 1;
        } else if (strcmp(config_name, "hopping_mode") == 0) {
            payload[5 + num_configs] = HOPPING_MODE;
            num_configs++;
            valid_config = 1;
        } else if (strcmp(config_name, "result_report_config") == 0) {
            payload[5 + num_configs] = RESULT_REPORT_CONFIG;
            num_configs++;
            valid_config = 1;
        } else if (strcmp(config_name, "max_rr_retry") == 0) {
            payload[5 + num_configs] = MAX_RR_RETRY;
            num_configs++;
            valid_config = 1;
        } else if (strcmp(config_name, "uwb_initiation_time") == 0 || strcmp(config_name, "initiation_time") == 0) {
            payload[5 + num_configs] = UWB_INITIATION_TIME;
            num_configs++;
            valid_config = 1;
        } else if (strcmp(config_name, "sub_session_id") == 0) {
            payload[5 + num_configs] = SUB_SESSION_ID;
            num_configs++;
            valid_config = 1;
        }
        if (!valid_config) {
            printf("Unknown config_name: %s\n", config_name);
            printf("Supported config names: device_type, ranging_usage, sts_config, multi_node_mode, channel, device_role, aoa_request, scheduled_mode, slot_duration, ranging_duration, sts_index, preamble_code_index, sfd_id, psdu_data_rate, prf_mode, hopping_mode, result_report_config, max_rr_retry, uwb_initiation_time, sub_session_id\n");
        }
        config_name = strtok(NULL, " ");
    }

    if (num_configs == 0) {
        printf("No valid config names provided.\n");
        return -1;
    }

    payload[4] = num_configs;
    send_uci_command(COMMAND, 0, SESSION_CONFIG, SESSION_GET_APP_CONFIG, payload, 5 + num_configs);
    return 0;
}

int handle_update_multicast_list_command(char* session_id_str, char* action_str, char* short_address_str) {
    char* subsession_id_str = strtok(NULL, " ");

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
    UpdateMulticastListAction action;

    // Support both technical and friendly names for actions
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

    unsigned short short_address = (unsigned short)strtoul(short_address_str, NULL, 0);
    unsigned int subsession_id = (unsigned int)strtoul(subsession_id_str, NULL, 0);

    unsigned char payload[12];
    // Send session_id in little-endian format to match UCI spec and read_u32_le parsing
    payload[0] = session_id & 0xFF;           // LSB first
    payload[1] = (session_id >> 8) & 0xFF;
    payload[2] = (session_id >> 16) & 0xFF;
    payload[3] = (session_id >> 24) & 0xFF;   // MSB last
    payload[4] = 1; // Number of entries
    payload[5] = action;
    // Send short_address in little-endian format to match UCI spec and read_u16_le parsing
    payload[6] = short_address & 0xFF;        // LSB first
    payload[7] = (short_address >> 8) & 0xFF; // MSB last
    // Send subsession_id in little-endian format to match UCI spec and read_u32_le parsing
    payload[8] = subsession_id & 0xFF;           // LSB first
    payload[9] = (subsession_id >> 8) & 0xFF;
    payload[10] = (subsession_id >> 16) & 0xFF;
    payload[11] = (subsession_id >> 24) & 0xFF;  // MSB last

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

    unsigned char payload[7 + 64];
    unsigned int session_id = (unsigned int)session_id_ul;
    payload[0] = session_id & 0xFF;
    payload[1] = (session_id >> 8) & 0xFF;
    payload[2] = (session_id >> 16) & 0xFF;
    payload[3] = (session_id >> 24) & 0xFF;
    payload[4] = (unsigned char)repetition_val;
    payload[5] = (unsigned char)control_val;
    payload[6] = (unsigned char)size_val;

    for (int i = 0; i < payload_count; i++) {
        char* arg = payload_values[i];
        char* local_endptr = NULL;
        long value = strtol(arg, &local_endptr, 0);
        if (local_endptr == arg || *local_endptr != '\0' || value < 0 || value > 0xFF) {
            printf("Error: Invalid payload byte '%s'. Provide values between 0 and 255 (decimal or hex).\n", arg);
            return -1;
        }
        payload[7 + i] = (unsigned char)value;
    }

    int payload_len = 7 + payload_count;
    send_uci_command(COMMAND, 0, SESSION_CONFIG, SESSION_DATA_TRANSFER_PHASE_CONFIG, payload, payload_len);
    return 0;
}
