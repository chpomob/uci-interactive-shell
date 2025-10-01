#ifndef UCI_H
#define UCI_H

#include "uci_pdl.h"

// UCI Packet Header
struct uci_packet_header {
    unsigned char mt : 3;
    unsigned char pbf : 1;
    unsigned char gid : 4;
    unsigned char oid : 8;
    unsigned char payload_len;
};

#endif // UCI_H
