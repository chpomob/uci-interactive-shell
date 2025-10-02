#include <stdio.h>
#include <string.h>
#include "uci.h"
#include "uci_functions.h"

void send_uci_command(unsigned char mt, unsigned char pbf, unsigned char gid, unsigned char oid, unsigned char* payload, int payload_len) {
    struct uci_packet_header header;
    header.mt = mt;
    header.pbf = pbf;
    header.gid = gid;
    header.oid = oid;
    header.payload_len = payload_len;

    printf("Sending UCI packet:\n");
    printf("  Header: %02X %02X %02X\n", *(unsigned char*)&header, header.oid, header.payload_len);
    printf("  Payload: ");
    for (int i = 0; i < payload_len; i++) {
        printf("%02X ", payload[i]);
    }
    printf("\n");

    // Simulate receiving a response
    printf("Simulating UCI response...\n");
    unsigned char response_packet[sizeof(struct uci_packet_header) + 1];
    struct uci_packet_header* response_header = (struct uci_packet_header*)response_packet;
    response_header->mt = RESPONSE;
    response_header->pbf = COMPLETE;
    response_header->gid = gid;
    response_header->oid = oid;
    response_header->payload_len = 1;
    response_packet[sizeof(struct uci_packet_header)] = UCI_STATUS_OK;

    if (gid == CORE && oid == CORE_GET_CAPS_INFO) {
        // Simulate a CORE_GET_CAPS_INFO_RSP
        unsigned char caps_rsp_payload[] = {UCI_STATUS_OK, 0x01, SUPPORTED_V1_FIRA_PHY_VERSION_RANGE_V2_MAX_MESSAGE_SIZE, 0x02, 0x01, 0x00};
        memcpy(response_packet + sizeof(struct uci_packet_header), caps_rsp_payload, sizeof(caps_rsp_payload));
        response_header->payload_len = sizeof(caps_rsp_payload);
    } else {
        response_header->payload_len = 1;
        response_packet[sizeof(struct uci_packet_header)] = UCI_STATUS_OK;
    }
    parse_uci_packet(response_packet, sizeof(struct uci_packet_header) + response_header->payload_len);
}

void handle_core_device_info_rsp(unsigned char* payload, int payload_len) {
    if (payload_len < 7) { // status (1) + uci_version (2) + mac_version (2) + phy_version (2)
        printf("Error: CORE_DEVICE_INFO_RSP payload too short.\n");
        return;
    }

    unsigned char status = payload[0];
    unsigned short uci_version = (payload[1] << 8) | payload[2];
    unsigned short mac_version = (payload[3] << 8) | payload[4];
    unsigned short phy_version = (payload[5] << 8) | payload[6];

    printf("  Status: 0x%02X\n", status);
    printf("  UCI Version: 0x%04X\n", uci_version);
    printf("  MAC Version: 0x%04X\n", mac_version);
    printf("  PHY Version: 0x%04X\n", phy_version);
}

void handle_core_device_status_ntf(unsigned char* payload, int payload_len) {
    if (payload_len < 1) {
        printf("Error: CORE_DEVICE_STATUS_NTF payload too short.\n");
        return;
    }

    unsigned char device_state = payload[0];
    printf("  Device State: 0x%02X\n", device_state);
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

void parse_uci_packet(unsigned char* packet, int packet_len) {
    if (packet_len < sizeof(struct uci_packet_header)) {
        printf("Error: UCI packet too short to contain a header.\n");
        return;
    }

    struct uci_packet_header* header = (struct uci_packet_header*)packet;

    printf("Received UCI packet:\n");
    printf("  MT: 0x%01X\n", header->mt);
    printf("  PBF: 0x%01X\n", header->pbf);
    printf("  GID: 0x%01X\n", header->gid);
    printf("  OID: 0x%02X\n", header->oid);
    printf("  Payload Length: %d\n", header->payload_len);

    if (header->payload_len > 0) {
        printf("  Payload: ");
        for (int i = 0; i < header->payload_len; i++) {
            printf("%02X ", packet[sizeof(struct uci_packet_header) + i]);
        }
        printf("\n");
    }

    if (header->mt == RESPONSE && header->gid == CORE && header->oid == CORE_DEVICE_INFO) {
        handle_core_device_info_rsp(packet + sizeof(struct uci_packet_header), header->payload_len);
    } else if (header->mt == NOTIFICATION && header->gid == CORE && header->oid == CORE_DEVICE_STATUS_NTF) {
        handle_core_device_status_ntf(packet + sizeof(struct uci_packet_header), header->payload_len);
    } else if (header->mt == RESPONSE && header->gid == CORE && header->oid == CORE_GET_CAPS_INFO) {
        handle_core_get_caps_info_rsp(packet + sizeof(struct uci_packet_header), header->payload_len);
    }
}

