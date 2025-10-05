#ifndef UCI_H
#define UCI_H

#include "uci_pdl.h"
#include <stddef.h>

// UCI Packet Header - aligned with Android UWB specification
// For control packets (COMMAND, RESPONSE, NOTIFICATION):
// Byte 0: [GID:4][PBF:1][MT:3] where GID occupies the least significant bits
// Byte 1: [Opcode:6][R:2] where Opcode occupies the least significant bits
// Byte 2: Reserved
// Byte 3: Payload Length
struct uci_packet_header {
    unsigned char first_byte;   // GID | (PBF << 4) | (MT << 5)
    unsigned char second_byte;  // Opcode in bits[5:0], reserved bits[7:6]
    unsigned char reserved2;    // Reserved
    unsigned char payload_len;  // Payload length
};

// UCI Session context structure for tracking session state
#define MAX_SESSIONS 10
#define MAX_SESSION_CONFIGS 32
#define MAX_SESSION_CONFIG_VALUE_SIZE 255

typedef struct {
    unsigned char cfg_id;
    unsigned char length;
    unsigned char value[MAX_SESSION_CONFIG_VALUE_SIZE];
    unsigned char in_use;
} uci_session_config_entry;

struct uci_session {
    unsigned int session_id;
    SessionType session_type;
    unsigned char session_state;  // Using unsigned char to avoid direct dependency
    unsigned char is_allocated;  // 1 if session slot is in use, 0 otherwise
    unsigned int session_handle;  // Simulated UWBS-generated session handle
    unsigned short ranging_count; // Tracks completed ranging rounds
    uci_session_config_entry configs[MAX_SESSION_CONFIGS];
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
    header->second_byte = (opcode_id & 0x3F);  // opcode occupies lower 6 bits
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
    return header->second_byte & 0x3F;
}

static inline unsigned char get_reserved_opcode_bits(const struct uci_packet_header *header) {
    return (header->second_byte >> 6) & 0x03;
}

// UCI packet analysis function
void analyze_uci_packet(unsigned char* packet, size_t packet_len);

// Payload decoding functions
// CORE group decoders
void decode_core_device_info_rsp(unsigned char* payload, int payload_len);
void decode_core_get_caps_info_rsp(unsigned char* payload, int payload_len);
void decode_core_set_config_rsp(unsigned char* payload, int payload_len);
void decode_core_get_config_rsp(unsigned char* payload, int payload_len);
void decode_core_device_reset_rsp(unsigned char* payload, int payload_len);
void decode_core_device_suspend_rsp(unsigned char* payload, int payload_len);

// SESSION_CONFIG group decoders
void decode_session_init_rsp(unsigned char* payload, int payload_len);
void decode_session_deinit_rsp(unsigned char* payload, int payload_len);
void decode_session_set_app_config_rsp(unsigned char* payload, int payload_len);
void decode_session_get_app_config_rsp(unsigned char* payload, int payload_len);
void decode_session_get_count_rsp(unsigned char* payload, int payload_len);
void decode_session_get_state_rsp(unsigned char* payload, int payload_len);

// SESSION_CONTROL group decoders
void decode_session_start_rsp(unsigned char* payload, int payload_len);
void decode_session_stop_rsp(unsigned char* payload, int payload_len);
void decode_session_get_ranging_count_rsp(unsigned char* payload, int payload_len);

// CORE notification decoders
void decode_core_device_status_ntf(unsigned char* payload, int payload_len);
void decode_core_generic_error_ntf(unsigned char* payload, int payload_len);

// SESSION_CONFIG notification decoders
void decode_session_status_ntf(unsigned char* payload, int payload_len);

// SESSION_CONTROL notification decoders
void decode_session_info_ntf(unsigned char* payload, int payload_len);
void decode_session_data_credit_ntf(unsigned char* payload, int payload_len);
void decode_session_data_transfer_status_ntf(unsigned char* payload, int payload_len);

#endif // UCI_H
