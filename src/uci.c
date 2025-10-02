#include <stdio.h>
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

