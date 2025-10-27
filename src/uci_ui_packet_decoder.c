#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/uci.h"
#include "../include/uci_ui.h"
#include "../include/uci_ui_packet_decoder.h"
#include "../include/uci_packet_analyzer.h"

// Function declaration for enhanced error analysis from packet_analyzer.c
void enhanced_error_analysis(unsigned char status_code);

// Enhanced UI version of analyze_uci_packet - now just calls unified analyzer
void ui_analyze_uci_packet(unsigned char* packet, size_t packet_len) {
    // Call unified analyzer which respects ui_color_enabled
    uci_analyze_packet_core(packet, packet_len);
}


// Enhanced packet decoding functions with UI enhancements

void ui_decode_core_device_info_rsp(unsigned char* payload, int payload_len) {
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

void ui_decode_core_get_caps_info_rsp(unsigned char* payload, int payload_len) {
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

void ui_decode_core_set_config_rsp(unsigned char* payload, int payload_len) {
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
            offset += 2;

            if (ui_color_enabled) {
                printf("    %s%sConfig %d:%s\n", ANSI_COLOR_BRIGHT_CYAN, ANSI_BOLD, i, ANSI_RESET);
                printf("      %s%sConfig ID:%s 0x%02X", ANSI_COLOR_YELLOW, ANSI_BOLD, ANSI_RESET, cfg_id);
                switch(cfg_id) {
                    case DEVICE_STATE: printf(" %s(DEVICE_STATE)%s\n", ANSI_COLOR_CYAN, ANSI_RESET); break;
                    case LOW_POWER_MODE: printf(" %s(LOW_POWER_MODE)%s\n", ANSI_COLOR_CYAN, ANSI_RESET); break;
                    default: printf(" %s(UNKNOWN)%s\n", ANSI_COLOR_YELLOW, ANSI_RESET); break;
                }
                printf("      %s%sStatus:%s 0x%02X", ANSI_COLOR_YELLOW, ANSI_BOLD, ANSI_RESET, cfg_status);
                switch(cfg_status) {
                    case UCI_STATUS_OK: printf(" %s(OK)%s\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET); break;
                    case UCI_STATUS_REJECTED: printf(" %s(REJECTED)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
                    case UCI_STATUS_FAILED: printf(" %s(FAILED)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
                    case UCI_STATUS_INVALID_PARAM: printf(" %s(INVALID_PARAM)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
                    default: printf(" %s(UNKNOWN)%s\n", ANSI_COLOR_YELLOW, ANSI_RESET); break;
                }
            } else {
                printf("    Config %d:\n", i);
                printf("      Config ID: 0x%02X", cfg_id);
                switch(cfg_id) {
                    case DEVICE_STATE: printf(" (DEVICE_STATE)\n"); break;
                    case LOW_POWER_MODE: printf(" (LOW_POWER_MODE)\n"); break;
                    default: printf(" (UNKNOWN)\n"); break;
                }
                printf("      Status: 0x%02X", cfg_status);
                switch(cfg_status) {
                    case UCI_STATUS_OK: printf(" (OK)\n"); break;
                    case UCI_STATUS_REJECTED: printf(" (REJECTED)\n"); break;
                    case UCI_STATUS_FAILED: printf(" (FAILED)\n"); break;
                    case UCI_STATUS_INVALID_PARAM: printf(" (INVALID_PARAM)\n"); break;
                    default: printf(" (UNKNOWN)\n"); break;
                }
            }
        }
    }
}

void ui_decode_core_get_config_rsp(unsigned char* payload, int payload_len) {
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

            if (ui_color_enabled) {
                printf("    %s%sTLV %d:%s\n", ANSI_COLOR_BRIGHT_CYAN, ANSI_BOLD, i, ANSI_RESET);
                printf("      %s%sConfig ID:%s 0x%02X", ANSI_COLOR_YELLOW, ANSI_BOLD, ANSI_RESET, cfg_id);
                switch(cfg_id) {
                    case DEVICE_STATE: printf(" %s(DEVICE_STATE)%s\n", ANSI_COLOR_CYAN, ANSI_RESET); break;
                    case LOW_POWER_MODE: printf(" %s(LOW_POWER_MODE)%s\n", ANSI_COLOR_CYAN, ANSI_RESET); break;
                    default: printf(" %s(UNKNOWN)%s\n", ANSI_COLOR_YELLOW, ANSI_RESET); break;
                }
                printf("      %s%sLength:%s %d\n", ANSI_COLOR_YELLOW, ANSI_BOLD, ANSI_RESET, cfg_len);
            } else {
                printf("    TLV %d:\n", i);
                printf("      Config ID: 0x%02X", cfg_id);
                switch(cfg_id) {
                    case DEVICE_STATE: printf(" (DEVICE_STATE)\n"); break;
                    case LOW_POWER_MODE: printf(" (LOW_POWER_MODE)\n"); break;
                    default: printf(" (UNKNOWN)\n"); break;
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

                // Interpret common values
                if (cfg_id == DEVICE_STATE && cfg_len == 1) {
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

void ui_decode_core_device_reset_rsp(unsigned char* payload, int payload_len) {
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

void ui_decode_core_device_suspend_rsp(unsigned char* payload, int payload_len) {
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

void ui_decode_core_query_uwbs_timestamp_rsp(unsigned char* payload, int payload_len) {
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

void ui_decode_core_get_state_rsp(unsigned char* payload, int payload_len) {
    (void)payload;
    (void)payload_len;
    if (ui_color_enabled) {
        printf("  %s%sCORE_GET_STATE Response:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  CORE_GET_STATE Response:\n");
    }
}

void ui_decode_core_set_active_rsp(unsigned char* payload, int payload_len) {
    (void)payload;
    (void)payload_len;
    if (ui_color_enabled) {
        printf("  %s%sCORE_SET_ACTIVE Response:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  CORE_SET_ACTIVE Response:\n");
    }
}

void ui_decode_core_set_ready_rsp(unsigned char* payload, int payload_len) {
    (void)payload;
    (void)payload_len;
    if (ui_color_enabled) {
        printf("  %s%sCORE_SET_READY Response:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  CORE_SET_READY Response:\n");
    }
}

void ui_decode_core_device_ready_rsp(unsigned char* payload, int payload_len) {
    (void)payload;
    (void)payload_len;
    if (ui_color_enabled) {
        printf("  %s%sCORE_DEVICE_READY Response:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  CORE_DEVICE_READY Response:\n");
    }
}

void ui_decode_core_get_caps_rsp(unsigned char* payload, int payload_len) {
    (void)payload;
    (void)payload_len;
    if (ui_color_enabled) {
        printf("  %s%sCORE_GET_CAPS Response:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  CORE_GET_CAPS Response:\n");
    }
}

void ui_decode_core_set_power_rsp(unsigned char* payload, int payload_len) {
    (void)payload;
    (void)payload_len;
    if (ui_color_enabled) {
        printf("  %s%sCORE_SET_POWER Response:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  CORE_SET_POWER Response:\n");
    }
}

void ui_decode_core_get_power_rsp(unsigned char* payload, int payload_len) {
    (void)payload;
    (void)payload_len;
    if (ui_color_enabled) {
        printf("  %s%sCORE_GET_POWER Response:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  CORE_GET_POWER Response:\n");
    }
}

void ui_decode_core_device_on_rsp(unsigned char* payload, int payload_len) {
    (void)payload;
    (void)payload_len;
    if (ui_color_enabled) {
        printf("  %s%sCORE_DEVICE_ON Response:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  CORE_DEVICE_ON Response:\n");
    }
}

void ui_decode_core_device_off_rsp(unsigned char* payload, int payload_len) {
    (void)payload;
    (void)payload_len;
    if (ui_color_enabled) {
        printf("  %s%sCORE_DEVICE_OFF Response:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  CORE_DEVICE_OFF Response:\n");
    }
}

void ui_decode_core_device_suspend_cmd_rsp(unsigned char* payload, int payload_len) {
    (void)payload;
    (void)payload_len;
    if (ui_color_enabled) {
        printf("  %s%sCORE_DEVICE_SUSPEND_CMD Response:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  CORE_DEVICE_SUSPEND_CMD Response:\n");
    }
}

void ui_decode_core_device_status_ntf(unsigned char* payload, int payload_len) {
    (void)payload;
    (void)payload_len;
    if (ui_color_enabled) {
        printf("  %s%sCORE_DEVICE_STATUS_NTF:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  CORE_DEVICE_STATUS_NTF:\n");
    }
}

void ui_decode_core_generic_error_ntf(unsigned char* payload, int payload_len) {
    (void)payload;
    (void)payload_len;
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

void ui_decode_session_deinit_rsp(unsigned char* payload, int payload_len) {
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

void ui_decode_session_set_app_config_rsp(unsigned char* payload, int payload_len) {
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

        if (ui_color_enabled) {
            printf("    %s%sConfig[%d]:%s ID=0x%02X, Status=0x%02X",
                   ANSI_COLOR_BRIGHT_CYAN, ANSI_BOLD, i, ANSI_RESET, cfg_id, cfg_status);
            switch(cfg_status) {
                case UCI_STATUS_OK: printf(" %s(OK)%s\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET); break;
                case UCI_STATUS_REJECTED: printf(" %s(REJECTED)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
                case UCI_STATUS_FAILED: printf(" %s(FAILED)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
                default: printf(" %s(UNKNOWN)%s\n", ANSI_COLOR_YELLOW, ANSI_RESET); break;
            }
        } else {
            printf("    Config[%d]: ID=0x%02X, Status=0x%02X", i, cfg_id, cfg_status);
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

void ui_decode_session_get_app_config_rsp(unsigned char* payload, int payload_len) {
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

        if (ui_color_enabled) {
            printf("    %s%sTLV[%d]:%s Config ID=0x%02X, Length=%d bytes\n",
                   ANSI_COLOR_BRIGHT_CYAN, ANSI_BOLD, i, ANSI_RESET, cfg_id, cfg_len);
            printf("      %s%sValue:%s ", ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_RESET);
        } else {
            printf("    TLV[%d]: Config ID=0x%02X, Length=%d bytes\n", i, cfg_id, cfg_len);
            printf("      Value: ");
        }

        // Print hex value
        for (int j = 0; j < cfg_len; j++) {
            printf("0x%02X ", payload[offset + 2 + j]);
        }
        printf("\n");

        offset += 2 + cfg_len;
    }
}

void ui_decode_session_get_count_rsp(unsigned char* payload, int payload_len) {
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

void ui_decode_session_get_state_rsp(unsigned char* payload, int payload_len) {
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

void ui_decode_session_update_controller_multicast_list_rsp(unsigned char* payload, int payload_len) {
    (void)payload;
    (void)payload_len;
    if (ui_color_enabled) {
        printf("  %s%sSESSION_UPDATE_CONTROLLER_MULTICAST_LIST Response:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  SESSION_UPDATE_CONTROLLER_MULTICAST_LIST Response:\n");
    }
}

void ui_decode_session_update_active_rounds_dt_tag_rsp(unsigned char* payload, int payload_len) {
    (void)payload;
    (void)payload_len;
    if (ui_color_enabled) {
        printf("  %s%sSESSION_UPDATE_ACTIVE_ROUNDS_DT_TAG Response:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  SESSION_UPDATE_ACTIVE_ROUNDS_DT_TAG Response:\n");
    }
}

void ui_decode_session_data_transfer_phase_config_rsp(unsigned char* payload, int payload_len) {
    (void)payload;
    (void)payload_len;
    if (ui_color_enabled) {
        printf("  %s%sSESSION_DATA_TRANSFER_PHASE_CONFIG Response:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  SESSION_DATA_TRANSFER_PHASE_CONFIG Response:\n");
    }
}

void ui_decode_session_query_data_size_in_ranging_rsp(unsigned char* payload, int payload_len) {
    if (ui_color_enabled) {
        printf("  %s%sSESSION_QUERY_DATA_SIZE_IN_RANGING Response:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  SESSION_QUERY_DATA_SIZE_IN_RANGING Response:\n");
    }

    if (payload_len < 7) {  // Need at least 7 bytes: session_token(4) + status(1) + max_data_size(2)
        if (ui_color_enabled) {
            printf("    %s%sError: Payload too short (%d bytes, need at least 7)%s\n",
                   ANSI_COLOR_RED, ANSI_BOLD, payload_len, ANSI_RESET);
        } else {
            printf("    Error: Payload too short (%d bytes, need at least 7)\n", payload_len);
        }
        return;
    }

    // Extract fields
    unsigned int session_token = (unsigned int)payload[0] |
                                ((unsigned int)payload[1] << 8) |
                                ((unsigned int)payload[2] << 16) |
                                ((unsigned int)payload[3] << 24);
    
    unsigned char status = payload[4];
    unsigned short max_data_size = (unsigned short)payload[5] |
                                  ((unsigned short)payload[6] << 8);

    // Display decoded information
    if (ui_color_enabled) {
        printf("    %s%sSession Token:%s 0x%08X\n", ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, session_token);
        printf("    %s%sStatus:%s 0x%02X", ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, status);
        switch(status) {
            case UCI_STATUS_OK: printf(" %s(OK)%s\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET); break;
            case UCI_STATUS_REJECTED: printf(" %s(REJECTED)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            case UCI_STATUS_FAILED: printf(" %s(FAILED)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            default: printf(" %s(UNKNOWN)%s\n", ANSI_COLOR_YELLOW, ANSI_RESET); break;
        }
        printf("    %s%sMax Data Size:%s %u bytes\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_RESET, max_data_size);
    } else {
        printf("    Session Token: 0x%08X\n", session_token);
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

void ui_decode_session_set_hybrid_controller_config_rsp(unsigned char* payload, int payload_len) {
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

void ui_decode_session_set_hybrid_controlee_config_rsp(unsigned char* payload, int payload_len) {
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

void ui_decode_session_start_rsp(unsigned char* payload, int payload_len) {
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

void ui_decode_session_stop_rsp(unsigned char* payload, int payload_len) {
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

void ui_decode_session_get_ranging_count_rsp(unsigned char* payload, int payload_len) {
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

void ui_decode_session_status_ntf(unsigned char* payload, int payload_len) {
    (void)payload;
    (void)payload_len;
    if (ui_color_enabled) {
        printf("  %s%sSESSION_STATUS_NTF:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  SESSION_STATUS_NTF:\n");
    }
}

void ui_decode_session_data_credit_ntf(unsigned char* payload, int payload_len) {
    (void)payload;
    (void)payload_len;
    if (ui_color_enabled) {
        printf("  %s%sSESSION_DATA_CREDIT_NTF:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  SESSION_DATA_CREDIT_NTF:\n");
    }
}

void ui_decode_session_data_transfer_status_ntf(unsigned char* payload, int payload_len) {
    (void)payload;
    (void)payload_len;
    if (ui_color_enabled) {
        printf("  %s%sSESSION_DATA_TRANSFER_STATUS_NTF:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  SESSION_DATA_TRANSFER_STATUS_NTF:\n");
    }
}

void ui_decode_session_info_ntf(unsigned char* payload, int payload_len) {
    if (ui_color_enabled) {
        printf("  %s%sSESSION_INFO_NTF - Standard FiRa Ranging Notification:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  SESSION_INFO_NTF - Standard FiRa Ranging Notification:\n");
    }
    
    // Validate payload length - should be at least enough for basic header fields
    if (payload_len < 12) {
        if (ui_color_enabled) {
            printf("    %s%sERROR:%s Payload too short (%d bytes, need at least 12)\n", 
                   ANSI_COLOR_RED, ANSI_BOLD, ANSI_RESET, payload_len);
        } else {
            printf("    ERROR: Payload too short (%d bytes, need at least 12)\n", payload_len);
        }
        return;
    }

    // Decode standard FiRa ranging notification structure
    // Based on FiRa Consortium UWB Command Interface Generic Technical Specification v2.0.0
    // SESSION_INFO_NTF payload structure:
    // - Sequence Counter (4 bytes) - Little Endian
    // - Session Token/Handle (4 bytes) - Little Endian
    // - RCR Indicator (1 byte)
    // - Current Ranging Interval (4 bytes) - in units of 1200 RSTU (=1ms)
    // - Ranging Measurement Type (1 byte)
    // - RFU (1 byte)
    // - MAC Addressing Mode (1 byte) - 0: Short Address (2 bytes), 1: Extended Address (8 bytes)
    // - HUS Primary Session ID (4 bytes) - Little Endian
    // - RFU (4 bytes)
    // - Number of Ranging Measurements (1 byte)
    // - Variable length ranging measurements data
    
    uint32_t sequence_number = ui_read_u32_le(&payload[0]);
    uint32_t session_token = ui_read_u32_le(&payload[4]);
    uint8_t rcr_indicator = payload[8];
    uint32_t current_ranging_interval = ui_read_u32_le(&payload[9]); // in units of 1200 RSTU (=1ms)
    uint8_t ranging_measurement_type = payload[13];
    // payload[14] is RFU
    uint8_t mac_addressing_mode = payload[15]; // 0: Short Address (2 bytes), 1: Extended Address (8 bytes)
    uint32_t hus_primary_session_id = ui_read_u32_le(&payload[16]);
    // payload[20-23] are RFU
    uint8_t num_measurements = payload[24];
    
    if (ui_color_enabled) {
        printf("    %s%sSequence Number:%s %u\n", ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, sequence_number);
        printf("    %s%sSession Token:%s 0x%08X\n", ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, session_token);
        printf("    %s%sRCR Indicator:%s 0x%02X", ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, rcr_indicator);
        if (rcr_indicator == 0x01) {
            printf(" %s(Initiator)%s", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET);
        }
        printf("\n");
        printf("    %s%sCurrent Ranging Interval:%s %u ms\n", ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, current_ranging_interval);
        
        // Decode ranging measurement type
        const char* meas_type_str = "Unknown";
        switch(ranging_measurement_type) {
            case 0x00: meas_type_str = "OWR UL-TDoA"; break;
            case 0x01: meas_type_str = "TWR (SS-TWR & DS-TWR)"; break;
            case 0x02: meas_type_str = "OWR DL-TDoA"; break;
            case 0x03: meas_type_str = "OWR for AoA"; break;
            case 0x04: meas_type_str = "CCC Controller"; break;
            case 0x05: meas_type_str = "CCC Controlee"; break;
            case 0x06: meas_type_str = "OWR DL-TDoA v2"; break;
            default: meas_type_str = "Unknown"; break;
        }
        printf("    %s%sRanging Measurement Type:%s 0x%02X (%s)\n", 
               ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, ranging_measurement_type, meas_type_str);
               
        printf("    %s%sMAC Addressing Mode:%s 0x%02X (%s)\n",
               ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, mac_addressing_mode,
               mac_addressing_mode ? "Extended Address (8 bytes)" : "Short Address (2 bytes)");
        printf("    %s%sHUS Primary Session ID:%s 0x%08X\n", ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, hus_primary_session_id);
        printf("    %s%sNumber of Measurements:%s %u\n", ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, num_measurements);
    } else {
        printf("    Sequence Number: %u\n", sequence_number);
        printf("    Session Token: 0x%08X\n", session_token);
        printf("    RCR Indicator: 0x%02X (%s)\n", rcr_indicator, rcr_indicator ? "Initiator" : "Responder");
        printf("    Current Ranging Interval: %u ms\n", current_ranging_interval);
        printf("    Ranging Measurement Type: 0x%02X\n", ranging_measurement_type);
        printf("    MAC Addressing Mode: 0x%02X (%s)\n", mac_addressing_mode, 
               mac_addressing_mode ? "Extended Address" : "Short Address");
        printf("    HUS Primary Session ID: 0x%08X\n", hus_primary_session_id);
        printf("    Number of Measurements: %u\n", num_measurements);
    }

    // Parse ranging measurements
    int offset = 25; // Starting after the fixed header fields
    for (uint8_t i = 0; i < num_measurements; i++) {
        if (ui_color_enabled) {
            printf("    %s%sMeasurement %u:%s\n", ANSI_COLOR_BRIGHT_CYAN, ANSI_BOLD, i + 1, ANSI_RESET);
        } else {
            printf("    Measurement %u:\n", i + 1);
        }
        
        // Check if we have enough data for this measurement
        if (offset >= payload_len) {
            if (ui_color_enabled) {
                printf("      %s%sERROR:%s Insufficient data for measurement %u\n", 
                       ANSI_COLOR_RED, ANSI_BOLD, ANSI_RESET, i + 1);
            } else {
                printf("      ERROR: Insufficient data for measurement %u\n", i + 1);
            }
            break;
        }
        
        // Parse based on MAC addressing mode and ranging measurement type
        if (ranging_measurement_type == 0x01) { // TWR measurements
            int mac_addr_size = mac_addressing_mode ? 8 : 2; // Extended vs Short
            if (offset + mac_addr_size + 18 > payload_len) {
                if (ui_color_enabled) {
                    printf("      %s%sERROR:%s Insufficient data for TWR measurement (need %d bytes, have %d)\n", 
                           ANSI_COLOR_RED, ANSI_BOLD, ANSI_RESET, mac_addr_size + 18, payload_len - offset);
                } else {
                    printf("      ERROR: Insufficient data for TWR measurement (need %d bytes, have %d)\n", 
                           mac_addr_size + 18, payload_len - offset);
                }
                break;
            }
            
            // Parse MAC address
            if (ui_color_enabled) {
                printf("      %s%sMAC Address:%s ", ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_RESET);
            } else {
                printf("      MAC Address: ");
            }
            
            if (mac_addressing_mode == 0) {
                // Short address (2 bytes)
                uint16_t mac_addr = ui_read_u16_le(&payload[offset]);
                if (ui_color_enabled) {
                    printf("0x%04X\n", mac_addr);
                } else {
                    printf("0x%04X\n", mac_addr);
                }
                offset += 2;
            } else {
                // Extended address (8 bytes) - display in reverse order to match typical MAC address notation
                if (ui_color_enabled) {
                    for (int j = 7; j >= 0; j--) {
                        printf("%02X", payload[offset + j]);
                        if (j > 0) printf(":");
                    }
                    printf("\n");
                } else {
                    for (int j = 7; j >= 0; j--) {
                        printf("%02X", payload[offset + j]);
                        if (j > 0) printf(":");
                    }
                    printf("\n");
                }
                offset += 8;
            }
            
            // Parse TWR measurement fields (18 bytes)
            uint8_t status = payload[offset];
            uint8_t nlos = payload[offset + 1];
            uint16_t distance = ui_read_u16_le(&payload[offset + 2]);
            uint16_t aoa_azimuth = ui_read_u16_le(&payload[offset + 4]);
            uint8_t aoa_azimuth_fom = payload[offset + 6];
            uint16_t aoa_elevation = ui_read_u16_le(&payload[offset + 7]);
            uint8_t aoa_elevation_fom = payload[offset + 9];
            uint16_t dst_aoa_azimuth = ui_read_u16_le(&payload[offset + 10]);
            uint8_t dst_aoa_azimuth_fom = payload[offset + 12];
            uint16_t dst_aoa_elevation = ui_read_u16_le(&payload[offset + 13]);
            uint8_t dst_aoa_elevation_fom = payload[offset + 15];
            uint8_t slot_index = payload[offset + 16];
            int8_t rssi = (int8_t)payload[offset + 17];
            
            if (ui_color_enabled) {
                printf("      %s%sStatus:%s 0x%02X", ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_RESET, status);
                if (status == UCI_STATUS_OK) {
                    printf(" %s(OK)%s", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET);
                }
                printf("\n");
                printf("      %s%sNLOS:%s 0x%02X\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_RESET, nlos);
                printf("      %s%sDistance:%s %u cm\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_RESET, distance);
                printf("      %s%sAoA Azimuth:%s %u degrees\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_RESET, aoa_azimuth);
                printf("      %s%sAoA Azimuth FoM:%s %u\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_RESET, aoa_azimuth_fom);
                printf("      %s%sAoA Elevation:%s %u degrees\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_RESET, aoa_elevation);
                printf("      %s%sAoA Elevation FoM:%s %u\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_RESET, aoa_elevation_fom);
                printf("      %s%sDest AoA Azimuth:%s %u degrees\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_RESET, dst_aoa_azimuth);
                printf("      %s%sDest AoA Azimuth FoM:%s %u\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_RESET, dst_aoa_azimuth_fom);
                printf("      %s%sDest AoA Elevation:%s %u degrees\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_RESET, dst_aoa_elevation);
                printf("      %s%sDest AoA Elevation FoM:%s %u\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_RESET, dst_aoa_elevation_fom);
                printf("      %s%sSlot Index:%s %u\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_RESET, slot_index);
                printf("      %s%sRSSI:%s %d dBm\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_RESET, rssi);
            } else {
                printf("      Status: 0x%02X (%s)\n", status, status == UCI_STATUS_OK ? "OK" : "ERROR");
                printf("      NLOS: 0x%02X\n", nlos);
                printf("      Distance: %u cm\n", distance);
                printf("      AoA Azimuth: %u degrees\n", aoa_azimuth);
                printf("      AoA Azimuth FoM: %u\n", aoa_azimuth_fom);
                printf("      AoA Elevation: %u degrees\n", aoa_elevation);
                printf("      AoA Elevation FoM: %u\n", aoa_elevation_fom);
                printf("      Dest AoA Azimuth: %u degrees\n", dst_aoa_azimuth);
                printf("      Dest AoA Azimuth FoM: %u\n", dst_aoa_azimuth_fom);
                printf("      Dest AoA Elevation: %u degrees\n", dst_aoa_elevation);
                printf("      Dest AoA Elevation FoM: %u\n", dst_aoa_elevation_fom);
                printf("      Slot Index: %u\n", slot_index);
                printf("      RSSI: %d dBm\n", rssi);
            }
            
            offset += 18;
        } else {
            // For other measurement types, just show that we detected them
            if (ui_color_enabled) {
                printf("      %s%sRanging measurement type 0x%02X not fully implemented.%s\n", 
                       ANSI_COLOR_YELLOW, ANSI_BOLD, ranging_measurement_type, ANSI_RESET);
            } else {
                printf("      Ranging measurement type 0x%02X not fully implemented.\n", ranging_measurement_type);
            }
            // Skip to next measurement (we don't know the exact size, so we'll skip a reasonable amount)
            offset += 32; // Skip 32 bytes for unknown measurement type
            if (offset > payload_len) {
                break;
            }
        }
    }

    // Check for any remaining vendor-specific data
    if (offset < payload_len) {
        if (ui_color_enabled) {
            printf("    %s%sVendor-specific Data:%s %d bytes\n", ANSI_COLOR_BRIGHT_BLACK, ANSI_BOLD, ANSI_RESET, payload_len - offset);
        } else {
            printf("    Vendor-specific Data: %d bytes\n", payload_len - offset);
        }
    }
}

void ui_decode_android_range_diagnostics_ntf(unsigned char* payload, int payload_len) {
    (void)payload;
    (void)payload_len;
    if (ui_color_enabled) {
        printf("  %s%sANDROID_RANGE_DIAGNOSTICS_NTF:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  ANDROID_RANGE_DIAGNOSTICS_NTF:\n");
    }
}

void ui_decode_range_data_ntf(unsigned char* payload, int payload_len) {
    if (ui_color_enabled) {
        printf("  %s%sRANGE_DATA_NTF:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
        
        // Proper RangingData structure according to Qorvo spec:
        // - Sequence Counter (4 bytes)
        // - Session ID (4 bytes) 
        // - RFU (1 byte)
        // - Ranging interval (4 bytes)
        // - Ranging measurement type (1 byte)
        // - RFU (1 byte)
        // - MAC addressing mode (1 byte)
        // - Primary session ID (4 bytes)
        // - RFU (4 bytes)
        // - Number of measurements (1 byte)
        if (payload_len < 24) {
            printf("    %s%sERROR:%s Payload too short (%d bytes, need at least 24)\n", 
                   ANSI_COLOR_RED, ANSI_BOLD, ANSI_RESET, payload_len);
            return;
        }

        uint32_t sequence_number = ui_read_u32_le(&payload[0]);
        uint32_t session_handle = ui_read_u32_le(&payload[4]);
        // payload[8] is RFU
        uint32_t ranging_interval = ui_read_u32_le(&payload[9]); // in units of 1200 RSTU (=1ms) 
        uint8_t ranging_meas_type = payload[13]; // Ranging measurement type
        // payload[14] is RFU
        uint8_t mac_addr_mode = payload[15]; // 0=2 bytes, 1=8 bytes
        uint32_t primary_session_id = ui_read_u32_le(&payload[16]);
        // payload[20-23] is RFU
        uint8_t measurement_count = payload[24];

        printf("    %s%sSequence Number:%s %u\n", ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, sequence_number);
        printf("    %s%sSession Handle:%s 0x%08X\n", ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, session_handle);
        printf("    %s%sRanging Interval:%s %u ms\n", ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, ranging_interval);
        
        const char* meas_type_str = "Unknown";
        switch(ranging_meas_type) {
            case 0: meas_type_str = "OWR UL-TDoA"; break;
            case 1: meas_type_str = "TWR (SS-TWR & DS-TWR)"; break;
            case 2: meas_type_str = "OWR DL-TDoA"; break;
            case 3: meas_type_str = "OWR for AoA"; break;
            case 4: meas_type_str = "CCC Controller"; break;
            case 5: meas_type_str = "CCC Controlee"; break;
            case 6: meas_type_str = "OWR DL-TDoA v2"; break;
            default: meas_type_str = "Unknown"; break;
        }
        printf("    %s%sRanging Measurement Type:%s %u (%s)\n", 
               ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, 
               ranging_meas_type, meas_type_str);
        printf("    %s%sMAC Addressing Mode:%s %s (%s)\n",
               ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET,
               mac_addr_mode ? "8 bytes" : "2 bytes",
               mac_addr_mode ? "EXTENDED_ADDRESS" : "SHORT_ADDRESS");
        printf("    %s%sPrimary Session ID:%s 0x%08X\n", ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, primary_session_id);
        printf("    %s%sMeasurement Count:%s %u\n", ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, measurement_count);

        int offset = 25;
        for (uint8_t i = 0; i < measurement_count; i++) {
            printf("    %s%sMeasurement %u:%s\n", ANSI_COLOR_BRIGHT_CYAN, ANSI_BOLD, i + 1, ANSI_RESET);
            
            // Determine measurement size based on ranging measurement type
            int measurement_size = 0;
            if (ranging_meas_type == 1) { // TWR
                // TWR measurement structure: MAC address + status + NLOS + distance + AoA + etc. (20 bytes for short address)
                measurement_size = (mac_addr_mode == 0) ? 20 : 26; // 20 for short address, 26 for extended
            } else if (ranging_meas_type == 3) { // OWR AOA
                // OWR AOA measurement structure is smaller
                measurement_size = (mac_addr_mode == 0) ? 12 : 18; // 12 for short, 18 for extended address
            } else {
                // For other types, we'll use a default size and try to parse as TWR
                measurement_size = (mac_addr_mode == 0) ? 20 : 26;
            }
            
            if (offset + measurement_size > payload_len) {
                printf("      %s%sWARNING:%s Incomplete measurement (expected %d bytes, have %d left).\n",
                       ANSI_COLOR_YELLOW, ANSI_BOLD, ANSI_RESET, measurement_size, payload_len - offset);
                return;
            }
            
            // Parse based on ranging measurement type
            if (ranging_meas_type == 1) { // TWR measurements
                int mac_offset = (mac_addr_mode == 0) ? 2 : 8; // MAC address size
                uint16_t mac_address;
                if (mac_addr_mode == 0) {
                    mac_address = ui_read_u16_le(&payload[offset]);
                } else {
                    // For extended MAC, just print first 2 bytes as example
                    mac_address = ui_read_u16_le(&payload[offset]);
                }
                
                uint8_t meas_status = payload[offset + mac_offset];
                uint8_t nlos_byte = payload[offset + mac_offset + 1];
                uint16_t distance = ui_read_u16_le(&payload[offset + mac_offset + 2]);
                uint16_t aoa_azimuth = ui_read_u16_le(&payload[offset + mac_offset + 4]);
                uint8_t aoa_azimuth_fom = payload[offset + mac_offset + 6];
                uint16_t aoa_elevation = ui_read_u16_le(&payload[offset + mac_offset + 7]);
                uint8_t aoa_elevation_fom = payload[offset + mac_offset + 9];
                uint16_t dst_aoa_azimuth = ui_read_u16_le(&payload[offset + mac_offset + 10]);
                uint8_t dst_aoa_azimuth_fom = payload[offset + mac_offset + 12];
                uint16_t dst_aoa_elevation = ui_read_u16_le(&payload[offset + mac_offset + 13]);
                uint8_t dst_aoa_elevation_fom = payload[offset + mac_offset + 15];
                uint8_t slot_index = payload[offset + mac_offset + 16];
                int8_t rssi = (int8_t)payload[offset + mac_offset + 17];
                
                printf("      %s%sMAC Address:%s 0x%04X\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_RESET, mac_address);
                printf("      %s%sStatus:%s 0x%02X", ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_RESET, meas_status);
                if (meas_status == UCI_STATUS_OK) {
                    printf(" %s(OK)%s", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET);
                }
                printf("\n");
                printf("      %s%sNLOS byte:%s 0x%02X\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_RESET, nlos_byte);
                printf("      %s%sDistance:%s %u cm\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_RESET, distance);
                printf("      %s%sAoA Azimuth:%s %u degrees\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_RESET, aoa_azimuth);
                printf("      %s%sAoA Azimuth FoM:%s %u\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_RESET, aoa_azimuth_fom);
                printf("      %s%sAoA Elevation:%s %u degrees\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_RESET, aoa_elevation);
                printf("      %s%sAoA Elevation FoM:%s %u\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_RESET, aoa_elevation_fom);
                printf("      %s%sDest AoA Azimuth:%s %u degrees\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_RESET, dst_aoa_azimuth);
                printf("      %s%sDest AoA Azimuth FoM:%s %u\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_RESET, dst_aoa_azimuth_fom);
                printf("      %s%sDest AoA Elevation:%s %u degrees\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_RESET, dst_aoa_elevation);
                printf("      %s%sDest AoA Elevation FoM:%s %u\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_RESET, dst_aoa_elevation_fom);
                printf("      %s%sSlot Index:%s %u\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_RESET, slot_index);
                printf("      %s%sRSSI:%s %d dBm\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_RESET, rssi);
            } else if (ranging_meas_type == 3) { // OWR AOA measurements
                int mac_offset = (mac_addr_mode == 0) ? 2 : 8;
                uint16_t mac_address;
                if (mac_addr_mode == 0) {
                    mac_address = ui_read_u16_le(&payload[offset]);
                } else {
                    mac_address = ui_read_u16_le(&payload[offset]);
                }
                
                uint8_t meas_status = payload[offset + mac_offset];
                uint8_t nlos = payload[offset + mac_offset + 1];
                uint8_t frame_seq_num = payload[offset + mac_offset + 2];
                uint16_t block_index = ui_read_u16_le(&payload[offset + mac_offset + 3]);
                uint16_t aoa_azimuth = ui_read_u16_le(&payload[offset + mac_offset + 5]);
                uint8_t aoa_azimuth_fom = payload[offset + mac_offset + 7];
                uint16_t aoa_elevation = ui_read_u16_le(&payload[offset + mac_offset + 8]);
                uint8_t aoa_elevation_fom = payload[offset + mac_offset + 10];
                
                printf("      %s%sMAC Address:%s 0x%04X\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_RESET, mac_address);
                printf("      %s%sStatus:%s 0x%02X", ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_RESET, meas_status);
                if (meas_status == UCI_STATUS_OK) {
                    printf(" %s(OK)%s", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET);
                }
                printf("\n");
                printf("      %s%sNLOS:%s %s\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_RESET, 
                       nlos ? "Yes" : "No");
                printf("      %s%sFrame Sequence Num:%s %u\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_RESET, frame_seq_num);
                printf("      %s%sBlock Index:%s %u\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_RESET, block_index);
                printf("      %s%sAoA Azimuth:%s %u degrees\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_RESET, aoa_azimuth);
                printf("      %s%sAoA Azimuth FoM:%s %u\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_RESET, aoa_azimuth_fom);
                printf("      %s%sAoA Elevation:%s %u degrees\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_RESET, aoa_elevation);
                printf("      %s%sAoA Elevation FoM:%s %u\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_RESET, aoa_elevation_fom);
            } else {
                printf("      %s%sRanging measurement type %u not fully implemented.%s\n", 
                       ANSI_COLOR_YELLOW, ANSI_BOLD, ranging_meas_type, ANSI_RESET);
                printf("      %s%sMAC Address (first 2 bytes):%s 0x%04X\n", 
                       ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_RESET, 
                       ui_read_u16_le(&payload[offset]));
            }
                
            offset += measurement_size;
        }

        if (offset < payload_len) {
            printf("    %s%sVendor-specific Data:%s %d bytes\n", ANSI_COLOR_BRIGHT_BLACK, ANSI_BOLD, ANSI_RESET, payload_len - offset);
        }
    } else {
        printf("  RANGE_DATA_NTF:\n");
        if (payload_len >= 24) {
            uint32_t sequence_number = ui_read_u32_le(&payload[0]);
            uint32_t session_handle = ui_read_u32_le(&payload[4]);
            uint32_t ranging_interval = ui_read_u32_le(&payload[9]);
            uint8_t ranging_meas_type = payload[13];
            uint8_t measurement_count = payload[24];
            
            printf("    Sequence Number: %u\n", sequence_number);
            printf("    Session Handle: 0x%08X\n", session_handle);
            printf("    Ranging Interval: %u ms\n", ranging_interval);
            printf("    Ranging Measurement Type: %u\n", ranging_meas_type);
            printf("    Measurement Count: %u\n", measurement_count);
        }
    }
}
