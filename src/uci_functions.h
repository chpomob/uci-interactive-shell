#ifndef UCI_FUNCTIONS_H
#define UCI_FUNCTIONS_H

#include "uci.h"

void send_uci_command(unsigned char mt, unsigned char pbf, unsigned char gid, unsigned char oid, unsigned char* payload, int payload_len);
void parse_uci_packet(unsigned char* packet, int packet_len);

#endif // UCI_FUNCTIONS_H
