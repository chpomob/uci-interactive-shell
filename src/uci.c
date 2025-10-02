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
    // In a real implementation, this would involve reading from a UWB device
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
}

