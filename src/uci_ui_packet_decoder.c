#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <inttypes.h>
#include "../include/uci.h"
#include "../include/uci_ui.h"
#include "../include/uci_ui_packet_decoder.h"
#include "../include/uci_packet_analyzer.h"
#include "../include/uci_packet_utils.h"
#include "../include/uci_config_manager.h"

static void print_lookup_line_with_indent(int indent,
                                          const char* label,
                                          unsigned char value,
                                          const uci_lookup_entry_t* entry,
                                          const char* color) {
    if (ui_color_enabled) {
        printf("%*s%s%s%s: 0x%02X ", indent, "", ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, label, value);
        if (entry) {
            const char* entry_color = color ? color : ANSI_COLOR_BRIGHT_GREEN;
            printf("%s(%s)%s", entry_color, entry->label, ANSI_RESET);
            if (entry->description && entry->description[0] != '\0') {
                printf(" - %s", entry->description);
            }
        } else {
            printf("%s(UNKNOWN)%s", ANSI_COLOR_YELLOW, ANSI_RESET);
        }
        printf("\n");
    } else {
        if (entry) {
            printf("%*s%s: 0x%02X (%s)\n", indent, "", label, value, entry->label);
            if (entry->description && entry->description[0] != '\0') {
                printf("%*s%s Description: %s\n", indent, "", label, entry->description);
            }
        } else {
            printf("%*s%s: 0x%02X (UNKNOWN)\n", indent, "", label, value);
        }
    }
}

static void print_lookup_line(const char* label,
                              unsigned char value,
                              const uci_lookup_entry_t* entry,
                              const char* color) {
    print_lookup_line_with_indent(4, label, value, entry, color);
}

static void ui_print_status_lookup_line_internal(const char* label,
                                                 unsigned char status_code,
                                                 int indent);

static void ui_print_session_state_lookup_line_internal(const char* label,
                                                        unsigned char session_state,
                                                        int indent);

static void ui_print_session_reason_lookup_line_internal(const char* label,
                                                         unsigned char reason_code,
                                                         int indent);

static void print_additional_payload_bytes(const unsigned char* payload,
                                           int payload_len,
                                           int start_index) {
    if (payload_len <= start_index) {
        return;
    }

    if (ui_color_enabled) {
        printf("    %s%sAdditional Payload Bytes:%s ", ANSI_COLOR_BRIGHT_BLACK, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("    Additional Payload Bytes: ");
    }

    for (int i = start_index; i < payload_len; i++) {
        printf("%02X ", payload[i]);
    }
    printf("\n");
}

static void ui_print_status_lookup_line_internal(const char* label,
                                                 unsigned char status_code,
                                                 int indent) {
    const uci_lookup_entry_t* entry = uci_lookup_status(status_code);

    const char* status_color = ANSI_COLOR_BRIGHT_YELLOW;
    if (entry && entry->label) {
        if (strncasecmp(entry->label, "OK", 2) == 0 || strstr(entry->label, "SUCCESS") != NULL) {
            status_color = ANSI_COLOR_BRIGHT_GREEN;
        } else if (strstr(entry->label, "ERROR") != NULL || strstr(entry->label, "FAILED") != NULL) {
            status_color = ANSI_COLOR_RED;
        } else if (strstr(entry->label, "RFU") != NULL || strstr(entry->label, "RESERVED") != NULL) {
            status_color = ANSI_COLOR_BRIGHT_BLACK;
        } else {
            status_color = ANSI_COLOR_BRIGHT_CYAN;
        }
    } else if (status_code == UCI_STATUS_OK) {
        status_color = ANSI_COLOR_BRIGHT_GREEN;
    }

    print_lookup_line_with_indent(indent, label, status_code, entry, status_color);
}

void ui_print_status_lookup_line(const char* label, unsigned char status_code) {
    ui_print_status_lookup_line_internal(label, status_code, 4);
}

static void ui_print_session_state_lookup_line_internal(const char* label,
                                                        unsigned char session_state,
                                                        int indent) {
    const uci_lookup_entry_t* entry = uci_lookup_session_state(session_state);

    const char* state_color = ANSI_COLOR_RED;
    switch (session_state) {
        case SESSION_STATE_INIT:
            state_color = ANSI_COLOR_BRIGHT_BLUE;
            break;
        case SESSION_STATE_DEINIT:
            state_color = ANSI_COLOR_YELLOW;
            break;
        case SESSION_STATE_ACTIVE:
            state_color = ANSI_COLOR_BRIGHT_GREEN;
            break;
        case SESSION_STATE_IDLE:
            state_color = ANSI_COLOR_BRIGHT_CYAN;
            break;
        default:
            state_color = ANSI_COLOR_RED;
            break;
    }

    print_lookup_line_with_indent(indent, label, session_state, entry, state_color);
}

void ui_print_session_state_lookup_line(const char* label, unsigned char session_state) {
    ui_print_session_state_lookup_line_internal(label, session_state, 4);
}

static void ui_print_session_reason_lookup_line_internal(const char* label,
                                                         unsigned char reason_code,
                                                         int indent) {
    const uci_lookup_entry_t* entry = uci_lookup_session_reason(reason_code);

    const char* reason_color = ANSI_COLOR_BRIGHT_CYAN;
    if (entry && entry->label && strstr(entry->label, "ERROR") != NULL) {
        reason_color = ANSI_COLOR_RED;
    }

    print_lookup_line_with_indent(indent, label, reason_code, entry, reason_color);
}

void ui_print_session_reason_lookup_line(const char* label, unsigned char reason_code) {
    ui_print_session_reason_lookup_line_internal(label, reason_code, 4);
}

// Function declaration for enhanced error analysis from packet_analyzer.c
void enhanced_error_analysis(unsigned char status_code);

// Enhanced UI version of analyze_uci_packet - now just calls unified analyzer
void ui_analyze_uci_packet(unsigned char* packet, size_t packet_len) {
    // Call unified analyzer which respects ui_color_enabled
    uci_analyze_packet_core(packet, packet_len);
}


// Enhanced packet decoding functions with UI enhancements

void ui_decode_core_device_info_rsp(const unsigned char* payload, int payload_len) {
    if (ui_color_enabled) {
        printf("  %s%sCORE_DEVICE_INFO Response:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  CORE_DEVICE_INFO Response:\n");
    }

    if (payload_len < 10) {
        if (ui_color_enabled) {
            printf("    %s%sError: Payload too short (%d bytes, need at least 10)%s\n",
                   ANSI_COLOR_RED, ANSI_BOLD, payload_len, ANSI_RESET);
        } else {
            printf("    Error: Payload too short (%d bytes, need at least 10)\n", payload_len);
        }
        return;
    }

    // Per FiRa UCI spec (GetDeviceInfoRsp):
    // Byte 0: status
    // Bytes 1-2: uci_version (16-bit)
    // Bytes 3-4: mac_version (16-bit)
    // Bytes 5-6: phy_version (16-bit)
    // Bytes 7-8: uci_test_version (16-bit)
    // Byte 9: vendor_spec_info count
    // Bytes 10+: vendor_spec_info array

    uint8_t status = payload[0];
    uint16_t uci_version = ui_read_u16_le(payload + 1);
    uint16_t mac_version = ui_read_u16_le(payload + 3);
    uint16_t phy_version = ui_read_u16_le(payload + 5);
    uint16_t uci_test_version = ui_read_u16_le(payload + 7);
    uint8_t vendor_info_count = payload[9];

    if (ui_color_enabled) {
        printf("    %s%sStatus:%s 0x%02X", ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, status);
        switch(status) {
            case UCI_STATUS_OK: printf(" %s(OK)%s\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET); break;
            case UCI_STATUS_REJECTED: printf(" %s(REJECTED)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            case UCI_STATUS_FAILED: printf(" %s(FAILED)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            default: printf(" %s(UNKNOWN)%s\n", ANSI_COLOR_YELLOW, ANSI_RESET); break;
        }
        printf("    %s%sUCI Version:%s 0x%04X\n", ANSI_COLOR_BRIGHT_CYAN, ANSI_BOLD, ANSI_RESET, uci_version);
        printf("    %s%sMAC Version:%s 0x%04X\n", ANSI_COLOR_BRIGHT_CYAN, ANSI_BOLD, ANSI_RESET, mac_version);
        printf("    %s%sPHY Version:%s 0x%04X\n", ANSI_COLOR_BRIGHT_CYAN, ANSI_BOLD, ANSI_RESET, phy_version);
        printf("    %s%sUCI Test Version:%s 0x%04X\n", ANSI_COLOR_BRIGHT_CYAN, ANSI_BOLD, ANSI_RESET, uci_test_version);
        printf("    %s%sVendor Info Count:%s %u\n", ANSI_COLOR_BRIGHT_CYAN, ANSI_BOLD, ANSI_RESET, vendor_info_count);
    } else {
        printf("    Status: 0x%02X", status);
        switch(status) {
            case UCI_STATUS_OK: printf(" (OK)\n"); break;
            case UCI_STATUS_REJECTED: printf(" (REJECTED)\n"); break;
            case UCI_STATUS_FAILED: printf(" (FAILED)\n"); break;
            default: printf(" (UNKNOWN)\n"); break;
        }
        printf("    UCI Version: 0x%04X\n", uci_version);
        printf("    MAC Version: 0x%04X\n", mac_version);
        printf("    PHY Version: 0x%04X\n", phy_version);
        printf("    UCI Test Version: 0x%04X\n", uci_test_version);
        printf("    Vendor Info Count: %u\n", vendor_info_count);
    }

    // Display vendor-specific info if present
    if (vendor_info_count > 0 && payload_len >= 10 + vendor_info_count) {
        if (ui_color_enabled) {
            printf("    %s%sVendor Spec Info:%s ", ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_RESET);
        } else {
            printf("    Vendor Spec Info: ");
        }
        for (int i = 0; i < vendor_info_count; i++) {
            printf("0x%02X ", payload[10 + i]);
        }
        printf("\n");
    }
}

void ui_decode_core_get_caps_info_rsp(const unsigned char* payload, int payload_len) {
    if (ui_color_enabled) {
        printf("  %s%sCORE_GET_CAPS_INFO Response:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
        printf("  %s%sDevice Capabilities Information:%s\n", ANSI_COLOR_BRIGHT_CYAN, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  CORE_GET_CAPS_INFO Response:\n");
        printf("  Device Capabilities Information:\n");
    }
    
    if (payload_len < 2) {
        if (ui_color_enabled) {
            printf("  %s%sError: Payload too short (%d bytes, need at least 2)%s\n", 
                   ANSI_COLOR_RED, ANSI_BOLD, payload_len, ANSI_RESET);
        } else {
            printf("  Error: Payload too short (%d bytes, need at least 2)\n", payload_len);
        }
        return;
    }
    
    unsigned char status = payload[0];
    unsigned char num_tlvs = payload[1];
    
    if (ui_color_enabled) {
        printf("  %sStatus:%s 0x%02X", ANSI_COLOR_BRIGHT_YELLOW, ANSI_RESET, status);
        switch(status) {
            case UCI_STATUS_OK: printf(" %s(OK)%s\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET); break;
            case UCI_STATUS_REJECTED: printf(" %s(REJECTED)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            case UCI_STATUS_FAILED: printf(" %s(FAILED)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            case UCI_STATUS_INVALID_PARAM: printf(" %s(INVALID_PARAM)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            default: printf(" %s(UNKNOWN)%s\n", ANSI_COLOR_YELLOW, ANSI_RESET); break;
        }
        printf("  %sNumber of Capability TLVs:%s %d\n", ANSI_COLOR_BRIGHT_YELLOW, ANSI_RESET, num_tlvs);
    } else {
        printf("  Status: 0x%02X", status);
        switch(status) {
            case UCI_STATUS_OK: printf(" (OK)\n"); break;
            case UCI_STATUS_REJECTED: printf(" (REJECTED)\n"); break;
            case UCI_STATUS_FAILED: printf(" (FAILED)\n"); break;
            case UCI_STATUS_INVALID_PARAM: printf(" (INVALID_PARAM)\n"); break;
            default: printf(" (UNKNOWN)\n"); break;
        }
        printf("  Number of Capability TLVs: %d\n", num_tlvs);
    }
    
    if (num_tlvs == 0) {
        if (ui_color_enabled) {
            printf("  %s%sNo capabilities reported%s\n", ANSI_COLOR_YELLOW, ANSI_BOLD, ANSI_RESET);
        } else {
            printf("  No capabilities reported\n");
        }
        return;
    }
    
    int offset = 2;
    for (int i = 0; i < num_tlvs; i++) {
        if (offset + 2 > payload_len) {
            if (ui_color_enabled) {
                printf("  %s%sError: Incomplete TLV at index %d%s\n", ANSI_COLOR_RED, ANSI_BOLD, i, ANSI_RESET);
            } else {
                printf("  Error: Incomplete TLV at index %d\n", i);
            }
            return;
        }
        
        CapTlvType tlv_type = (CapTlvType)payload[offset];
        unsigned char tlv_len = payload[offset + 1];
        offset += 2;
        
        // Print TLV header with color coding
        if (ui_color_enabled) {
            printf("  %sCapability TLV %d:%s\n", ANSI_COLOR_BRIGHT_CYAN, i, ANSI_RESET);
            printf("    %sType:%s 0x%02X", ANSI_COLOR_BRIGHT_YELLOW, ANSI_RESET, tlv_type);
        } else {
            printf("  Capability TLV %d:\n", i);
            printf("    Type: 0x%02X", tlv_type);
        }
        
        // Print descriptive name for TLV type
        switch(tlv_type) {
            case SUPPORTED_V1_FIRA_PHY_VERSION_RANGE_V2_MAX_MESSAGE_SIZE: 
                if (ui_color_enabled) printf(" %s(FIRA_PHY_VERSION_RANGE)%s\n", ANSI_COLOR_BRIGHT_BLUE, ANSI_RESET); 
                else printf(" (FIRA_PHY_VERSION_RANGE)\n"); 
                break;
            case SUPPORTED_V1_FIRA_MAC_VERSION_RANGE_V2_MAX_DATA_PAYLOAD_SIZE: 
                if (ui_color_enabled) printf(" %s(FIRA_MAC_VERSION_RANGE)%s\n", ANSI_COLOR_BRIGHT_BLUE, ANSI_RESET); 
                else printf(" (FIRA_MAC_VERSION_RANGE)\n"); 
                break;
            case SUPPORTED_V1_DEVICE_ROLES_V2_FIRA_PHY_VERSION_RANGE: 
                if (ui_color_enabled) printf(" %s(DEVICE_ROLES)%s\n", ANSI_COLOR_BRIGHT_BLUE, ANSI_RESET); 
                else printf(" (DEVICE_ROLES)\n"); 
                break;
            case SUPPORTED_V1_RANGING_METHOD_V2_FIRA_MAC_VERSION_RANGE: 
                if (ui_color_enabled) printf(" %s(RANGING_METHOD)%s\n", ANSI_COLOR_BRIGHT_BLUE, ANSI_RESET); 
                else printf(" (RANGING_METHOD)\n"); 
                break;
            case SUPPORTED_V1_STS_CONFIG_V2_DEVICE_TYPE: 
                if (ui_color_enabled) printf(" %s(STS_CONFIG)%s\n", ANSI_COLOR_BRIGHT_BLUE, ANSI_RESET); 
                else printf(" (STS_CONFIG)\n"); 
                break;
            case SUPPORTED_V1_MULTI_NODE_MODES_V2_DEVICE_ROLES: 
                if (ui_color_enabled) printf(" %s(MULTI_NODE_MODES)%s\n", ANSI_COLOR_BRIGHT_BLUE, ANSI_RESET); 
                else printf(" (MULTI_NODE_MODES)\n"); 
                break;
            case SUPPORTED_V1_AOA_V2_AOA_SUPPORT: 
                if (ui_color_enabled) printf(" %s(AOA_SUPPORT)%s\n", ANSI_COLOR_BRIGHT_BLUE, ANSI_RESET); 
                else printf(" (AOA_SUPPORT)\n"); 
                break;
            case SUPPORTED_V1_EXTENDED_MAC_ADDRESS_V2_EXTENDED_MAC_ADDRESS: 
                if (ui_color_enabled) printf(" %s(EXTENDED_MAC_ADDRESS)%s\n", ANSI_COLOR_BRIGHT_BLUE, ANSI_RESET); 
                else printf(" (EXTENDED_MAC_ADDRESS)\n"); 
                break;
            case CCC_SUPPORTED_CHANNELS: 
                if (ui_color_enabled) printf(" %s(CCC_CHANNELS)%s\n", ANSI_COLOR_BRIGHT_BLUE, ANSI_RESET); 
                else printf(" (CCC_CHANNELS)\n"); 
                break;
            case CCC_SUPPORTED_VERSIONS: 
                if (ui_color_enabled) printf(" %s(CCC_VERSIONS)%s\n", ANSI_COLOR_BRIGHT_BLUE, ANSI_RESET); 
                else printf(" (CCC_VERSIONS)\n"); 
                break;
            case RADAR_SUPPORT: 
                if (ui_color_enabled) printf(" %s(RADAR_SUPPORT)%s\n", ANSI_COLOR_BRIGHT_BLUE, ANSI_RESET); 
                else printf(" (RADAR_SUPPORT)\n"); 
                break;
            case SUPPORTED_POWER_STATS: 
                if (ui_color_enabled) printf(" %s(POWER_STATS)%s\n", ANSI_COLOR_BRIGHT_BLUE, ANSI_RESET); 
                else printf(" (POWER_STATS)\n"); 
                break;
            case SUPPORTED_DIAGNOSTICS: 
                if (ui_color_enabled) printf(" %s(DIAGNOSTICS)%s\n", ANSI_COLOR_BRIGHT_BLUE, ANSI_RESET); 
                else printf(" (DIAGNOSTICS)\n"); 
                break;
            case SUPPORTED_MAX_RANGING_SESSION_NUMBER: 
                if (ui_color_enabled) printf(" %s(MAX_RANGING_SESSIONS)%s\n", ANSI_COLOR_BRIGHT_BLUE, ANSI_RESET); 
                else printf(" (MAX_RANGING_SESSIONS)\n"); 
                break;
            default:
                if (ui_color_enabled) printf(" %s(UNKNOWN)%s\n", ANSI_COLOR_YELLOW, ANSI_RESET);
                else printf(" (UNKNOWN)\n");
                break;
        }
        
        if (ui_color_enabled) {
            printf("    %sLength:%s %d bytes\n", ANSI_COLOR_BRIGHT_YELLOW, ANSI_RESET, tlv_len);
        } else {
            printf("    Length: %d bytes\n", tlv_len);
        }
        
        if (offset + tlv_len > payload_len) {
            if (ui_color_enabled) {
                printf("    %s%sError: Incomplete TLV value%s\n", ANSI_COLOR_RED, ANSI_BOLD, ANSI_RESET);
            } else {
                printf("    Error: Incomplete TLV value\n");
            }
            return;
        }
        
        // Print value with hex dump and interpretation
        if (ui_color_enabled) {
            printf("    %sValue:%s ", ANSI_COLOR_BRIGHT_YELLOW, ANSI_RESET);
        } else {
            printf("    Value: ");
        }
        
        for (int j = 0; j < tlv_len; j++) {
            if (ui_color_enabled) {
                printf("%s%02X%s ", ANSI_COLOR_BRIGHT_GREEN, payload[offset + j], ANSI_RESET);
            } else {
                printf("%02X ", payload[offset + j]);
            }
        }
        printf("\n");
        
        // Interpret value based on TLV type and length
        if (tlv_len > 0) {
            if (ui_color_enabled) {
                printf("    %sInterpreted:%s ", ANSI_COLOR_BRIGHT_YELLOW, ANSI_RESET);
            } else {
                printf("    Interpreted: ");
            }
            
            if (tlv_len == 1) {
                unsigned char val = payload[offset];
                printf("%u", val);
                // Special interpretations for boolean-like values
                if (tlv_type == SUPPORTED_V1_AOA_V2_AOA_SUPPORT ||
                    tlv_type == SUPPORTED_V2_DT_TAG_BLOCK_SHIPPING ||
                    tlv_type == RADAR_SUPPORT) {
                    if (ui_color_enabled) {
                        printf(" %s(%s)%s", 
                               val ? ANSI_COLOR_BRIGHT_GREEN : ANSI_COLOR_RED,
                               val ? "SUPPORTED" : "NOT_SUPPORTED",
                               ANSI_RESET);
                    } else {
                        printf(" (%s)", val ? "SUPPORTED" : "NOT_SUPPORTED");
                    }
                }
            } else if (tlv_len == 2) {
                unsigned short val = payload[offset] | (payload[offset + 1] << 8); // Little-endian
                printf("%u", val);
            } else if (tlv_len == 4) {
                unsigned int val = payload[offset] | 
                                  (payload[offset + 1] << 8) | 
                                  (payload[offset + 2] << 16) | 
                                  (payload[offset + 3] << 24); // Little-endian
                printf("%u", val);
            } else {
                // For longer values, show as hex dump with interpretation
                printf("(complex value)");
            }
            printf("\n");
        }
        
        offset += tlv_len;
    }
}

void ui_decode_core_set_config_rsp(const unsigned char* payload, int payload_len) {
    if (ui_color_enabled) {
        printf("  %s%sCORE_SET_CONFIG Response:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  CORE_SET_CONFIG Response:\n");
    }

    if (payload_len < 2) {
        if (ui_color_enabled) {
            printf("    %s%sError: Payload too short (%d bytes, need at least 2)%s\n",
                   ANSI_COLOR_RED, ANSI_BOLD, payload_len, ANSI_RESET);
        } else {
            printf("    Error: Payload too short (%d bytes, need at least 2)\n", payload_len);
        }
        return;
    }

    unsigned char status = payload[0];
    unsigned char num_configs = payload[1];

    // Use enhanced error analysis
    if (ui_color_enabled) {
        printf("    %s%sStatus:%s 0x%02X", ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, status);
        switch(status) {
            case UCI_STATUS_OK: printf(" %s(OK)%s\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET); break;
            case UCI_STATUS_REJECTED: printf(" %s(REJECTED)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            case UCI_STATUS_FAILED: printf(" %s(FAILED)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            case UCI_STATUS_INVALID_PARAM: printf(" %s(INVALID_PARAM)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            default: printf(" %s(UNKNOWN)%s\n", ANSI_COLOR_YELLOW, ANSI_RESET); break;
        }
        // Call enhanced analysis for more detailed information
        if (status != UCI_STATUS_OK) {
            enhanced_error_analysis(status);
        }
        printf("    %s%sNumber of Config Status:%s %d\n",
               ANSI_COLOR_BRIGHT_CYAN, ANSI_BOLD, ANSI_RESET, num_configs);
    } else {
        printf("    Status: 0x%02X", status);
        switch(status) {
            case UCI_STATUS_OK: printf(" (OK)\n"); break;
            case UCI_STATUS_REJECTED: printf(" (REJECTED)\n"); break;
            case UCI_STATUS_FAILED: printf(" (FAILED)\n"); break;
            case UCI_STATUS_INVALID_PARAM: printf(" (INVALID_PARAM)\n"); break;
            default: printf(" (UNKNOWN)\n"); break;
        }
        // Call enhanced analysis for more detailed information
        if (status != UCI_STATUS_OK) {
            enhanced_error_analysis(status);
        }
        printf("    Number of Config Status: %d\n", num_configs);
    }

    if (num_configs > 0 && payload_len >= 2) {
        int offset = 2;
        for (int i = 0; i < num_configs && offset + 2 <= payload_len; i++) {
            unsigned char cfg_id = payload[offset];
            unsigned char cfg_status = payload[offset + 1];
            
            // Get human-readable parameter name
            const char* param_name = uci_config_get_device_param_name((DeviceConfigId)cfg_id);

            if (ui_color_enabled) {
                printf("    %s%sConfig[%d]:%s ID=0x%02X", 
                       ANSI_COLOR_BRIGHT_CYAN, ANSI_BOLD, i, ANSI_RESET, cfg_id);
                if (param_name) {
                    printf(" (%s)", param_name);
                }
                printf(", Status=0x%02X", cfg_status);
                switch(cfg_status) {
                    case UCI_STATUS_OK: printf(" %s(OK)%s\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET); break;
                    case UCI_STATUS_REJECTED: printf(" %s(REJECTED)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
                    case UCI_STATUS_FAILED: printf(" %s(FAILED)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
                    case UCI_STATUS_INVALID_PARAM: printf(" %s(INVALID_PARAM)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
                    default: printf(" %s(UNKNOWN)%s\n", ANSI_COLOR_YELLOW, ANSI_RESET); break;
                }
            } else {
                printf("    Config[%d]: ID=0x%02X", i, cfg_id);
                if (param_name) {
                    printf(" (%s)", param_name);
                }
                printf(", Status=0x%02X", cfg_status);
                switch(cfg_status) {
                    case UCI_STATUS_OK: printf(" (OK)\n"); break;
                    case UCI_STATUS_REJECTED: printf(" (REJECTED)\n"); break;
                    case UCI_STATUS_FAILED: printf(" (FAILED)\n"); break;
                    case UCI_STATUS_INVALID_PARAM: printf(" (INVALID_PARAM)\n"); break;
                    default: printf(" (UNKNOWN)\n"); break;
                }
            }
            offset += 2;
        }
    }
}

void ui_decode_core_get_config_rsp(const unsigned char* payload, int payload_len) {
    if (ui_color_enabled) {
        printf("  %s%sCORE_GET_CONFIG Response:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  CORE_GET_CONFIG Response:\n");
    }

    if (payload_len < 2) {
        if (ui_color_enabled) {
            printf("    %s%sError: Payload too short (%d bytes, need at least 2)%s\n",
                   ANSI_COLOR_RED, ANSI_BOLD, payload_len, ANSI_RESET);
        } else {
            printf("    Error: Payload too short (%d bytes, need at least 2)\n", payload_len);
        }
        return;
    }

    unsigned char status = payload[0];
    unsigned char num_tlvs = payload[1];

    if (ui_color_enabled) {
        printf("    %s%sStatus:%s 0x%02X", ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, status);
        switch(status) {
            case UCI_STATUS_OK: printf(" %s(OK)%s\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET); break;
            case UCI_STATUS_REJECTED: printf(" %s(REJECTED)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            case UCI_STATUS_FAILED: printf(" %s(FAILED)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            case UCI_STATUS_INVALID_PARAM: printf(" %s(INVALID_PARAM)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            default: printf(" %s(UNKNOWN)%s\n", ANSI_COLOR_YELLOW, ANSI_RESET); break;
        }
        printf("    %s%sNumber of TLVs:%s %d\n",
               ANSI_COLOR_BRIGHT_CYAN, ANSI_BOLD, ANSI_RESET, num_tlvs);
    } else {
        printf("    Status: 0x%02X", status);
        switch(status) {
            case UCI_STATUS_OK: printf(" (OK)\n"); break;
            case UCI_STATUS_REJECTED: printf(" (REJECTED)\n"); break;
            case UCI_STATUS_FAILED: printf(" (FAILED)\n"); break;
            case UCI_STATUS_INVALID_PARAM: printf(" (INVALID_PARAM)\n"); break;
            default: printf(" (UNKNOWN)\n"); break;
        }
        printf("    Number of TLVs: %d\n", num_tlvs);
    }

    if (num_tlvs > 0 && payload_len >= 2) {
        int offset = 2;
        for (int i = 0; i < num_tlvs && offset + 2 <= payload_len; i++) {
            unsigned char cfg_id = payload[offset];
            unsigned char cfg_len = payload[offset + 1];
            offset += 2;

            // Get human-readable parameter name
            const char* param_name = uci_config_get_device_param_name((DeviceConfigId)cfg_id);
            
            if (ui_color_enabled) {
                printf("    %s%sTLV %d:%s\n", ANSI_COLOR_BRIGHT_CYAN, ANSI_BOLD, i, ANSI_RESET);
                printf("      %s%sConfig ID:%s 0x%02X", ANSI_COLOR_YELLOW, ANSI_BOLD, ANSI_RESET, cfg_id);
                if (param_name) {
                    printf(" (%s)", param_name);
                }
                switch(cfg_id) {
                    case DEVICE_STATE: printf(" %s(DEVICE_STATE)%s\n", ANSI_COLOR_CYAN, ANSI_RESET); break;
                    case LOW_POWER_MODE: printf(" %s(LOW_POWER_MODE)%s\n", ANSI_COLOR_CYAN, ANSI_RESET); break;
                    default: 
                        if (!param_name) {
                            printf(" %s(UNKNOWN)%s\n", ANSI_COLOR_YELLOW, ANSI_RESET);
                        } else {
                            printf("\n");
                        }
                        break;
                }
                printf("      %s%sLength:%s %d\n", ANSI_COLOR_YELLOW, ANSI_BOLD, ANSI_RESET, cfg_len);
            } else {
                printf("    TLV %d:\n", i);
                printf("      Config ID: 0x%02X", cfg_id);
                if (param_name) {
                    printf(" (%s)", param_name);
                }
                switch(cfg_id) {
                    case DEVICE_STATE: printf(" (DEVICE_STATE)\n"); break;
                    case LOW_POWER_MODE: printf(" (LOW_POWER_MODE)\n"); break;
                    default: 
                        if (!param_name) {
                            printf(" (UNKNOWN)\n");
                        } else {
                            printf("\n");
                        }
                        break;
                }
                printf("      Length: %d\n", cfg_len);
            }

            if (offset + cfg_len <= payload_len) {
                if (ui_color_enabled) {
                    printf("      %s%sValue:%s ", ANSI_COLOR_YELLOW, ANSI_BOLD, ANSI_RESET);
                    for (int j = 0; j < cfg_len; j++) {
                        printf("%s%02X%s ", ANSI_COLOR_BRIGHT_WHITE, payload[offset + j], ANSI_RESET);
                    }
                    printf("\n");
                } else {
                    printf("      Value: ");
                    for (int j = 0; j < cfg_len; j++) {
                        printf("%02X ", payload[offset + j]);
                    }
                    printf("\n");
                }

                // Interpret common values with enhanced interpretation
                if (param_name) {
                    if (ui_color_enabled) {
                        printf("        %s%sInterpreted:%s ", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
                    } else {
                        printf("        Interpreted: ");
                    }
                    
                    // Handle specific parameter interpretations
                    if (strcasecmp(param_name, "device_state") == 0 && cfg_len == 1) {
                        unsigned char state = payload[offset];
                        if (ui_color_enabled) {
                            switch(state) {
                                case DEVICE_STATE_READY: printf("%sREADY%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, state); break;
                                case DEVICE_STATE_ACTIVE: printf("%sACTIVE%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, state); break;
                                case DEVICE_STATE_ERROR: printf("%sERROR%s (0x%02X)\n", ANSI_COLOR_RED, ANSI_RESET, state); break;
                                default: printf("%sUNKNOWN%s (0x%02X)\n", ANSI_COLOR_YELLOW, ANSI_RESET, state); break;
                            }
                        } else {
                            switch(state) {
                                case DEVICE_STATE_READY: printf("READY (0x%02X)\n", state); break;
                                case DEVICE_STATE_ACTIVE: printf("ACTIVE (0x%02X)\n", state); break;
                                case DEVICE_STATE_ERROR: printf("ERROR (0x%02X)\n", state); break;
                                default: printf("UNKNOWN (0x%02X)\n", state); break;
                            }
                        }
                    } else if (strcasecmp(param_name, "low_power_mode") == 0 && cfg_len == 1) {
                        unsigned char lpm = payload[offset];
                        if (ui_color_enabled) {
                            printf("%s%s%s (0x%02X)\n",
                                   lpm ? ANSI_COLOR_BRIGHT_GREEN : ANSI_COLOR_YELLOW,
                                   lpm ? "ON" : "OFF", ANSI_RESET, lpm);
                        } else {
                            printf("%s (0x%02X)\n", lpm ? "ON" : "OFF", lpm);
                        }
                    } else if (cfg_len <= 8) {
                        // For numeric values, try to display as integer
                        uint64_t value = 0;
                        for (int j = 0; j < cfg_len && j < 8; j++) {
                            value |= ((uint64_t)payload[offset + j]) << (j * 8);
                        }
                        
                        // Get parameter range information
                        uint64_t min_val, max_val;
                        if (uci_config_get_device_param_range((DeviceConfigId)cfg_id, &min_val, &max_val) == 0) {
                            if (ui_color_enabled) {
                                if (value >= min_val && value <= max_val) {
                                    printf("%s%" PRIu64 "%s (0x%02X", ANSI_COLOR_BRIGHT_GREEN, value, ANSI_RESET, payload[offset]);
                                    for (int j = 1; j < cfg_len; j++) {
                                        printf(" %02X", payload[offset + j]);
                                    }
                                    printf(") [Range: %" PRIu64 "-%" PRIu64 "]\n", min_val, max_val);
                                } else {
                                    printf("%s%" PRIu64 "%s (0x%02X", ANSI_COLOR_YELLOW, value, ANSI_RESET, payload[offset]);
                                    for (int j = 1; j < cfg_len; j++) {
                                        printf(" %02X", payload[offset + j]);
                                    }
                                    printf(") [OUT OF RANGE: %" PRIu64 "-%" PRIu64 "]\n", min_val, max_val);
                                }
                            } else {
                                if (value >= min_val && value <= max_val) {
                                    printf("%" PRIu64 " (0x%02X", value, payload[offset]);
                                    for (int j = 1; j < cfg_len; j++) {
                                        printf(" %02X", payload[offset + j]);
                                    }
                                    printf(") [Range: %" PRIu64 "-%" PRIu64 "]\n", min_val, max_val);
                                } else {
                                    printf("%" PRIu64 " (0x%02X", value, payload[offset]);
                                    for (int j = 1; j < cfg_len; j++) {
                                        printf(" %02X", payload[offset + j]);
                                    }
                                    printf(") [OUT OF RANGE: %" PRIu64 "-%" PRIu64 "]\n", min_val, max_val);
                                }
                            }
                        } else {
                            if (ui_color_enabled) {
                                printf("%s%" PRIu64 "%s (0x%02X", ANSI_COLOR_BRIGHT_WHITE, value, ANSI_RESET, payload[offset]);
                                for (int j = 1; j < cfg_len; j++) {
                                    printf(" %02X", payload[offset + j]);
                                }
                                printf(")\n");
                            } else {
                                printf("%" PRIu64 " (0x%02X", value, payload[offset]);
                                for (int j = 1; j < cfg_len; j++) {
                                    printf(" %02X", payload[offset + j]);
                                }
                                printf(")\n");
                            }
                        }
                    } else {
                        // For longer values, just show hex
                        if (ui_color_enabled) {
                            printf("%s", ANSI_COLOR_BRIGHT_WHITE);
                        }
                        for (int j = 0; j < cfg_len; j++) {
                            printf("%02X ", payload[offset + j]);
                        }
                        if (ui_color_enabled) {
                            printf("%s\n", ANSI_RESET);
                        } else {
                            printf("\n");
                        }
                    }
                } else if (cfg_id == DEVICE_STATE && cfg_len == 1) {
                    unsigned char state = payload[offset];
                    if (ui_color_enabled) {
                        printf("        %s%sInterpreted:%s ", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
                        switch(state) {
                            case DEVICE_STATE_READY: printf("%sREADY%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, state); break;
                            case DEVICE_STATE_ACTIVE: printf("%sACTIVE%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, state); break;
                            case DEVICE_STATE_ERROR: printf("%sERROR%s (0x%02X)\n", ANSI_COLOR_RED, ANSI_RESET, state); break;
                            default: printf("%sUNKNOWN%s (0x%02X)\n", ANSI_COLOR_YELLOW, ANSI_RESET, state); break;
                        }
                    } else {
                        printf("        Interpreted: ");
                        switch(state) {
                            case DEVICE_STATE_READY: printf("READY (0x%02X)\n", state); break;
                            case DEVICE_STATE_ACTIVE: printf("ACTIVE (0x%02X)\n", state); break;
                            case DEVICE_STATE_ERROR: printf("ERROR (0x%02X)\n", state); break;
                            default: printf("UNKNOWN (0x%02X)\n", state); break;
                        }
                    }
                } else if (cfg_id == LOW_POWER_MODE && cfg_len == 1) {
                    unsigned char lpm = payload[offset];
                    if (ui_color_enabled) {
                        printf("        %s%sInterpreted:%s %s%s%s (0x%02X)\n",
                               ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET,
                               lpm ? ANSI_COLOR_BRIGHT_GREEN : ANSI_COLOR_YELLOW,
                               lpm ? "ON" : "OFF", ANSI_RESET, lpm);
                    } else {
                        printf("        Interpreted: %s (0x%02X)\n", lpm ? "ON" : "OFF", lpm);
                    }
                }
            } else {
                if (ui_color_enabled) {
                    printf("      %s%sValue: TRUNCATED%s (expected %d bytes, only %d available)\n",
                           ANSI_COLOR_RED, ANSI_BOLD, ANSI_RESET, cfg_len, payload_len - offset);
                } else {
                    printf("      Value: TRUNCATED (expected %d bytes, only %d available)\n",
                           cfg_len, payload_len - offset);
                }
            }

            offset += cfg_len;
        }
    }
}

void ui_decode_core_device_reset_rsp(const unsigned char* payload, int payload_len) {
    if (ui_color_enabled) {
        printf("  %s%sCORE_DEVICE_RESET Response:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  CORE_DEVICE_RESET Response:\n");
    }

    if (payload_len < 1) {
        if (ui_color_enabled) {
            printf("    %s%sError: Payload too short (%d bytes, need at least 1)%s\n",
                   ANSI_COLOR_RED, ANSI_BOLD, payload_len, ANSI_RESET);
        } else {
            printf("    Error: Payload too short (%d bytes, need at least 1)\n", payload_len);
        }
        return;
    }

    unsigned char status = payload[0];

    if (ui_color_enabled) {
        printf("    %s%sStatus:%s 0x%02X", ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, status);
        switch(status) {
            case UCI_STATUS_OK: printf(" %s(OK)%s\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET); break;
            case UCI_STATUS_REJECTED: printf(" %s(REJECTED)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            case UCI_STATUS_FAILED: printf(" %s(FAILED)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            default: printf(" %s(UNKNOWN)%s\n", ANSI_COLOR_YELLOW, ANSI_RESET); break;
        }
    } else {
        printf("    Status: 0x%02X", status);
        switch(status) {
            case UCI_STATUS_OK: printf(" (OK)\n"); break;
            case UCI_STATUS_REJECTED: printf(" (REJECTED)\n"); break;
            case UCI_STATUS_FAILED: printf(" (FAILED)\n"); break;
            default: printf(" (UNKNOWN)\n"); break;
        }
    }
}

void ui_decode_core_device_suspend_rsp(const unsigned char* payload, int payload_len) {
    if (ui_color_enabled) {
        printf("  %s%sCORE_DEVICE_SUSPEND Response:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  CORE_DEVICE_SUSPEND Response:\n");
    }

    if (payload_len < 1) {
        if (ui_color_enabled) {
            printf("    %s%sError: Payload too short (%d bytes, need at least 1)%s\n",
                   ANSI_COLOR_RED, ANSI_BOLD, payload_len, ANSI_RESET);
        } else {
            printf("    Error: Payload too short (%d bytes, need at least 1)\n", payload_len);
        }
        return;
    }

    unsigned char status = payload[0];

    if (ui_color_enabled) {
        printf("    %s%sStatus:%s 0x%02X", ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, status);
        switch(status) {
            case UCI_STATUS_OK: printf(" %s(OK)%s\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET); break;
            case UCI_STATUS_REJECTED: printf(" %s(REJECTED)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            case UCI_STATUS_FAILED: printf(" %s(FAILED)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            default: printf(" %s(UNKNOWN)%s\n", ANSI_COLOR_YELLOW, ANSI_RESET); break;
        }
    } else {
        printf("    Status: 0x%02X", status);
        switch(status) {
            case UCI_STATUS_OK: printf(" (OK)\n"); break;
            case UCI_STATUS_REJECTED: printf(" (REJECTED)\n"); break;
            case UCI_STATUS_FAILED: printf(" (FAILED)\n"); break;
            default: printf(" (UNKNOWN)\n"); break;
        }
    }
}

void ui_decode_core_query_uwbs_timestamp_rsp(const unsigned char* payload, int payload_len) {
    if (ui_color_enabled) {
        printf("  %s%sCORE_QUERY_UWBS_TIMESTAMP Response:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  CORE_QUERY_UWBS_TIMESTAMP Response:\n");
    }

    if (payload_len < 9) {
        if (ui_color_enabled) {
            printf("    %s%sError: Payload too short (%d bytes, need at least 9)%s\n",
                   ANSI_COLOR_RED, ANSI_BOLD, payload_len, ANSI_RESET);
        } else {
            printf("    Error: Payload too short (%d bytes, need at least 9)\n", payload_len);
        }
        return;
    }

    unsigned char status = payload[0];
    unsigned long long timestamp = ui_read_u64_le(payload + 1);

    if (ui_color_enabled) {
        printf("    %s%sStatus:%s 0x%02X", ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, status);
        switch(status) {
            case UCI_STATUS_OK:
                printf(" %s(OK)%s\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET);
                printf("    %s%sTimestamp:%s %llu\n",
                       ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_RESET, timestamp);
                break;
            case UCI_STATUS_REJECTED: printf(" %s(REJECTED)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            case UCI_STATUS_FAILED: printf(" %s(FAILED)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            default: printf(" %s(UNKNOWN)%s\n", ANSI_COLOR_YELLOW, ANSI_RESET); break;
        }
    } else {
        printf("    Status: 0x%02X", status);
        switch(status) {
            case UCI_STATUS_OK:
                printf(" (OK)\n");
                printf("    Timestamp: %llu\n", timestamp);
                break;
            case UCI_STATUS_REJECTED: printf(" (REJECTED)\n"); break;
            case UCI_STATUS_FAILED: printf(" (FAILED)\n"); break;
            default: printf(" (UNKNOWN)\n"); break;
        }
    }
}

void ui_decode_core_device_status_ntf(const unsigned char* payload, int payload_len) {
    if (ui_color_enabled) {
        printf("  %s%sCORE_DEVICE_STATUS_NTF:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  CORE_DEVICE_STATUS_NTF:\n");
    }

    if (payload_len < 1) {
        if (ui_color_enabled) {
            printf("    %s%sError: Payload too short (%d bytes, need at least 1)%s\n",
                   ANSI_COLOR_RED, ANSI_BOLD, payload_len, ANSI_RESET);
        } else {
            printf("    Error: Payload too short (%d bytes, need at least 1)\n", payload_len);
        }
        return;
    }

    unsigned char device_state = payload[0];
    const uci_lookup_entry_t* entry = uci_lookup_device_state(device_state);

    const char* state_color = ANSI_COLOR_YELLOW;
    if (device_state == DEVICE_STATE_READY || device_state == DEVICE_STATE_ACTIVE) {
        state_color = ANSI_COLOR_BRIGHT_GREEN;
    } else if (device_state == DEVICE_STATE_ERROR) {
        state_color = ANSI_COLOR_RED;
    }

    print_lookup_line("Device State", device_state, entry, state_color);

    print_additional_payload_bytes(payload, payload_len, 1);
}

void ui_decode_core_generic_error_ntf(const unsigned char* payload, int payload_len) {
    if (ui_color_enabled) {
        printf("  %s%sCORE_GENERIC_ERROR_NTF:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  CORE_GENERIC_ERROR_NTF:\n");
    }

    if (payload_len < 1) {
        if (ui_color_enabled) {
            printf("    %s%sError: Payload too short (%d bytes, need at least 1)%s\n",
                   ANSI_COLOR_RED, ANSI_BOLD, payload_len, ANSI_RESET);
        } else {
            printf("    Error: Payload too short (%d bytes, need at least 1)\n", payload_len);
        }
        return;
    }

    unsigned char status = payload[0];
    const uci_lookup_entry_t* entry = uci_lookup_status(status);

    const char* status_color = (status == UCI_STATUS_OK) ? ANSI_COLOR_BRIGHT_GREEN : ANSI_COLOR_RED;

    print_lookup_line("Status Code", status, entry, status_color);

    enhanced_error_analysis(status);

    print_additional_payload_bytes(payload, payload_len, 1);
}

void ui_decode_session_init_cmd(const unsigned char* payload, int payload_len) {
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

    const char* type_name = uci_session_type_to_string(session_type);

    if (ui_color_enabled) {
        printf("    %s%sSession Type:%s 0x%02X (%s%s%s)\n",
               ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_RESET, session_type,
               ANSI_COLOR_CYAN, type_name, ANSI_RESET);
    } else {
        printf("    Session Type: 0x%02X (%s)\n", session_type, type_name);
    }
}

void ui_decode_session_init_rsp(const unsigned char* payload, int payload_len) {
    if (ui_color_enabled) {
        printf("  %s%sSESSION_INIT Response:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  SESSION_INIT Response:\n");
    }

    if (payload_len < 5) {
        if (ui_color_enabled) {
            printf("    %s%sError: Payload too short (%d bytes, need at least 5)%s\n",
                   ANSI_COLOR_RED, ANSI_BOLD, payload_len, ANSI_RESET);
        } else {
            printf("    Error: Payload too short (%d bytes, need at least 5)\n", payload_len);
        }
        return;
    }

    unsigned char status = payload[0];
    unsigned int session_handle = ui_read_u32_le(payload + 1);

    if (ui_color_enabled) {
        printf("    %s%sStatus:%s 0x%02X", ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, status);
        switch(status) {
            case UCI_STATUS_OK: printf(" %s(OK)%s\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET); break;
            case UCI_STATUS_REJECTED: printf(" %s(REJECTED)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            case UCI_STATUS_FAILED: printf(" %s(FAILED)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            case UCI_STATUS_INVALID_PARAM: printf(" %s(INVALID_PARAM)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            case UCI_STATUS_SESSION_DUPLICATE: printf(" %s(SESSION_DUPLICATE)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            default: printf(" %s(UNKNOWN)%s\n", ANSI_COLOR_YELLOW, ANSI_RESET); break;
        }
        printf("    %s%sSession Handle:%s 0x%08X\n",
               ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_RESET, session_handle);
    } else {
        printf("    Status: 0x%02X", status);
        switch(status) {
            case UCI_STATUS_OK: printf(" (OK)\n"); break;
            case UCI_STATUS_REJECTED: printf(" (REJECTED)\n"); break;
            case UCI_STATUS_FAILED: printf(" (FAILED)\n"); break;
            case UCI_STATUS_INVALID_PARAM: printf(" (INVALID_PARAM)\n"); break;
            case UCI_STATUS_SESSION_DUPLICATE: printf(" (SESSION_DUPLICATE)\n"); break;
            default: printf(" (UNKNOWN)\n"); break;
        }
        printf("    Session Handle: 0x%08X\n", session_handle);
    }
}

void ui_decode_session_deinit_rsp(const unsigned char* payload, int payload_len) {
    if (ui_color_enabled) {
        printf("  %s%sSESSION_DEINIT Response:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  SESSION_DEINIT Response:\n");
    }

    if (payload_len < 1) {
        if (ui_color_enabled) {
            printf("    %s%sError: Payload too short (%d bytes, need at least 1)%s\n",
                   ANSI_COLOR_RED, ANSI_BOLD, payload_len, ANSI_RESET);
        } else {
            printf("    Error: Payload too short (%d bytes, need at least 1)\n", payload_len);
        }
        return;
    }

    unsigned char status = payload[0];

    if (ui_color_enabled) {
        printf("    %s%sStatus:%s 0x%02X", ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, status);
        switch(status) {
            case UCI_STATUS_OK: printf(" %s(OK)%s\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET); break;
            case UCI_STATUS_REJECTED: printf(" %s(REJECTED)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            case UCI_STATUS_FAILED: printf(" %s(FAILED)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            case UCI_STATUS_INVALID_PARAM: printf(" %s(INVALID_PARAM)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            case UCI_STATUS_SESSION_NOT_EXIST: printf(" %s(SESSION_NOT_EXIST)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            default: printf(" %s(UNKNOWN)%s\n", ANSI_COLOR_YELLOW, ANSI_RESET); break;
        }
    } else {
        printf("    Status: 0x%02X", status);
        switch(status) {
            case UCI_STATUS_OK: printf(" (OK)\n"); break;
            case UCI_STATUS_REJECTED: printf(" (REJECTED)\n"); break;
            case UCI_STATUS_FAILED: printf(" (FAILED)\n"); break;
            case UCI_STATUS_INVALID_PARAM: printf(" (INVALID_PARAM)\n"); break;
            case UCI_STATUS_SESSION_NOT_EXIST: printf(" (SESSION_NOT_EXIST)\n"); break;
            default: printf(" (UNKNOWN)\n"); break;
        }
    }
}

void ui_decode_session_set_app_config_rsp(const unsigned char* payload, int payload_len) {
    if (ui_color_enabled) {
        printf("  %s%sSESSION_SET_APP_CONFIG Response:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  SESSION_SET_APP_CONFIG Response:\n");
    }

    if (payload_len < 2) {
        if (ui_color_enabled) {
            printf("    %s%sError: Payload too short (%d bytes, need at least 2)%s\n",
                   ANSI_COLOR_RED, ANSI_BOLD, payload_len, ANSI_RESET);
        } else {
            printf("    Error: Payload too short (%d bytes, need at least 2)\n", payload_len);
        }
        return;
    }

    unsigned char status = payload[0];
    unsigned char num_configs = payload[1];

    if (ui_color_enabled) {
        printf("    %s%sStatus:%s 0x%02X", ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, status);
        switch(status) {
            case UCI_STATUS_OK: printf(" %s(OK)%s\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET); break;
            case UCI_STATUS_REJECTED: printf(" %s(REJECTED)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            case UCI_STATUS_FAILED: printf(" %s(FAILED)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            case UCI_STATUS_INVALID_PARAM: printf(" %s(INVALID_PARAM)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            case UCI_STATUS_INVALID_RANGE: printf(" %s(INVALID_RANGE)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            case UCI_STATUS_INVALID_MSG_SIZE: printf(" %s(INVALID_MSG_SIZE)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            default: printf(" %s(UNKNOWN)%s\n", ANSI_COLOR_YELLOW, ANSI_RESET); break;
        }
        // Call enhanced analysis for more detailed information
        if (status != UCI_STATUS_OK) {
            enhanced_error_analysis(status);
        }
        printf("    %s%sNumber of Config Status:%s %d\n",
               ANSI_COLOR_BRIGHT_CYAN, ANSI_BOLD, ANSI_RESET, num_configs);
    } else {
        printf("    Status: 0x%02X", status);
        switch(status) {
            case UCI_STATUS_OK: printf(" (OK)\n"); break;
            case UCI_STATUS_REJECTED: printf(" (REJECTED)\n"); break;
            case UCI_STATUS_FAILED: printf(" (FAILED)\n"); break;
            case UCI_STATUS_INVALID_PARAM: printf(" (INVALID_PARAM)\n"); break;
            case UCI_STATUS_INVALID_RANGE: printf(" (INVALID_RANGE)\n"); break;
            case UCI_STATUS_INVALID_MSG_SIZE: printf(" (INVALID_MSG_SIZE)\n"); break;
            default: printf(" (UNKNOWN)\n"); break;
        }
        // Call enhanced analysis for more detailed information
        if (status != UCI_STATUS_OK) {
            enhanced_error_analysis(status);
        }
        printf("    Number of Config Status: %d\n", num_configs);
    }

    // Parse config status entries (each is 2 bytes: config_id + status)
    int offset = 2;
    for (int i = 0; i < num_configs && offset + 2 <= payload_len; i++) {
        unsigned char cfg_id = payload[offset];
        unsigned char cfg_status = payload[offset + 1];
        
        // Get human-readable parameter name
        const char* param_name = uci_config_get_app_param_name((AppConfigTlvType)cfg_id);

        if (ui_color_enabled) {
            printf("    %s%sConfig[%d]:%s ID=0x%02X", 
                   ANSI_COLOR_BRIGHT_CYAN, ANSI_BOLD, i, ANSI_RESET, cfg_id);
            if (param_name) {
                printf(" (%s)", param_name);
            }
            printf(", Status=0x%02X", cfg_status);
            switch(cfg_status) {
                case UCI_STATUS_OK: printf(" %s(OK)%s\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET); break;
                case UCI_STATUS_REJECTED: printf(" %s(REJECTED)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
                case UCI_STATUS_FAILED: printf(" %s(FAILED)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
                default: printf(" %s(UNKNOWN)%s\n", ANSI_COLOR_YELLOW, ANSI_RESET); break;
            }
        } else {
            printf("    Config[%d]: ID=0x%02X", i, cfg_id);
            if (param_name) {
                printf(" (%s)", param_name);
            }
            printf(", Status=0x%02X", cfg_status);
            switch(cfg_status) {
                case UCI_STATUS_OK: printf(" (OK)\n"); break;
                case UCI_STATUS_REJECTED: printf(" (REJECTED)\n"); break;
                case UCI_STATUS_FAILED: printf(" (FAILED)\n"); break;
                default: printf(" (UNKNOWN)\n"); break;
            }
        }
        offset += 2;
    }
}

void ui_decode_session_get_app_config_rsp(const unsigned char* payload, int payload_len) {
    if (ui_color_enabled) {
        printf("  %s%sSESSION_GET_APP_CONFIG Response:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  SESSION_GET_APP_CONFIG Response:\n");
    }

    if (payload_len < 2) {
        if (ui_color_enabled) {
            printf("    %s%sError: Payload too short (%d bytes, need at least 2)%s\n",
                   ANSI_COLOR_RED, ANSI_BOLD, payload_len, ANSI_RESET);
        } else {
            printf("    Error: Payload too short (%d bytes, need at least 2)\n", payload_len);
        }
        return;
    }

    unsigned char status = payload[0];
    unsigned char num_tlvs = payload[1];

    if (ui_color_enabled) {
        printf("    %s%sStatus:%s 0x%02X", ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, status);
        switch(status) {
            case UCI_STATUS_OK: printf(" %s(OK)%s\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET); break;
            case UCI_STATUS_REJECTED: printf(" %s(REJECTED)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            case UCI_STATUS_FAILED: printf(" %s(FAILED)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            case UCI_STATUS_INVALID_PARAM: printf(" %s(INVALID_PARAM)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            default: printf(" %s(UNKNOWN)%s\n", ANSI_COLOR_YELLOW, ANSI_RESET); break;
        }
        // Call enhanced analysis for more detailed information
        if (status != UCI_STATUS_OK) {
            enhanced_error_analysis(status);
        }
        printf("    %s%sNumber of TLVs:%s %d\n",
               ANSI_COLOR_BRIGHT_CYAN, ANSI_BOLD, ANSI_RESET, num_tlvs);
    } else {
        printf("    Status: 0x%02X", status);
        switch(status) {
            case UCI_STATUS_OK: printf(" (OK)\n"); break;
            case UCI_STATUS_REJECTED: printf(" (REJECTED)\n"); break;
            case UCI_STATUS_FAILED: printf(" (FAILED)\n"); break;
            case UCI_STATUS_INVALID_PARAM: printf(" (INVALID_PARAM)\n"); break;
            default: printf(" (UNKNOWN)\n"); break;
        }
        // Call enhanced analysis for more detailed information
        if (status != UCI_STATUS_OK) {
            enhanced_error_analysis(status);
        }
        printf("    Number of TLVs: %d\n", num_tlvs);
    }

    // Parse TLV entries (each TLV: config_id (1 byte) + length (1 byte) + value bytes)
    int offset = 2;
    for (int i = 0; i < num_tlvs && offset + 2 <= payload_len; i++) {
        unsigned char cfg_id = payload[offset];
        unsigned char cfg_len = payload[offset + 1];

        if (offset + 2 + cfg_len > payload_len) {
            if (ui_color_enabled) {
                printf("    %s%sWarning: TLV[%d] extends beyond payload%s\n",
                       ANSI_COLOR_YELLOW, ANSI_BOLD, i, ANSI_RESET);
            } else {
                printf("    Warning: TLV[%d] extends beyond payload\n", i);
            }
            break;
        }

        // Get human-readable parameter name
        const char* param_name = uci_config_get_app_param_name((AppConfigTlvType)cfg_id);
        
        if (ui_color_enabled) {
            printf("    %s%sTLV[%d]:%s Config ID=0x%02X", 
                   ANSI_COLOR_BRIGHT_CYAN, ANSI_BOLD, i, ANSI_RESET, cfg_id);
            if (param_name) {
                printf(" (%s)", param_name);
            }
            printf(", Length=%d bytes\n", cfg_len);
            printf("      %s%sValue:%s ", ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_RESET);
        } else {
            printf("    TLV[%d]: Config ID=0x%02X", i, cfg_id);
            if (param_name) {
                printf(" (%s)", param_name);
            }
            printf(", Length=%d bytes\n", cfg_len);
            printf("      Value: ");
        }

        // Print hex value
        for (int j = 0; j < cfg_len; j++) {
            printf("0x%02X ", payload[offset + 2 + j]);
        }
        printf("\n");

        // Try to interpret common parameter values
        if (param_name) {
            if (ui_color_enabled) {
                printf("      %s%sInterpreted:%s ", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
            } else {
                printf("      Interpreted: ");
            }
            
            // Handle specific parameter interpretations
            if (strcasecmp(param_name, "device_type") == 0 && cfg_len == 1) {
                unsigned char value = payload[offset + 2];
                if (ui_color_enabled) {
                    switch(value) {
                        case 0x00: printf("%sCONTROLEE%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, value); break;
                        case 0x01: printf("%sCONTROLLER%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, value); break;
                        default: printf("%sUNKNOWN%s (0x%02X)\n", ANSI_COLOR_YELLOW, ANSI_RESET, value); break;
                    }
                } else {
                    switch(value) {
                        case 0x00: printf("CONTROLEE (0x%02X)\n", value); break;
                        case 0x01: printf("CONTROLLER (0x%02X)\n", value); break;
                        default: printf("UNKNOWN (0x%02X)\n", value); break;
                    }
                }
            } else if (strcasecmp(param_name, "device_role") == 0 && cfg_len == 1) {
                unsigned char value = payload[offset + 2];
                if (ui_color_enabled) {
                    switch(value) {
                        case 0x00: printf("%sRESPONDER%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, value); break;
                        case 0x01: printf("%sINITIATOR%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, value); break;
                        case 0x05: printf("%sADVERTISER%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, value); break;
                        case 0x06: printf("%sOBSERVER%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, value); break;
                        case 0x07: printf("%sDT_ANCHOR%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, value); break;
                        case 0x08: printf("%sDT_TAG%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, value); break;
                        default: printf("%sUNKNOWN%s (0x%02X)\n", ANSI_COLOR_YELLOW, ANSI_RESET, value); break;
                    }
                } else {
                    switch(value) {
                        case 0x00: printf("RESPONDER (0x%02X)\n", value); break;
                        case 0x01: printf("INITIATOR (0x%02X)\n", value); break;
                        case 0x05: printf("ADVERTISER (0x%02X)\n", value); break;
                        case 0x06: printf("OBSERVER (0x%02X)\n", value); break;
                        case 0x07: printf("DT_ANCHOR (0x%02X)\n", value); break;
                        case 0x08: printf("DT_TAG (0x%02X)\n", value); break;
                        default: printf("UNKNOWN (0x%02X)\n", value); break;
                    }
                }
            } else if (strcasecmp(param_name, "channel_number") == 0 && cfg_len == 1) {
                unsigned char value = payload[offset + 2];
                if (ui_color_enabled) {
                    switch(value) {
                        case 5: printf("%sChannel 5%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, value); break;
                        case 9: printf("%sChannel 9%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, value); break;
                        default: printf("%sUNKNOWN%s (0x%02X)\n", ANSI_COLOR_YELLOW, ANSI_RESET, value); break;
                    }
                } else {
                    switch(value) {
                        case 5: printf("Channel 5 (0x%02X)\n", value); break;
                        case 9: printf("Channel 9 (0x%02X)\n", value); break;
                        default: printf("UNKNOWN (0x%02X)\n", value); break;
                    }
                }
            } else if (strcasecmp(param_name, "no_of_controlee") == 0 && cfg_len == 1) {
                unsigned char value = payload[offset + 2];
                if (ui_color_enabled) {
                    printf("%s%u controlees%s (0x%02X)\n",
                           ANSI_COLOR_BRIGHT_GREEN, value, ANSI_RESET, value);
                } else {
                    printf("%u controlees (0x%02X)\n", value, value);
                }
            } else if (strcasecmp(param_name, "device_mac_address") == 0 && cfg_len == 2) {
                unsigned int value = (unsigned int)payload[offset + 2] |
                                     ((unsigned int)payload[offset + 3] << 8);
                if (ui_color_enabled) {
                    printf("%sSHORT%s 0x%04X\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, value);
                } else {
                    printf("SHORT 0x%04X\n", value);
                }
            } else if (strcasecmp(param_name, "dst_mac_address") == 0 && cfg_len >= 2 && (cfg_len % 2) == 0) {
                int address_count = cfg_len / 2;
                if (ui_color_enabled) {
                    printf("%s%d addresses%s: ", ANSI_COLOR_BRIGHT_GREEN, address_count, ANSI_RESET);
                } else {
                    printf("%d addresses: ", address_count);
                }
                for (int addr_idx = 0; addr_idx < address_count; addr_idx++) {
                    unsigned int value = (unsigned int)payload[offset + 2 + (addr_idx * 2)] |
                                         ((unsigned int)payload[offset + 3 + (addr_idx * 2)] << 8);
                    if (addr_idx > 0) {
                        printf(", ");
                    }
                    printf("0x%04X", value);
                }
                printf("\n");
            } else if (strcasecmp(param_name, "mac_address_mode") == 0 && cfg_len == 1) {
                unsigned char value = payload[offset + 2];
                if (ui_color_enabled) {
                    switch(value) {
                        case 0: printf("%sSHORT%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, value); break;
                        case 1: printf("%sEXTENDED%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, value); break;
                        default: printf("%sUNKNOWN%s (0x%02X)\n", ANSI_COLOR_YELLOW, ANSI_RESET, value); break;
                    }
                } else {
                    switch(value) {
                        case 0: printf("SHORT (0x%02X)\n", value); break;
                        case 1: printf("EXTENDED (0x%02X)\n", value); break;
                        default: printf("UNKNOWN (0x%02X)\n", value); break;
                    }
                }
            } else if (strcasecmp(param_name, "slot_duration") == 0 && cfg_len == 2) {
                unsigned int value = (unsigned int)payload[offset + 2] |
                                     ((unsigned int)payload[offset + 3] << 8);
                if (ui_color_enabled) {
                    printf("%s%u RSTU%s (0x%04X)\n", ANSI_COLOR_BRIGHT_GREEN, value, ANSI_RESET, value);
                } else {
                    printf("%u RSTU (0x%04X)\n", value, value);
                }
            } else if (strcasecmp(param_name, "ranging_duration") == 0 && cfg_len == 4) {
                unsigned int value = ui_read_u32_le(&payload[offset + 2]);
                if (ui_color_enabled) {
                    printf("%s%u ms%s (0x%08X)\n", ANSI_COLOR_BRIGHT_GREEN, value, ANSI_RESET, value);
                } else {
                    printf("%u ms (0x%08X)\n", value, value);
                }
            } else if (strcasecmp(param_name, "multi_node_mode") == 0 && cfg_len == 1) {
                unsigned char value = payload[offset + 2];
                if (ui_color_enabled) {
                    switch(value) {
                        case 0x00: printf("%sUNICAST%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, value); break;
                        case 0x01: printf("%sONE_TO_MANY%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, value); break;
                        case 0x02: printf("%sMANY_TO_MANY%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, value); break;
                        default: printf("%sUNKNOWN%s (0x%02X)\n", ANSI_COLOR_YELLOW, ANSI_RESET, value); break;
                    }
                } else {
                    switch(value) {
                        case 0x00: printf("UNICAST (0x%02X)\n", value); break;
                        case 0x01: printf("ONE_TO_MANY (0x%02X)\n", value); break;
                        case 0x02: printf("MANY_TO_MANY (0x%02X)\n", value); break;
                        default: printf("UNKNOWN (0x%02X)\n", value); break;
                    }
                }
            } else if (strcasecmp(param_name, "sts_config") == 0 && cfg_len == 1) {
                unsigned char value = payload[offset + 2];
                if (ui_color_enabled) {
                    switch(value) {
                        case 0: printf("%sSTATIC_STS%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, value); break;
                        case 1: printf("%sDYNAMIC_STS%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, value); break;
                        case 2: printf("%sDYNAMIC_STS_RESPONDER_SUBSESSION%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, value); break;
                        case 3: printf("%sPROVISIONED_STS%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, value); break;
                        case 4: printf("%sPROVISIONED_STS_RESPONDER_SUBSESSION%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, value); break;
                        default: printf("%sUNKNOWN%s (0x%02X)\n", ANSI_COLOR_YELLOW, ANSI_RESET, value); break;
                    }
                } else {
                    switch(value) {
                        case 0: printf("STATIC_STS (0x%02X)\n", value); break;
                        case 1: printf("DYNAMIC_STS (0x%02X)\n", value); break;
                        case 2: printf("DYNAMIC_STS_RESPONDER_SUBSESSION (0x%02X)\n", value); break;
                        case 3: printf("PROVISIONED_STS (0x%02X)\n", value); break;
                        case 4: printf("PROVISIONED_STS_RESPONDER_SUBSESSION (0x%02X)\n", value); break;
                        default: printf("UNKNOWN (0x%02X)\n", value); break;
                    }
                }
            } else if (strcasecmp(param_name, "aoa_result_req") == 0 && cfg_len == 1) {
                unsigned char value = payload[offset + 2];
                if (ui_color_enabled) {
                    switch(value) {
                        case 0: printf("%sNO_AOA_REPORT%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, value); break;
                        case 1: printf("%sAOA_ELEVATION%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, value); break;
                        case 2: printf("%sAOA_AZIMUTH%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, value); break;
                        case 3: printf("%sAOA_ELEVATION_AND_AZIMUTH%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, value); break;
                        default: printf("%sUNKNOWN%s (0x%02X)\n", ANSI_COLOR_YELLOW, ANSI_RESET, value); break;
                    }
                } else {
                    switch(value) {
                        case 0: printf("NO_AOA_REPORT (0x%02X)\n", value); break;
                        case 1: printf("AOA_ELEVATION (0x%02X)\n", value); break;
                        case 2: printf("AOA_AZIMUTH (0x%02X)\n", value); break;
                        case 3: printf("AOA_ELEVATION_AND_AZIMUTH (0x%02X)\n", value); break;
                        default: printf("UNKNOWN (0x%02X)\n", value); break;
                    }
                }
            } else if (strcasecmp(param_name, "ranging_round_usage") == 0 && cfg_len == 1) {
                unsigned char value = payload[offset + 2];
                if (ui_color_enabled) {
                    switch(value) {
                        case 0x01: printf("%sSS_TWR_DEFERRED%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, value); break;
                        case 0x02: printf("%sDS_TWR_DEFERRED%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, value); break;
                        case 0x03: printf("%sSS_TWR_NON_DEFERRED%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, value); break;
                        case 0x04: printf("%sDS_TWR_NON_DEFERRED%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, value); break;
                        case 0x05: printf("%sOWR_DL_TDOA%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, value); break;
                        case 0x06: printf("%sOWR_AOA%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, value); break;
                        case 0x07: printf("%sESS_TWR_CONTENTION_BASED%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, value); break;
                        case 0x08: printf("%sADS_TWR_CONTENTION_BASED%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, value); break;
                        default: printf("%sUNKNOWN%s (0x%02X)\n", ANSI_COLOR_YELLOW, ANSI_RESET, value); break;
                    }
                } else {
                    switch(value) {
                        case 0x01: printf("SS_TWR_DEFERRED (0x%02X)\n", value); break;
                        case 0x02: printf("DS_TWR_DEFERRED (0x%02X)\n", value); break;
                        case 0x03: printf("SS_TWR_NON_DEFERRED (0x%02X)\n", value); break;
                        case 0x04: printf("DS_TWR_NON_DEFERRED (0x%02X)\n", value); break;
                        case 0x05: printf("OWR_DL_TDOA (0x%02X)\n", value); break;
                        case 0x06: printf("OWR_AOA (0x%02X)\n", value); break;
                        case 0x07: printf("ESS_TWR_CONTENTION_BASED (0x%02X)\n", value); break;
                        case 0x08: printf("ADS_TWR_CONTENTION_BASED (0x%02X)\n", value); break;
                        default: printf("UNKNOWN (0x%02X)\n", value); break;
                    }
                }
            } else if (strcasecmp(param_name, "mac_fcs_type") == 0 && cfg_len == 1) {
                unsigned char value = payload[offset + 2];
                if (ui_color_enabled) {
                    switch(value) {
                        case 0: printf("%sCRC16%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, value); break;
                        case 1: printf("%sCRC32%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, value); break;
                        default: printf("%sUNKNOWN%s (0x%02X)\n", ANSI_COLOR_YELLOW, ANSI_RESET, value); break;
                    }
                } else {
                    switch(value) {
                        case 0: printf("CRC16 (0x%02X)\n", value); break;
                        case 1: printf("CRC32 (0x%02X)\n", value); break;
                        default: printf("UNKNOWN (0x%02X)\n", value); break;
                    }
                }
            } else if (strcasecmp(param_name, "rframe_config") == 0 && cfg_len == 1) {
                unsigned char value = payload[offset + 2];
                if (ui_color_enabled) {
                    switch(value) {
                        case 0: printf("%sSP0%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, value); break;
                        case 1: printf("%sSP1%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, value); break;
                        case 2: printf("%sSP2%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, value); break;
                        case 3: printf("%sSP3%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, value); break;
                        default: printf("%sUNKNOWN%s (0x%02X)\n", ANSI_COLOR_YELLOW, ANSI_RESET, value); break;
                    }
                } else {
                    switch(value) {
                        case 0: printf("SP0 (0x%02X)\n", value); break;
                        case 1: printf("SP1 (0x%02X)\n", value); break;
                        case 2: printf("SP2 (0x%02X)\n", value); break;
                        case 3: printf("SP3 (0x%02X)\n", value); break;
                        default: printf("UNKNOWN (0x%02X)\n", value); break;
                    }
                }
            } else if (strcasecmp(param_name, "rssi_reporting") == 0 && cfg_len == 1) {
                unsigned char value = payload[offset + 2];
                if (ui_color_enabled) {
                    switch(value) {
                        case 0: printf("%sDISABLED%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, value); break;
                        case 1: printf("%sENABLED%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, value); break;
                        default: printf("%sUNKNOWN%s (0x%02X)\n", ANSI_COLOR_YELLOW, ANSI_RESET, value); break;
                    }
                } else {
                    switch(value) {
                        case 0: printf("DISABLED (0x%02X)\n", value); break;
                        case 1: printf("ENABLED (0x%02X)\n", value); break;
                        default: printf("UNKNOWN (0x%02X)\n", value); break;
                    }
                }
            } else if (strcasecmp(param_name, "link_layer_mode") == 0 && cfg_len == 1) {
                unsigned char value = payload[offset + 2];
                if (ui_color_enabled) {
                    switch(value) {
                        case 0: printf("%sBASIC%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, value); break;
                        case 1: printf("%sEXTENDED%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, value); break;
                        default: printf("%sUNKNOWN%s (0x%02X)\n", ANSI_COLOR_YELLOW, ANSI_RESET, value); break;
                    }
                } else {
                    switch(value) {
                        case 0: printf("BASIC (0x%02X)\n", value); break;
                        case 1: printf("EXTENDED (0x%02X)\n", value); break;
                        default: printf("UNKNOWN (0x%02X)\n", value); break;
                    }
                }
            } else if (strcasecmp(param_name, "tx_adaptive_payload_power") == 0 && cfg_len == 1) {
                unsigned char value = payload[offset + 2];
                if (ui_color_enabled) {
                    switch(value) {
                        case 0: printf("%sDISABLED%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, value); break;
                        case 1: printf("%sENABLED%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, value); break;
                        default: printf("%sUNKNOWN%s (0x%02X)\n", ANSI_COLOR_YELLOW, ANSI_RESET, value); break;
                    }
                } else {
                    switch(value) {
                        case 0: printf("DISABLED (0x%02X)\n", value); break;
                        case 1: printf("ENABLED (0x%02X)\n", value); break;
                        default: printf("UNKNOWN (0x%02X)\n", value); break;
                    }
                }
            } else if (strcasecmp(param_name, "scheduled_mode") == 0 && cfg_len == 1) {
                unsigned char value = payload[offset + 2];
                if (ui_color_enabled) {
                    switch(value) {
                        case 0: printf("%sCONTENTION_BASED%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, value); break;
                        case 1: printf("%sTIME_SCHEDULED%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, value); break;
                        case 2: printf("%sHYBRID%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, value); break;
                        default: printf("%sUNKNOWN%s (0x%02X)\n", ANSI_COLOR_YELLOW, ANSI_RESET, value); break;
                    }
                } else {
                    switch(value) {
                        case 0: printf("CONTENTION_BASED (0x%02X)\n", value); break;
                        case 1: printf("TIME_SCHEDULED (0x%02X)\n", value); break;
                        case 2: printf("HYBRID (0x%02X)\n", value); break;
                        default: printf("UNKNOWN (0x%02X)\n", value); break;
                    }
                }
            } else if (strcasecmp(param_name, "key_rotation") == 0 && cfg_len == 1) {
                unsigned char value = payload[offset + 2];
                if (ui_color_enabled) {
                    switch(value) {
                        case 0: printf("%sDISABLED%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, value); break;
                        case 1: printf("%sENABLED%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, value); break;
                        default: printf("%sUNKNOWN%s (0x%02X)\n", ANSI_COLOR_YELLOW, ANSI_RESET, value); break;
                    }
                } else {
                    switch(value) {
                        case 0: printf("DISABLED (0x%02X)\n", value); break;
                        case 1: printf("ENABLED (0x%02X)\n", value); break;
                        default: printf("UNKNOWN (0x%02X)\n", value); break;
                    }
                }
            } else if (strcasecmp(param_name, "prf_mode") == 0 && cfg_len == 1) {
                unsigned char value = payload[offset + 2];
                if (ui_color_enabled) {
                    switch(value) {
                        case 0: printf("%sBPRF%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, value); break;
                        case 1: printf("%sHPRF_124_8M%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, value); break;
                        case 2: printf("%sHPRF_249_6M%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, value); break;
                        default: printf("%sUNKNOWN%s (0x%02X)\n", ANSI_COLOR_YELLOW, ANSI_RESET, value); break;
                    }
                } else {
                    switch(value) {
                        case 0: printf("BPRF (0x%02X)\n", value); break;
                        case 1: printf("HPRF_124_8M (0x%02X)\n", value); break;
                        case 2: printf("HPRF_249_6M (0x%02X)\n", value); break;
                        default: printf("UNKNOWN (0x%02X)\n", value); break;
                    }
                }
            } else if (strcasecmp(param_name, "hopping_mode") == 0 && cfg_len == 1) {
                unsigned char value = payload[offset + 2];
                if (ui_color_enabled) {
                    switch(value) {
                        case 0: printf("%sDISABLED%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, value); break;
                        case 1: printf("%sENABLED%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, value); break;
                        default: printf("%sUNKNOWN%s (0x%02X)\n", ANSI_COLOR_YELLOW, ANSI_RESET, value); break;
                    }
                } else {
                    switch(value) {
                        case 0: printf("DISABLED (0x%02X)\n", value); break;
                        case 1: printf("ENABLED (0x%02X)\n", value); break;
                        default: printf("UNKNOWN (0x%02X)\n", value); break;
                    }
                }
            } else if (cfg_len <= 8) {
                // For numeric values, try to display as integer
                uint64_t value = 0;
                for (int j = 0; j < cfg_len && j < 8; j++) {
                    value |= ((uint64_t)payload[offset + 2 + j]) << (j * 8);
                }
                
                // Get parameter range information
                uint64_t min_val, max_val;
                if (uci_config_get_app_param_range((AppConfigTlvType)cfg_id, &min_val, &max_val) == 0) {
                    if (ui_color_enabled) {
                        if (value >= min_val && value <= max_val) {
                            printf("%s%" PRIu64 "%s (0x%02X", ANSI_COLOR_BRIGHT_GREEN, value, ANSI_RESET, payload[offset + 2]);
                            for (int j = 1; j < cfg_len; j++) {
                                printf(" %02X", payload[offset + 2 + j]);
                            }
                            printf(") [Range: %" PRIu64 "-%" PRIu64 "]\n", min_val, max_val);
                        } else {
                            printf("%s%" PRIu64 "%s (0x%02X", ANSI_COLOR_YELLOW, value, ANSI_RESET, payload[offset + 2]);
                            for (int j = 1; j < cfg_len; j++) {
                                printf(" %02X", payload[offset + 2 + j]);
                            }
                            printf(") [OUT OF RANGE: %" PRIu64 "-%" PRIu64 "]\n", min_val, max_val);
                        }
                    } else {
                        if (value >= min_val && value <= max_val) {
                            printf("%" PRIu64 " (0x%02X", value, payload[offset + 2]);
                            for (int j = 1; j < cfg_len; j++) {
                                printf(" %02X", payload[offset + 2 + j]);
                            }
                            printf(") [Range: %" PRIu64 "-%" PRIu64 "]\n", min_val, max_val);
                        } else {
                            printf("%" PRIu64 " (0x%02X", value, payload[offset + 2]);
                            for (int j = 1; j < cfg_len; j++) {
                                printf(" %02X", payload[offset + 2 + j]);
                            }
                            printf(") [OUT OF RANGE: %" PRIu64 "-%" PRIu64 "]\n", min_val, max_val);
                        }
                    }
                } else {
                    if (ui_color_enabled) {
                        printf("%s%" PRIu64 "%s (0x%02X", ANSI_COLOR_BRIGHT_WHITE, value, ANSI_RESET, payload[offset + 2]);
                        for (int j = 1; j < cfg_len; j++) {
                            printf(" %02X", payload[offset + 2 + j]);
                        }
                        printf(")\n");
                    } else {
                        printf("%" PRIu64 " (0x%02X", value, payload[offset + 2]);
                        for (int j = 1; j < cfg_len; j++) {
                            printf(" %02X", payload[offset + 2 + j]);
                        }
                        printf(")\n");
                    }
                }
            } else {
                // For longer values, just show hex
                if (ui_color_enabled) {
                    printf("%s", ANSI_COLOR_BRIGHT_WHITE);
                }
                for (int j = 0; j < cfg_len; j++) {
                    printf("%02X ", payload[offset + 2 + j]);
                }
                if (ui_color_enabled) {
                    printf("%s\n", ANSI_RESET);
                } else {
                    printf("\n");
                }
            }
        }

        offset += 2 + cfg_len;
    }
}

void ui_decode_session_get_count_rsp(const unsigned char* payload, int payload_len) {
    if (ui_color_enabled) {
        printf("  %s%sSESSION_GET_COUNT Response:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  SESSION_GET_COUNT Response:\n");
    }

    if (payload_len < 2) {
        if (ui_color_enabled) {
            printf("    %s%sError: Payload too short (%d bytes, need at least 2)%s\n",
                   ANSI_COLOR_RED, ANSI_BOLD, payload_len, ANSI_RESET);
        } else {
            printf("    Error: Payload too short (%d bytes, need at least 2)\n", payload_len);
        }
        return;
    }

    unsigned char status = payload[0];
    unsigned char count = payload[1];

    if (ui_color_enabled) {
        printf("    %s%sStatus:%s 0x%02X", ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, status);
        switch(status) {
            case UCI_STATUS_OK: printf(" %s(OK)%s\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET); break;
            case UCI_STATUS_REJECTED: printf(" %s(REJECTED)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            case UCI_STATUS_FAILED: printf(" %s(FAILED)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            default: printf(" %s(UNKNOWN)%s\n", ANSI_COLOR_YELLOW, ANSI_RESET); break;
        }
        printf("    %s%sSession Count:%s %d\n",
               ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_RESET, count);
    } else {
        printf("    Status: 0x%02X", status);
        switch(status) {
            case UCI_STATUS_OK: printf(" (OK)\n"); break;
            case UCI_STATUS_REJECTED: printf(" (REJECTED)\n"); break;
            case UCI_STATUS_FAILED: printf(" (FAILED)\n"); break;
            default: printf(" (UNKNOWN)\n"); break;
        }
        printf("    Session Count: %d\n", count);
    }
}

void ui_decode_session_get_state_rsp(const unsigned char* payload, int payload_len) {
    if (ui_color_enabled) {
        printf("  %s%sSESSION_GET_STATE Response:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  SESSION_GET_STATE Response:\n");
    }

    if (payload_len < 2) {
        if (ui_color_enabled) {
            printf("    %s%sError: Payload too short (%d bytes, need at least 2)%s\n",
                   ANSI_COLOR_RED, ANSI_BOLD, payload_len, ANSI_RESET);
        } else {
            printf("    Error: Payload too short (%d bytes, need at least 2)\n", payload_len);
        }
        return;
    }

    unsigned char status = payload[0];
    unsigned char state = payload[1];

    if (ui_color_enabled) {
        printf("    %s%sStatus:%s 0x%02X", ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, status);
        switch(status) {
            case UCI_STATUS_OK: printf(" %s(OK)%s\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET); break;
            case UCI_STATUS_REJECTED: printf(" %s(REJECTED)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            case UCI_STATUS_FAILED: printf(" %s(FAILED)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            case UCI_STATUS_SESSION_NOT_EXIST: printf(" %s(SESSION_NOT_EXIST)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            default: printf(" %s(UNKNOWN)%s\n", ANSI_COLOR_YELLOW, ANSI_RESET); break;
        }
        printf("    %s%sSession State:%s 0x%02X", ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_RESET, state);
        switch(state) {
            case SESSION_STATE_INIT: printf(" %s(INIT)%s\n", ANSI_COLOR_CYAN, ANSI_RESET); break;
            case SESSION_STATE_DEINIT: printf(" %s(DEINIT)%s\n", ANSI_COLOR_CYAN, ANSI_RESET); break;
            case SESSION_STATE_ACTIVE: printf(" %s(ACTIVE)%s\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET); break;
            case SESSION_STATE_IDLE: printf(" %s(IDLE)%s\n", ANSI_COLOR_CYAN, ANSI_RESET); break;
            default: printf(" %s(UNKNOWN)%s\n", ANSI_COLOR_YELLOW, ANSI_RESET); break;
        }
    } else {
        printf("    Status: 0x%02X", status);
        switch(status) {
            case UCI_STATUS_OK: printf(" (OK)\n"); break;
            case UCI_STATUS_REJECTED: printf(" (REJECTED)\n"); break;
            case UCI_STATUS_FAILED: printf(" (FAILED)\n"); break;
            case UCI_STATUS_SESSION_NOT_EXIST: printf(" (SESSION_NOT_EXIST)\n"); break;
            default: printf(" (UNKNOWN)\n"); break;
        }
        printf("    Session State: 0x%02X", state);
        switch(state) {
            case SESSION_STATE_INIT: printf(" (INIT)\n"); break;
            case SESSION_STATE_DEINIT: printf(" (DEINIT)\n"); break;
            case SESSION_STATE_ACTIVE: printf(" (ACTIVE)\n"); break;
            case SESSION_STATE_IDLE: printf(" (IDLE)\n"); break;
            default: printf(" (UNKNOWN)\n"); break;
        }
    }
}

void ui_decode_session_update_controller_multicast_list_rsp(const unsigned char* payload, int payload_len) {
    if (ui_color_enabled) {
        printf("  %s%sSESSION_UPDATE_CONTROLLER_MULTICAST_LIST Response:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  SESSION_UPDATE_CONTROLLER_MULTICAST_LIST Response:\n");
    }

    if (payload_len < 2) {
        if (ui_color_enabled) {
            printf("    %s%sError: Payload too short (%d bytes, need at least 2)%s\n",
                   ANSI_COLOR_RED, ANSI_BOLD, payload_len, ANSI_RESET);
        } else {
            printf("    Error: Payload too short (%d bytes, need at least 2)\n", payload_len);
        }
        return;
    }

    unsigned char status = payload[0];
    unsigned char processed = payload[1];
    int offset = 2;

    if (ui_color_enabled) {
        printf("    %s%sStatus:%s 0x%02X", ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, status);
        switch (status) {
            case UCI_STATUS_OK: printf(" %s(OK)%s\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET); break;
            case UCI_STATUS_REJECTED: printf(" %s(REJECTED)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            case UCI_STATUS_FAILED: printf(" %s(FAILED)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            case UCI_STATUS_INVALID_PARAM: printf(" %s(INVALID_PARAM)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            default: printf(" %s(UNKNOWN)%s\n", ANSI_COLOR_YELLOW, ANSI_RESET); break;
        }
        if (status != UCI_STATUS_OK) {
            enhanced_error_analysis(status);
        }
        printf("    %s%sEntries Processed:%s %u\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_RESET, processed);
    } else {
        printf("    Status: 0x%02X", status);
        switch (status) {
            case UCI_STATUS_OK: printf(" (OK)\n"); break;
            case UCI_STATUS_REJECTED: printf(" (REJECTED)\n"); break;
            case UCI_STATUS_FAILED: printf(" (FAILED)\n"); break;
            case UCI_STATUS_INVALID_PARAM: printf(" (INVALID_PARAM)\n"); break;
            default: printf(" (UNKNOWN)\n"); break;
        }
        if (status != UCI_STATUS_OK) {
            enhanced_error_analysis(status);
        }
        printf("    Entries Processed: %u\n", processed);
    }

    for (unsigned char i = 0; i < processed; i++) {
        if (offset + 7 > payload_len) {
            if (ui_color_enabled) {
                printf("    %s%sError: Truncated entry %u in response%s\n",
                       ANSI_COLOR_RED, ANSI_BOLD, i, ANSI_RESET);
            } else {
                printf("    Error: Truncated entry %u in response\n", i);
            }
            return;
        }

        unsigned short short_address = ui_read_u16_le(&payload[offset]);
        offset += 2;
        unsigned int subsession_id = ui_read_u32_le(&payload[offset]);
        offset += 4;
        unsigned char entry_status = payload[offset++];

        if (ui_color_enabled) {
            printf("    %sEntry[%u]:%s Short=0x%04X, Subsession=0x%08X, Status=0x%02X",
                   ANSI_COLOR_BRIGHT_CYAN, i, ANSI_RESET, short_address, subsession_id, entry_status);
            switch (entry_status) {
                case UCI_STATUS_OK: printf(" %s(OK)%s\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET); break;
                case UCI_STATUS_INVALID_PARAM: printf(" %s(INVALID_PARAM)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
                case UCI_STATUS_MULTICAST_LIST_FULL: printf(" %s(LIST_FULL)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
                case UCI_STATUS_ADDRESS_ALREADY_PRESENT: printf(" %s(ALREADY_PRESENT)%s\n", ANSI_COLOR_YELLOW, ANSI_RESET); break;
                case UCI_STATUS_ADDRESS_NOT_FOUND: printf(" %s(NOT_FOUND)%s\n", ANSI_COLOR_YELLOW, ANSI_RESET); break;
                default: printf(" %s(UNKNOWN)%s\n", ANSI_COLOR_YELLOW, ANSI_RESET); break;
            }
        } else {
            printf("    Entry[%u]: Short=0x%04X, Subsession=0x%08X, Status=0x%02X",
                   i, short_address, subsession_id, entry_status);
            switch (entry_status) {
                case UCI_STATUS_OK: printf(" (OK)\n"); break;
                case UCI_STATUS_INVALID_PARAM: printf(" (INVALID_PARAM)\n"); break;
                case UCI_STATUS_MULTICAST_LIST_FULL: printf(" (LIST_FULL)\n"); break;
                case UCI_STATUS_ADDRESS_ALREADY_PRESENT: printf(" (ALREADY_PRESENT)\n"); break;
                case UCI_STATUS_ADDRESS_NOT_FOUND: printf(" (NOT_FOUND)\n"); break;
                default: printf(" (UNKNOWN)\n"); break;
            }
        }
    }
}

void ui_decode_session_update_active_rounds_dt_tag_rsp(const unsigned char* payload, int payload_len) {
    if (ui_color_enabled) {
        printf("  %s%sSESSION_UPDATE_ACTIVE_ROUNDS_DT_TAG Response:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  SESSION_UPDATE_ACTIVE_ROUNDS_DT_TAG Response:\n");
    }

    if (payload_len < 2) {
        if (ui_color_enabled) {
            printf("    %s%sError: Payload too short (%d bytes, need at least 2)%s\n",
                   ANSI_COLOR_RED, ANSI_BOLD, payload_len, ANSI_RESET);
        } else {
            printf("    Error: Payload too short (%d bytes, need at least 2)\n", payload_len);
        }
        return;
    }

    unsigned char status = payload[0];
    unsigned char stored_count = payload[1];

    if (ui_color_enabled) {
        printf("    %s%sStatus:%s 0x%02X", ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, status);
        switch (status) {
            case UCI_STATUS_OK: printf(" %s(OK)%s\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET); break;
            case UCI_STATUS_INVALID_PARAM: printf(" %s(INVALID_PARAM)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            case UCI_STATUS_INVALID_MSG_SIZE: printf(" %s(INVALID_MSG_SIZE)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            default: printf(" %s(UNKNOWN)%s\n", ANSI_COLOR_YELLOW, ANSI_RESET); break;
        }
        if (status != UCI_STATUS_OK) {
            enhanced_error_analysis(status);
        }
        printf("    %s%sStored Round Indices:%s %u\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_RESET, stored_count);
    } else {
        printf("    Status: 0x%02X", status);
        switch (status) {
            case UCI_STATUS_OK: printf(" (OK)\n"); break;
            case UCI_STATUS_INVALID_PARAM: printf(" (INVALID_PARAM)\n"); break;
            case UCI_STATUS_INVALID_MSG_SIZE: printf(" (INVALID_MSG_SIZE)\n"); break;
            default: printf(" (UNKNOWN)\n"); break;
        }
        if (status != UCI_STATUS_OK) {
            enhanced_error_analysis(status);
        }
        printf("    Stored Round Indices: %u\n", stored_count);
    }

    if (stored_count == 0) {
        return;
    }

    if (payload_len < 2 + stored_count) {
        if (ui_color_enabled) {
            printf("    %s%sError: Declared %u indices but payload shorter than expected%s\n",
                   ANSI_COLOR_RED, ANSI_BOLD, stored_count, ANSI_RESET);
        } else {
            printf("    Error: Declared %u indices but payload shorter than expected\n", stored_count);
        }
        return;
    }

    if (ui_color_enabled) {
        printf("    %sRound Indices:%s ", ANSI_COLOR_BRIGHT_CYAN, ANSI_RESET);
    } else {
        printf("    Round Indices: ");
    }

    for (unsigned char i = 0; i < stored_count; i++) {
        unsigned char value = payload[2 + i];
        if (ui_color_enabled) {
            printf("%s0x%02X%s", ANSI_COLOR_BRIGHT_GREEN, value, ANSI_RESET);
        } else {
            printf("0x%02X", value);
        }
        if (i + 1 < stored_count) {
            printf(" ");
        }
    }
    printf("\n");
}

void ui_decode_session_data_transfer_phase_config_rsp(const unsigned char* payload, int payload_len) {
    if (ui_color_enabled) {
        printf("  %s%sSESSION_DATA_TRANSFER_PHASE_CONFIG Response:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  SESSION_DATA_TRANSFER_PHASE_CONFIG Response:\n");
    }

    if (payload_len < 1) {
        if (ui_color_enabled) {
            printf("    %s%sError: Payload too short (%d bytes, need at least 1)%s\n",
                   ANSI_COLOR_RED, ANSI_BOLD, payload_len, ANSI_RESET);
        } else {
            printf("    Error: Payload too short (%d bytes, need at least 1)\n", payload_len);
        }
        return;
    }

    unsigned char status = payload[0];

    if (ui_color_enabled) {
        printf("    %s%sStatus:%s 0x%02X", ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, status);
        switch (status) {
            case UCI_STATUS_OK: printf(" %s(OK)%s\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET); break;
            case UCI_STATUS_INVALID_PARAM: printf(" %s(INVALID_PARAM)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            case UCI_STATUS_INVALID_MSG_SIZE: printf(" %s(INVALID_MSG_SIZE)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            default: printf(" %s(UNKNOWN)%s\n", ANSI_COLOR_YELLOW, ANSI_RESET); break;
        }
    } else {
        printf("    Status: 0x%02X", status);
        switch (status) {
            case UCI_STATUS_OK: printf(" (OK)\n"); break;
            case UCI_STATUS_INVALID_PARAM: printf(" (INVALID_PARAM)\n"); break;
            case UCI_STATUS_INVALID_MSG_SIZE: printf(" (INVALID_MSG_SIZE)\n"); break;
            default: printf(" (UNKNOWN)\n"); break;
        }
    }

    if (status != UCI_STATUS_OK) {
        enhanced_error_analysis(status);
    }
}

void ui_decode_session_query_data_size_in_ranging_rsp(const unsigned char* payload, int payload_len) {
    if (ui_color_enabled) {
        printf("  %s%sSESSION_QUERY_DATA_SIZE_IN_RANGING Response:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  SESSION_QUERY_DATA_SIZE_IN_RANGING Response:\n");
    }

    if (payload_len < 3) {  // Need at least 3 bytes: status(1) + max_data_size(2)
        if (ui_color_enabled) {
            printf("    %s%sError: Payload too short (%d bytes, need at least 3)%s\n",
                   ANSI_COLOR_RED, ANSI_BOLD, payload_len, ANSI_RESET);
        } else {
            printf("    Error: Payload too short (%d bytes, need at least 3)\n", payload_len);
        }
        return;
    }

    // Extract fields
    unsigned char status = payload[0];
    unsigned short max_data_size = (unsigned short)payload[1] |
                                  ((unsigned short)payload[2] << 8);

    // Display decoded information
    if (ui_color_enabled) {
        printf("    %s%sStatus:%s 0x%02X", ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, status);
        switch(status) {
            case UCI_STATUS_OK: printf(" %s(OK)%s\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET); break;
            case UCI_STATUS_REJECTED: printf(" %s(REJECTED)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            case UCI_STATUS_FAILED: printf(" %s(FAILED)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            default: printf(" %s(UNKNOWN)%s\n", ANSI_COLOR_YELLOW, ANSI_RESET); break;
        }
        printf("    %s%sMax Data Size:%s %u bytes\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_RESET, max_data_size);
    } else {
        printf("    Status: 0x%02X", status);
        switch(status) {
            case UCI_STATUS_OK: printf(" (OK)\n"); break;
            case UCI_STATUS_REJECTED: printf(" (REJECTED)\n"); break;
            case UCI_STATUS_FAILED: printf(" (FAILED)\n"); break;
            default: printf(" (UNKNOWN)\n"); break;
        }
        printf("    Max Data Size: %u bytes\n", max_data_size);
    }
}

void ui_decode_session_set_hybrid_controller_config_rsp(const unsigned char* payload, int payload_len) {
    if (ui_color_enabled) {
        printf("  %s%sSESSION_SET_HYBRID_CONTROLLER_CONFIG Response:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  SESSION_SET_HYBRID_CONTROLLER_CONFIG Response:\n");
    }

    if (payload_len < 1) {  // Need at least 1 byte for status
        if (ui_color_enabled) {
            printf("    %s%sError: Payload too short (%d bytes, need at least 1)%s\n",
                   ANSI_COLOR_RED, ANSI_BOLD, payload_len, ANSI_RESET);
        } else {
            printf("    Error: Payload too short (%d bytes, need at least 1)\n", payload_len);
        }
        return;
    }

    // Extract status
    unsigned char status = payload[0];

    // Display decoded information
    if (ui_color_enabled) {
        printf("    %s%sStatus:%s 0x%02X", ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, status);
        switch(status) {
            case UCI_STATUS_OK: printf(" %s(OK)%s\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET); break;
            case UCI_STATUS_REJECTED: printf(" %s(REJECTED)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            case UCI_STATUS_FAILED: printf(" %s(FAILED)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            default: printf(" %s(UNKNOWN)%s\n", ANSI_COLOR_YELLOW, ANSI_RESET); break;
        }
    } else {
        printf("    Status: 0x%02X", status);
        switch(status) {
            case UCI_STATUS_OK: printf(" (OK)\n"); break;
            case UCI_STATUS_REJECTED: printf(" (REJECTED)\n"); break;
            case UCI_STATUS_FAILED: printf(" (FAILED)\n"); break;
            default: printf(" (UNKNOWN)\n"); break;
        }
    }
}

void ui_decode_session_set_hybrid_controlee_config_rsp(const unsigned char* payload, int payload_len) {
    if (ui_color_enabled) {
        printf("  %s%sSESSION_SET_HYBRID_CONTROLEE_CONFIG Response:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  SESSION_SET_HYBRID_CONTROLEE_CONFIG Response:\n");
    }

    if (payload_len < 1) {  // Need at least 1 byte for status
        if (ui_color_enabled) {
            printf("    %s%sError: Payload too short (%d bytes, need at least 1)%s\n",
                   ANSI_COLOR_RED, ANSI_BOLD, payload_len, ANSI_RESET);
        } else {
            printf("    Error: Payload too short (%d bytes, need at least 1)\n", payload_len);
        }
        return;
    }

    // Extract status
    unsigned char status = payload[0];

    // Display decoded information
    if (ui_color_enabled) {
        printf("    %s%sStatus:%s 0x%02X", ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, status);
        switch(status) {
            case UCI_STATUS_OK: printf(" %s(OK)%s\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET); break;
            case UCI_STATUS_REJECTED: printf(" %s(REJECTED)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            case UCI_STATUS_FAILED: printf(" %s(FAILED)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            default: printf(" %s(UNKNOWN)%s\n", ANSI_COLOR_YELLOW, ANSI_RESET); break;
        }
    } else {
        printf("    Status: 0x%02X", status);
        switch(status) {
            case UCI_STATUS_OK: printf(" (OK)\n"); break;
            case UCI_STATUS_REJECTED: printf(" (REJECTED)\n"); break;
            case UCI_STATUS_FAILED: printf(" (FAILED)\n"); break;
            default: printf(" (UNKNOWN)\n"); break;
        }
    }
}

void ui_decode_session_logical_link_create_rsp(const unsigned char* payload, int payload_len) {
    if (ui_color_enabled) {
        printf("  %s%sSESSION_LOGICAL_LINK_CREATE Response:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  SESSION_LOGICAL_LINK_CREATE Response:\n");
    }

    if (payload_len < 3) {
        if (ui_color_enabled) {
            printf("    %s%sError: Payload too short (%d bytes, need at least 3)%s\n",
                   ANSI_COLOR_RED, ANSI_BOLD, payload_len, ANSI_RESET);
        } else {
            printf("    Error: Payload too short (%d bytes, need at least 3)\n", payload_len);
        }
        return;
    }

    unsigned char status = payload[0];
    unsigned char link_id = payload[1];
    unsigned char credit = payload[2];

    if (ui_color_enabled) {
        printf("    %s%sStatus:%s 0x%02X", ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, status);
        switch (status) {
            case UCI_STATUS_OK: printf(" %s(OK)%s\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET); break;
            case UCI_STATUS_MULTICAST_LIST_FULL: printf(" %s(NO_SLOTS)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            case UCI_STATUS_INVALID_PARAM: printf(" %s(INVALID_PARAM)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            default: printf(" %s(UNKNOWN)%s\n", ANSI_COLOR_YELLOW, ANSI_RESET); break;
        }
        printf("    %s%sLogical Link ID:%s 0x%02X\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_RESET, link_id);
        printf("    %s%sInitial Credit:%s %u\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_RESET, credit);
    } else {
        printf("    Status: 0x%02X", status);
        switch (status) {
            case UCI_STATUS_OK: printf(" (OK)\n"); break;
            case UCI_STATUS_MULTICAST_LIST_FULL: printf(" (NO_SLOTS)\n"); break;
            case UCI_STATUS_INVALID_PARAM: printf(" (INVALID_PARAM)\n"); break;
            default: printf(" (UNKNOWN)\n"); break;
        }
        printf("    Logical Link ID: 0x%02X\n", link_id);
        printf("    Initial Credit: %u\n", credit);
    }
}

void ui_decode_session_logical_link_close_rsp(const unsigned char* payload, int payload_len) {
    if (ui_color_enabled) {
        printf("  %s%sSESSION_LOGICAL_LINK_CLOSE Response:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  SESSION_LOGICAL_LINK_CLOSE Response:\n");
    }

    if (payload_len < 2) {
        if (ui_color_enabled) {
            printf("    %s%sError: Payload too short (%d bytes, need at least 2)%s\n",
                   ANSI_COLOR_RED, ANSI_BOLD, payload_len, ANSI_RESET);
        } else {
            printf("    Error: Payload too short (%d bytes, need at least 2)\n", payload_len);
        }
        return;
    }

    unsigned char status = payload[0];
    unsigned char link_id = payload[1];

    if (ui_color_enabled) {
        printf("    %s%sStatus:%s 0x%02X", ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, status);
        switch (status) {
            case UCI_STATUS_OK: printf(" %s(OK)%s\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET); break;
            case UCI_STATUS_INVALID_PARAM: printf(" %s(INVALID_PARAM)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            default: printf(" %s(UNKNOWN)%s\n", ANSI_COLOR_YELLOW, ANSI_RESET); break;
        }
        printf("    %s%sLogical Link ID:%s 0x%02X\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_RESET, link_id);
    } else {
        printf("    Status: 0x%02X", status);
        switch (status) {
            case UCI_STATUS_OK: printf(" (OK)\n"); break;
            case UCI_STATUS_INVALID_PARAM: printf(" (INVALID_PARAM)\n"); break;
            default: printf(" (UNKNOWN)\n"); break;
        }
        printf("    Logical Link ID: 0x%02X\n", link_id);
    }
}

void ui_decode_session_logical_link_get_param_rsp(const unsigned char* payload, int payload_len) {
    if (ui_color_enabled) {
        printf("  %s%sSESSION_LOGICAL_LINK_GET_PARAM Response:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  SESSION_LOGICAL_LINK_GET_PARAM Response:\n");
    }

    if (payload_len < 4) {
        if (ui_color_enabled) {
            printf("    %s%sError: Payload too short (%d bytes, need at least 4)%s\n",
                   ANSI_COLOR_RED, ANSI_BOLD, payload_len, ANSI_RESET);
        } else {
            printf("    Error: Payload too short (%d bytes, need at least 4)\n", payload_len);
        }
        return;
    }

    unsigned char status = payload[0];
    unsigned char link_id = payload[1];
    unsigned char mode = payload[2];
    unsigned char credit = payload[3];

    if (ui_color_enabled) {
        printf("    %s%sStatus:%s 0x%02X", ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, status);
        switch (status) {
            case UCI_STATUS_OK: printf(" %s(OK)%s\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET); break;
            case UCI_STATUS_INVALID_PARAM: printf(" %s(INVALID_PARAM)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            default: printf(" %s(UNKNOWN)%s\n", ANSI_COLOR_YELLOW, ANSI_RESET); break;
        }
        printf("    %s%sLogical Link ID:%s 0x%02X\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_RESET, link_id);
        printf("    %s%sMode:%s 0x%02X\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_RESET, mode);
        printf("    %s%sCredits:%s %u\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_RESET, credit);
    } else {
        printf("    Status: 0x%02X", status);
        switch (status) {
            case UCI_STATUS_OK: printf(" (OK)\n"); break;
            case UCI_STATUS_INVALID_PARAM: printf(" (INVALID_PARAM)\n"); break;
            default: printf(" (UNKNOWN)\n"); break;
        }
        printf("    Logical Link ID: 0x%02X\n", link_id);
        printf("    Mode: 0x%02X\n", mode);
        printf("    Credits: %u\n", credit);
    }
}

void ui_decode_session_start_rsp(const unsigned char* payload, int payload_len) {
    if (ui_color_enabled) {
        printf("  %s%sSESSION_START Response:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  SESSION_START Response:\n");
    }

    if (payload_len < 1) {
        if (ui_color_enabled) {
            printf("    %s%sError: Payload too short (%d bytes, need at least 1)%s\n",
                   ANSI_COLOR_RED, ANSI_BOLD, payload_len, ANSI_RESET);
        } else {
            printf("    Error: Payload too short (%d bytes, need at least 1)\n", payload_len);
        }
        return;
    }

    unsigned char status = payload[0];

    if (ui_color_enabled) {
        printf("    %s%sStatus:%s 0x%02X", ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, status);
        switch(status) {
            case UCI_STATUS_OK: printf(" %s(OK)%s\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET); break;
            case UCI_STATUS_REJECTED: printf(" %s(REJECTED)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            case UCI_STATUS_FAILED: printf(" %s(FAILED)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            case UCI_STATUS_INVALID_PARAM: printf(" %s(INVALID_PARAM)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            case UCI_STATUS_SESSION_NOT_EXIST: printf(" %s(SESSION_NOT_EXIST)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            case UCI_STATUS_SESSION_ACTIVE: printf(" %s(SESSION_ACTIVE)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            default: printf(" %s(UNKNOWN)%s\n", ANSI_COLOR_YELLOW, ANSI_RESET); break;
        }
    } else {
        printf("    Status: 0x%02X", status);
        switch(status) {
            case UCI_STATUS_OK: printf(" (OK)\n"); break;
            case UCI_STATUS_REJECTED: printf(" (REJECTED)\n"); break;
            case UCI_STATUS_FAILED: printf(" (FAILED)\n"); break;
            case UCI_STATUS_INVALID_PARAM: printf(" (INVALID_PARAM)\n"); break;
            case UCI_STATUS_SESSION_NOT_EXIST: printf(" (SESSION_NOT_EXIST)\n"); break;
            case UCI_STATUS_SESSION_ACTIVE: printf(" (SESSION_ACTIVE)\n"); break;
            default: printf(" (UNKNOWN)\n"); break;
        }
    }
}

void ui_decode_session_stop_rsp(const unsigned char* payload, int payload_len) {
    if (ui_color_enabled) {
        printf("  %s%sSESSION_STOP Response:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  SESSION_STOP Response:\n");
    }

    if (payload_len < 1) {
        if (ui_color_enabled) {
            printf("    %s%sError: Payload too short (%d bytes, need at least 1)%s\n",
                   ANSI_COLOR_RED, ANSI_BOLD, payload_len, ANSI_RESET);
        } else {
            printf("    Error: Payload too short (%d bytes, need at least 1)\n", payload_len);
        }
        return;
    }

    unsigned char status = payload[0];

    if (ui_color_enabled) {
        printf("    %s%sStatus:%s 0x%02X", ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, status);
        switch(status) {
            case UCI_STATUS_OK: printf(" %s(OK)%s\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET); break;
            case UCI_STATUS_REJECTED: printf(" %s(REJECTED)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            case UCI_STATUS_FAILED: printf(" %s(FAILED)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            case UCI_STATUS_INVALID_PARAM: printf(" %s(INVALID_PARAM)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            case UCI_STATUS_SESSION_NOT_EXIST: printf(" %s(SESSION_NOT_EXIST)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            default: printf(" %s(UNKNOWN)%s\n", ANSI_COLOR_YELLOW, ANSI_RESET); break;
        }
    } else {
        printf("    Status: 0x%02X", status);
        switch(status) {
            case UCI_STATUS_OK: printf(" (OK)\n"); break;
            case UCI_STATUS_REJECTED: printf(" (REJECTED)\n"); break;
            case UCI_STATUS_FAILED: printf(" (FAILED)\n"); break;
            case UCI_STATUS_INVALID_PARAM: printf(" (INVALID_PARAM)\n"); break;
            case UCI_STATUS_SESSION_NOT_EXIST: printf(" (SESSION_NOT_EXIST)\n"); break;
            default: printf(" (UNKNOWN)\n"); break;
        }
    }
}

void ui_decode_session_get_ranging_count_rsp(const unsigned char* payload, int payload_len) {
    if (ui_color_enabled) {
        printf("  %s%sSESSION_GET_RANGING_COUNT Response:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  SESSION_GET_RANGING_COUNT Response:\n");
    }

    if (payload_len < 5) {
        if (ui_color_enabled) {
            printf("    %s%sError: Payload too short (%d bytes, need at least 5)%s\n",
                   ANSI_COLOR_RED, ANSI_BOLD, payload_len, ANSI_RESET);
        } else {
            printf("    Error: Payload too short (%d bytes, need at least 5)\n", payload_len);
        }
        return;
    }

    unsigned char status = payload[0];
    unsigned int count = ui_read_u32_le(payload + 1);

    if (ui_color_enabled) {
        printf("    %s%sStatus:%s 0x%02X", ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, status);
        switch(status) {
            case UCI_STATUS_OK: printf(" %s(OK)%s\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET); break;
            case UCI_STATUS_REJECTED: printf(" %s(REJECTED)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            case UCI_STATUS_FAILED: printf(" %s(FAILED)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            case UCI_STATUS_INVALID_PARAM: printf(" %s(INVALID_PARAM)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            case UCI_STATUS_SESSION_NOT_EXIST: printf(" %s(SESSION_NOT_EXIST)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            default: printf(" %s(UNKNOWN)%s\n", ANSI_COLOR_YELLOW, ANSI_RESET); break;
        }
        printf("    %s%sRanging Count:%s %u\n",
               ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_RESET, count);
    } else {
        printf("    Status: 0x%02X", status);
        switch(status) {
            case UCI_STATUS_OK: printf(" (OK)\n"); break;
            case UCI_STATUS_REJECTED: printf(" (REJECTED)\n"); break;
            case UCI_STATUS_FAILED: printf(" (FAILED)\n"); break;
            case UCI_STATUS_INVALID_PARAM: printf(" (INVALID_PARAM)\n"); break;
            case UCI_STATUS_SESSION_NOT_EXIST: printf(" (SESSION_NOT_EXIST)\n"); break;
            default: printf(" (UNKNOWN)\n"); break;
        }
        printf("    Ranging Count: %u\n", count);
    }
}

void ui_decode_session_status_ntf(const unsigned char* payload, int payload_len) {
    if (ui_color_enabled) {
        printf("  %s%sSESSION_STATUS_NTF:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  SESSION_STATUS_NTF:\n");
    }

    if (payload_len < 6) {
        if (ui_color_enabled) {
            printf("    %s%sError: Payload too short (%d bytes, need at least 6)%s\n",
                   ANSI_COLOR_RED, ANSI_BOLD, payload_len, ANSI_RESET);
        } else {
            printf("    Error: Payload too short (%d bytes, need at least 6)\n", payload_len);
        }
        return;
    }

    uint32_t session_token = ui_read_u32_le(payload);
    unsigned char session_state = payload[4];
    unsigned char reason_code = payload[5];

    const uci_lookup_entry_t* state_entry = uci_lookup_session_state(session_state);
    const uci_lookup_entry_t* reason_entry = uci_lookup_session_reason(reason_code);

    const char* state_color = ANSI_COLOR_YELLOW;
    switch (session_state) {
        case SESSION_STATE_INIT:
            state_color = ANSI_COLOR_BRIGHT_BLUE;
            break;
        case SESSION_STATE_DEINIT:
            state_color = ANSI_COLOR_YELLOW;
            break;
        case SESSION_STATE_ACTIVE:
            state_color = ANSI_COLOR_BRIGHT_GREEN;
            break;
        case SESSION_STATE_IDLE:
            state_color = ANSI_COLOR_BRIGHT_CYAN;
            break;
        default:
            state_color = ANSI_COLOR_RED;
            break;
    }

    if (ui_color_enabled) {
        printf("    %s%sSession Token:%s 0x%08X\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_RESET, session_token);
    } else {
        printf("    Session Token: 0x%08X\n", session_token);
    }

    print_lookup_line("Session State", session_state, state_entry, state_color);
    print_lookup_line("Reason", reason_code, reason_entry, ANSI_COLOR_BRIGHT_CYAN);

    print_additional_payload_bytes(payload, payload_len, 6);
}

void ui_decode_session_data_credit_ntf(const unsigned char* payload, int payload_len) {
    if (ui_color_enabled) {
        printf("  %s%sSESSION_DATA_CREDIT_NTF:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  SESSION_DATA_CREDIT_NTF:\n");
    }

    if (payload_len < 5) {
        if (ui_color_enabled) {
            printf("    %s%sError: Payload too short (%d bytes, need at least 5)%s\n",
                   ANSI_COLOR_RED, ANSI_BOLD, payload_len, ANSI_RESET);
        } else {
            printf("    Error: Payload too short (%d bytes, need at least 5)\n", payload_len);
        }
        return;
    }

    uint32_t session_token = ui_read_u32_le(payload);
    unsigned char credit_flag = payload[4];
    const char* availability_text = (credit_flag != 0) ? "Credits Available" : "No Credits Available";

    if (ui_color_enabled) {
        printf("    %s%sSession Token:%s 0x%08X\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_RESET, session_token);
        printf("    %s%sCredit Availability:%s 0x%02X %s(%s)%s\n",
               ANSI_COLOR_BRIGHT_YELLOW,
               ANSI_BOLD,
               ANSI_RESET,
               credit_flag,
               credit_flag != 0 ? ANSI_COLOR_BRIGHT_GREEN : ANSI_COLOR_RED,
               availability_text,
               ANSI_RESET);
    } else {
        printf("    Session Token: 0x%08X\n", session_token);
        printf("    Credit Availability: 0x%02X (%s)\n", credit_flag, availability_text);
    }

    print_additional_payload_bytes(payload, payload_len, 5);
}

void ui_decode_session_data_transfer_status_ntf(const unsigned char* payload, int payload_len) {
    if (ui_color_enabled) {
        printf("  %s%sSESSION_DATA_TRANSFER_STATUS_NTF:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  SESSION_DATA_TRANSFER_STATUS_NTF:\n");
    }

    if (payload_len < 8) {
        if (ui_color_enabled) {
            printf("    %s%sError: Payload too short (%d bytes, need at least 8)%s\n",
                   ANSI_COLOR_RED, ANSI_BOLD, payload_len, ANSI_RESET);
        } else {
            printf("    Error: Payload too short (%d bytes, need at least 8)\n", payload_len);
        }
        return;
    }

    uint32_t session_token = ui_read_u32_le(payload);
    uint16_t sequence_number = ui_read_u16_le(&payload[4]);
    unsigned char status = payload[6];
    unsigned char tx_count = payload[7];

    const uci_lookup_entry_t* status_entry = uci_lookup_data_transfer_status(status);

    const char* status_color = (status == UCI_DATA_TRANSFER_STATUS_REPETITION_OK ||
                                status == UCI_DATA_TRANSFER_STATUS_OK)
                                   ? ANSI_COLOR_BRIGHT_GREEN
                                   : ANSI_COLOR_RED;

    if (ui_color_enabled) {
        printf("    %s%sSession Token:%s 0x%08X\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_RESET, session_token);
        printf("    %s%sUCI Sequence Number:%s %u\n", ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, sequence_number);
    } else {
        printf("    Session Token: 0x%08X\n", session_token);
        printf("    UCI Sequence Number: %u\n", sequence_number);
    }

    print_lookup_line("Transfer Status", status, status_entry, status_color);

    if (ui_color_enabled) {
        printf("    %s%sTX Attempt Count:%s %u\n", ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, tx_count);
    } else {
        printf("    TX Attempt Count: %u\n", tx_count);
    }

    print_additional_payload_bytes(payload, payload_len, 8);
}

void ui_decode_session_logical_link_uwbs_create_ntf(const unsigned char* payload, int payload_len) {
    if (ui_color_enabled) {
        printf("  %s%sSESSION_LOGICAL_LINK_UWBS_CREATE_NTF:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  SESSION_LOGICAL_LINK_UWBS_CREATE_NTF:\n");
    }

    if (payload_len < 6) {
        if (ui_color_enabled) {
            printf("    %s%sError: Payload too short (%d bytes, need at least 6)%s\n",
                   ANSI_COLOR_RED, ANSI_BOLD, payload_len, ANSI_RESET);
        } else {
            printf("    Error: Payload too short (%d bytes, need at least 6)\n", payload_len);
        }
        return;
    }

    uint32_t session_handle = ui_read_u32_le(payload);
    unsigned char link_id = payload[4];
    unsigned char credit = payload[5];

    if (ui_color_enabled) {
        printf("    %s%sSession Handle:%s 0x%08X\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_RESET, session_handle);
        printf("    %s%sLogical Link ID:%s 0x%02X\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_RESET, link_id);
        printf("    %s%sCredits:%s %u\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_RESET, credit);
    } else {
        printf("    Session Handle: 0x%08X\n", session_handle);
        printf("    Logical Link ID: 0x%02X\n", link_id);
        printf("    Credits: %u\n", credit);
    }
}

void ui_decode_session_logical_link_uwbs_close_ntf(const unsigned char* payload, int payload_len) {
    if (ui_color_enabled) {
        printf("  %s%sSESSION_LOGICAL_LINK_UWBS_CLOSE_NTF:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  SESSION_LOGICAL_LINK_UWBS_CLOSE_NTF:\n");
    }

    if (payload_len < 6) {
        if (ui_color_enabled) {
            printf("    %s%sError: Payload too short (%d bytes, need at least 6)%s\n",
                   ANSI_COLOR_RED, ANSI_BOLD, payload_len, ANSI_RESET);
        } else {
            printf("    Error: Payload too short (%d bytes, need at least 6)\n", payload_len);
        }
        return;
    }

    uint32_t session_handle = ui_read_u32_le(payload);
    unsigned char link_id = payload[4];
    unsigned char reason = payload[5];

    if (ui_color_enabled) {
        printf("    %s%sSession Handle:%s 0x%08X\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_RESET, session_handle);
        printf("    %s%sLogical Link ID:%s 0x%02X\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_RESET, link_id);
        printf("    %s%sReason:%s 0x%02X\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_RESET, reason);
    } else {
        printf("    Session Handle: 0x%08X\n", session_handle);
        printf("    Logical Link ID: 0x%02X\n", link_id);
        printf("    Reason: 0x%02X\n", reason);
    }
}
