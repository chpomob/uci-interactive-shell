#include <stdio.h>
#include <string.h>
#include "uci.h"
#include "uci_functions.h"

void send_uci_command(unsigned char mt, unsigned char pbf, unsigned char gid, unsigned char oid, unsigned char* payload, int payload_len) {
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
        // Simulate a CORE_SET_CONFIG_RSP
        unsigned char set_config_rsp_payload[] = {UCI_STATUS_OK, 0x01, DEVICE_STATE, UCI_STATUS_OK};
        memcpy(response_packet + sizeof(struct uci_packet_header), set_config_rsp_payload, sizeof(set_config_rsp_payload));
        response_header->payload_len = sizeof(set_config_rsp_payload);
    } else if (gid == CORE && oid == CORE_GET_CONFIG) {
        // Simulate a CORE_GET_CONFIG_RSP
        unsigned char get_config_rsp_payload[] = {UCI_STATUS_OK, 0x01, DEVICE_STATE, 0x01, DEVICE_STATE_ACTIVE};
        memcpy(response_packet + sizeof(struct uci_packet_header), get_config_rsp_payload, sizeof(get_config_rsp_payload));
        response_header->payload_len = sizeof(get_config_rsp_payload);
    } else if (gid == CORE && oid == CORE_DEVICE_RESET) {
        // Simulate a CORE_DEVICE_RESET_RSP
        unsigned char reset_rsp_payload[] = {UCI_STATUS_OK};
        memcpy(response_packet + sizeof(struct uci_packet_header), reset_rsp_payload, sizeof(reset_rsp_payload));
        response_header->payload_len = sizeof(reset_rsp_payload);
    } else if (gid == SESSION_CONFIG && oid == SESSION_INIT) {
        // Simulate a SESSION_INIT_RSP (FIRA v2.0 format with session handle)
        unsigned char session_init_rsp_payload[] = {UCI_STATUS_OK, 0x01, 0x02, 0x03, 0x04}; // status + session_handle (4 bytes)
        memcpy(response_packet + sizeof(struct uci_packet_header), session_init_rsp_payload, sizeof(session_init_rsp_payload));
        response_header->payload_len = sizeof(session_init_rsp_payload);
    } else if (gid == SESSION_CONFIG && oid == SESSION_DEINIT) {
        // Simulate a SESSION_DEINIT_RSP
        unsigned char session_deinit_rsp_payload[] = {UCI_STATUS_OK};
        memcpy(response_packet + sizeof(struct uci_packet_header), session_deinit_rsp_payload, sizeof(session_deinit_rsp_payload));
        response_header->payload_len = sizeof(session_deinit_rsp_payload);
    } else if (gid == SESSION_CONTROL && oid == SESSION_START) {
        // Simulate a SESSION_START_RSP
        unsigned char session_start_rsp_payload[] = {UCI_STATUS_OK};
        memcpy(response_packet + sizeof(struct uci_packet_header), session_start_rsp_payload, sizeof(session_start_rsp_payload));
        response_header->payload_len = sizeof(session_start_rsp_payload);
    } else if (gid == SESSION_CONTROL && oid == SESSION_STOP) {
        // Simulate a SESSION_STOP_RSP
        unsigned char session_stop_rsp_payload[] = {UCI_STATUS_OK};
        memcpy(response_packet + sizeof(struct uci_packet_header), session_stop_rsp_payload, sizeof(session_stop_rsp_payload));
        response_header->payload_len = sizeof(session_stop_rsp_payload);
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
               cfg_id == DEVICE_STATE ? "DEVICE_STATE" : 
               cfg_id == LOW_POWER_MODE ? "LOW_POWER_MODE" : "UNKNOWN", 
               tlv_len);
        
        for (int j = 0; j < tlv_len; j++) {
            printf("%02X ", payload[offset + j]);
        }
        
        // Interpret the value based on config ID
        if (cfg_id == DEVICE_STATE && tlv_len == 1) {
            unsigned char device_state = payload[offset];
            if (device_state == DEVICE_STATE_READY) {
                printf("(READY)");
            } else if (device_state == DEVICE_STATE_ACTIVE) {
                printf("(ACTIVE)");
            } else if (device_state == DEVICE_STATE_ERROR) {
                printf("(ERROR)");
            } else {
                printf("(UNKNOWN_STATE)");
            }
        } else if (cfg_id == LOW_POWER_MODE && tlv_len == 1) {
            unsigned char lpm_state = payload[offset];
            printf("(LPM: %s)", lpm_state ? "ON" : "OFF");
        }
        
        printf("\n");
        offset += tlv_len;
    }
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

