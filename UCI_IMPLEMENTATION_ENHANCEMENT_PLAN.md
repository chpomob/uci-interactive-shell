# UCI Implementation Enhancement Plan

## Overview
This document provides a detailed plan for enhancing the UCI implementation based on analysis of the Qorvo SDK and identification of improvement opportunities.

## Phase 1: Core Protocol Compliance Enhancement

### 1.1 Implement Missing CORE Commands

#### CORE_DEVICE_SUSPEND Command Implementation

**Current State**: Partial implementation exists but lacks full specification compliance.

**Enhancement Plan**:
```c
// Add complete CORE_DEVICE_SUSPEND implementation in uci.c
void handle_core_device_suspend_cmd(unsigned char* payload, int payload_len) {
    if (ui_color_enabled) {
        printf("  %s%sCORE_DEVICE_SUSPEND_CMD - Suspend Device Command:%s\n", 
               ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  CORE_DEVICE_SUSPEND_CMD - Suspend Device Command:\n");
    }
    
    if (payload_len < 1) {
        if (ui_color_enabled) {
            printf("    %s%sError: Payload too short (need at least 1 byte, got %d)%s\n",
                   ANSI_COLOR_RED, ANSI_BOLD, payload_len, ANSI_RESET);
        } else {
            printf("    Error: Payload too short (need at least 1 byte, got %d)\n", payload_len);
        }
        return;
    }

    unsigned char reset_config = payload[0];
    
    if (ui_color_enabled) {
        printf("    %s%sReset Configuration:%s 0x%02X", 
               ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_RESET, reset_config);
        switch(reset_config) {
            case UWBS_RESET:
                printf(" %s(UWBS_RESET)%s\n", ANSI_COLOR_BRIGHT_BLUE, ANSI_RESET);
                break;
            default:
                printf(" %s(CUSTOM_RESET)%s\n", ANSI_COLOR_YELLOW, ANSI_RESET);
                break;
        }
    } else {
        printf("    Reset Configuration: 0x%02X", reset_config);
        switch(reset_config) {
            case UWBS_RESET:
                printf(" (UWBS_RESET)\n");
                break;
            default:
                printf(" (CUSTOM_RESET)\n");
                break;
        }
    }
    
    // Simulate device suspension
    // In a real implementation, this would trigger actual hardware suspension
    unsigned char device_state_payload = DEVICE_STATE_READY;
    enqueue_notification(CORE, CORE_DEVICE_STATUS_NTF, &device_state_payload, 1);
}

// Add response handler
void ui_decode_core_device_suspend_rsp(unsigned char* payload, int payload_len) {
    if (ui_color_enabled) {
        printf("  %s%sCORE_DEVICE_SUSPEND_RSP - Suspend Device Response:%s\n", 
               ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  CORE_DEVICE_SUSPEND_RSP - Suspend Device Response:\n");
    }
    
    if (payload_len < 1) {
        if (ui_color_enabled) {
            printf("    %s%sError: Payload too short (need at least 1 byte, got %d)%s\n",
                   ANSI_COLOR_RED, ANSI_BOLD, payload_len, ANSI_RESET);
        } else {
            printf("    Error: Payload too short (need at least 1 byte, got %d)\n", payload_len);
        }
        return;
    }
    
    unsigned char status = payload[0];
    
    if (ui_color_enabled) {
        printf("    %s%sStatus:%s 0x%02X", ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, status);
        switch(status) {
            case UCI_STATUS_OK:
                printf(" %s(OK)%s\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET);
                break;
            case UCI_STATUS_REJECTED:
                printf(" %s(REJECTED)%s\n", ANSI_COLOR_RED, ANSI_RESET);
                break;
            case UCI_STATUS_FAILED:
                printf(" %s(FAILED)%s\n", ANSI_COLOR_RED, ANSI_RESET);
                break;
            default:
                printf(" %s(UNKNOWN)%s\n", ANSI_COLOR_YELLOW, ANSI_RESET);
                break;
        }
    } else {
        printf("    Status: 0x%02X", status);
        switch(status) {
            case UCI_STATUS_OK:
                printf(" (OK)\n");
                break;
            case UCI_STATUS_REJECTED:
                printf(" (REJECTED)\n");
                break;
            case UCI_STATUS_FAILED:
                printf(" (FAILED)\n");
                break;
            default:
                printf(" (UNKNOWN)\n");
                break;
        }
    }
}
```

#### Enhanced CORE_GET_CONFIG Implementation

**Current State**: Basic implementation exists but lacks comprehensive TLV handling.

**Enhancement Plan**:
```c
// Enhanced CORE_GET_CONFIG implementation in uci.c
void handle_core_get_config_cmd(unsigned char* payload, int payload_len) {
    if (ui_color_enabled) {
        printf("  %s%sCORE_GET_CONFIG_CMD - Get Device Configuration Command:%s\n", 
               ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  CORE_GET_CONFIG_CMD - Get Device Configuration Command:\n");
    }
    
    if (payload_len < 2) {
        if (ui_color_enabled) {
            printf("    %s%sError: Payload too short (need at least 2 bytes, got %d)%s\n",
                   ANSI_COLOR_RED, ANSI_BOLD, payload_len, ANSI_RESET);
        } else {
            printf("    Error: Payload too short (need at least 2 bytes, got %d)\n", payload_len);
        }
        return;
    }
    
    unsigned char num_configs = payload[0];
    unsigned char config_count = 0;
    
    if (ui_color_enabled) {
        printf("    %s%sRequested Configurations:%s %d\n", 
               ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_RESET, num_configs);
    } else {
        printf("    Requested Configurations: %d\n", num_configs);
    }
    
    // Prepare response buffer
    unsigned char response_payload[256] = {0};
    unsigned char status = UCI_STATUS_OK;
    unsigned char response_offset = 2; // Reserve space for status and count
    
    // Validate that we have enough data for all requested configs
    if (payload_len < 1 + num_configs) {
        status = UCI_STATUS_INVALID_PARAM;
        num_configs = 0;
    }
    
    response_payload[0] = status;
    
    // Process each requested configuration
    for (unsigned char i = 0; i < num_configs && response_offset < sizeof(response_payload) - 32; i++) {
        DeviceConfigId cfg_id = (DeviceConfigId)payload[1 + i];
        
        if (ui_color_enabled) {
            printf("    %s%sConfiguration %d:%s\n", ANSI_COLOR_BRIGHT_CYAN, ANSI_BOLD, ANSI_RESET, i);
            printf("      %s%sConfig ID:%s 0x%02X", ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, cfg_id);
        } else {
            printf("    Configuration %d:\n", i);
            printf("      Config ID: 0x%02X", cfg_id);
        }
        
        // Print config name
        switch(cfg_id) {
            case DEVICE_STATE:
                if (ui_color_enabled) {
                    printf(" %s(DEVICE_STATE)%s\n", ANSI_COLOR_BRIGHT_BLUE, ANSI_RESET);
                } else {
                    printf(" (DEVICE_STATE)\n");
                }
                break;
            case LOW_POWER_MODE:
                if (ui_color_enabled) {
                    printf(" %s(LOW_POWER_MODE)%s\n", ANSI_COLOR_BRIGHT_BLUE, ANSI_RESET);
                } else {
                    printf(" (LOW_POWER_MODE)\n");
                }
                break;
            default:
                if (ui_color_enabled) {
                    printf(" %s(UNKNOWN)%s\n", ANSI_COLOR_RED, ANSI_RESET);
                } else {
                    printf(" (UNKNOWN)\n");
                }
                break;
        }
        
        // Get configuration value
        unsigned char config_value[256] = {0};
        unsigned char config_len = sizeof(config_value);
        int result = get_device_config(cfg_id, config_value, &config_len);
        
        if (result == 0 && config_len > 0) {
            // Successfully retrieved config
            if (response_offset + 2 + config_len < sizeof(response_payload)) {
                response_payload[response_offset] = cfg_id;
                response_payload[response_offset + 1] = config_len;
                memcpy(&response_payload[response_offset + 2], config_value, config_len);
                response_offset += 2 + config_len;
                config_count++;
                
                if (ui_color_enabled) {
                    printf("      %s%sLength:%s %d bytes\n", ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, config_len);
                    printf("      %s%sValue:%s ", ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET);
                    for (int j = 0; j < config_len; j++) {
                        printf("%s%02X%s ", ANSI_COLOR_BRIGHT_WHITE, config_value[j], ANSI_RESET);
                    }
                    printf("\n");
                } else {
                    printf("      Length: %d bytes\n", config_len);
                    printf("      Value: ");
                    for (int j = 0; j < config_len; j++) {
                        printf("%02X ", config_value[j]);
                    }
                    printf("\n");
                }
            } else {
                // Response buffer full
                if (ui_color_enabled) {
                    printf("      %s%sError: Response buffer full%s\n", ANSI_COLOR_RED, ANSI_BOLD, ANSI_RESET);
                } else {
                    printf("      Error: Response buffer full\n");
                }
                break;
            }
        } else {
            // Failed to retrieve config
            if (ui_color_enabled) {
                printf("      %s%sError: Failed to retrieve configuration%s\n", ANSI_COLOR_RED, ANSI_BOLD, ANSI_RESET);
            } else {
                printf("      Error: Failed to retrieve configuration\n");
            }
        }
    }
    
    // Update response header
    response_payload[0] = (status == UCI_STATUS_OK) ? UCI_STATUS_OK : status;
    response_payload[1] = config_count;
    
    // Send response
    struct uci_packet_header response_header;
    set_header_values_safe(&response_header, RESPONSE, COMPLETE, CORE, CORE_GET_CONFIG, response_offset);
    unsigned char response_packet[sizeof(struct uci_packet_header) + 256];
    memcpy(response_packet, &response_header, sizeof(response_header));
    memcpy(response_packet + sizeof(response_header), response_payload, response_offset);
    
    parse_uci_packet(response_packet, sizeof(response_header) + response_offset);
}
```

### 1.2 Enhanced Session Configuration Commands

#### SESSION_SET_HYBRID_CONTROLLER_CONFIG Enhancement

```c
// Enhanced implementation in uci.c
void handle_session_set_hybrid_controller_config_cmd(unsigned char* payload, int payload_len) {
    if (ui_color_enabled) {
        printf("  %s%sSESSION_SET_HYBRID_CONTROLLER_CONFIG_CMD - Set Hybrid Controller Config:%s\n", 
               ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  SESSION_SET_HYBRID_CONTROLLER_CONFIG_CMD - Set Hybrid Controller Config:\n");
    }
    
    if (payload_len < 5) { // Minimum: session_id(4) + num_tlvs(1)
        if (ui_color_enabled) {
            printf("    %s%sError: Payload too short (need at least 5 bytes, got %d)%s\n",
                   ANSI_COLOR_RED, ANSI_BOLD, payload_len, ANSI_RESET);
        } else {
            printf("    Error: Payload too short (need at least 5 bytes, got %d)\n", payload_len);
        }
        send_simple_response(SESSION_CONFIG, SESSION_SET_HYBRID_CONTROLLER_CONFIG, UCI_STATUS_INVALID_PARAM);
        return;
    }
    
    unsigned int session_id = read_u32_le(payload);
    unsigned char num_tlvs = payload[4];
    unsigned char processed_tlvs = 0;
    int offset = 5;
    
    if (ui_color_enabled) {
        printf("    %s%sSession ID:%s 0x%08X\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_RESET, session_id);
        printf("    %s%sNumber of TLVs:%s %d\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_RESET, num_tlvs);
    } else {
        printf("    Session ID: 0x%08X\n", session_id);
        printf("    Number of TLVs: %d\n", num_tlvs);
    }
    
    // Find session
    int session_idx = find_session_by_id(session_id);
    if (session_idx < 0) {
        if (ui_color_enabled) {
            printf("    %s%sError: Session not found%s\n", ANSI_COLOR_RED, ANSI_BOLD, ANSI_RESET);
        } else {
            printf("    Error: Session not found\n");
        }
        send_simple_response(SESSION_CONFIG, SESSION_SET_HYBRID_CONTROLLER_CONFIG, UCI_STATUS_SESSION_NOT_EXIST);
        return;
    }
    
    struct uci_session* session = &uci_sessions[session_idx];
    
    // Process each TLV
    unsigned char response_tlvs[64] = {0}; // Response buffer for TLV statuses
    unsigned char response_offset = 0;
    
    for (unsigned char i = 0; i < num_tlvs && offset + 2 <= payload_len; i++) {
        unsigned char tlv_type = payload[offset];
        unsigned char tlv_len = payload[offset + 1];
        offset += 2;
        
        if (offset + tlv_len > payload_len) {
            if (ui_color_enabled) {
                printf("    %s%sError: Incomplete TLV at index %d%s\n", 
                       ANSI_COLOR_RED, ANSI_BOLD, i, ANSI_RESET);
            } else {
                printf("    Error: Incomplete TLV at index %d\n", i);
            }
            break;
        }
        
        if (ui_color_enabled) {
            printf("    %s%sTLV %d:%s\n", ANSI_COLOR_BRIGHT_CYAN, ANSI_BOLD, ANSI_RESET, i);
            printf("      %s%sType:%s 0x%02X\n", ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, tlv_type);
            printf("      %s%sLength:%s %d bytes\n", ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, tlv_len);
            printf("      %s%sValue:%s ", ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET);
            for (int j = 0; j < tlv_len; j++) {
                printf("%s%02X%s ", ANSI_COLOR_BRIGHT_WHITE, payload[offset + j], ANSI_RESET);
            }
            printf("\n");
        } else {
            printf("    TLV %d:\n", i);
            printf("      Type: 0x%02X\n", tlv_type);
            printf("      Length: %d bytes\n", tlv_len);
            printf("      Value: ");
            for (int j = 0; j < tlv_len; j++) {
                printf("%02X ", payload[offset + j]);
            }
            printf("\n");
        }
        
        // Store configuration in session
        unsigned char status = UCI_STATUS_OK;
        if (store_session_config(session_idx, tlv_type, &payload[offset], tlv_len) != 0) {
            status = UCI_STATUS_INVALID_PARAM;
        }
        
        // Add to response
        if (response_offset + 2 <= sizeof(response_tlvs)) {
            response_tlvs[response_offset] = tlv_type;
            response_tlvs[response_offset + 1] = status;
            response_offset += 2;
            processed_tlvs++;
        }
        
        offset += tlv_len;
    }
    
    // Send response
    unsigned char response_payload[128] = {0};
    response_payload[0] = (processed_tlvs == num_tlvs) ? UCI_STATUS_OK : UCI_STATUS_INVALID_PARAM;
    response_payload[1] = processed_tlvs;
    memcpy(&response_payload[2], response_tlvs, response_offset);
    
    send_response_with_payload(SESSION_CONFIG, SESSION_SET_HYBRID_CONTROLLER_CONFIG, 
                              response_payload, 2 + response_offset);
}
```

## Phase 2: Data Transfer and Notifications Enhancement

### 2.1 Enhanced Data Transfer Status Reporting

```c
// Enhanced data transfer status notification in uci.c
void send_session_data_transfer_status_ntf(unsigned int session_token, 
                                          unsigned short uci_sequence_number,
                                          unsigned char status,
                                          unsigned char tx_count,
                                          const unsigned char* additional_data,
                                          unsigned char additional_data_len) {
    // Calculate payload size
    unsigned char payload_len = 6 + ((additional_data && additional_data_len > 0) ? additional_data_len : 0);
    unsigned char* payload = malloc(payload_len);
    
    if (!payload) {
        if (ui_color_enabled) {
            printf("%s%sError: Failed to allocate memory for data transfer status notification%s\n",
                   ANSI_COLOR_RED, ANSI_BOLD, ANSI_RESET);
        } else {
            printf("Error: Failed to allocate memory for data transfer status notification\n");
        }
        return;
    }
    
    // Build payload
    write_u32_le(payload, session_token);
    write_u16_le(&payload[4], uci_sequence_number);
    payload[6] = status;
    payload[7] = tx_count;
    
    if (additional_data && additional_data_len > 0) {
        memcpy(&payload[8], additional_data, additional_data_len);
    }
    
    // Send notification
    enqueue_notification(SESSION_CONTROL, SESSION_DATA_TRANSFER_STATUS_NTF, payload, payload_len);
    
    free(payload);
}

// Enhanced decoder in uci_ui_packet_decoder.c
void ui_decode_session_data_transfer_status_ntf(unsigned char* payload, int payload_len) {
    if (ui_color_enabled) {
        printf("  %s%sSESSION_DATA_TRANSFER_STATUS_NTF - Data Transfer Status Notification:%s\n", 
               ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  SESSION_DATA_TRANSFER_STATUS_NTF - Data Transfer Status Notification:\n");
    }
    
    if (payload_len < 8) {
        if (ui_color_enabled) {
            printf("    %s%sError: Payload too short (need at least 8 bytes, got %d)%s\n",
                   ANSI_COLOR_RED, ANSI_BOLD, payload_len, ANSI_RESET);
        } else {
            printf("    Error: Payload too short (need at least 8 bytes, got %d)\n", payload_len);
        }
        return;
    }
    
    unsigned int session_token = read_u32_le(payload);
    unsigned short uci_sequence_number = read_u16_le(&payload[4]);
    unsigned char status = payload[6];
    unsigned char tx_count = payload[7];
    
    if (ui_color_enabled) {
        printf("    %s%sSession Token:%s 0x%08X\n", 
               ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_RESET, session_token);
        printf("    %s%sUCI Sequence Number:%s %u\n", 
               ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_RESET, uci_sequence_number);
        printf("    %s%sStatus:%s 0x%02X", ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, status);
    } else {
        printf("    Session Token: 0x%08X\n", session_token);
        printf("    UCI Sequence Number: %u\n", uci_sequence_number);
        printf("    Status: 0x%02X", status);
    }
    
    // Interpret status
    switch(status) {
        case UCI_DATA_TRANSFER_STATUS_REPETITION_OK:
            if (ui_color_enabled) {
                printf(" %s(REPETITION_OK)%s\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET);
            } else {
                printf(" (REPETITION_OK)\n");
            }
            break;
        case UCI_DATA_TRANSFER_STATUS_OK:
            if (ui_color_enabled) {
                printf(" %s(OK)%s\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET);
            } else {
                printf(" (OK)\n");
            }
            break;
        case UCI_DATA_TRANSFER_STATUS_ERROR_DATA_TRANSFER:
            if (ui_color_enabled) {
                printf(" %s(ERROR_DATA_TRANSFER)%s\n", ANSI_COLOR_RED, ANSI_RESET);
            } else {
                printf(" (ERROR_DATA_TRANSFER)\n");
            }
            break;
        case UCI_DATA_TRANSFER_STATUS_ERROR_NO_CREDIT_AVAILABLE:
            if (ui_color_enabled) {
                printf(" %s(ERROR_NO_CREDIT_AVAILABLE)%s\n", ANSI_COLOR_RED, ANSI_RESET);
            } else {
                printf(" (ERROR_NO_CREDIT_AVAILABLE)\n");
            }
            break;
        case UCI_DATA_TRANSFER_STATUS_ERROR_REJECTED:
            if (ui_color_enabled) {
                printf(" %s(ERROR_REJECTED)%s\n", ANSI_COLOR_RED, ANSI_RESET);
            } else {
                printf(" (ERROR_REJECTED)\n");
            }
            break;
        case UCI_DATA_TRANSFER_STATUS_SESSION_TYPE_NOT_SUPPORTED:
            if (ui_color_enabled) {
                printf(" %s(SESSION_TYPE_NOT_SUPPORTED)%s\n", ANSI_COLOR_RED, ANSI_RESET);
            } else {
                printf(" (SESSION_TYPE_NOT_SUPPORTED)\n");
            }
            break;
        case UCI_DATA_TRANSFER_STATUS_ERROR_DATA_TRANSFER_IS_ONGOING:
            if (ui_color_enabled) {
                printf(" %s(ERROR_DATA_TRANSFER_IS_ONGOING)%s\n", ANSI_COLOR_RED, ANSI_RESET);
            } else {
                printf(" (ERROR_DATA_TRANSFER_IS_ONGOING)\n");
            }
            break;
        case UCI_DATA_TRANSFER_STATUS_INVALID_FORMAT:
            if (ui_color_enabled) {
                printf(" %s(INVALID_FORMAT)%s\n", ANSI_COLOR_RED, ANSI_RESET);
            } else {
                printf(" (INVALID_FORMAT)\n");
            }
            break;
        default:
            if (ui_color_enabled) {
                printf(" %s(UNKNOWN)%s\n", ANSI_COLOR_YELLOW, ANSI_RESET);
            } else {
                printf(" (UNKNOWN)\n");
            }
            break;
    }
    
    if (ui_color_enabled) {
        printf("    %s%sTX Count:%s %d\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_RESET, tx_count);
    } else {
        printf("    TX Count: %d\n", tx_count);
    }
    
    // Handle additional data
    if (payload_len > 8) {
        unsigned char additional_len = payload_len - 8;
        if (ui_color_enabled) {
            printf("    %s%sAdditional Data:%s %d bytes: ", 
                   ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, additional_len);
            for (int i = 0; i < additional_len; i++) {
                printf("%s%02X%s ", ANSI_COLOR_BRIGHT_WHITE, payload[8 + i], ANSI_RESET);
            }
            printf("\n");
        } else {
            printf("    Additional Data: %d bytes: ", additional_len);
            for (int i = 0; i < additional_len; i++) {
                printf("%02X ", payload[8 + i]);
            }
            printf("\n");
        }
    }
}
```

### 2.2 Enhanced Credit Management System

```c
// Enhanced credit management in uci.c
#define MAX_SESSION_CREDITS 16

typedef struct {
    unsigned int session_token;
    unsigned char credits_available;
    unsigned char max_credits;
} session_credit_info_t;

static session_credit_info_t g_session_credits[MAX_SESSIONS];

// Initialize credit system
void init_session_credit_system() {
    memset(g_session_credits, 0, sizeof(g_session_credits));
    
    // Initialize default credit values
    for (int i = 0; i < MAX_SESSIONS; i++) {
        g_session_credits[i].max_credits = MAX_SESSION_CREDITS;
        g_session_credits[i].credits_available = MAX_SESSION_CREDITS;
    }
    
    if (ui_color_enabled) {
        printf("%s%sSession credit system initialized%s\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("Session credit system initialized\n");
    }
}

// Consume session credits
unsigned char consume_session_credits(unsigned int session_token, unsigned char credits_needed) {
    int session_idx = find_session_by_token_or_id(session_token);
    if (session_idx < 0) {
        return 0; // Session not found
    }
    
    session_credit_info_t* credit_info = &g_session_credits[session_idx];
    
    if (credit_info->credits_available >= credits_needed) {
        credit_info->credits_available -= credits_needed;
        return 1; // Success
    }
    
    return 0; // Not enough credits
}

// Return session credits
void return_session_credits(unsigned int session_token, unsigned char credits_returned) {
    int session_idx = find_session_by_token_or_id(session_token);
    if (session_idx < 0) {
        return; // Session not found
    }
    
    session_credit_info_t* credit_info = &g_session_credits[session_idx];
    
    unsigned char new_credits = credit_info->credits_available + credits_returned;
    if (new_credits > credit_info->max_credits) {
        new_credits = credit_info->max_credits;
    }
    
    credit_info->credits_available = new_credits;
}

// Send credit notification
void send_session_data_credit_ntf(unsigned int session_token, unsigned char credit_availability) {
    unsigned char payload[5];
    write_u32_le(payload, session_token);
    payload[4] = credit_availability ? 0x01 : 0x00; // AVAILABLE : NOT_AVAILABLE
    
    enqueue_notification(SESSION_CONTROL, SESSION_DATA_CREDIT_NTF, payload, sizeof(payload));
}

// Enhanced credit notification decoder
void ui_decode_session_data_credit_ntf(unsigned char* payload, int payload_len) {
    if (ui_color_enabled) {
        printf("  %s%sSESSION_DATA_CREDIT_NTF - Data Credit Notification:%s\n", 
               ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  SESSION_DATA_CREDIT_NTF - Data Credit Notification:\n");
    }
    
    if (payload_len < 5) {
        if (ui_color_enabled) {
            printf("    %s%sError: Payload too short (need at least 5 bytes, got %d)%s\n",
                   ANSI_COLOR_RED, ANSI_BOLD, payload_len, ANSI_RESET);
        } else {
            printf("    Error: Payload too short (need at least 5 bytes, got %d)\n", payload_len);
        }
        return;
    }
    
    unsigned int session_token = read_u32_le(payload);
    unsigned char credit_availability = payload[4];
    
    if (ui_color_enabled) {
        printf("    %s%sSession Token:%s 0x%08X\n", 
               ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_RESET, session_token);
        printf("    %s%sCredit Availability:%s 0x%02X", 
               ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, credit_availability);
    } else {
        printf("    Session Token: 0x%08X\n", session_token);
        printf("    Credit Availability: 0x%02X", credit_availability);
    }
    
    if (credit_availability == 0x00) {
        if (ui_color_enabled) {
            printf(" %s(NOT_AVAILABLE)%s\n", ANSI_COLOR_RED, ANSI_RESET);
        } else {
            printf(" (NOT_AVAILABLE)\n");
        }
    } else {
        if (ui_color_enabled) {
            printf(" %s(AVAILABLE)%s\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET);
        } else {
            printf(" (AVAILABLE)\n");
        }
    }
}
```

## Phase 3: Security and Validation Enhancement

### 3.1 Enhanced Input Validation

```c
// Enhanced parameter validation in uci.c
typedef struct {
    unsigned char min_value;
    unsigned char max_value;
    const char* description;
} param_validation_rule_t;

static const param_validation_rule_t session_config_validation_rules[] = {
    {DEVICE_TYPE, 0x00, 0x01, "Device Type"},
    {RANGING_ROUND_USAGE, 0x00, 0x07, "Ranging Round Usage"},
    {STS_CONFIG, 0x00, 0x07, "STS Configuration"},
    {MULTI_NODE_MODE, 0x00, 0x07, "Multi-Node Mode"},
    {CHANNEL_NUMBER, 0x05, 0x0F, "Channel Number"},
    {NO_OF_CONTROLEE, 0x00, 0x08, "Number of Controlees"},
    // Add more rules as needed
};

// Validate configuration parameter
int validate_session_config_param(AppConfigTlvType cfg_id, const unsigned char* value, unsigned char len) {
    // Check for known validation rules
    size_t num_rules = sizeof(session_config_validation_rules) / sizeof(session_config_validation_rules[0]);
    for (size_t i = 0; i < num_rules; i++) {
        if (session_config_validation_rules[i].cfg_id == cfg_id) {
            // For single-byte values, check range
            if (len == 1) {
                unsigned char val = value[0];
                if (val < session_config_validation_rules[i].min_value || 
                    val > session_config_validation_rules[i].max_value) {
                    if (ui_color_enabled) {
                        printf("    %s%sValidation Failed:%s %s value 0x%02X out of range [0x%02X-0x%02X]\n",
                               ANSI_COLOR_RED, ANSI_BOLD, ANSI_RESET,
                               session_config_validation_rules[i].description,
                               val,
                               session_config_validation_rules[i].min_value,
                               session_config_validation_rules[i].max_value);
                    } else {
                        printf("    Validation Failed: %s value 0x%02X out of range [0x%02X-0x%02X]\n",
                               session_config_validation_rules[i].description,
                               val,
                               session_config_validation_rules[i].min_value,
                               session_config_validation_rules[i].max_value);
                    }
                    return -1; // Out of range
                }
            }
            return 0; // Valid
        }
    }
    
    // No specific validation rule, assume valid
    return 0;
}

// Enhanced configuration storage with validation
int store_session_config_safe(int session_idx, unsigned char cfg_id, 
                              const unsigned char* value, unsigned char len) {
    if (session_idx < 0 || session_idx >= MAX_SESSIONS) {
        return -1;
    }
    
    if (!value || len == 0 || len > MAX_SESSION_CONFIG_VALUE_SIZE) {
        return -1;
    }
    
    // Validate parameter
    if (validate_session_config_param((AppConfigTlvType)cfg_id, value, len) != 0) {
        return -1; // Validation failed
    }
    
    // Store configuration
    return store_session_config(session_idx, cfg_id, value, len);
}
```

### 3.2 Enhanced Error Recovery

```c
// Enhanced error handling in uci.c
typedef enum {
    ERROR_RECOVERY_NONE = 0,
    ERROR_RECOVERY_RETRY = 1,
    ERROR_RECOVERY_FALLBACK = 2,
    ERROR_RECOVERY_ABORT = 3
} error_recovery_strategy_t;

typedef struct {
    unsigned char error_code;
    error_recovery_strategy_t strategy;
    unsigned char max_retries;
    const char* description;
} error_recovery_rule_t;

static const error_recovery_rule_t error_recovery_rules[] = {
    {UCI_STATUS_RANGING_TX_FAILED, ERROR_RECOVERY_RETRY, 3, "Ranging TX Failed"},
    {UCI_STATUS_RANGING_RX_TIMEOUT, ERROR_RECOVERY_RETRY, 3, "Ranging RX Timeout"},
    {UCI_STATUS_RANGING_RX_PHY_DEC_FAILED, ERROR_RECOVERY_RETRY, 2, "PHY Decode Failed"},
    {UCI_STATUS_RANGING_RX_PHY_TOA_FAILED, ERROR_RECOVERY_RETRY, 2, "PHY TOA Failed"},
    {UCI_STATUS_RANGING_RX_PHY_STS_FAILED, ERROR_RECOVERY_RETRY, 2, "PHY STS Failed"},
    {UCI_STATUS_RANGING_RX_MAC_DEC_FAILED, ERROR_RECOVERY_RETRY, 2, "MAC Decode Failed"},
    {UCI_STATUS_INVALID_PARAM, ERROR_RECOVERY_ABORT, 0, "Invalid Parameter"},
    {UCI_STATUS_INVALID_RANGE, ERROR_RECOVERY_ABORT, 0, "Invalid Range"},
    {UCI_STATUS_INVALID_MSG_SIZE, ERROR_RECOVERY_ABORT, 0, "Invalid Message Size"},
};

// Handle recoverable errors
error_recovery_strategy_t handle_recoverable_error(unsigned char error_code) {
    size_t num_rules = sizeof(error_recovery_rules) / sizeof(error_recovery_rules[0]);
    for (size_t i = 0; i < num_rules; i++) {
        if (error_recovery_rules[i].error_code == error_code) {
            if (ui_color_enabled) {
                printf("%s%sRecoverable Error Detected:%s %s (Code: 0x%02X)\n",
                       ANSI_COLOR_YELLOW, ANSI_BOLD, ANSI_RESET,
                       error_recovery_rules[i].description,
                       error_code);
            } else {
                printf("Recoverable Error Detected: %s (Code: 0x%02X)\n",
                       error_recovery_rules[i].description,
                       error_code);
            }
            return error_recovery_rules[i].strategy;
        }
    }
    
    // Unknown error, no automatic recovery
    if (ui_color_enabled) {
        printf("%s%sUnknown Error:%s No recovery strategy for code 0x%02X\n",
               ANSI_COLOR_YELLOW, ANSI_BOLD, ANSI_RESET, error_code);
    } else {
        printf("Unknown Error: No recovery strategy for code 0x%02X\n", error_code);
    }
    
    return ERROR_RECOVERY_NONE;
}
```

## Phase 4: Transport Layer Enhancement

### 4.1 Enhanced Transport Abstraction

```c
// Enhanced transport abstraction in uci_hw_interface.c
typedef enum {
    TRANSPORT_CHARDEV = 0,
    TRANSPORT_UART = 1,
    TRANSPORT_SPI = 2,
    TRANSPORT_HSSPI = 3,
    TRANSPORT_QMUTILS = 4
} transport_type_t;

typedef struct {
    transport_type_t type;
    const char* name;
    int (*init)(const char* device_path);
    int (*send)(const unsigned char* data, size_t len);
    int (*receive)(unsigned char* buffer, size_t max_len, size_t* received_len);
    int (*close)();
    int (*reset)();
    int (*suspend)();
    int (*resume)();
} transport_interface_t;

// Character device transport implementation
static int chardev_transport_init(const char* device_path) {
    // Implementation from existing code
    return uci_chardev_init(device_path);
}

static int chardev_transport_send(const unsigned char* data, size_t len) {
    // Implementation from existing code
    return uci_chardev_write(data, len);
}

static int chardev_transport_receive(unsigned char* buffer, size_t max_len, size_t* received_len) {
    // Implementation from existing code
    return uci_chardev_read(buffer, max_len, received_len);
}

static int chardev_transport_close() {
    // Implementation from existing code
    return uci_chardev_close();
}

static int chardev_transport_reset() {
    // Implementation from existing code
    return uci_chardev_reset_device();
}

static int chardev_transport_suspend() {
    // Implementation from existing code
    return 0; // Not implemented for chardev
}

static int chardev_transport_resume() {
    // Implementation from existing code
    return 0; // Not implemented for chardev
}

// Transport interface table
static const transport_interface_t transport_interfaces[] = {
    {
        TRANSPORT_CHARDEV,
        "Character Device",
        chardev_transport_init,
        chardev_transport_send,
        chardev_transport_receive,
        chardev_transport_close,
        chardev_transport_reset,
        chardev_transport_suspend,
        chardev_transport_resume
    },
    // Add other transports as needed
};

static const transport_interface_t* g_current_transport = NULL;

// Select transport
int uci_select_transport(transport_type_t type, const char* device_path) {
    size_t num_transports = sizeof(transport_interfaces) / sizeof(transport_interfaces[0]);
    for (size_t i = 0; i < num_transports; i++) {
        if (transport_interfaces[i].type == type) {
            g_current_transport = &transport_interfaces[i];
            
            if (ui_color_enabled) {
                printf("%s%sSelected Transport:%s %s (%s)\n",
                       ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_RESET,
                       g_current_transport->name, device_path);
            } else {
                printf("Selected Transport: %s (%s)\n", g_current_transport->name, device_path);
            }
            
            return g_current_transport->init(device_path);
        }
    }
    
    if (ui_color_enabled) {
        printf("%s%sError: Unsupported transport type: %d%s\n",
               ANSI_COLOR_RED, ANSI_BOLD, type, ANSI_RESET);
    } else {
        printf("Error: Unsupported transport type: %d\n", type);
    }
    
    return -1;
}

// Send data through selected transport
int uci_transport_send(const unsigned char* data, size_t len) {
    if (!g_current_transport) {
        if (ui_color_enabled) {
            printf("%s%sError: No transport selected%s\n",
                   ANSI_COLOR_RED, ANSI_BOLD, ANSI_RESET);
        } else {
            printf("Error: No transport selected\n");
        }
        return -1;
    }
    
    return g_current_transport->send(data, len);
}

// Receive data through selected transport
int uci_transport_receive(unsigned char* buffer, size_t max_len, size_t* received_len) {
    if (!g_current_transport) {
        if (ui_color_enabled) {
            printf("%s%sError: No transport selected%s\n",
                   ANSI_COLOR_RED, ANSI_BOLD, ANSI_RESET);
        } else {
            printf("Error: No transport selected\n");
        }
        return -1;
    }
    
    return g_current_transport->receive(buffer, max_len, received_len);
}
```

## Implementation Priority Matrix

### High Priority (Must Have)
1. Complete CORE command set implementation
2. Enhanced session configuration command validation
3. Improved data transfer status reporting
4. Enhanced credit management system

### Medium Priority (Should Have)
1. Advanced error recovery mechanisms
2. Enhanced transport abstraction layer
3. Comprehensive input validation
4. Detailed notification system

### Low Priority (Nice to Have)
1. Extended vendor-specific command support
2. Advanced configuration management
3. Performance optimizations
4. Additional test coverage

## Testing Strategy

### Unit Tests
1. Test all new CORE commands with valid and invalid inputs
2. Test enhanced session configuration with boundary conditions
3. Test data transfer status reporting with various scenarios
4. Test credit management under load

### Integration Tests
1. End-to-end session management flows
2. Data transfer scenarios
3. Error recovery workflows
4. Transport layer interoperability

### Performance Tests
1. Measure throughput of enhanced credit management
2. Verify latency of notification delivery
3. Stress test with concurrent sessions

## Conclusion

This enhancement plan provides a comprehensive roadmap for improving the UCI implementation based on analysis of the Qorvo SDK. The enhancements focus on achieving complete protocol compliance, improving robustness, and adding advanced features while maintaining backward compatibility. The phased approach allows for incremental improvements with measurable benefits at each stage.