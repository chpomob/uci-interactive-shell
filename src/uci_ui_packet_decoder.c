#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/uci.h"
#include "../include/uci_ui.h"
#include "../include/uci_ui_packet_decoder.h"

// Enhanced UI version of analyze_uci_packet
void ui_analyze_uci_packet(unsigned char* packet, size_t packet_len) {
    if (packet_len < sizeof(struct uci_packet_header)) {
        if (ui_color_enabled) {
            printf("%s%sError: UCI packet too short to contain a header (need at least %zu bytes, got %zu)%s\n", 
                   ANSI_COLOR_RED, ANSI_BOLD, sizeof(struct uci_packet_header), packet_len, ANSI_RESET);
        } else {
            printf("Error: UCI packet too short to contain a header (need at least %zu bytes, got %zu)\n", 
                   sizeof(struct uci_packet_header), packet_len);
        }
        return;
    }

    struct uci_packet_header* header = (struct uci_packet_header*)packet;
    uci_header_fields_t header_fields;
    uci_extract_header_fields(header, &header_fields);

    if (ui_color_enabled) {
        printf("%s%s%s=== UCI Packet Analysis ===%s\n", 
               ANSI_COLOR_BRIGHT_CYAN, ANSI_BOLD, ANSI_BG_BLUE, ANSI_RESET);
        printf("%s%sTotal Packet Length:%s %zu bytes\n", 
               ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, packet_len);
        printf("%s%sHeader Bytes:%s %02X %02X %02X %02X\n", 
               ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, 
               packet[0], packet[1], packet[2], packet[3]);
    } else {
        printf("=== UCI Packet Analysis ===\n");
        printf("Total Packet Length: %zu bytes\n", packet_len);
        printf("Header Bytes: %02X %02X %02X %02X\n", 
               packet[0], packet[1], packet[2], packet[3]);
    }

    // Extract header fields
    unsigned char gid = header_fields.group_id;
    unsigned char pbf = header_fields.packet_boundary;
    unsigned char mt = header_fields.message_type;
    unsigned char opcode = header_fields.opcode_id;
    unsigned char opcode_reserved_bits = header_fields.reserved_opcode_bits;
    unsigned char payload_len_field = header_fields.payload_length;

    if (ui_color_enabled) {
        printf("  %s%sMessage Type (MT):%s 0x%01X", 
               ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_RESET, mt);
    } else {
        printf("  Message Type (MT): 0x%01X", mt);
    }
    switch(mt) {
        case DATA: 
            if (ui_color_enabled) {
                printf(" %s(DATA)%s", ANSI_COLOR_GREEN, ANSI_RESET);
            } else {
                printf(" (DATA)");
            }
            break;
        case COMMAND: 
            if (ui_color_enabled) {
                printf(" %s(COMMAND)%s", ANSI_COLOR_BRIGHT_BLUE, ANSI_RESET);
            } else {
                printf(" (COMMAND)");
            }
            break;
        case RESPONSE: 
            if (ui_color_enabled) {
                printf(" %s(RESPONSE)%s", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET);
            } else {
                printf(" (RESPONSE)");
            }
            break;
        case NOTIFICATION: 
            if (ui_color_enabled) {
                printf(" %s(NOTIFICATION)%s", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_RESET);
            } else {
                printf(" (NOTIFICATION)");
            }
            break;
        default: 
            if (ui_color_enabled) {
                printf(" %s(UNKNOWN)%s", ANSI_COLOR_RED, ANSI_RESET);
            } else {
                printf(" (UNKNOWN)");
            }
            break;
    }
    printf("\n");

    if (ui_color_enabled) {
        printf("  %s%sPacket Boundary Flag (PBF):%s 0x%01X", 
               ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_RESET, pbf);
    } else {
        printf("  Packet Boundary Flag (PBF): 0x%01X", pbf);
    }
    switch(pbf) {
        case COMPLETE: 
            if (ui_color_enabled) {
                printf(" %s(COMPLETE)%s", ANSI_COLOR_GREEN, ANSI_RESET);
            } else {
                printf(" (COMPLETE)");
            }
            break;
        case NOT_COMPLETE: 
            if (ui_color_enabled) {
                printf(" %s(NOT_COMPLETE)%s", ANSI_COLOR_YELLOW, ANSI_RESET);
            } else {
                printf(" (NOT_COMPLETE)");
            }
            break;
        default: 
            if (ui_color_enabled) {
                printf(" %s(UNKNOWN)%s", ANSI_COLOR_RED, ANSI_RESET);
            } else {
                printf(" (UNKNOWN)");
            }
            break;
    }
    printf("\n");

    if (ui_color_enabled) {
        printf("  %s%sGroup ID (GID):%s 0x%01X", 
               ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_RESET, gid);
    } else {
        printf("  Group ID (GID): 0x%01X", gid);
    }
    switch(gid) {
        case CORE: 
            if (ui_color_enabled) {
                printf(" %s(CORE)%s", ANSI_COLOR_BRIGHT_BLUE, ANSI_RESET);
            } else {
                printf(" (CORE)");
            }
            break;
        case SESSION_CONFIG: 
            if (ui_color_enabled) {
                printf(" %s(SESSION_CONFIG)%s", ANSI_COLOR_BRIGHT_BLUE, ANSI_RESET);
            } else {
                printf(" (SESSION_CONFIG)");
            }
            break;
        case SESSION_CONTROL: 
            if (ui_color_enabled) {
                printf(" %s(SESSION_CONTROL)%s", ANSI_COLOR_BRIGHT_BLUE, ANSI_RESET);
            } else {
                printf(" (SESSION_CONTROL)");
            }
            break;
        case DATA_CONTROL: 
            if (ui_color_enabled) {
                printf(" %s(DATA_CONTROL)%s", ANSI_COLOR_BRIGHT_BLUE, ANSI_RESET);
            } else {
                printf(" (DATA_CONTROL)");
            }
            break;
        case RANGING_DATA: 
            if (ui_color_enabled) {
                printf(" %s(RANGING_DATA)%s", ANSI_COLOR_BRIGHT_BLUE, ANSI_RESET);
            } else {
                printf(" (RANGING_DATA)");
            }
            break;
        case TEST: 
            if (ui_color_enabled) {
                printf(" %s(TEST)%s", ANSI_COLOR_BRIGHT_BLUE, ANSI_RESET);
            } else {
                printf(" (TEST)");
            }
            break;
        case VENDOR_ANDROID: 
            if (ui_color_enabled) {
                printf(" %s(VENDOR_ANDROID)%s", ANSI_COLOR_BRIGHT_BLUE, ANSI_RESET);
            } else {
                printf(" (VENDOR_ANDROID)");
            }
            break;
        default: 
            if (ui_color_enabled) {
                printf(" %s(UNKNOWN)%s", ANSI_COLOR_RED, ANSI_RESET);
            } else {
                printf(" (UNKNOWN)");
            }
            break;
    }
    printf("\n");

    if (ui_color_enabled) {
        printf("  %s%sOpcode:%s 0x%02X\n", 
               ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_RESET, opcode);
        printf("  %s%sReserved Opcode Bits:%s 0x%01X\n", 
               ANSI_COLOR_BRIGHT_BLACK, ANSI_BOLD, ANSI_RESET, opcode_reserved_bits);
        printf("  %s%sPayload Length:%s %u bytes\n", 
               ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_RESET, payload_len_field);
    } else {
        printf("  Opcode: 0x%02X\n", opcode);
        printf("  Reserved Opcode Bits: 0x%01X\n", opcode_reserved_bits);
        printf("  Payload Length: %u bytes\n", payload_len_field);
    }

    // Analyze payload if present
    if (payload_len_field > 0) {
        size_t available_payload = packet_len - sizeof(struct uci_packet_header);
        if (payload_len_field > available_payload) {
            if (ui_color_enabled) {
                printf("  %s%sWarning: Header payload length %u exceeds available data %zu. Clamping.%s\n", 
                       ANSI_COLOR_YELLOW, ANSI_BOLD, payload_len_field, available_payload, ANSI_RESET);
            } else {
                printf("  Warning: Header payload length %u exceeds available data %zu. Clamping.\n", 
                       payload_len_field, available_payload);
            }
            payload_len_field = (unsigned char)available_payload;
        }

        if (ui_color_enabled) {
            printf("  %s%sPayload:%s ", ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_RESET);
        } else {
            printf("  Payload: ");
        }
        for (size_t i = 0; i < payload_len_field && i < 32; i++) {
            if (ui_color_enabled) {
                printf("%s%02X%s ", ANSI_COLOR_BRIGHT_CYAN, packet[sizeof(struct uci_packet_header) + i], ANSI_RESET);
            } else {
                printf("%02X ", packet[sizeof(struct uci_packet_header) + i]);
            }
        }
        if (payload_len_field > 32) {
            if (ui_color_enabled) {
                printf("%s... (and %u more bytes)%s", ANSI_COLOR_BRIGHT_BLACK, payload_len_field - 32, ANSI_RESET);
            } else {
                printf("... (and %u more bytes)", payload_len_field - 32);
            }
        }
        printf("\n\n");

        // Call appropriate decoder based on MT, GID, and Opcode
        unsigned char* payload_ptr = packet + sizeof(struct uci_packet_header);
        int payload_len_int = (int)payload_len_field;

        if (mt == COMMAND && gid == SESSION_CONFIG) {
            switch(opcode) {
                case SESSION_INIT:
                    ui_decode_session_init_cmd(payload_ptr, payload_len_int);
                    break;
                case SESSION_DEINIT:
                    if (ui_color_enabled) {
                        printf("  %s%sSESSION_DEINIT_CMD - Session Deinitialization Command:%s\n",
                               ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
                    } else {
                        printf("  SESSION_DEINIT_CMD - Session Deinitialization Command:\n");
                    }
                    if (payload_len_int >= 4) {
                        unsigned int session_token = (unsigned int)payload_ptr[0] |
                                                    ((unsigned int)payload_ptr[1] << 8) |
                                                    ((unsigned int)payload_ptr[2] << 16) |
                                                    ((unsigned int)payload_ptr[3] << 24);
                        if (ui_color_enabled) {
                            printf("    %s%sSession Token:%s 0x%08X\n",
                                   ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_RESET, session_token);
                        } else {
                            printf("    Session Token: 0x%08X\n", session_token);
                        }
                    }
                    break;
                case SESSION_SET_APP_CONFIG:
                    if (ui_color_enabled) {
                        printf("  %s%sSESSION_SET_APP_CONFIG_CMD - Set Application Config Command:%s\n",
                               ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
                    } else {
                        printf("  SESSION_SET_APP_CONFIG_CMD - Set Application Config Command:\n");
                    }
                    if (payload_len_int >= 5) {
                        unsigned int session_token = (unsigned int)payload_ptr[0] |
                                                    ((unsigned int)payload_ptr[1] << 8) |
                                                    ((unsigned int)payload_ptr[2] << 16) |
                                                    ((unsigned int)payload_ptr[3] << 24);
                        unsigned char num_tlvs = payload_ptr[4];
                        if (ui_color_enabled) {
                            printf("    %s%sSession Token:%s 0x%08X\n",
                                   ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_RESET, session_token);
                            printf("    %s%sNumber of TLVs:%s %u\n",
                                   ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_RESET, num_tlvs);
                        } else {
                            printf("    Session Token: 0x%08X\n", session_token);
                            printf("    Number of TLVs: %u\n", num_tlvs);
                        }
                    }
                    break;
                default:
                    if (ui_color_enabled) {
                        printf("  %s%sNo specific decoder for SESSION_CONFIG_COMMAND opcode 0x%02X%s\n",
                               ANSI_COLOR_BRIGHT_BLACK, ANSI_BOLD, opcode, ANSI_RESET);
                    } else {
                        printf("  No specific decoder for SESSION_CONFIG_COMMAND opcode 0x%02X\n", opcode);
                    }
                    break;
            }
        } else if (mt == COMMAND && gid == SESSION_CONTROL) {
            switch(opcode) {
                case SESSION_START:
                    if (ui_color_enabled) {
                        printf("  %s%sSESSION_START_CMD - Start Session Command:%s\n",
                               ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
                    } else {
                        printf("  SESSION_START_CMD - Start Session Command:\n");
                    }
                    if (payload_len_int >= 4) {
                        unsigned int session_token = (unsigned int)payload_ptr[0] |
                                                    ((unsigned int)payload_ptr[1] << 8) |
                                                    ((unsigned int)payload_ptr[2] << 16) |
                                                    ((unsigned int)payload_ptr[3] << 24);
                        if (ui_color_enabled) {
                            printf("    %s%sSession Token:%s 0x%08X\n",
                                   ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_RESET, session_token);
                        } else {
                            printf("    Session Token: 0x%08X\n", session_token);
                        }
                    }
                    break;
                case SESSION_STOP:
                    if (ui_color_enabled) {
                        printf("  %s%sSESSION_STOP_CMD - Stop Session Command:%s\n",
                               ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
                    } else {
                        printf("  SESSION_STOP_CMD - Stop Session Command:\n");
                    }
                    if (payload_len_int >= 4) {
                        unsigned int session_token = (unsigned int)payload_ptr[0] |
                                                    ((unsigned int)payload_ptr[1] << 8) |
                                                    ((unsigned int)payload_ptr[2] << 16) |
                                                    ((unsigned int)payload_ptr[3] << 24);
                        if (ui_color_enabled) {
                            printf("    %s%sSession Token:%s 0x%08X\n",
                                   ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_RESET, session_token);
                        } else {
                            printf("    Session Token: 0x%08X\n", session_token);
                        }
                    }
                    break;
                default:
                    if (ui_color_enabled) {
                        printf("  %s%sNo specific decoder for SESSION_CONTROL_COMMAND opcode 0x%02X%s\n",
                               ANSI_COLOR_BRIGHT_BLACK, ANSI_BOLD, opcode, ANSI_RESET);
                    } else {
                        printf("  No specific decoder for SESSION_CONTROL_COMMAND opcode 0x%02X\n", opcode);
                    }
                    break;
            }
        } else if (mt == RESPONSE && gid == CORE) {
            switch(opcode) {
                case CORE_DEVICE_INFO:
                    ui_decode_core_device_info_rsp(payload_ptr, payload_len_int);
                    break;
                case CORE_GET_CAPS_INFO:
                    ui_decode_core_get_caps_info_rsp(payload_ptr, payload_len_int);
                    break;
                case CORE_SET_CONFIG:
                    ui_decode_core_set_config_rsp(payload_ptr, payload_len_int);
                    break;
                case CORE_GET_CONFIG:
                    ui_decode_core_get_config_rsp(payload_ptr, payload_len_int);
                    break;
                case CORE_DEVICE_RESET:
                    ui_decode_core_device_reset_rsp(payload_ptr, payload_len_int);
                    break;
                case CORE_DEVICE_SUSPEND:
                    ui_decode_core_device_suspend_rsp(payload_ptr, payload_len_int);
                    break;
                case CORE_QUERY_UWBS_TIMESTAMP:
                    ui_decode_core_query_uwbs_timestamp_rsp(payload_ptr, payload_len_int);
                    break;
                default:
                    if (ui_color_enabled) {
                        printf("  %s%sNo specific decoder for CORE_RESPONSE opcode 0x%02X%s\n", 
                               ANSI_COLOR_BRIGHT_BLACK, ANSI_BOLD, opcode, ANSI_RESET);
                    } else {
                        printf("  No specific decoder for CORE_RESPONSE opcode 0x%02X\n", opcode);
                    }
                    break;
            }
        } else if (mt == NOTIFICATION && gid == CORE) {
            switch(opcode) {
                case CORE_DEVICE_STATUS_NTF:
                    ui_decode_core_device_status_ntf(payload_ptr, payload_len_int);
                    break;
                case CORE_GENERIC_ERROR_NTF:
                    ui_decode_core_generic_error_ntf(payload_ptr, payload_len_int);
                    break;
                default:
                    if (ui_color_enabled) {
                        printf("  %s%sNo specific decoder for CORE_NOTIFICATION opcode 0x%02X%s\n", 
                               ANSI_COLOR_BRIGHT_BLACK, ANSI_BOLD, opcode, ANSI_RESET);
                    } else {
                        printf("  No specific decoder for CORE_NOTIFICATION opcode 0x%02X\n", opcode);
                    }
                    break;
            }
        } else if (mt == RESPONSE && gid == SESSION_CONFIG) {
            switch(opcode) {
                case SESSION_INIT:
                    ui_decode_session_init_rsp(payload_ptr, payload_len_int);
                    break;
                case SESSION_DEINIT:
                    ui_decode_session_deinit_rsp(payload_ptr, payload_len_int);
                    break;
                case SESSION_SET_APP_CONFIG:
                    ui_decode_session_set_app_config_rsp(payload_ptr, payload_len_int);
                    break;
                case SESSION_GET_APP_CONFIG:
                    ui_decode_session_get_app_config_rsp(payload_ptr, payload_len_int);
                    break;
                case SESSION_GET_COUNT:
                    ui_decode_session_get_count_rsp(payload_ptr, payload_len_int);
                    break;
                case SESSION_GET_STATE:
                    ui_decode_session_get_state_rsp(payload_ptr, payload_len_int);
                    break;
                case SESSION_UPDATE_CONTROLLER_MULTICAST_LIST:
                    ui_decode_session_update_controller_multicast_list_rsp(payload_ptr, payload_len_int);
                    break;
                case SESSION_UPDATE_ACTIVE_ROUNDS_DT_TAG:
                    ui_decode_session_update_active_rounds_dt_tag_rsp(payload_ptr, payload_len_int);
                    break;
                case SESSION_DATA_TRANSFER_PHASE_CONFIG:
                    ui_decode_session_data_transfer_phase_config_rsp(payload_ptr, payload_len_int);
                    break;
                default:
                    if (ui_color_enabled) {
                        printf("  %s%sNo specific decoder for SESSION_CONFIG_RESPONSE opcode 0x%02X%s\n", 
                               ANSI_COLOR_BRIGHT_BLACK, ANSI_BOLD, opcode, ANSI_RESET);
                    } else {
                        printf("  No specific decoder for SESSION_CONFIG_RESPONSE opcode 0x%02X\n", opcode);
                    }
                    break;
            }
        } else if (mt == NOTIFICATION && gid == SESSION_CONFIG) {
            switch(opcode) {
                case SESSION_STATUS_NTF:
                    ui_decode_session_status_ntf(payload_ptr, payload_len_int);
                    break;
                case SESSION_DATA_CREDIT_NTF:
                    ui_decode_session_data_credit_ntf(payload_ptr, payload_len_int);
                    break;
                case SESSION_DATA_TRANSFER_STATUS_NTF:
                    ui_decode_session_data_transfer_status_ntf(payload_ptr, payload_len_int);
                    break;
                case SESSION_INFO_NTF:
                    ui_decode_session_info_ntf(payload_ptr, payload_len_int);
                    break;
                default:
                    if (ui_color_enabled) {
                        printf("  %s%sNo specific decoder for SESSION_CONFIG_NOTIFICATION opcode 0x%02X%s\n", 
                               ANSI_COLOR_BRIGHT_BLACK, ANSI_BOLD, opcode, ANSI_RESET);
                    } else {
                        printf("  No specific decoder for SESSION_CONFIG_NOTIFICATION opcode 0x%02X\n", opcode);
                    }
                    break;
            }
        } else if (mt == RESPONSE && gid == SESSION_CONTROL) {
            switch(opcode) {
                case SESSION_START:
                    ui_decode_session_start_rsp(payload_ptr, payload_len_int);
                    break;
                case SESSION_STOP:
                    ui_decode_session_stop_rsp(payload_ptr, payload_len_int);
                    break;
                case SESSION_GET_RANGING_COUNT:
                    ui_decode_session_get_ranging_count_rsp(payload_ptr, payload_len_int);
                    break;
                default:
                    if (ui_color_enabled) {
                        printf("  %s%sNo specific decoder for SESSION_CONTROL_RESPONSE opcode 0x%02X%s\n", 
                               ANSI_COLOR_BRIGHT_BLACK, ANSI_BOLD, opcode, ANSI_RESET);
                    } else {
                        printf("  No specific decoder for SESSION_CONTROL_RESPONSE opcode 0x%02X\n", opcode);
                    }
                    break;
            }
        } else if (mt == NOTIFICATION && gid == SESSION_CONTROL) {
            switch(opcode) {
                case SESSION_STATUS_NTF:
                    ui_decode_session_status_ntf(payload_ptr, payload_len_int);
                    break;
                case SESSION_DATA_CREDIT_NTF:
                    ui_decode_session_data_credit_ntf(payload_ptr, payload_len_int);
                    break;
                case SESSION_DATA_TRANSFER_STATUS_NTF:
                    ui_decode_session_data_transfer_status_ntf(payload_ptr, payload_len_int);
                    break;
                case SESSION_INFO_NTF:
                    ui_decode_session_info_ntf(payload_ptr, payload_len_int);
                    break;
                default:
                    if (ui_color_enabled) {
                        printf("  %s%sNo specific decoder for SESSION_CONTROL_NOTIFICATION opcode 0x%02X%s\n", 
                               ANSI_COLOR_BRIGHT_BLACK, ANSI_BOLD, opcode, ANSI_RESET);
                    } else {
                        printf("  No specific decoder for SESSION_CONTROL_NOTIFICATION opcode 0x%02X\n", opcode);
                    }
                    break;
            }
        } else if (mt == NOTIFICATION && gid == VENDOR_ANDROID) {
            // Simplified handling for vendor notifications
            if (ui_color_enabled) {
                printf("  %s%sNo specific decoder for VENDOR_ANDROID_NOTIFICATION opcode 0x%02X%s\n", 
                       ANSI_COLOR_BRIGHT_BLACK, ANSI_BOLD, opcode, ANSI_RESET);
            } else {
                printf("  No specific decoder for VENDOR_ANDROID_NOTIFICATION opcode 0x%02X\n", opcode);
            }
        } else if (mt == NOTIFICATION && gid == RANGING_DATA) {
            switch(opcode) {
                case RANGE_DATA_NTF_OPCODE:
                    ui_decode_range_data_ntf(payload_ptr, payload_len_int);
                    break;
                default:
                    if (ui_color_enabled) {
                        printf("  %s%sNo specific decoder for RANGING_DATA_NOTIFICATION opcode 0x%02X%s\n", 
                               ANSI_COLOR_BRIGHT_BLACK, ANSI_BOLD, opcode, ANSI_RESET);
                    } else {
                        printf("  No specific decoder for RANGING_DATA_NOTIFICATION opcode 0x%02X\n", opcode);
                    }
                    break;
            }
        } else {
            if (ui_color_enabled) {
                printf("  %s%sNo specific decoder for MT=%d, GID=%d, OP=0x%02X%s\n", 
                       ANSI_COLOR_BRIGHT_BLACK, ANSI_BOLD, mt, gid, opcode, ANSI_RESET);
            } else {
                printf("  No specific decoder for MT=%d, GID=%d, OP=0x%02X\n", mt, gid, opcode);
            }
        }
    }

    if (ui_color_enabled) {
        printf("%s%s%s=== Packet Analysis Complete ===%s\n", 
               ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_BG_GREEN, ANSI_RESET);
    } else {
        printf("=== Packet Analysis Complete ===\n");
    }
}

// Enhanced packet decoding functions with UI enhancements

void ui_decode_core_device_info_rsp(unsigned char* payload, int payload_len) {
    if (ui_color_enabled) {
        printf("  %s%sCORE_DEVICE_INFO Response:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
        if (payload_len >= 8) {
            uint16_t vendor_id = ui_read_u16_le(payload);
            uint16_t device_state = ui_read_u16_le(payload + 2);
            uint8_t uci_version = payload[4];
            uint8_t mac_version = payload[5];
            uint8_t phy_version = payload[6];
            uint8_t test_version = payload[7];
            
            printf("    %s%sVendor ID:%s 0x%04X\n", ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, vendor_id);
            printf("    %s%sDevice State:%s %d\n", ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, device_state);
            printf("    %s%sUCI Version:%s %d.%d\n", ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, uci_version >> 4, uci_version & 0x0F);
            printf("    %s%sMAC Version:%s %d.%d\n", ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, mac_version >> 4, mac_version & 0x0F);
            printf("    %s%sPHY Version:%s %d.%d\n", ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, phy_version >> 4, phy_version & 0x0F);
            printf("    %s%sTest Version:%s %d.%d\n", ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, test_version >> 4, test_version & 0x0F);
        }
    } else {
        printf("  CORE_DEVICE_INFO Response:\n");
        if (payload_len >= 8) {
            uint16_t vendor_id = ui_read_u16_le(payload);
            uint16_t device_state = ui_read_u16_le(payload + 2);
            uint8_t uci_version = payload[4];
            uint8_t mac_version = payload[5];
            uint8_t phy_version = payload[6];
            uint8_t test_version = payload[7];
            
            printf("    Vendor ID: 0x%04X\n", vendor_id);
            printf("    Device State: %d\n", device_state);
            printf("    UCI Version: %d.%d\n", uci_version >> 4, uci_version & 0x0F);
            printf("    MAC Version: %d.%d\n", mac_version >> 4, mac_version & 0x0F);
            printf("    PHY Version: %d.%d\n", phy_version >> 4, phy_version & 0x0F);
            printf("    Test Version: %d.%d\n", test_version >> 4, test_version & 0x0F);
        }
    }
}

void ui_decode_core_get_caps_info_rsp(unsigned char* payload, int payload_len) {
    if (ui_color_enabled) {
        printf("  %s%sCORE_GET_CAPS_INFO Response:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  CORE_GET_CAPS_INFO Response:\n");
    }
}

void ui_decode_core_set_config_rsp(unsigned char* payload, int payload_len) {
    if (ui_color_enabled) {
        printf("  %s%sCORE_SET_CONFIG Response:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  CORE_SET_CONFIG Response:\n");
    }
}

void ui_decode_core_get_config_rsp(unsigned char* payload, int payload_len) {
    if (ui_color_enabled) {
        printf("  %s%sCORE_GET_CONFIG Response:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  CORE_GET_CONFIG Response:\n");
    }
}

void ui_decode_core_device_reset_rsp(unsigned char* payload, int payload_len) {
    if (ui_color_enabled) {
        printf("  %s%sCORE_DEVICE_RESET Response:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  CORE_DEVICE_RESET Response:\n");
    }
}

void ui_decode_core_device_suspend_rsp(unsigned char* payload, int payload_len) {
    if (ui_color_enabled) {
        printf("  %s%sCORE_DEVICE_SUSPEND Response:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  CORE_DEVICE_SUSPEND Response:\n");
    }
}

void ui_decode_core_query_uwbs_timestamp_rsp(unsigned char* payload, int payload_len) {
    if (ui_color_enabled) {
        printf("  %s%sCORE_QUERY_UWBS_TIMESTAMP Response:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  CORE_QUERY_UWBS_TIMESTAMP Response:\n");
    }
}

void ui_decode_core_get_state_rsp(unsigned char* payload, int payload_len) {
    if (ui_color_enabled) {
        printf("  %s%sCORE_GET_STATE Response:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  CORE_GET_STATE Response:\n");
    }
}

void ui_decode_core_set_active_rsp(unsigned char* payload, int payload_len) {
    if (ui_color_enabled) {
        printf("  %s%sCORE_SET_ACTIVE Response:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  CORE_SET_ACTIVE Response:\n");
    }
}

void ui_decode_core_set_ready_rsp(unsigned char* payload, int payload_len) {
    if (ui_color_enabled) {
        printf("  %s%sCORE_SET_READY Response:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  CORE_SET_READY Response:\n");
    }
}

void ui_decode_core_device_ready_rsp(unsigned char* payload, int payload_len) {
    if (ui_color_enabled) {
        printf("  %s%sCORE_DEVICE_READY Response:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  CORE_DEVICE_READY Response:\n");
    }
}

void ui_decode_core_get_caps_rsp(unsigned char* payload, int payload_len) {
    if (ui_color_enabled) {
        printf("  %s%sCORE_GET_CAPS Response:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  CORE_GET_CAPS Response:\n");
    }
}

void ui_decode_core_set_power_rsp(unsigned char* payload, int payload_len) {
    if (ui_color_enabled) {
        printf("  %s%sCORE_SET_POWER Response:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  CORE_SET_POWER Response:\n");
    }
}

void ui_decode_core_get_power_rsp(unsigned char* payload, int payload_len) {
    if (ui_color_enabled) {
        printf("  %s%sCORE_GET_POWER Response:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  CORE_GET_POWER Response:\n");
    }
}

void ui_decode_core_device_on_rsp(unsigned char* payload, int payload_len) {
    if (ui_color_enabled) {
        printf("  %s%sCORE_DEVICE_ON Response:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  CORE_DEVICE_ON Response:\n");
    }
}

void ui_decode_core_device_off_rsp(unsigned char* payload, int payload_len) {
    if (ui_color_enabled) {
        printf("  %s%sCORE_DEVICE_OFF Response:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  CORE_DEVICE_OFF Response:\n");
    }
}

void ui_decode_core_device_suspend_cmd_rsp(unsigned char* payload, int payload_len) {
    if (ui_color_enabled) {
        printf("  %s%sCORE_DEVICE_SUSPEND_CMD Response:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  CORE_DEVICE_SUSPEND_CMD Response:\n");
    }
}

void ui_decode_core_device_status_ntf(unsigned char* payload, int payload_len) {
    if (ui_color_enabled) {
        printf("  %s%sCORE_DEVICE_STATUS_NTF:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  CORE_DEVICE_STATUS_NTF:\n");
    }
}

void ui_decode_core_generic_error_ntf(unsigned char* payload, int payload_len) {
    if (ui_color_enabled) {
        printf("  %s%sCORE_GENERIC_ERROR_NTF:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  CORE_GENERIC_ERROR_NTF:\n");
    }
}

void ui_decode_session_init_cmd(unsigned char* payload, int payload_len) {
    if (ui_color_enabled) {
        printf("  %s%sSESSION_INIT_CMD - Session Initialization Command:%s\n",
               ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  SESSION_INIT_CMD - Session Initialization Command:\n");
    }

    if (payload_len < 5) {
        if (ui_color_enabled) {
            printf("    %s%sERROR:%s Payload too short (%d bytes, need at least 5)\n",
                   ANSI_COLOR_RED, ANSI_BOLD, ANSI_RESET, payload_len);
        } else {
            printf("    ERROR: Payload too short (%d bytes, need at least 5)\n", payload_len);
        }
        return;
    }

    unsigned int session_id = (unsigned int)payload[0] |
                             ((unsigned int)payload[1] << 8) |
                             ((unsigned int)payload[2] << 16) |
                             ((unsigned int)payload[3] << 24);
    unsigned char session_type = payload[4];

    if (ui_color_enabled) {
        printf("    %s%sSession ID:%s 0x%08X\n",
               ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_RESET, session_id);
    } else {
        printf("    Session ID: 0x%08X\n", session_id);
    }

    // Print session type with name
    const char* type_name = "UNKNOWN";
    switch(session_type) {
        case 0x00: type_name = "FIRA_RANGING_SESSION"; break;
        case 0x01: type_name = "FIRA_RANGING_AND_IN_BAND_DATA_SESSION"; break;
        case 0x02: type_name = "FIRA_DATA_TRANSFER_SESSION"; break;
        case 0x03: type_name = "FIRA_RANGING_ONLY_PHASE"; break;
        case 0x04: type_name = "FIRA_IN_BAND_DATA_PHASE"; break;
        case 0x05: type_name = "FIRA_RANGING_WITH_DATA_PHASE"; break;
        case 0xA0: type_name = "CCC_RANGING_SESSION"; break;
        case 0xD0: type_name = "DEVICE_TEST_MODE"; break;
    }

    if (ui_color_enabled) {
        printf("    %s%sSession Type:%s 0x%02X (%s%s%s)\n",
               ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_RESET, session_type,
               ANSI_COLOR_CYAN, type_name, ANSI_RESET);
    } else {
        printf("    Session Type: 0x%02X (%s)\n", session_type, type_name);
    }
}

void ui_decode_session_init_rsp(unsigned char* payload, int payload_len) {
    if (ui_color_enabled) {
        printf("  %s%sSESSION_INIT Response:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  SESSION_INIT Response:\n");
    }
}

void ui_decode_session_deinit_rsp(unsigned char* payload, int payload_len) {
    if (ui_color_enabled) {
        printf("  %s%sSESSION_DEINIT Response:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  SESSION_DEINIT Response:\n");
    }
}

void ui_decode_session_set_app_config_rsp(unsigned char* payload, int payload_len) {
    if (ui_color_enabled) {
        printf("  %s%sSESSION_SET_APP_CONFIG Response:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  SESSION_SET_APP_CONFIG Response:\n");
    }
}

void ui_decode_session_get_app_config_rsp(unsigned char* payload, int payload_len) {
    if (ui_color_enabled) {
        printf("  %s%sSESSION_GET_APP_CONFIG Response:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  SESSION_GET_APP_CONFIG Response:\n");
    }
}

void ui_decode_session_get_count_rsp(unsigned char* payload, int payload_len) {
    if (ui_color_enabled) {
        printf("  %s%sSESSION_GET_COUNT Response:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  SESSION_GET_COUNT Response:\n");
    }
}

void ui_decode_session_get_state_rsp(unsigned char* payload, int payload_len) {
    if (ui_color_enabled) {
        printf("  %s%sSESSION_GET_STATE Response:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  SESSION_GET_STATE Response:\n");
    }
}

void ui_decode_session_update_controller_multicast_list_rsp(unsigned char* payload, int payload_len) {
    if (ui_color_enabled) {
        printf("  %s%sSESSION_UPDATE_CONTROLLER_MULTICAST_LIST Response:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  SESSION_UPDATE_CONTROLLER_MULTICAST_LIST Response:\n");
    }
}

void ui_decode_session_update_active_rounds_dt_tag_rsp(unsigned char* payload, int payload_len) {
    if (ui_color_enabled) {
        printf("  %s%sSESSION_UPDATE_ACTIVE_ROUNDS_DT_TAG Response:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  SESSION_UPDATE_ACTIVE_ROUNDS_DT_TAG Response:\n");
    }
}

void ui_decode_session_data_transfer_phase_config_rsp(unsigned char* payload, int payload_len) {
    if (ui_color_enabled) {
        printf("  %s%sSESSION_DATA_TRANSFER_PHASE_CONFIG Response:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  SESSION_DATA_TRANSFER_PHASE_CONFIG Response:\n");
    }
}

void ui_decode_session_start_rsp(unsigned char* payload, int payload_len) {
    if (ui_color_enabled) {
        printf("  %s%sSESSION_START Response:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  SESSION_START Response:\n");
    }
}

void ui_decode_session_stop_rsp(unsigned char* payload, int payload_len) {
    if (ui_color_enabled) {
        printf("  %s%sSESSION_STOP Response:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  SESSION_STOP Response:\n");
    }
}

void ui_decode_session_get_ranging_count_rsp(unsigned char* payload, int payload_len) {
    if (ui_color_enabled) {
        printf("  %s%sSESSION_GET_RANGING_COUNT Response:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  SESSION_GET_RANGING_COUNT Response:\n");
    }
}

void ui_decode_session_status_ntf(unsigned char* payload, int payload_len) {
    if (ui_color_enabled) {
        printf("  %s%sSESSION_STATUS_NTF:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  SESSION_STATUS_NTF:\n");
    }
}

void ui_decode_session_data_credit_ntf(unsigned char* payload, int payload_len) {
    if (ui_color_enabled) {
        printf("  %s%sSESSION_DATA_CREDIT_NTF:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  SESSION_DATA_CREDIT_NTF:\n");
    }
}

void ui_decode_session_data_transfer_status_ntf(unsigned char* payload, int payload_len) {
    if (ui_color_enabled) {
        printf("  %s%sSESSION_DATA_TRANSFER_STATUS_NTF:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  SESSION_DATA_TRANSFER_STATUS_NTF:\n");
    }
}

void ui_decode_session_info_ntf(unsigned char* payload, int payload_len) {
    if (ui_color_enabled) {
        printf("  %s%sSESSION_INFO_NTF:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  SESSION_INFO_NTF:\n");
    }
}

void ui_decode_android_range_diagnostics_ntf(unsigned char* payload, int payload_len) {
    if (ui_color_enabled) {
        printf("  %s%sANDROID_RANGE_DIAGNOSTICS_NTF:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  ANDROID_RANGE_DIAGNOSTICS_NTF:\n");
    }
}

void ui_decode_range_data_ntf(unsigned char* payload, int payload_len) {
    if (ui_color_enabled) {
        printf("  %s%sRANGE_DATA_NTF:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
        if (payload_len >= 3) {
            uint8_t seq_num = payload[0];
            uint32_t session_token = ui_read_u32_le(payload + 1);
            printf("    %s%sSequence Number:%s %d\n", ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, seq_num);
            printf("    %s%sSession Token:%s 0x%08X\n", ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, session_token);
        }
    } else {
        printf("  RANGE_DATA_NTF:\n");
        if (payload_len >= 3) {
            uint8_t seq_num = payload[0];
            uint32_t session_token = ui_read_u32_le(payload + 1);
            printf("    Sequence Number: %d\n", seq_num);
            printf("    Session Token: 0x%08X\n", session_token);
        }
    }
}
