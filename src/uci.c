#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "uci.h"
#include "uci_functions.h"

// Global session storage
struct uci_session uci_sessions[MAX_SESSIONS];

void send_uci_command(unsigned char mt, unsigned char pbf, unsigned char gid, unsigned char oid, unsigned char* payload, int payload_len) {
    // Initialize session storage on first call
    static int initialized = 0;
    if (!initialized) {
        init_uci_sessions();
        initialized = 1;
    }

    struct uci_packet_header header;
    set_header_values(&header, mt, pbf, gid, oid, payload_len);

    printf("Sending UCI packet:\n");
    // Print the raw bytes as they would appear on the wire
    unsigned char* header_bytes = (unsigned char*)&header;
    printf("  Header: %02X %02X %02X %02X\n", header_bytes[0], header_bytes[1], header_bytes[2], header_bytes[3]);
    printf("  Payload: ");
    for (int i = 0; i < payload_len; i++) {
        printf("%02X ", payload[i]);
    }
    printf("\n");

    // Simulate receiving a response
    printf("Simulating UCI response...\n");
    unsigned char response_packet[sizeof(struct uci_packet_header) + 255]; // Increased size to handle all possible payloads
    struct uci_packet_header* response_header = (struct uci_packet_header*)response_packet;
    set_header_values(response_header, RESPONSE, COMPLETE, gid, oid, 0); // Initialize with 0, will be updated below
    
    if (gid == CORE && oid == CORE_DEVICE_INFO) {
        // Simulate a CORE_DEVICE_INFO_RSP according to Android UWB specification
        unsigned char device_info_rsp_payload[] = {
            UCI_STATUS_OK,      // status
            0x01, 0x00,         // uci_version (1.0)
            0x02, 0x00,         // mac_version (2.0) 
            0x02, 0x00,         // phy_version (2.0)
            0x01, 0x00          // uci_test_version (1.0)
        };
        memcpy(response_packet + sizeof(struct uci_packet_header), device_info_rsp_payload, sizeof(device_info_rsp_payload));
        response_header->payload_len = sizeof(device_info_rsp_payload);
    } else if (gid == CORE && oid == CORE_GET_CAPS_INFO) {
        // Simulate a CORE_GET_CAPS_INFO_RSP
        unsigned char caps_rsp_payload[] = {UCI_STATUS_OK, 0x01, SUPPORTED_V1_FIRA_PHY_VERSION_RANGE_V2_MAX_MESSAGE_SIZE, 0x02, 0x01, 0x00};
        memcpy(response_packet + sizeof(struct uci_packet_header), caps_rsp_payload, sizeof(caps_rsp_payload));
        response_header->payload_len = sizeof(caps_rsp_payload);
    } else if (gid == CORE && oid == CORE_SET_CONFIG) {
        // Simulate a CORE_SET_CONFIG_RSP with correct TLV format
        // Response format: [status][num_cfg_status][cfg_id_1][status_1]...
        unsigned char num_tlvs = payload[0]; // First byte is number of TLVs
        int response_size = 2 + (num_tlvs * 2); // status + num_cfg_status + (cfg_id + status) * num_tlvs
        unsigned char* set_config_rsp_payload = malloc(response_size);
        if (set_config_rsp_payload) {
            set_config_rsp_payload[0] = UCI_STATUS_OK; // status
            set_config_rsp_payload[1] = num_tlvs;      // num_cfg_status
            
            // For each config TLV in the request, send back its status
            for (int i = 0; i < num_tlvs; i++) {
                // Calculate offset to the cfg_id in the request payload
                // Format is: [num_tlvs][cfg_id_1][len_1][val_1...][cfg_id_2][len_2][val_2...]...
                int offset = 1; // Start after num_tlvs byte
                for (int j = 0; j < i; j++) {
                    offset += 2; // cfg_id + len
                    // Add the value length to offset
                    offset += payload[1 + 2*j + 1]; // payload[1] is first length, [1+2*j+1] is j-th length
                }
                DeviceConfigId cfg_id = (DeviceConfigId)payload[offset];
                set_config_rsp_payload[2 + i*2] = cfg_id;           // cfg_id
                set_config_rsp_payload[2 + i*2 + 1] = UCI_STATUS_OK; // status (success for all)
            }
            
            memcpy(response_packet + sizeof(struct uci_packet_header), set_config_rsp_payload, response_size);
            response_header->payload_len = response_size;
            free(set_config_rsp_payload);
        } else {
            // Fallback to simple response if malloc fails
            unsigned char fallback_payload[] = {UCI_STATUS_OK, 0x01, DEVICE_STATE, UCI_STATUS_OK};
            memcpy(response_packet + sizeof(struct uci_packet_header), fallback_payload, sizeof(fallback_payload));
            response_header->payload_len = sizeof(fallback_payload);
        }
    } else if (gid == CORE && oid == CORE_GET_CONFIG) {
        // Simulate a CORE_GET_CONFIG_RSP with correct TLV format
        // Response format: [status][num_tlvs][cfg_id_1][len_1][val_1...][cfg_id_2][len_2][val_2...]...
        unsigned char num_req_cfgs = payload[0]; // First byte is number of requested config IDs
        int total_response_size = 2; // status + num_tlvs
        
        // Calculate response size (simplified - assuming each config returns 1 byte value)
        for (int i = 0; i < num_req_cfgs; i++) {
            total_response_size += 3; // cfg_id(1) + len(1) + value(1)
        }
        
        unsigned char* get_config_rsp_payload = malloc(total_response_size);
        if (get_config_rsp_payload) {
            get_config_rsp_payload[0] = UCI_STATUS_OK; // status
            get_config_rsp_payload[1] = num_req_cfgs;  // num_tlvs
            
            int response_offset = 2;
            for (int i = 0; i < num_req_cfgs; i++) {
                DeviceConfigId cfg_id = (DeviceConfigId)payload[1 + i]; // cfg_ids start at byte 1
                get_config_rsp_payload[response_offset] = cfg_id;        // cfg_id
                get_config_rsp_payload[response_offset + 1] = 1;         // length (1 byte for this example)
                
                // Set value based on config ID
                if (cfg_id == DEVICE_STATE) {
                    get_config_rsp_payload[response_offset + 2] = DEVICE_STATE_ACTIVE; // return active state
                } else if (cfg_id == LOW_POWER_MODE) {
                    get_config_rsp_payload[response_offset + 2] = 0; // return OFF
                } else {
                    get_config_rsp_payload[response_offset + 2] = 0; // default value
                }
                
                response_offset += 3; // cfg_id + len + value
            }
            
            memcpy(response_packet + sizeof(struct uci_packet_header), get_config_rsp_payload, total_response_size);
            response_header->payload_len = total_response_size;
            free(get_config_rsp_payload);
        } else {
            // Fallback to simple response if malloc fails
            unsigned char fallback_payload[] = {UCI_STATUS_OK, 0x01, DEVICE_STATE, 0x01, DEVICE_STATE_ACTIVE};
            memcpy(response_packet + sizeof(struct uci_packet_header), fallback_payload, sizeof(fallback_payload));
            response_header->payload_len = sizeof(fallback_payload);
        }
    } else if (gid == CORE && oid == CORE_DEVICE_RESET) {
        // Simulate a CORE_DEVICE_RESET_RSP
        unsigned char reset_rsp_payload[] = {UCI_STATUS_OK};
        memcpy(response_packet + sizeof(struct uci_packet_header), reset_rsp_payload, sizeof(reset_rsp_payload));
        response_header->payload_len = sizeof(reset_rsp_payload);
        
        // Reset all sessions when device is reset
        init_uci_sessions();
    } else if (gid == SESSION_CONFIG && oid == SESSION_INIT) {
        // Extract session ID from payload (first 4 bytes)
        unsigned int session_id = (payload[0] << 24) | (payload[1] << 16) | (payload[2] << 8) | payload[3];
        SessionType session_type = (SessionType)payload[4];  // Session type is 5th byte
        
        // Find an available slot for the session
        int session_idx = find_free_session_slot();
        if (session_idx < 0) {
            // No free slots available - return error
            unsigned char session_init_error_rsp[] = {UCI_STATUS_MAX_SESSIONS_EXCEEDED};
            memcpy(response_packet + sizeof(struct uci_packet_header), session_init_error_rsp, sizeof(session_init_error_rsp));
            response_header->payload_len = sizeof(session_init_error_rsp);
        } else {
            // Initialize the session
            uci_sessions[session_idx].session_id = session_id;
            uci_sessions[session_idx].session_type = session_type;
            uci_sessions[session_idx].session_state = SESSION_STATE_INIT;
            uci_sessions[session_idx].is_allocated = 1;
            uci_sessions[session_idx].num_configs = 0;
            
            // For FIRA v2.0, return SESSION_INIT_RSP with session handle
            unsigned char session_init_rsp_payload[] = {
                UCI_STATUS_OK, 
                (session_idx & 0xFF), ((session_idx >> 8) & 0xFF), 
                ((session_idx >> 16) & 0xFF), ((session_idx >> 24) & 0xFF)  // Session handle (4 bytes)
            };
            memcpy(response_packet + sizeof(struct uci_packet_header), session_init_rsp_payload, sizeof(session_init_rsp_payload));
            response_header->payload_len = sizeof(session_init_rsp_payload);
        }
    } else if (gid == SESSION_CONFIG && oid == SESSION_DEINIT) {
        // Extract session ID from payload (first 4 bytes)
        unsigned int session_id = (payload[0] << 24) | (payload[1] << 16) | (payload[2] << 8) | payload[3];
        
        int session_idx = find_session_by_id(session_id);
        if (session_idx >= 0) {
            // Mark session as deinitialized
            uci_sessions[session_idx].session_state = SESSION_STATE_DEINIT;
            uci_sessions[session_idx].is_allocated = 0;
        }
        
        // Return response
        unsigned char session_deinit_rsp_payload[] = {UCI_STATUS_OK};
        memcpy(response_packet + sizeof(struct uci_packet_header), session_deinit_rsp_payload, sizeof(session_deinit_rsp_payload));
        response_header->payload_len = sizeof(session_deinit_rsp_payload);
    } else if (gid == SESSION_CONTROL && oid == SESSION_START) {
        // Extract session ID from payload (first 4 bytes)
        unsigned int session_id = (payload[0] << 24) | (payload[1] << 16) | (payload[2] << 8) | payload[3];
        
        int session_idx = find_session_by_id(session_id);
        if (session_idx >= 0) {
            // Update session state to active
            uci_sessions[session_idx].session_state = SESSION_STATE_ACTIVE;
        }
        
        // Return response
        unsigned char session_start_rsp_payload[] = {UCI_STATUS_OK};
        memcpy(response_packet + sizeof(struct uci_packet_header), session_start_rsp_payload, sizeof(session_start_rsp_payload));
        response_header->payload_len = sizeof(session_start_rsp_payload);
    } else if (gid == SESSION_CONTROL && oid == SESSION_STOP) {
        // Extract session ID from payload (first 4 bytes)
        unsigned int session_id = (payload[0] << 24) | (payload[1] << 16) | (payload[2] << 8) | payload[3];
        
        int session_idx = find_session_by_id(session_id);
        if (session_idx >= 0) {
            // Update session state to idle
            uci_sessions[session_idx].session_state = SESSION_STATE_IDLE;
        }
        
        // Return response
        unsigned char session_stop_rsp_payload[] = {UCI_STATUS_OK};
        memcpy(response_packet + sizeof(struct uci_packet_header), session_stop_rsp_payload, sizeof(session_stop_rsp_payload));
        response_header->payload_len = sizeof(session_stop_rsp_payload);
    } else if (gid == SESSION_CONFIG && oid == SESSION_GET_STATE) {
        // Extract session ID from payload (first 4 bytes)
        unsigned int session_id = (payload[0] << 24) | (payload[1] << 16) | (payload[2] << 8) | payload[3];
        
        int session_idx = find_session_by_id(session_id);
        unsigned char session_state;
        if (session_idx >= 0) {
            session_state = uci_sessions[session_idx].session_state;
        } else {
            session_state = SESSION_STATE_DEINIT; // Session not found
        }
        
        // Return response: status + session_state
        unsigned char get_state_rsp_payload[] = {UCI_STATUS_OK, session_state};
        memcpy(response_packet + sizeof(struct uci_packet_header), get_state_rsp_payload, sizeof(get_state_rsp_payload));
        response_header->payload_len = sizeof(get_state_rsp_payload);
    } else if (gid == SESSION_CONFIG && oid == SESSION_SET_APP_CONFIG) {
        // Extract session ID from payload (first 4 bytes)
        unsigned int session_id = (payload[0] << 24) | (payload[1] << 16) | (payload[2] << 8) | payload[3];
        unsigned char num_tlvs = payload[4];  // Number of TLVs at byte 4

        int session_idx = find_session_by_id(session_id);
        if (session_idx >= 0) {
            // Parse and store each configuration TLV
            int offset = 5;  // Start after session_id and num_tlvs
            for (int i = 0; i < num_tlvs; i++) {
                unsigned char cfg_id = payload[offset];
                unsigned char cfg_len = payload[offset + 1];
                offset += 2;
                
                if (offset + cfg_len <= payload_len) {
                    store_session_config(session_idx, cfg_id, &payload[offset], cfg_len);
                }
                
                offset += cfg_len;
            }
        }

        // Return proper response based on the original logic 
        // Parse and store each configuration TLV
        int offset = 5;  // Start after session_id and num_tlvs
        for (int i = 0; i < num_tlvs; i++) {
            unsigned char cfg_id = payload[offset];
            unsigned char cfg_len = payload[offset + 1];
            offset += 2;
            
            if (session_idx >= 0 && offset + cfg_len <= payload_len) {
                store_session_config(session_idx, cfg_id, &payload[offset], cfg_len);
            }
            
            offset += cfg_len;
        }

        // Response format: [status][num_config_statuses][cfg_id_1][status_1]...
        int rsp_size = 2 + (num_tlvs * 2); // status + num_tlvs + (cfg_id + status) * num_tlvs
        unsigned char* full_rsp = malloc(rsp_size);
        if (full_rsp) {
            full_rsp[0] = UCI_STATUS_OK;  // status
            full_rsp[1] = num_tlvs;       // num_cfg_status
            
            offset = 5;  // Reset to start after session_id and num_tlvs
            for (int i = 0; i < num_tlvs; i++) {
                unsigned char cfg_id = payload[offset];
                full_rsp[2 + i*2] = cfg_id;                   // cfg_id
                full_rsp[2 + i*2 + 1] = UCI_STATUS_OK;        // status for each config (success)
                
                // Skip to next config: cfg_id + len + value_len
                offset += 2 + payload[offset + 1];           // cfg_id + len + value_len
            }
            
            memcpy(response_packet + sizeof(struct uci_packet_header), full_rsp, rsp_size);
            response_header->payload_len = rsp_size;
            free(full_rsp);
        } else {
            // Fallback to simple response if malloc fails
            unsigned char fallback_payload[] = {UCI_STATUS_OK, num_tlvs};
            // Add dummy status for each configuration
            for (int i = 0; i < num_tlvs; i++) {
                // Need to get cfg_id from original payload to add to response
                int temp_offset = 5 + (i > 0 ? 2 + payload[5 + 1] : 0);
                if (temp_offset < payload_len && (temp_offset + 1) < payload_len) {
                    fallback_payload[2 + i*2] = payload[temp_offset];      // cfg_id
                    fallback_payload[2 + i*2 + 1] = UCI_STATUS_OK;         // status
                }
            }
            memcpy(response_packet + sizeof(struct uci_packet_header), fallback_payload, sizeof(fallback_payload));
            response_header->payload_len = 2 + (num_tlvs * 2);
        }
    } else if (gid == SESSION_CONFIG && oid == SESSION_GET_APP_CONFIG) {
        // Extract session ID from payload (first 4 bytes)
        unsigned int session_id = (payload[0] << 24) | (payload[1] << 16) | (payload[2] << 8) | payload[3];
        unsigned char num_req_cfgs = payload[4];  // Number of requested config IDs at byte 4

        int session_idx = find_session_by_id(session_id);
        
        // Calculate response size: status + num_tlvs + (cfg_id + len + value) * num_req_cfgs
        int total_response_size = 2; // status + num_tlvs
        for (int i = 0; i < num_req_cfgs; i++) {
            unsigned char cfg_id = payload[5 + i]; // cfg_ids start at byte 5
            unsigned char value, len;
            if (session_idx >= 0 && get_session_config(session_idx, cfg_id, &value, &len)) {
                total_response_size += 3; // cfg_id + len + value
            } else {
                total_response_size += 3; // Still account for it, but with default value
            }
        }
        
        unsigned char* get_app_config_rsp = malloc(total_response_size);
        if (get_app_config_rsp) {
            get_app_config_rsp[0] = UCI_STATUS_OK;  // status
            get_app_config_rsp[1] = num_req_cfgs;   // num_tlvs
            
            int response_offset = 2;
            for (int i = 0; i < num_req_cfgs; i++) {
                unsigned char cfg_id = payload[5 + i]; // cfg_ids start at byte 5
                get_app_config_rsp[response_offset] = cfg_id;  // cfg_id
                get_app_config_rsp[response_offset + 1] = 1;   // length (1 byte for this example)
                
                // Get stored value or set default
                unsigned char value, len;
                if (session_idx >= 0 && get_session_config(session_idx, cfg_id, &value, &len)) {
                    get_app_config_rsp[response_offset + 2] = value; // stored value
                } else {
                    get_app_config_rsp[response_offset + 2] = 0; // default value
                }
                
                response_offset += 3; // cfg_id + len + value
            }
            
            memcpy(response_packet + sizeof(struct uci_packet_header), get_app_config_rsp, total_response_size);
            response_header->payload_len = total_response_size;
            free(get_app_config_rsp);
        } else {
            // Fallback to simple response if malloc fails
            unsigned char fallback_payload[] = {UCI_STATUS_OK, 0x00};
            memcpy(response_packet + sizeof(struct uci_packet_header), fallback_payload, sizeof(fallback_payload));
            response_header->payload_len = sizeof(fallback_payload);
        }
    } else {
        unsigned char status = UCI_STATUS_OK;
        memcpy(response_packet + sizeof(struct uci_packet_header), &status, 1);
        response_header->payload_len = 1;
    }
    
    parse_uci_packet(response_packet, sizeof(struct uci_packet_header) + response_header->payload_len);
}

void handle_core_device_info_rsp(unsigned char* payload, int payload_len) {
    if (payload_len < 9) { // status (1) + uci_version (2) + mac_version (2) + phy_version (2) + uci_test_version (2)
        printf("Error: CORE_DEVICE_INFO_RSP payload too short.\n");
        return;
    }

    unsigned char status = payload[0];
    unsigned short uci_version = (payload[1] << 8) | payload[2];
    unsigned short mac_version = (payload[3] << 8) | payload[4];
    unsigned short phy_version = (payload[5] << 8) | payload[6];
    unsigned short uci_test_version = (payload[7] << 8) | payload[8];
    // Remaining bytes are vendor_spec_info (may be 0 or more bytes)

    printf("  Status: 0x%02X\n", status);
    printf("  UCI Version: 0x%04X\n", uci_version);
    printf("  MAC Version: 0x%04X\n", mac_version);
    printf("  PHY Version: 0x%04X\n", phy_version);
    printf("  UCI Test Version: 0x%04X\n", uci_test_version);
    
    if (payload_len > 9) {
        printf("  Vendor Specific Info: ");
        for (int i = 9; i < payload_len; i++) {
            printf("%02X ", payload[i]);
        }
        printf("\n");
    }
}

void handle_core_get_caps_info_rsp(unsigned char* payload, int payload_len) {
    if (payload_len < 2) { // status (1) + num_tlvs (1)
        printf("Error: CORE_GET_CAPS_INFO_RSP payload too short.\n");
        return;
    }

    unsigned char status = payload[0];
    unsigned char num_tlvs = payload[1];
    printf("  Status: 0x%02X\n", status);
    printf("  Number of TLVs: %d\n", num_tlvs);

    int offset = 2;
    for (int i = 0; i < num_tlvs; i++) {
        if (offset + 2 > payload_len) {
            printf("Error: Incomplete TLV in CORE_GET_CAPS_INFO_RSP payload.\n");
            return;
        }
        CapTlvType tlv_type = (CapTlvType)payload[offset];
        unsigned char tlv_len = payload[offset + 1];
        printf("    TLV Type: 0x%02X, Length: %d, Value: ", tlv_type, tlv_len);
        offset += 2;
        if (offset + tlv_len > payload_len) {
            printf("Error: Incomplete TLV value in CORE_GET_CAPS_INFO_RSP payload.\n");
            return;
        }
        for (int j = 0; j < tlv_len; j++) {
            printf("%02X ", payload[offset + j]);
        }
        printf("\n");
        offset += tlv_len;
    }
}

void handle_core_set_config_rsp(unsigned char* payload, int payload_len) {
    if (payload_len < 2) { // status (1) + num_cfg_status (1)
        printf("Error: CORE_SET_CONFIG_RSP payload too short.\n");
        return;
    }

    unsigned char status = payload[0];
    unsigned char num_cfg_status = payload[1];
    printf("  Status: 0x%02X\n", status);
    printf("  Number of Config Status: %d\n", num_cfg_status);

    int offset = 2;
    for (int i = 0; i < num_cfg_status; i++) {
        if (offset + 2 > payload_len) {
            printf("Error: Incomplete Config Status in CORE_SET_CONFIG_RSP payload.\n");
            return;
        }
        DeviceConfigId cfg_id = (DeviceConfigId)payload[offset];
        unsigned char cfg_status = payload[offset + 1];
        printf("    Config ID: 0x%02X (%s), Status: 0x%02X", cfg_id, 
               cfg_id == DEVICE_STATE ? "DEVICE_STATE" : 
               cfg_id == LOW_POWER_MODE ? "LOW_POWER_MODE" : "UNKNOWN", 
               cfg_status);
        if (cfg_status == UCI_STATUS_OK) {
            printf(" (OK)");
        } else if (cfg_status == UCI_STATUS_INVALID_PARAM) {
            printf(" (Invalid Parameter)");
        } else if (cfg_status == UCI_STATUS_REJECTED) {
            printf(" (Rejected)");
        }
        printf("\n");
        offset += 2;
    }
}

// Helper function to print device config ID name
const char* get_device_config_name(DeviceConfigId cfg_id) {
    switch(cfg_id) {
        case DEVICE_STATE: return "DEVICE_STATE";
        case LOW_POWER_MODE: return "LOW_POWER_MODE";
        default: return "UNKNOWN";
    }
}

// Helper function to interpret and print device state value
void print_device_state_value(unsigned char value) {
    switch(value) {
        case DEVICE_STATE_READY: printf("(READY)"); break;
        case DEVICE_STATE_ACTIVE: printf("(ACTIVE)"); break;
        case DEVICE_STATE_ERROR: printf("(ERROR)"); break;
        default: printf("(UNKNOWN: 0x%02X)", value); break;
    }
}

void handle_core_get_config_rsp(unsigned char* payload, int payload_len) {
    if (payload_len < 2) { // status (1) + num_tlvs (1)
        printf("Error: CORE_GET_CONFIG_RSP payload too short.\n");
        return;
    }

    unsigned char status = payload[0];
    unsigned char num_tlvs = payload[1];
    printf("  Status: 0x%02X\n", status);
    printf("  Number of TLVs: %d\n", num_tlvs);

    int offset = 2;
    for (int i = 0; i < num_tlvs; i++) {
        if (offset + 2 > payload_len) {
            printf("Error: Incomplete TLV in CORE_GET_CONFIG_RSP payload.\n");
            return;
        }
        DeviceConfigId cfg_id = (DeviceConfigId)payload[offset];
        unsigned char tlv_len = payload[offset + 1];
        offset += 2;
        if (offset + tlv_len > payload_len) {
            printf("Error: Incomplete TLV value in CORE_GET_CONFIG_RSP payload.\n");
            return;
        }
        
        printf("    Config ID: 0x%02X (%s), Length: %d, Value: ", cfg_id, 
               get_device_config_name(cfg_id), tlv_len);
        
        for (int j = 0; j < tlv_len; j++) {
            printf("%02X ", payload[offset + j]);
        }
        
        // Interpret the value based on config ID
        if (cfg_id == DEVICE_STATE && tlv_len == 1) {
            print_device_state_value(payload[offset]);
        } else if (cfg_id == LOW_POWER_MODE && tlv_len == 1) {
            unsigned char lpm_state = payload[offset];
            printf("(LPM: %s)", lpm_state ? "ON" : "OFF");
        }
        
        printf("\n");
        offset += tlv_len;
    }
}

// Helper function to initialize session storage
void init_uci_sessions() {
    for (int i = 0; i < MAX_SESSIONS; i++) {
        uci_sessions[i].is_allocated = 0;
        uci_sessions[i].session_state = SESSION_STATE_DEINIT;
        uci_sessions[i].num_configs = 0;
    }
}

// Helper function to find an available session slot
int find_free_session_slot() {
    for (int i = 0; i < MAX_SESSIONS; i++) {
        if (!uci_sessions[i].is_allocated) {
            return i;
        }
    }
    return -1; // No free slots
}

// Helper function to find a session by ID
int find_session_by_id(unsigned int session_id) {
    for (int i = 0; i < MAX_SESSIONS; i++) {
        if (uci_sessions[i].is_allocated && uci_sessions[i].session_id == session_id) {
            return i;
        }
    }
    return -1; // Session not found
}

// Helper function to store configuration value in session
void store_session_config(int session_idx, unsigned char cfg_id, unsigned char* value, unsigned char len) {
    if (session_idx < 0 || session_idx >= MAX_SESSIONS) return;
    if (uci_sessions[session_idx].num_configs >= MAX_SESSION_CONFIGS) return;
    
    // Store the configuration
    uci_sessions[session_idx].config_values[cfg_id] = (len > 0) ? value[0] : 0; // Simplified for single byte values
    uci_sessions[session_idx].config_lengths[cfg_id] = len;
    uci_sessions[session_idx].num_configs++;
}

// Helper function to get configuration value from session
int get_session_config(int session_idx, unsigned char cfg_id, unsigned char* value, unsigned char* len) {
    if (session_idx < 0 || session_idx >= MAX_SESSIONS) return 0;
    if (cfg_id >= 255) return 0;
    
    *len = uci_sessions[session_idx].config_lengths[cfg_id];
    if (*len > 0) {
        value[0] = uci_sessions[session_idx].config_values[cfg_id];
        return 1;
    }
    return 0;
}

void handle_core_device_status_ntf(unsigned char* payload, int payload_len) {
    if (payload_len < 1) {
        printf("Error: CORE_DEVICE_STATUS_NTF payload too short.\n");
        return;
    }

    unsigned char device_state = payload[0];
    printf("  Device State: 0x%02X\n", device_state);
}

void parse_uci_packet(unsigned char* packet, size_t packet_len) {
    if (packet_len < sizeof(struct uci_packet_header)) {
        printf("Error: UCI packet too short to contain a header.\n");
        return;
    }

    struct uci_packet_header* header = (struct uci_packet_header*)packet;

    printf("Received UCI packet:\n");
    printf("  MT: 0x%01X\n", get_mt(header));
    printf("  PBF: 0x%01X\n", get_pbf(header));
    printf("  GID: 0x%01X\n", get_gid(header));
    printf("  Opcode: 0x%02X\n", get_opcode(header));
    printf("  Payload Length: %d\n", header->payload_len);

    if (header->payload_len > 0) {
        printf("  Payload: ");
        for (int i = 0; i < header->payload_len; i++) {
            printf("%02X ", packet[sizeof(struct uci_packet_header) + i]);
        }
        printf("\n");
    }

    if (get_mt(header) == RESPONSE && get_gid(header) == CORE && get_opcode(header) == CORE_DEVICE_INFO) {
        handle_core_device_info_rsp(packet + sizeof(struct uci_packet_header), header->payload_len);
    } else if (get_mt(header) == NOTIFICATION && get_gid(header) == CORE && get_opcode(header) == CORE_DEVICE_STATUS_NTF) {
        handle_core_device_status_ntf(packet + sizeof(struct uci_packet_header), header->payload_len);
    } else if (get_mt(header) == RESPONSE && get_gid(header) == CORE && get_opcode(header) == CORE_GET_CAPS_INFO) {
        handle_core_get_caps_info_rsp(packet + sizeof(struct uci_packet_header), header->payload_len);
    } else if (get_mt(header) == RESPONSE && get_gid(header) == CORE && get_opcode(header) == CORE_SET_CONFIG) {
        handle_core_set_config_rsp(packet + sizeof(struct uci_packet_header), header->payload_len);
    } else if (get_mt(header) == RESPONSE && get_gid(header) == CORE && get_opcode(header) == CORE_GET_CONFIG) {
        handle_core_get_config_rsp(packet + sizeof(struct uci_packet_header), header->payload_len);
    }
}

