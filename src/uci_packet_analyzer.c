#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/uci.h"
#include "../include/uci_ui.h"
#include "../include/uci_packet_analyzer.h"
#include "../include/uci_ui_packet_decoder.h"
#include "../include/uci_packet_utils.h"
#include "../include/uci_qorvo_utils.h"

static const char *dpf_to_string(unsigned char dpf)
{
    switch (dpf) {
        case DATA_PACKET_FORMAT_SEND:
            return "DATA_MESSAGE_SND";
        case DATA_PACKET_FORMAT_RECEIVE:
            return "DATA_MESSAGE_RCV";
        case DATA_PACKET_FORMAT_LL_SEND:
            return "LL_DATA_MESSAGE_SND";
        case DATA_PACKET_FORMAT_LL_RECEIVE:
            return "LL_DATA_MESSAGE_RCV";
        case DATA_PACKET_FORMAT_RADAR:
            return "RADAR_DATA_MESSAGE";
        default:
            return "UNKNOWN";
    }
}

static const char* qorvo_device_boot_reason_to_string(uint8_t reason) {
    switch (reason) {
        case 0:
            return "unknown";
        case 1:
            return "fatal_reset";
        default:
            return "reserved";
    }
}


static void analyze_data_message_payload(unsigned char dpf,
                                         unsigned char pbf,
                                         const unsigned char *payload,
                                         int payload_len)
{
    if (dpf != DATA_PACKET_FORMAT_SEND) {
        if (ui_color_enabled) {
            printf("  %s%sNo specific decoder for DPF 0x%02X%s\n",
                   ANSI_COLOR_BRIGHT_BLACK, ANSI_BOLD, dpf, ANSI_RESET);
        } else {
            printf("  No specific decoder for DPF 0x%02X\n", dpf);
        }
        return;
    }

    if (payload_len < UCI_DATA_MESSAGE_SND_HEADER) {
        if (ui_color_enabled) {
            printf("  %s%sDATA_MESSAGE_SND continuation fragment:%s %d application bytes\n",
                   ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET, payload_len);
        } else {
            printf("  DATA_MESSAGE_SND continuation fragment: %d application bytes\n",
                   payload_len);
        }
        if (payload_len > 0) {
            size_t preview = (payload_len < 32) ? (size_t)payload_len : 32;
            printf("    Data: ");
            for (size_t i = 0; i < preview; i++) {
                printf("%02X ", payload[i]);
            }
            if (payload_len > (int)preview) {
                printf("... (%d more bytes)", payload_len - (int)preview);
            }
            printf("\n");
        }
        return;
    }

    uint32_t session_handle = read_u32_le(payload);
    uint64_t destination_address = read_u64_le(payload + 4);
    uint16_t sequence_number = read_u16_le(payload + 12);
    uint16_t declared_len = read_u16_le(payload + 14);
    size_t fragment_bytes = 0;
    if (payload_len > UCI_DATA_MESSAGE_SND_HEADER) {
        fragment_bytes = (size_t)(payload_len - UCI_DATA_MESSAGE_SND_HEADER);
    }

    if (ui_color_enabled) {
        printf("  %s%sDATA_MESSAGE_SND:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  DATA_MESSAGE_SND:\n");
    }

    if (ui_color_enabled) {
        printf("    %s%sSession Handle:%s 0x%08X\n",
               ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_RESET, session_handle);
        printf("    %s%sDestination Address:%s 0x%016llX\n",
               ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_RESET,
               (unsigned long long)destination_address);
        printf("    %s%sSequence Number:%s %u\n",
               ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_RESET, sequence_number);
        printf("    %s%sDeclared Data Length:%s %u bytes\n",
               ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_RESET, declared_len);
        printf("    %s%sFragment Payload:%s %zu bytes%s\n",
               ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_RESET, fragment_bytes,
               (pbf == NOT_COMPLETE) ? " (more fragments expected)" : "");
    } else {
        printf("    Session Handle: 0x%08X\n", session_handle);
        printf("    Destination Address: 0x%016llX\n", (unsigned long long)destination_address);
        printf("    Sequence Number: %u\n", sequence_number);
        printf("    Declared Data Length: %u bytes\n", declared_len);
        printf("    Fragment Payload: %zu bytes%s\n", fragment_bytes,
               (pbf == NOT_COMPLETE) ? " (more fragments expected)" : "");
    }

    if (fragment_bytes > 0) {
        size_t preview = fragment_bytes;
        if (preview > 16) {
            preview = 16;
        }
        printf("    Data Preview: ");
        for (size_t i = 0; i < preview; i++) {
            printf("%02X ", payload[UCI_DATA_MESSAGE_SND_HEADER + i]);
        }
        if (fragment_bytes > preview) {
            printf("... (%zu more bytes in this fragment)", fragment_bytes - preview);
        }
        printf("\n");
    }
}

// Enhanced error code analysis based on QM SDK patterns
void enhanced_error_analysis(unsigned char status_code) {
    if (ui_color_enabled) {
        printf("  %s%sStatus Code Analysis:%s\n", ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET);
        printf("    %sCode: 0x%02X%s - ", ANSI_COLOR_BRIGHT_WHITE, status_code, ANSI_RESET);
        
        switch (status_code) {
            case UCI_STATUS_OK:
                printf("%sSUCCESS - Operation completed successfully%s\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET);
                break;
            case UCI_STATUS_REJECTED:
                printf("%sREJECTED - Request rejected by device%s\n", ANSI_COLOR_YELLOW, ANSI_RESET);
                break;
            case UCI_STATUS_INVALID_PARAM:
                printf("%sINVALID_PARAM - Invalid parameter provided%s\n", ANSI_COLOR_RED, ANSI_RESET);
                break;
            case UCI_STATUS_UNKNOWN_GID:
                printf("%sUNKNOWN_GID - Unknown Group ID (check device capabilities)%s\n", ANSI_COLOR_RED, ANSI_RESET);
                break;
            case UCI_STATUS_UNKNOWN_OID:
                printf("%sUNKNOWN_OID - Unknown Opcode ID (command not supported)%s\n", ANSI_COLOR_RED, ANSI_RESET);
                break;
            case UCI_STATUS_SESSION_DUPLICATE:
                printf("%sSESSION_DUPLICATE - Session ID already exists%s\n", ANSI_COLOR_RED, ANSI_RESET);
                break;
            case UCI_STATUS_SESSION_NOT_EXIST:
                printf("%sSESSION_NOT_EXIST - Session does not exist%s\n", ANSI_COLOR_RED, ANSI_RESET);
                break;
            case UCI_STATUS_FAILED:
                printf("%sFAILED - Generic failure status%s\n", ANSI_COLOR_RED, ANSI_RESET);
                break;
            case UCI_STATUS_INVALID_RANGE:
                printf("%sINVALID_RANGE - Parameter value out of range%s\n", ANSI_COLOR_RED, ANSI_RESET);
                break;
            case UCI_STATUS_INVALID_MSG_SIZE:
                printf("%sINVALID_MSG_SIZE - Message size invalid%s\n", ANSI_COLOR_RED, ANSI_RESET);
                break;
            default:
                printf("%sUNKNOWN - Status code 0x%02X%s\n", ANSI_COLOR_BRIGHT_BLACK, status_code, ANSI_RESET);
                break;
        }
    } else {
        printf("  Status Code Analysis:\n");
        printf("    Code: 0x%02X - ", status_code);
        
        switch (status_code) {
            case UCI_STATUS_OK:
                printf("SUCCESS - Operation completed successfully\n");
                break;
            case UCI_STATUS_REJECTED:
                printf("REJECTED - Request rejected by device\n");
                break;
            case UCI_STATUS_INVALID_PARAM:
                printf("INVALID_PARAM - Invalid parameter provided\n");
                break;
            case UCI_STATUS_UNKNOWN_GID:
                printf("UNKNOWN_GID - Unknown Group ID\n");
                break;
            case UCI_STATUS_UNKNOWN_OID:
                printf("UNKNOWN_OID - Unknown Opcode ID\n");
                break;
            case UCI_STATUS_SESSION_DUPLICATE:
                printf("SESSION_DUPLICATE - Session ID already exists\n");
                break;
            case UCI_STATUS_SESSION_NOT_EXIST:
                printf("SESSION_NOT_EXIST - Session does not exist\n");
                break;
            case UCI_STATUS_FAILED:
                printf("FAILED - Generic failure status\n");
                break;
            case UCI_STATUS_INVALID_RANGE:
                printf("INVALID_RANGE - Parameter value out of range\n");
                break;
            case UCI_STATUS_INVALID_MSG_SIZE:
                printf("INVALID_MSG_SIZE - Message size invalid\n");
                break;
            default:
                printf("UNKNOWN - Status code 0x%02X\n", status_code);
                break;
        }
    }
}

// Unified packet analyzer that respects ui_color_enabled for formatted output
// This is the single source of truth for packet analysis logic
// Enhanced UI version of analyze_uci_packet
void uci_analyze_packet_core(unsigned char* packet, size_t packet_len) {
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
    uci_extract_header_fields_safe(header, &header_fields);

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
    unsigned char* payload_ptr = packet + sizeof(struct uci_packet_header);
    int payload_len_int = (int)payload_len_field;

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

    if (mt == DATA) {
        if (ui_color_enabled) {
            printf("  %s%sData Packet Format (DPF):%s 0x%01X (%s)\n",
                   ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_RESET, gid, dpf_to_string(gid));
        } else {
            printf("  Data Packet Format (DPF): 0x%01X (%s)\n", gid, dpf_to_string(gid));
        }
    } else {
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
            case 0x03:  // DATA_CONTROL - RFU (Reserved for Future Use)
                if (ui_color_enabled) {
                    printf(" %s(DATA_CONTROL)%s", ANSI_COLOR_BRIGHT_BLUE, ANSI_RESET);
                } else {
                    printf(" (DATA_CONTROL)");
                }
                break;
            case SESSION_CONTROL: 
                if (ui_color_enabled) {
                    printf(" %s(SESSION_CONTROL)%s", ANSI_COLOR_BRIGHT_BLUE, ANSI_RESET);
                } else {
                    printf(" (SESSION_CONTROL)");
                }
                break;
            case TEST:
                if (ui_color_enabled) {
                    printf(" %s(TEST)%s", ANSI_COLOR_BRIGHT_BLUE, ANSI_RESET);
                } else {
                    printf(" (TEST)");
                }
                break;
            case ANDROID: 
                if (ui_color_enabled) {
                    printf(" %s(ANDROID)%s", ANSI_COLOR_BRIGHT_BLUE, ANSI_RESET);
                } else {
                    printf(" (ANDROID)");
                }
                break;
            case 0x0B:  // QORVO_EXT2 - Qorvo vendor-specific commands (QM SDK compatibility)
                if (ui_color_enabled) {
                    printf(" %s(QORVO_EXT2)%s", ANSI_COLOR_BRIGHT_BLUE, ANSI_RESET);
                } else {
                    printf(" (QORVO_EXT2)");
                }
                
                // Decode QM SDK vendor-specific commands
                if (mt == NOTIFICATION) {
                    switch(opcode) {
                        case 0x00:  // QORVO_TEST_DEBUG
                            if (ui_color_enabled) {
                                printf("\n  %s%sQORVO_TEST_DEBUG_NTF:%s Vendor debug notification%s\n", 
                                       ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, ANSI_RESET);
                            } else {
                                printf("\n  QORVO_TEST_DEBUG_NTF: Vendor debug notification\n");
                            }
                            break;
                        case 0x01:  // QORVO_TEST_TX_CW
                            if (ui_color_enabled) {
                                printf("\n  %s%sQORVO_TEST_TX_CW_NTF:%s Continuous wave transmission test notification%s\n", 
                                       ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, ANSI_RESET);
                            } else {
                                printf("\n  QORVO_TEST_TX_CW_NTF: Continuous wave transmission test notification\n");
                            }
                            break;
                        case 0x02:  // QORVO_TEST_PLLRF
                            if (ui_color_enabled) {
                                printf("\n  %s%sQORVO_TEST_PLLRF_NTF:%s PLL status test notification%s\n", 
                                       ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, ANSI_RESET);
                            } else {
                                printf("\n  QORVO_TEST_PLLRF_NTF: PLL status test notification\n");
                            }
                            break;
                        case 0x03:  // QORVO_FIRA_RANGE_DIAGNOSTICS
                            if (ui_color_enabled) {
                                printf("\n  %s%sQORVO_FIRA_RANGE_DIAGNOSTICS_NTF:%s Diagnostics notification%s\n", 
                                       ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, ANSI_RESET);
                            } else {
                                printf("\n  QORVO_FIRA_RANGE_DIAGNOSTICS_NTF: Diagnostics notification\n");
                            }
                            break;
                        case 0x07:  // QORVO_SESSION_GET
                            if (ui_color_enabled) {
                                printf("\n  %s%sQORVO_SESSION_GET_NTF:%s Session retrieval notification%s\n", 
                                       ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, ANSI_RESET);
                            } else {
                                printf("\n  QORVO_SESSION_GET_NTF: Session retrieval notification\n");
                            }
                            break;
                        case QORVO_CORE_PSDU_DUMP:
                        {
                            uci_qorvo_psdu_report_t report;
                            if (uci_qorvo_decode_psdu_report(payload_ptr, payload_len_field, &report) == 0) {
                                if (ui_color_enabled) {
                                    printf("\n  %s%sQORVO_CORE_PSDU_DUMP_NTF:%s PSDU dump notification%s\n",
                                           ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, ANSI_RESET);
                                } else {
                                    printf("\n  QORVO_CORE_PSDU_DUMP_NTF: PSDU dump notification\n");
                                }
                                printf("    session_handle: 0x%08X\n", report.session_handle);
                                printf("    frames: %u\n", report.frame_count);
                                for (uint8_t i = 0; i < report.frame_count; i++) {
                                    const uci_qorvo_psdu_frame_t* frame = &report.frames[i];
                                    printf("      index %u length %u bytes:", frame->index, frame->length);
                                    for (uint16_t b = 0; b < frame->length; b++) {
                                        printf(" %02X", frame->data[b]);
                                    }
                                    printf("\n");
                                }
                            } else {
                                printf("\n  QORVO_CORE_PSDU_DUMP_NTF: PSDU dump notification (unable to decode)\n");
                            }
                            break;
                        }
                        case 0x23:  // QORVO_CORE_GET_MEM_STATS
                            if (ui_color_enabled) {
                                printf("\n  %s%sQORVO_CORE_GET_MEM_STATS_NTF:%s Memory statistics notification%s\n", 
                                       ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, ANSI_RESET);
                            } else {
                                printf("\n  QORVO_CORE_GET_MEM_STATS_NTF: Memory statistics notification\n");
                            }
                            break;
                        case 0x24:  // QORVO_CORE_GET_POWER_STATS
                            if (ui_color_enabled) {
                                printf("\n  %s%sQORVO_CORE_GET_POWER_STATS_NTF:%s Power statistics notification%s\n", 
                                       ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, ANSI_RESET);
                            } else {
                                printf("\n  QORVO_CORE_GET_POWER_STATS_NTF: Power statistics notification\n");
                            }
                            break;
                        case 0x25:  // QORVO_CORE_GET_CPU_STATS
                            if (ui_color_enabled) {
                                printf("\n  %s%sQORVO_CORE_GET_CPU_STATS_NTF:%s CPU statistics notification%s\n", 
                                       ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, ANSI_RESET);
                            } else {
                                printf("\n  QORVO_CORE_GET_CPU_STATS_NTF: CPU statistics notification\n");
                            }
                            break;
                        case 0x26:  // QORVO_CORE_RESET_CPU_STATS
                            if (ui_color_enabled) {
                                printf("\n  %s%sQORVO_CORE_RESET_CPU_STATS_RSP:%s CPU statistics reset response%s\n", 
                                       ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, ANSI_RESET);
                            } else {
                                printf("\n  QORVO_CORE_RESET_CPU_STATS_RSP: CPU statistics reset response\n");
                            }
                            break;
                        case 0x27:  // QORVO_CORE_GET_DEVICE_STATS
                            if (ui_color_enabled) {
                                printf("\n  %s%sQORVO_CORE_GET_DEVICE_STATS_NTF:%s Device statistics notification%s\n", 
                                       ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, ANSI_RESET);
                            } else {
                                printf("\n  QORVO_CORE_GET_DEVICE_STATS_NTF: Device statistics notification\n");
                            }
                            break;
                        case 0x31:  // QORVO_CORE_DEVICE_BOOT
                            if (ui_color_enabled) {
                                printf("\n  %s%sQORVO_CORE_DEVICE_BOOT_NTF:%s Device boot notification%s\n", 
                                       ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, ANSI_RESET);
                            } else {
                                printf("\n  QORVO_CORE_DEVICE_BOOT_NTF: Device boot notification\n");
                            }
                            break;
                        default:
                            if (ui_color_enabled) {
                                printf("\n  %s%sNo specific decoder for QORVO_EXT2_NOTIFICATION opcode 0x%02X%s\n", 
                                       ANSI_COLOR_BRIGHT_BLACK, ANSI_BOLD, opcode, ANSI_RESET);
                            } else {
                                printf("\n  No specific decoder for QORVO_EXT2_NOTIFICATION opcode 0x%02X\n", opcode);
                            }
                            break;
                    }
                } else if (mt == RESPONSE) {
                    switch(opcode) {
                        case 0x00:  // QORVO_TEST_DEBUG
                            if (ui_color_enabled) {
                                printf("\n  %s%sQORVO_TEST_DEBUG_RSP:%s Vendor debug response%s\n", 
                                       ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, ANSI_RESET);
                            } else {
                                printf("\n  QORVO_TEST_DEBUG_RSP: Vendor debug response\n");
                            }
                            break;
                        case 0x01:  // QORVO_TEST_TX_CW
                            if (ui_color_enabled) {
                                printf("\n  %s%sQORVO_TEST_TX_CW_RSP:%s Continuous wave transmission test response%s\n", 
                                       ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, ANSI_RESET);
                            } else {
                                printf("\n  QORVO_TEST_TX_CW_RSP: Continuous wave transmission test response\n");
                            }
                            break;
                        case 0x02:  // QORVO_TEST_PLLRF
                            if (ui_color_enabled) {
                                printf("\n  %s%sQORVO_TEST_PLLRF_RSP:%s PLL status test response%s\n", 
                                       ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, ANSI_RESET);
                            } else {
                                printf("\n  QORVO_TEST_PLLRF_RSP: PLL status test response\n");
                            }
                            break;
                        case 0x03:  // QORVO_FIRA_RANGE_DIAGNOSTICS
                            if (ui_color_enabled) {
                                printf("\n  %s%sQORVO_FIRA_RANGE_DIAGNOSTICS_RSP:%s Diagnostics response%s\n", 
                                       ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, ANSI_RESET);
                            } else {
                                printf("\n  QORVO_FIRA_RANGE_DIAGNOSTICS_RSP: Diagnostics response\n");
                            }
                            break;
                        case 0x07:  // QORVO_SESSION_GET
                            if (ui_color_enabled) {
                                printf("\n  %s%sQORVO_SESSION_GET_RSP:%s Session retrieval response%s\n", 
                                       ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, ANSI_RESET);
                            } else {
                                printf("\n  QORVO_SESSION_GET_RSP: Session retrieval response\n");
                            }
                            break;
                        case 0x08:  // QORVO_FIRA_SET_ANT_FLEX_CONFIG
                            if (ui_color_enabled) {
                                printf("\n  %s%sQORVO_FIRA_SET_ANT_FLEX_CONFIG_RSP:%s Antenna flexibility config response%s\n", 
                                       ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, ANSI_RESET);
                            } else {
                                printf("\n  QORVO_FIRA_SET_ANT_FLEX_CONFIG_RSP: Antenna flexibility config response\n");
                            }
                            break;
                        case 0x09:  // QORVO_FIRA_GET_ANT_FLEX_CONFIG
                            if (ui_color_enabled) {
                                printf("\n  %s%sQORVO_FIRA_GET_ANT_FLEX_CONFIG_RSP:%s Antenna flexibility config retrieval response%s\n", 
                                       ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, ANSI_RESET);
                            } else {
                                printf("\n  QORVO_FIRA_GET_ANT_FLEX_CONFIG_RSP: Antenna flexibility config retrieval response\n");
                            }
                            break;
                        case 0x0A:  // QORVO_CCC_SET_ANT_FLEX_CONFIG
                            if (ui_color_enabled) {
                                printf("\n  %s%sQORVO_CCC_SET_ANT_FLEX_CONFIG_RSP:%s CCC antenna flexibility config response%s\n", 
                                       ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, ANSI_RESET);
                            } else {
                                printf("\n  QORVO_CCC_SET_ANT_FLEX_CONFIG_RSP: CCC antenna flexibility config response\n");
                            }
                            break;
                        case 0x0B:  // QORVO_CCC_GET_ANT_FLEX_CONFIG
                            if (ui_color_enabled) {
                                printf("\n  %s%sQORVO_CCC_GET_ANT_FLEX_CONFIG_RSP:%s CCC antenna flexibility config retrieval response%s\n", 
                                       ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, ANSI_RESET);
                            } else {
                                printf("\n  QORVO_CCC_GET_ANT_FLEX_CONFIG_RSP: CCC antenna flexibility config retrieval response\n");
                            }
                            break;
                        case QORVO_CORE_PSDU_DUMP:
                        {
                            uci_qorvo_psdu_report_t report;
                            if (uci_qorvo_decode_psdu_report(payload_ptr, payload_len_field, &report) == 0) {
                                if (ui_color_enabled) {
                                    printf("\n  %s%sQORVO_CORE_PSDU_DUMP_RSP:%s PSDU dump response%s\n",
                                           ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, ANSI_RESET);
                                } else {
                                    printf("\n  QORVO_CORE_PSDU_DUMP_RSP: PSDU dump response\n");
                                }
                                printf("    session_handle: 0x%08X\n", report.session_handle);
                                printf("    frames: %u\n", report.frame_count);
                                for (uint8_t i = 0; i < report.frame_count; i++) {
                                    const uci_qorvo_psdu_frame_t* frame = &report.frames[i];
                                    printf("      index %u length %u bytes:", frame->index, frame->length);
                                    for (uint16_t b = 0; b < frame->length; b++) {
                                        printf(" %02X", frame->data[b]);
                                    }
                                    printf("\n");
                                }
                            } else {
                                printf("\n  QORVO_CORE_PSDU_DUMP_RSP: PSDU dump response (unable to decode)\n");
                            }
                            break;
                        }
                        case 0x23:  // QORVO_CORE_GET_MEM_STATS
                            if (ui_color_enabled) {
                                printf("\n  %s%sQORVO_CORE_GET_MEM_STATS_RSP:%s Memory statistics response%s\n", 
                                       ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, ANSI_RESET);
                            } else {
                                printf("\n  QORVO_CORE_GET_MEM_STATS_RSP: Memory statistics response\n");
                            }
                            break;
                        case 0x24:  // QORVO_CORE_GET_POWER_STATS
                            if (ui_color_enabled) {
                                printf("\n  %s%sQORVO_CORE_GET_POWER_STATS_RSP:%s Power statistics response%s\n", 
                                       ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, ANSI_RESET);
                            } else {
                                printf("\n  QORVO_CORE_GET_POWER_STATS_RSP: Power statistics response\n");
                            }
                            break;
                        case 0x25:  // QORVO_CORE_GET_CPU_STATS
                            if (ui_color_enabled) {
                                printf("\n  %s%sQORVO_CORE_GET_CPU_STATS_RSP:%s CPU statistics response%s\n", 
                                       ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, ANSI_RESET);
                            } else {
                                printf("\n  QORVO_CORE_GET_CPU_STATS_RSP: CPU statistics response\n");
                            }
                            break;
                        case 0x26:  // QORVO_CORE_RESET_CPU_STATS
                            if (ui_color_enabled) {
                                printf("\n  %s%sQORVO_CORE_RESET_CPU_STATS_RSP:%s CPU statistics reset response%s\n", 
                                       ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, ANSI_RESET);
                            } else {
                                printf("\n  QORVO_CORE_RESET_CPU_STATS_RSP: CPU statistics reset response\n");
                            }
                            break;
                        case 0x27:  // QORVO_CORE_GET_DEVICE_STATS
                            if (ui_color_enabled) {
                                printf("\n  %s%sQORVO_CORE_GET_DEVICE_STATS_RSP:%s Device statistics response%s\n", 
                                       ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, ANSI_RESET);
                            } else {
                                printf("\n  QORVO_CORE_GET_DEVICE_STATS_RSP: Device statistics response\n");
                            }
                            break;
                        case 0x30:  // QORVO_CORE_ERASE_CERTS
                            if (ui_color_enabled) {
                                printf("\n  %s%sQORVO_CORE_ERASE_CERTS_RSP:%s Certificate erase response%s\n", 
                                       ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, ANSI_RESET);
                            } else {
                                printf("\n  QORVO_CORE_ERASE_CERTS_RSP: Certificate erase response\n");
                            }
                            break;
                        case QORVO_CORE_DEVICE_BOOT:
                        {
                            uci_qorvo_device_boot_t boot;

                            if (uci_qorvo_decode_device_boot(payload_ptr, payload_len_field, &boot) == 0) {
                                const char* reason = qorvo_device_boot_reason_to_string(boot.reason);
                                if (ui_color_enabled) {
                                    printf("\n  %s%sQORVO_CORE_DEVICE_BOOT_RSP:%s reason=%s (0x%02X)%s\n",
                                           ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET,
                                           reason, boot.reason, ANSI_RESET);
                                } else {
                                    printf("\n  QORVO_CORE_DEVICE_BOOT_RSP: reason=%s (0x%02X)\n",
                                           reason, boot.reason);
                                }
                            } else {
                                printf("\n  QORVO_CORE_DEVICE_BOOT_RSP: Device boot response (unable to decode)\n");
                            }
                            break;
                        }
                        case 0x35:  // QORVO_CORE_TOGGLE_GPIO_TIMESYNC
                            if (ui_color_enabled) {
                                printf("\n  %s%sQORVO_CORE_TOGGLE_GPIO_TIMESYNC_RSP:%s GPIO timesync toggle response%s\n", 
                                       ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, ANSI_RESET);
                            } else {
                                printf("\n  QORVO_CORE_TOGGLE_GPIO_TIMESYNC_RSP: GPIO timesync toggle response\n");
                            }
                            break;
                        case 0x36:  // QORVO_CORE_QUERY_GPIO_TIMESTAMP
                            if (ui_color_enabled) {
                                printf("\n  %s%sQORVO_CORE_QUERY_GPIO_TIMESTAMP_RSP:%s GPIO timestamp query response%s\n", 
                                       ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, ANSI_RESET);
                            } else {
                                printf("\n  QORVO_CORE_QUERY_GPIO_TIMESTAMP_RSP: GPIO timestamp query response\n");
                            }
                            break;
                        default:
                            if (ui_color_enabled) {
                                printf("\n  %s%sNo specific decoder for QORVO_EXT2_RESPONSE opcode 0x%02X%s\n", 
                                       ANSI_COLOR_BRIGHT_BLACK, ANSI_BOLD, opcode, ANSI_RESET);
                            } else {
                                printf("\n  No specific decoder for QORVO_EXT2_RESPONSE opcode 0x%02X\n", opcode);
                            }
                            break;
                    }
                } else {
                    if (ui_color_enabled) {
                        printf("\n  %s%sNo specific decoder for MT=%d, GID=11, OP=0x%02X%s\n", 
                               ANSI_COLOR_BRIGHT_BLACK, ANSI_BOLD, mt, opcode, ANSI_RESET);
                    } else {
                        printf("\n  No specific decoder for MT=%d, GID=11, OP=0x%02X\n", mt, opcode);
                    }
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
    }
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
                printf("%s%02X%s ", ANSI_COLOR_BRIGHT_CYAN, payload_ptr[i], ANSI_RESET);
            } else {
                printf("%02X ", payload_ptr[i]);
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

        if (mt == DATA) {
            analyze_data_message_payload(gid, pbf, payload_ptr, payload_len_int);
            return;
        }

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
                case SESSION_QUERY_DATA_SIZE_IN_RANGING:
                    ui_decode_session_query_data_size_in_ranging_rsp(payload_ptr, payload_len_int);
                    break;
                case SESSION_SET_HYBRID_CONTROLLER_CONFIG:
                    ui_decode_session_set_hybrid_controller_config_rsp(payload_ptr, payload_len_int);
                    break;
                case SESSION_SET_HYBRID_CONTROLEE_CONFIG:
                    ui_decode_session_set_hybrid_controlee_config_rsp(payload_ptr, payload_len_int);
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
                case SESSION_LOGICAL_LINK_CREATE:
                    ui_decode_session_logical_link_create_rsp(payload_ptr, payload_len_int);
                    break;
                case SESSION_LOGICAL_LINK_CLOSE:
                    ui_decode_session_logical_link_close_rsp(payload_ptr, payload_len_int);
                    break;
                case SESSION_LOGICAL_LINK_GET_PARAM:
                    ui_decode_session_logical_link_get_param_rsp(payload_ptr, payload_len_int);
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
                case SESSION_LOGICAL_LINK_UWBS_CREATE:
                    ui_decode_session_logical_link_uwbs_create_ntf(payload_ptr, payload_len_int);
                    break;
                case SESSION_LOGICAL_LINK_UWBS_CLOSE:
                    ui_decode_session_logical_link_uwbs_close_ntf(payload_ptr, payload_len_int);
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
        } else if (mt == NOTIFICATION && gid == ANDROID) {
            // Simplified handling for vendor notifications
            if (ui_color_enabled) {
                printf("  %s%sNo specific decoder for ANDROID_NOTIFICATION opcode 0x%02X%s\n", 
                       ANSI_COLOR_BRIGHT_BLACK, ANSI_BOLD, opcode, ANSI_RESET);
            } else {
                printf("  No specific decoder for ANDROID_NOTIFICATION opcode 0x%02X\n", opcode);
            }
        } else if (mt == NOTIFICATION && gid == SESSION_CONTROL) {
            switch(opcode) {
                case SESSION_INFO_NTF_OPCODE:
                    ui_decode_range_data_ntf(payload_ptr, payload_len_int);
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
