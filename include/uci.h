#ifndef UCI_H
#define UCI_H

#include "uci_pdl.h"

// UCI Packet Header - aligned with Android UWB specification
// For control packets (COMMAND, RESPONSE, NOTIFICATION):
// Byte 0: [GID:4][PBF:1][MT:3] = GID | (PBF << 4) | (MT << 5)
// Byte 1: [Opcode:6][R:2] = (Opcode << 2) 
// Byte 2: Reserved
// Byte 3: Payload Length
struct uci_packet_header {
    unsigned char first_byte;   // GID | (PBF << 4) | (MT << 5)
    unsigned char second_byte;  // (Opcode << 2) 
    unsigned char reserved2;    // Reserved
    unsigned char payload_len;  // Payload length
};

// UCI Session context structure for tracking session state
#define MAX_SESSIONS 10
#define MAX_SESSION_CONFIGS 20

struct uci_session {
    unsigned int session_id;
    SessionType session_type;
    unsigned char session_state;  // Using unsigned char to avoid direct dependency
    unsigned char is_allocated;  // 1 if session slot is in use, 0 otherwise
    unsigned char config_values[255];  // Store configuration values
    unsigned char config_lengths[255]; // Store configuration lengths
    int num_configs;            // Number of stored configurations
};

// Global session storage
extern struct uci_session uci_sessions[MAX_SESSIONS];

// Helper functions to properly set up the header
static inline void set_header_values(struct uci_packet_header *header, 
                                    unsigned char message_type, 
                                    unsigned char packet_boundary, 
                                    unsigned char group_id, 
                                    unsigned char opcode_id,
                                    unsigned char payload_length) {
    header->first_byte = group_id | (packet_boundary << 4) | (message_type << 5);
    header->second_byte = (opcode_id << 2);  // opcode shifted left 2 bits
    header->reserved2 = 0;
    header->payload_len = payload_length;
}

// Helper functions to extract header values
static inline unsigned char get_gid(const struct uci_packet_header *header) {
    return header->first_byte & 0x0F;
}

static inline unsigned char get_pbf(const struct uci_packet_header *header) {
    return (header->first_byte >> 4) & 0x01;
}

static inline unsigned char get_mt(const struct uci_packet_header *header) {
    return (header->first_byte >> 5) & 0x07;
}

static inline unsigned char get_opcode(const struct uci_packet_header *header) {
    return (header->second_byte >> 2) & 0x3F;
}

#endif // UCI_H
