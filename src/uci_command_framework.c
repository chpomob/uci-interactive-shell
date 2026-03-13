#include "../include/uci_command_framework.h"
#include "../include/uci_ui.h"
#include "../include/uci_hw_interface.h"
#include "../include/uci_functions.h"
#include "../include/uci_pdl.h"

static uci_cmd_parsed_param_t g_parsed_params[UCI_CMD_MAX_PARAMS];
static int g_parsed_param_count = 0;

static void uci_cmd_reset_parsed_params(void) {
    memset(g_parsed_params, 0, sizeof(g_parsed_params));
    g_parsed_param_count = 0;
}

// Helper function to convert string to unsigned char (uint8_t)
int uci_cmd_validate_uint8(const char* str, unsigned char* value, unsigned char min_val, unsigned char max_val) {
    if (!str || !value) {
        return -1;
    }

    char* endptr;
    unsigned long val = strtoul(str, &endptr, 0);

    if (*endptr != '\0' || val < min_val || val > max_val) {
        return -1;
    }

    *value = (unsigned char)val;
    return 0;
}

// Helper function to convert string to unsigned short (uint16_t)
int uci_cmd_validate_uint16(const char* str, unsigned short* value, unsigned short min_val, unsigned short max_val) {
    if (!str || !value) {
        return -1;
    }

    char* endptr;
    unsigned long val = strtoul(str, &endptr, 0);

    if (*endptr != '\0' || val < min_val || val > max_val) {
        return -1;
    }

    *value = (unsigned short)val;
    return 0;
}

// Helper function to convert string to unsigned int (uint32_t)
int uci_cmd_validate_uint32(const char* str, unsigned int* value, unsigned int min_val, unsigned int max_val) {
    if (!str || !value) {
        return -1;
    }

    char* endptr;
    unsigned long val = strtoul(str, &endptr, 0);

    if (*endptr != '\0' || val < min_val || val > max_val) {
        return -1;
    }

    *value = (unsigned int)val;
    return 0;
}

// Validate and convert session ID string to uint32_t
int uci_cmd_validate_session_id(const char* str, unsigned int* session_id) {
    if (!str || !session_id) {
        return -1;
    }

    char* endptr;
    unsigned long val = strtoul(str, &endptr, 10);

    if (*endptr != '\0' || val > 0xFFFFFFFFUL) {
        return -1;
    }

    *session_id = (unsigned int)val;
    return 0;
}

// Validate hex string and convert to byte array
int uci_cmd_validate_hex_string(const char* str, unsigned char* buffer, size_t* len, size_t max_len) {
    if (!str || !buffer || !len) {
        return -1;
    }

    size_t str_len = strlen(str);
    if (str_len % 2 != 0) {
        return -1; // Hex string must have even length
    }

    size_t byte_count = str_len / 2;
    if (byte_count > max_len) {
        return -1; // Buffer too small
    }

    for (size_t i = 0; i < str_len; i += 2) {
        char byte_str[3] = {str[i], str[i+1], '\0'};
        char* endptr;
        unsigned long val = strtoul(byte_str, &endptr, 16);

        if (*endptr != '\0' || val > 0xFF) {
            return -1; // Invalid hex character
        }

        buffer[i/2] = (unsigned char)val;
    }

    *len = byte_count;
    return 0;
}

// Validate device state string
int uci_cmd_validate_device_state(const char* str, unsigned char* state) {
    if (!str || !state) {
        return -1;
    }

    if (strcmp(str, "active") == 0) {
        *state = DEVICE_STATE_ACTIVE;
        return 0;
    } else if (strcmp(str, "ready") == 0) {
        *state = DEVICE_STATE_READY;
        return 0;
    } else if (strcmp(str, "sleep") == 0 || strcmp(str, "suspend") == 0) {
        *state = 0x02;  // Use literal value since DEVICE_STATE_SLEEP may not be defined
        return 0;
    } else if (strcmp(str, "error") == 0) {
        *state = DEVICE_STATE_ERROR;
        return 0;
    }

    return -1; // Invalid device state
}

// Validate session type string
int uci_cmd_validate_session_type(const char* str, unsigned char* type) {
    if (!str || !type) {
        return -1;
    }

    if (strcmp(str, "fira_ranging") == 0 || strcmp(str, "ranging") == 0) {
        *type = FIRA_RANGING_SESSION;
        return 0;
    } else if (strcmp(str, "fira_ranging_and_data") == 0 || strcmp(str, "ranging_and_data") == 0) {
        *type = FIRA_RANGING_AND_IN_BAND_DATA_SESSION;
        return 0;
    } else if (strcmp(str, "fira_data_transfer") == 0 || strcmp(str, "data_transfer") == 0) {
        *type = FIRA_DATA_TRANSFER_SESSION;
        return 0;
    } else if (strcmp(str, "fira_ranging_only") == 0 || strcmp(str, "ranging_only") == 0) {
        *type = FIRA_RANGING_ONLY_PHASE;
        return 0;
    } else if (strcmp(str, "fira_in_band_data") == 0 || strcmp(str, "in_band_data") == 0) {
        *type = FIRA_IN_BAND_DATA_PHASE;
        return 0;
    } else if (strcmp(str, "fira_ranging_with_data") == 0 || strcmp(str, "ranging_with_data") == 0) {
        *type = FIRA_RANGING_WITH_DATA_PHASE;
        return 0;
    }

    return -1; // Invalid session type
}

// Add a byte to the payload
int uci_cmd_add_byte_to_payload(uci_command_context_t* ctx, unsigned char value) {
    if (!ctx || !ctx->payload || ctx->payload_len >= ctx->max_payload_len) {
        return -1;
    }

    ctx->payload[ctx->payload_len++] = value;
    return 0;
}

// Add a uint16 to the payload in little-endian format
int uci_cmd_add_uint16_to_payload(uci_command_context_t* ctx, unsigned short value) {
    if (!ctx || !ctx->payload || ctx->payload_len + 2 > ctx->max_payload_len) {
        return -1;
    }

    ctx->payload[ctx->payload_len++] = value & 0xFF;
    ctx->payload[ctx->payload_len++] = (value >> 8) & 0xFF;
    return 0;
}

// Add a uint32 to the payload in little-endian format
int uci_cmd_add_uint32_to_payload(uci_command_context_t* ctx, unsigned int value) {
    if (!ctx || !ctx->payload || ctx->payload_len + 4 > ctx->max_payload_len) {
        return -1;
    }

    ctx->payload[ctx->payload_len++] = value & 0xFF;
    ctx->payload[ctx->payload_len++] = (value >> 8) & 0xFF;
    ctx->payload[ctx->payload_len++] = (value >> 16) & 0xFF;
    ctx->payload[ctx->payload_len++] = (value >> 24) & 0xFF;
    return 0;
}

// Add multiple bytes to the payload
int uci_cmd_add_bytes_to_payload(uci_command_context_t* ctx, const unsigned char* data, size_t len) {
    if (!ctx || !ctx->payload || !data || len == 0) {
        return -1;
    }

    if (ctx->payload_len + len > ctx->max_payload_len) {
        return -1;
    }

    memcpy(&ctx->payload[ctx->payload_len], data, len);
    ctx->payload_len += len;
    return 0;
}

// Unified command execution function that handles both simulation and hardware modes
int uci_cmd_execute_unified(uci_command_context_t* ctx) {
    if (!ctx) {
        return -1;
    }

    if (uci_is_hardware_mode_enabled()) {
        int response_count;
        if (uci_hw_interface_send_command(ctx->mt, ctx->pbf, ctx->gid, ctx->oid,
                                          ctx->payload, (int)ctx->payload_len) != 0) {
            ui_print_error("Failed to send command to hardware");
            return -1;
        }
        response_count = uci_receive_hardware_packets(1000);
        if (response_count < 0) {
            ui_print_error("Error receiving response from hardware");
            return -1;
        }

        ui_print_success("Command sent to hardware successfully");
        if (response_count == 0) {
            ui_print_warning("No response received from hardware (timeout)");
        }
        return 0;
    } else {
        // Simulation mode
        send_uci_command(ctx->mt, ctx->pbf, ctx->gid, ctx->oid, ctx->payload, (int)ctx->payload_len);
        return 0;
    }
}

// Validate parameters based on command definition
int uci_cmd_validate_params(const uci_command_def_t* cmd_def, int argc, char** argv) {
    if (!cmd_def || !argv) {
        return -1;
    }

    if (cmd_def->param_count > UCI_CMD_MAX_PARAMS) {
        printf("Internal error: command '%s' defines %d parameters (max supported %d)\n",
               cmd_def->name ? cmd_def->name : "(unknown)",
               cmd_def->param_count,
               UCI_CMD_MAX_PARAMS);
        return -1;
    }

    uci_cmd_reset_parsed_params();
    g_parsed_param_count = cmd_def->param_count;

    // Check if required number of arguments matches
    int required_params = 0;
    for (int i = 0; i < cmd_def->param_count; i++) {
        if (cmd_def->params[i].flags & PARAM_FLAG_REQUIRED) {
            required_params++;
        }
    }

    // argc includes command name, so we need at least required_params + 1
    if (argc < required_params + 1) {
        printf("Usage: %s", cmd_def->name);
        for (int i = 0; i < cmd_def->param_count; i++) {
            if (cmd_def->params[i].flags & PARAM_FLAG_REQUIRED) {
                printf(" <%s>", cmd_def->params[i].name);
            } else {
                printf(" [%s]", cmd_def->params[i].name);
            }
        }
        printf("\n");
        if (cmd_def->description) {
            printf("  %s\n", cmd_def->description);
        }
        return -1;
    }

    // Validate each parameter based on its type
    for (int i = 0; i < cmd_def->param_count; i++) {
        const uci_param_def_t* param = &cmd_def->params[i];
        const char* param_str = (i + 1 < argc) ? argv[i + 1] : NULL; // Skip command name
        size_t param_len = param_str ? strlen(param_str) : 0;
        uci_cmd_parsed_param_t* parsed = &g_parsed_params[i];
        parsed->type = param->type;
        parsed->raw_value = param_str;
        parsed->raw_length = param_len;
        parsed->parsed_length = 0;
        parsed->present = false;

        if (!param_str || param_len == 0) {
            if (param->flags & PARAM_FLAG_REQUIRED) {
                ui_print_error("Missing required parameter");
                return -1;
            }
            continue;
        }

        parsed->present = true;

        switch (param->type) {
            case PARAM_TYPE_UINT8:
            {
                unsigned char val;
                unsigned char min_val = (unsigned char)param->min_value;
                unsigned char max_val = (unsigned char)(param->max_value == 0 ? 0xFF : param->max_value);
                if (uci_cmd_validate_uint8(param_str, &val, min_val, max_val) != 0) {
                    char error_msg[256];
                    snprintf(error_msg, sizeof(error_msg),
                             "Invalid parameter value for %s: %s (expected uint8 %u-%u)",
                             param->name, param_str, (unsigned int)min_val, (unsigned int)max_val);
                    ui_print_error(error_msg);
                    return -1;
                }
                parsed->value.u8 = val;
                break;
            }
            case PARAM_TYPE_HEX_BYTE:
            {
                char* endptr = NULL;
                unsigned long val = strtoul(param_str, &endptr, 16);
                if (!param_str || endptr == param_str || *endptr != '\0' || val > 0xFF) {
                    char error_msg[256];
                    snprintf(error_msg, sizeof(error_msg),
                             "Invalid hex byte for %s: %s (expected 00-FF)",
                             param->name, param_str ? param_str : "(null)");
                    ui_print_error(error_msg);
                    return -1;
                }
                parsed->value.u8 = (unsigned char)val;
                break;
            }
            case PARAM_TYPE_UINT16:
            {
                unsigned short val;
                unsigned short max_val = (unsigned short)(param->max_value == 0 ? 0xFFFF : param->max_value);
                if (uci_cmd_validate_uint16(param_str, &val,
                                            (unsigned short)param->min_value,
                                            max_val) != 0) {
                    char error_msg[256];
                    snprintf(error_msg, sizeof(error_msg), "Invalid parameter value for %s: %s (expected uint16)", 
                                   param->name, param_str);
                    ui_print_error(error_msg);
                    return -1;
                }
                parsed->value.u16 = val;
                break;
            }
            case PARAM_TYPE_UINT32:
            {
                unsigned int val;
                unsigned int max_val = (unsigned int)(param->max_value == 0 ? 0xFFFFFFFFu : param->max_value);
                if (uci_cmd_validate_uint32(param_str, &val,
                                            (unsigned int)param->min_value,
                                            max_val) != 0) {
                    char error_msg[256];
                    snprintf(error_msg, sizeof(error_msg), "Invalid parameter value for %s: %s (expected uint32)", 
                                   param->name, param_str);
                    ui_print_error(error_msg);
                    return -1;
                }
                parsed->value.u32 = val;
                break;
            }
            case PARAM_TYPE_UINT64:
            {
                unsigned long long max_val = (param->max_value == 0) ? 0xFFFFFFFFFFFFFFFFULL : param->max_value;
                unsigned long long val;
                if (uci_cmd_validate_uint64(param_str, &val, param->min_value, max_val) != 0) {
                    char error_msg[256];
                    snprintf(error_msg, sizeof(error_msg), "Invalid parameter value for %s: %s (expected uint64)", 
                                   param->name, param_str);
                    ui_print_error(error_msg);
                    return -1;
                }
                parsed->value.u64 = val;
                break;
            }
            case PARAM_TYPE_SESSION_ID:
            {
                unsigned int session_id;
                if (uci_cmd_validate_session_id(param_str, &session_id) != 0) {
                    char error_msg[256];
                    snprintf(error_msg, sizeof(error_msg), "Invalid session ID: %s", param_str);
                    ui_print_error(error_msg);
                    return -1;
                }
                parsed->value.session_id = session_id;
                break;
            }
            case PARAM_TYPE_HEX_STRING:
            {
                size_t allowed_len = (param->max_len > 0) ? param->max_len : UCI_CMD_MAX_HEX_PARAM_LEN;
                if (allowed_len > UCI_CMD_MAX_HEX_PARAM_LEN) {
                    allowed_len = UCI_CMD_MAX_HEX_PARAM_LEN;
                }
                size_t parsed_len = UCI_CMD_MAX_HEX_PARAM_LEN;
                if (uci_cmd_validate_hex_string(param_str,
                                                parsed->value.hex_bytes,
                                                &parsed_len,
                                                allowed_len) != 0) {
                    char error_msg[256];
                    snprintf(error_msg, sizeof(error_msg), "Invalid hex string: %s", param_str);
                    ui_print_error(error_msg);
                    return -1;
                }
                parsed->parsed_length = parsed_len;
                break;
            }
            case PARAM_TYPE_DEVICE_STATE:
            {
                unsigned char state;
                if (uci_cmd_validate_device_state(param_str, &state) != 0) {
                    char error_msg[256];
                    snprintf(error_msg, sizeof(error_msg), "Invalid device state: %s (expected active, ready, sleep, or suspend)", param_str);
                    ui_print_error(error_msg);
                    return -1;
                }
                parsed->value.device_state = state;
                break;
            }
            case PARAM_TYPE_SESSION_TYPE:
            {
                unsigned char type;
                if (uci_cmd_validate_session_type(param_str, &type) != 0) {
                    char error_msg[256];
                    snprintf(error_msg, sizeof(error_msg), "Invalid session type: %s", param_str);
                    ui_print_error(error_msg);
                    return -1;
                }
                parsed->value.session_type = type;
                break;
            }
            case PARAM_TYPE_STRING:
            {
                if (param->max_len > 0 && parsed->raw_length > param->max_len) {
                    char error_msg[256];
                    snprintf(error_msg, sizeof(error_msg),
                             "Parameter %s exceeds max length (%zu > %zu)",
                             param->name, parsed->raw_length, param->max_len);
                    ui_print_error(error_msg);
                    return -1;
                }
                break;
            }
            default:
                // For other types, just check if the string is not empty for required params
                if (param->flags & PARAM_FLAG_REQUIRED && strlen(param_str) == 0) {
                    char error_msg[256];
                    snprintf(error_msg, sizeof(error_msg), "Required parameter %s cannot be empty", param->name);
                    ui_print_error(error_msg);
                    return -1;
                }
                break;
        }
    }

    return 0;
}

// Main command dispatch function
int uci_cmd_dispatch(const uci_command_def_t* cmd_def, int argc, char** argv) {
    if (!cmd_def || !argv) {
        return -1;
    }

    // Check if command requires hardware mode
    if ((cmd_def->flags & CLI_CMD_FLAG_REQUIRES_HW_MODE) && !uci_is_hardware_mode_enabled()) {
        ui_print_error("Command requires hardware mode. Run hw_init or mode_hw first.");
        return -1;
    }

    // Validate parameters
    if (uci_cmd_validate_params(cmd_def, argc, argv) != 0) {
        return -1;
    }

    // Call the command handler
    if (cmd_def->handler) {
        return cmd_def->handler(cmd_def->name, argc, argv, cmd_def->params, cmd_def->param_count);
    }

    return 0;
}

const uci_cmd_parsed_param_t* uci_cmd_get_parsed_param(int index) {
    if (index < 0 || index >= g_parsed_param_count) {
        return NULL;
    }
    return &g_parsed_params[index];
}

int uci_cmd_get_parsed_param_count(void) {
    return g_parsed_param_count;
}
int uci_cmd_validate_uint64(const char* str, unsigned long long* value, unsigned long long min_val, unsigned long long max_val) {
    if (!str || !value) {
        return -1;
    }

    char* endptr;
    unsigned long long val = strtoull(str, &endptr, 0);

    if (*endptr != '\0' || val < min_val || val > max_val) {
        return -1;
    }

    *value = val;
    return 0;
}
