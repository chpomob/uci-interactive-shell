#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include "../include/uci.h"
#include "../include/uci_functions.h"
#include "../include/uci_pdl.h"
#include "uci_command_utils.h"
#include "../include/uci_config_manager.h"
#include "../include/uci_cmd_session.h"

// Helper function to encode session_id in little-endian format
static void encode_session_id_le(unsigned char* payload, uint32_t session_id) {
    payload[0] = (unsigned char)(session_id & 0xFF);           // LSB first
    payload[1] = (unsigned char)((session_id >> 8) & 0xFF);
    payload[2] = (unsigned char)((session_id >> 16) & 0xFF);
    payload[3] = (unsigned char)((session_id >> 24) & 0xFF);   // MSB last
}

static void print_session_usage(void) {
    printf("Session commands:\n");
    printf("  session_init <session_id> <session_type>\n");
    printf("  session_deinit <session_id>\n");
    printf("  session_start <session_id>\n");
    printf("  session_stop <session_id>\n");
    printf("  get_session_state <session_id>\n");
    printf("  session_logical_link_create <session_id> [link_id] [mode] [credit]\n");
    printf("  session_logical_link_close <session_id> <link_id>\n");
    printf("  session_logical_link_get_param <session_id> <link_id>\n");
}

static int parse_session_id(const char* session_id_str, uint32_t* session_id_out) {
    if (!session_id_str || !session_id_out) {
        print_session_usage();
        return -1;
    }

    char* endptr = NULL;
    unsigned long session_id_ul = strtoul(session_id_str, &endptr, 0);
    if (endptr == session_id_str || (endptr && *endptr != '\0')) {
        printf("Invalid session_id: %s\n", session_id_str);
        return -1;
    }
    if (session_id_ul > 0xFFFFFFFFUL) {
        printf("Error: session_id out of range (must be 0-0xFFFFFFFF).\n");
        return -1;
    }

    *session_id_out = (uint32_t)session_id_ul;
    return 0;
}

static int parse_session_and_link_ids(const char* session_id_str,
                                      const char* link_id_str,
                                      uint32_t* session_id_out,
                                      uint8_t* link_id_out) {
    if (parse_session_id(session_id_str, session_id_out) != 0) {
        return -1;
    }

    if (!link_id_str || !link_id_out) {
        print_session_usage();
        return -1;
    }

    char* endptr = NULL;
    unsigned long link_id_ul = strtoul(link_id_str, &endptr, 0);
    if (endptr == link_id_str || (endptr && *endptr != '\0') || link_id_ul > 0xFFUL) {
        printf("Error: link_id must be 0-255.\n");
        return -1;
    }

    *link_id_out = (uint8_t)link_id_ul;
    return 0;
}

static void print_session_type_help(void) {
    printf("  Supported session types:\n");
    printf("    - fira_ranging / ranging (0x00)\n");
    printf("    - fira_ranging_and_data / ranging_and_data (0x01)\n");
    printf("    - fira_data_transfer / data_transfer (0x02)\n");
    printf("    - fira_ranging_only / ranging_only (0x03)\n");
    printf("    - fira_in_band_data / in_band_data (0x04)\n");
    printf("    - fira_ranging_with_data / ranging_with_data (0x05)\n");
}

static SessionType parse_session_type(const char* session_type_str) {
    if (!session_type_str) {
        return (SessionType)0xFF;
    }

    struct {
        const char* name;
        const char* alias;
        SessionType type;
    } mapping[] = {
        { "fira_ranging", "ranging", FIRA_RANGING_SESSION },
        { "fira_ranging_and_data", "ranging_and_data", FIRA_RANGING_AND_IN_BAND_DATA_SESSION },
        { "fira_data_transfer", "data_transfer", FIRA_DATA_TRANSFER_SESSION },
        { "fira_ranging_only", "ranging_only", FIRA_RANGING_ONLY_PHASE },
        { "fira_in_band_data", "in_band_data", FIRA_IN_BAND_DATA_PHASE },
        { "fira_ranging_with_data", "ranging_with_data", FIRA_RANGING_WITH_DATA_PHASE },
    };

    for (size_t i = 0; i < sizeof(mapping) / sizeof(mapping[0]); i++) {
        if (strcasecmp(session_type_str, mapping[i].name) == 0 ||
            (mapping[i].alias && strcasecmp(session_type_str, mapping[i].alias) == 0)) {
            return mapping[i].type;
        }
    }

    return (SessionType)0xFF;
}

static int parse_uint_in_range(const char* value_str,
                               unsigned long max_value,
                               const char* label,
                               unsigned long* out_value) {
    if (!value_str || !out_value) {
        print_session_usage();
        return -1;
    }

    char* endptr = NULL;
    unsigned long parsed = strtoul(value_str, &endptr, 0);
    if (endptr == value_str || (endptr && *endptr != '\0') || parsed > max_value) {
        printf("Error: %s must be 0-%lu.\n", label, max_value);
        return -1;
    }

    *out_value = parsed;
    return 0;
}

int handle_session_init_command(char* session_id_str, char* session_type_str) {
    uint32_t session_id = 0;
    if (parse_session_id(session_id_str, &session_id) != 0 || !session_type_str) {
        printf("Usage: session_init <session_id> <session_type>\n");
        print_session_type_help();
        return -1;
    }

    SessionType session_type = parse_session_type(session_type_str);
    if (session_type == (SessionType)0xFF) {
        printf("Unknown session_type: %s\n", session_type_str);
        print_session_type_help();
        return -1;
    }

    return handle_session_init_command_values(session_id, session_type);
}

int handle_session_init_command_values(uint32_t session_id, SessionType session_type) {
    unsigned char payload[5];
    encode_session_id_le(payload, session_id);
    payload[4] = (unsigned char)session_type;
    send_uci_command(COMMAND, 0, SESSION_CONFIG, SESSION_INIT, payload, sizeof(payload));
    return 0;
}

int handle_session_deinit_command(char* session_id_str) {
    uint32_t session_id = 0;
    if (parse_session_id(session_id_str, &session_id) != 0) {
        printf("Usage: session_deinit <session_id>\n");
        return -1;
    }

    return handle_session_deinit_command_value(session_id);
}

int handle_session_deinit_command_value(uint32_t session_id) {
    unsigned char payload[4];
    encode_session_id_le(payload, session_id);
    send_uci_command(COMMAND, 0, SESSION_CONFIG, SESSION_DEINIT, payload, sizeof(payload));
    return 0;
}

int handle_session_start_command(char* session_id_str) {
    uint32_t session_id = 0;
    if (parse_session_id(session_id_str, &session_id) != 0) {
        printf("Usage: session_start <session_id>\n");
        printf("  Alternative: start_ranging <session_id>\n");
        return -1;
    }

    return handle_session_start_command_value(session_id);
}

int handle_session_start_command_value(uint32_t session_id) {
    unsigned char payload[4];
    encode_session_id_le(payload, session_id);
    send_uci_command(COMMAND, 0, SESSION_CONTROL, SESSION_START, payload, sizeof(payload));
    return 0;
}

int handle_session_stop_command(char* session_id_str) {
    uint32_t session_id = 0;
    if (parse_session_id(session_id_str, &session_id) != 0) {
        printf("Usage: session_stop <session_id>\n");
        printf("  Alternative: stop_ranging <session_id>\n");
        return -1;
    }

    return handle_session_stop_command_value(session_id);
}

int handle_session_stop_command_value(uint32_t session_id) {
    unsigned char payload[4];
    encode_session_id_le(payload, session_id);
    send_uci_command(COMMAND, 0, SESSION_CONTROL, SESSION_STOP, payload, sizeof(payload));
    return 0;
}

int handle_get_session_state_command(char* session_id_str) {
    uint32_t session_id = 0;
    if (parse_session_id(session_id_str, &session_id) != 0) {
        printf("Usage: get_session_state <session_id>\n");
        printf("  Alternative: session_status <session_id>\n");
        return -1;
    }

    return handle_get_session_state_command_value(session_id);
}

int handle_get_session_state_command_value(uint32_t session_id) {
    unsigned char payload[4];
    encode_session_id_le(payload, session_id);
    send_uci_command(COMMAND, 0, SESSION_CONFIG, SESSION_GET_STATE, payload, sizeof(payload));
    return 0;
}

int handle_session_send_data_command(char* session_id_str,
                                    char* destination_str,
                                    char* sequence_str,
                                    char* payload_str) {
    uint32_t session_id = 0;
    if (parse_session_id(session_id_str, &session_id) != 0 ||
        !destination_str || !sequence_str || !payload_str) {
        printf("Usage: session_send_data <session_id> <dest_address_hex> <sequence_number> <payload_hex>\n");
        printf("  Example: session_send_data 1 0x0011223344556677 1 AABBCC\n");
        return -1;
    }

    char* endptr = NULL;
    uint64_t destination = strtoull(destination_str, &endptr, 0);
    if (endptr == destination_str || (endptr && *endptr != '\0')) {
        printf("Invalid dest_address_hex: %s\n", destination_str);
        return -1;
    }

    unsigned long sequence_ul = 0;
    if (parse_uint_in_range(sequence_str, 0xFFFF, "sequence_number", &sequence_ul) != 0) {
        return -1;
    }

    unsigned char data_buffer[512];
    size_t data_len = sizeof(data_buffer);
    if (uci_config_parse_hex_value(payload_str, data_buffer, &data_len) != 0) {
        printf("Invalid payload_hex. Use hexadecimal characters (e.g., AABBCC).\n");
        return -1;
    }

    return handle_session_send_data_command_values(session_id,
                                                   destination,
                                                   (uint16_t)sequence_ul,
                                                   data_buffer,
                                                   data_len);
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

int handle_session_logical_link_create_command(char* session_id_str,
                                               char* link_id_str,
                                               char* mode_str,
                                               char* credit_str) {
    uint32_t session_id = 0;
    if (parse_session_id(session_id_str, &session_id) != 0) {
        printf("Usage: session_logical_link_create <session_id> [link_id] [mode] [credit]\n");
        return -1;
    }

    unsigned char payload[7];
    encode_session_id_le(payload, session_id);

    size_t payload_len = 5;
    unsigned long link_id = 0xFFUL;
    if (link_id_str) {
        if (parse_uint_in_range(link_id_str, 0xFF, "link_id", &link_id) != 0) {
            return -1;
        }
    }
    payload[4] = (unsigned char)link_id;

    if (mode_str) {
        unsigned long mode_val = 0;
        if (parse_uint_in_range(mode_str, 0xFF, "mode", &mode_val) != 0) {
            return -1;
        }
        payload[5] = (unsigned char)mode_val;
        payload_len = 6;
    }

    if (credit_str) {
        unsigned long credit_val = 0;
        if (parse_uint_in_range(credit_str, 0xFF, "credit", &credit_val) != 0) {
            return -1;
        }
        payload[6] = (unsigned char)credit_val;
        payload_len = 7;
    } else if (mode_str) {
        payload[6] = 1;
        payload_len = 7;
    }

    send_uci_command(COMMAND, 0, SESSION_CONTROL, SESSION_LOGICAL_LINK_CREATE,
                     payload, (int)payload_len);
    return 0;
}

int handle_session_logical_link_close_command(char* session_id_str,
                                              char* link_id_str) {
    uint32_t session_id = 0;
    uint8_t link_id = 0;
    if (parse_session_and_link_ids(session_id_str, link_id_str, &session_id, &link_id) != 0) {
        printf("Usage: session_logical_link_close <session_id> <link_id>\n");
        return -1;
    }

    unsigned char payload[5];
    encode_session_id_le(payload, session_id);
    payload[4] = link_id;

    send_uci_command(COMMAND, 0, SESSION_CONTROL, SESSION_LOGICAL_LINK_CLOSE,
                     payload, sizeof(payload));
    return 0;
}

int handle_session_logical_link_get_param_command(char* session_id_str,
                                                  char* link_id_str) {
    uint32_t session_id = 0;
    uint8_t link_id = 0;
    if (parse_session_and_link_ids(session_id_str, link_id_str, &session_id, &link_id) != 0) {
        printf("Usage: session_logical_link_get_param <session_id> <link_id>\n");
        return -1;
    }

    unsigned char payload[5];
    encode_session_id_le(payload, session_id);
    payload[4] = link_id;

    send_uci_command(COMMAND, 0, SESSION_CONTROL, SESSION_LOGICAL_LINK_GET_PARAM,
                     payload, sizeof(payload));
    return 0;
}
