#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../include/uci.h"
#include "../include/uci_functions.h"
#include "../include/uci_pdl.h"
#include "../include/uci_config_manager.h"

// Helper function to encode session_id in little-endian format
static void encode_session_id_le(unsigned char* payload, unsigned int session_id) {
    payload[0] = session_id & 0xFF;           // LSB first
    payload[1] = (session_id >> 8) & 0xFF;
    payload[2] = (session_id >> 16) & 0xFF;
    payload[3] = (session_id >> 24) & 0xFF;   // MSB last
}

int handle_session_init_command(char* session_id_str, char* session_type_str) {
    if (!session_id_str || !session_type_str) {
        printf("Usage: session_init <session_id> <session_type>\n");
        printf("  Examples:\n");
        printf("    session_init 1 fira_ranging\n");
        printf("    session_init 1 fira_ranging_and_data\n");
        printf("    session_init 1 fira_data_transfer\n");
        printf("    session_init 1 fira_ranging_only\n");
        printf("    session_init 1 fira_in_band_data\n");
        printf("    session_init 1 fira_ranging_with_data\n");
        printf("  Alternative names:\n");
        printf("    session_new 1 ranging\n");
        printf("    session_new 1 ranging_and_data\n");
        printf("    session_new 1 data_transfer\n");
        printf("    session_new 1 ranging_only\n");
        printf("    session_new 1 in_band_data\n");
        printf("    session_new 1 ranging_with_data\n");
        return -1;
    }

    unsigned int session_id = (unsigned int)strtoul(session_id_str, NULL, 10);
    SessionType session_type;

    // Support both technical and friendly names for session types
    if (strcmp(session_type_str, "fira_ranging") == 0 || strcmp(session_type_str, "ranging") == 0) {
        session_type = FIRA_RANGING_SESSION;
    } else if (strcmp(session_type_str, "fira_ranging_and_data") == 0 || strcmp(session_type_str, "ranging_and_data") == 0) {
        session_type = FIRA_RANGING_AND_IN_BAND_DATA_SESSION;
    } else if (strcmp(session_type_str, "fira_data_transfer") == 0 || strcmp(session_type_str, "data_transfer") == 0) {
        session_type = FIRA_DATA_TRANSFER_SESSION;
    } else if (strcmp(session_type_str, "fira_ranging_only") == 0 || strcmp(session_type_str, "ranging_only") == 0) {
        session_type = FIRA_RANGING_ONLY_PHASE;
    } else if (strcmp(session_type_str, "fira_in_band_data") == 0 || strcmp(session_type_str, "in_band_data") == 0) {
        session_type = FIRA_IN_BAND_DATA_PHASE;
    } else if (strcmp(session_type_str, "fira_ranging_with_data") == 0 || strcmp(session_type_str, "ranging_with_data") == 0) {
        session_type = FIRA_RANGING_WITH_DATA_PHASE;
    } else {
        printf("Unknown session_type: %s\n", session_type_str);
        printf("  Supported types:\n");
        printf("    - fira_ranging / ranging (0x00)\n");
        printf("    - fira_ranging_and_data / ranging_and_data (0x01)\n");
        printf("    - fira_data_transfer / data_transfer (0x02)\n");
        printf("    - fira_ranging_only / ranging_only (0x03)\n");
        printf("    - fira_in_band_data / in_band_data (0x04)\n");
        printf("    - fira_ranging_with_data / ranging_with_data (0x05)\n");
        return -1;
    }

    unsigned char payload[5];
    encode_session_id_le(payload, session_id);
    payload[4] = session_type;
    send_uci_command(COMMAND, 0, SESSION_CONFIG, SESSION_INIT, payload, sizeof(payload));
    return 0;
}

int handle_session_deinit_command(char* session_id_str) {
    if (!session_id_str) {
        printf("Usage: session_deinit <session_id>\n");
        return -1;
    }

    unsigned int session_id = (unsigned int)strtoul(session_id_str, NULL, 10);
    unsigned char payload[4];
    encode_session_id_le(payload, session_id);
    send_uci_command(COMMAND, 0, SESSION_CONFIG, SESSION_DEINIT, payload, sizeof(payload));
    return 0;
}

int handle_session_start_command(char* session_id_str) {
    if (!session_id_str) {
        printf("Usage: session_start <session_id>\n");
        printf("  Alternative: start_ranging <session_id>\n");
        return -1;
    }

    unsigned int session_id = (unsigned int)strtoul(session_id_str, NULL, 10);
    unsigned char payload[4];
    encode_session_id_le(payload, session_id);
    send_uci_command(COMMAND, 0, SESSION_CONTROL, SESSION_START, payload, sizeof(payload));
    return 0;
}

int handle_session_stop_command(char* session_id_str) {
    if (!session_id_str) {
        printf("Usage: session_stop <session_id>\n");
        printf("  Alternative: stop_ranging <session_id>\n");
        return -1;
    }

    unsigned int session_id = (unsigned int)strtoul(session_id_str, NULL, 10);
    unsigned char payload[4];
    encode_session_id_le(payload, session_id);
    send_uci_command(COMMAND, 0, SESSION_CONTROL, SESSION_STOP, payload, sizeof(payload));
    return 0;
}

int handle_get_session_state_command(char* session_id_str) {
    if (!session_id_str) {
        printf("Usage: get_session_state <session_id>\n");
        printf("  Alternative: session_status <session_id>\n");
        return -1;
    }

    unsigned int session_id = (unsigned int)strtoul(session_id_str, NULL, 10);
    unsigned char payload[4];
    encode_session_id_le(payload, session_id);
    send_uci_command(COMMAND, 0, SESSION_CONFIG, SESSION_GET_STATE, payload, sizeof(payload));
    return 0;
}

int handle_session_send_data_command(char* session_id_str,
                                    char* destination_str,
                                    char* sequence_str,
                                    char* payload_str) {
    if (!session_id_str || !destination_str || !sequence_str || !payload_str) {
        printf("Usage: session_send_data <session_id> <dest_address_hex> <sequence_number> <payload_hex>\n");
        printf("  Example: session_send_data 1 0x0011223344556677 1 AABBCC\n");
        return -1;
    }

    unsigned long session_id_ul = strtoul(session_id_str, NULL, 10);
    uint64_t destination = strtoull(destination_str, NULL, 0);
    unsigned long sequence_ul = strtoul(sequence_str, NULL, 0);

    if (sequence_ul > 0xFFFF) {
        printf("Invalid sequence_number: must be 0-65535.\n");
        return -1;
    }

    unsigned char data_buffer[512];
    size_t data_len = sizeof(data_buffer);
    if (uci_config_parse_hex_value(payload_str, data_buffer, &data_len) != 0) {
        printf("Invalid payload_hex. Use hexadecimal characters (e.g., AABBCC).\n");
        return -1;
    }

    uci_send_data_message((uint32_t)session_id_ul, destination,
                          (uint16_t)sequence_ul, data_buffer, data_len);
    return 0;
}

int handle_session_logical_link_create_command(char* session_id_str,
                                               char* link_id_str,
                                               char* mode_str,
                                               char* credit_str) {
    if (!session_id_str) {
        printf("Usage: session_logical_link_create <session_id> [link_id] [mode] [credit]\n");
        return -1;
    }

    unsigned long session_id_ul = strtoul(session_id_str, NULL, 10);
    if (session_id_ul > 0xFFFFFFFFUL) {
        printf("Error: session_id out of range.\n");
        return -1;
    }

    unsigned char payload[7];
    encode_session_id_le(payload, (uint32_t)session_id_ul);

    size_t payload_len = 5;
    unsigned long link_id = 0xFFUL;
    if (link_id_str) {
        link_id = strtoul(link_id_str, NULL, 0);
        if (link_id > 0xFFUL) {
            printf("Error: link_id must be 0-255.\n");
            return -1;
        }
    }
    payload[4] = (unsigned char)link_id;

    unsigned long mode = 0;
    if (mode_str) {
        mode = strtoul(mode_str, NULL, 0);
        if (mode > 0xFFUL) {
            printf("Error: mode must be 0-255.\n");
            return -1;
        }
        payload[5] = (unsigned char)mode;
        payload_len = 6;
    }

    unsigned long credit = 1;
    if (credit_str) {
        credit = strtoul(credit_str, NULL, 0);
        if (credit > 0xFFUL) {
            printf("Error: credit must be 0-255.\n");
            return -1;
        }
        payload[6] = (unsigned char)credit;
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
    if (!session_id_str || !link_id_str) {
        printf("Usage: session_logical_link_close <session_id> <link_id>\n");
        return -1;
    }

    unsigned long session_id_ul = strtoul(session_id_str, NULL, 10);
    unsigned long link_id_ul = strtoul(link_id_str, NULL, 0);
    if (session_id_ul > 0xFFFFFFFFUL || link_id_ul > 0xFFUL) {
        printf("Error: session_id or link_id out of range.\n");
        return -1;
    }

    unsigned char payload[5];
    encode_session_id_le(payload, (uint32_t)session_id_ul);
    payload[4] = (unsigned char)link_id_ul;

    send_uci_command(COMMAND, 0, SESSION_CONTROL, SESSION_LOGICAL_LINK_CLOSE,
                     payload, sizeof(payload));
    return 0;
}

int handle_session_logical_link_get_param_command(char* session_id_str,
                                                  char* link_id_str) {
    if (!session_id_str || !link_id_str) {
        printf("Usage: session_logical_link_get_param <session_id> <link_id>\n");
        return -1;
    }

    unsigned long session_id_ul = strtoul(session_id_str, NULL, 10);
    unsigned long link_id_ul = strtoul(link_id_str, NULL, 0);
    if (session_id_ul > 0xFFFFFFFFUL || link_id_ul > 0xFFUL) {
        printf("Error: session_id or link_id out of range.\n");
        return -1;
    }

    unsigned char payload[5];
    encode_session_id_le(payload, (uint32_t)session_id_ul);
    payload[4] = (unsigned char)link_id_ul;

    send_uci_command(COMMAND, 0, SESSION_CONTROL, SESSION_LOGICAL_LINK_GET_PARAM,
                     payload, sizeof(payload));
    return 0;
}
