#ifndef UCI_PACKET_UTILS_H
#define UCI_PACKET_UTILS_H

#include <stdint.h>
#include <stddef.h>
#include "uci.h"

// Utility functions for creating UCI packets with correct endianness

/**
 * Write a 16-bit value in little-endian format to buffer
 */
static inline void write_u16_le(unsigned char* buffer, uint16_t value) {
    buffer[0] = value & 0xFF;
    buffer[1] = (value >> 8) & 0xFF;
}

/**
 * Write a 32-bit value in little-endian format to buffer
 */
static inline void write_u32_le(unsigned char* buffer, uint32_t value) {
    buffer[0] = value & 0xFF;
    buffer[1] = (value >> 8) & 0xFF;
    buffer[2] = (value >> 16) & 0xFF;
    buffer[3] = (value >> 24) & 0xFF;
}

/**
 * Write a 64-bit value in little-endian format to buffer
 */
static inline void write_u64_le(unsigned char* buffer, uint64_t value) {
    buffer[0] = value & 0xFF;
    buffer[1] = (value >> 8) & 0xFF;
    buffer[2] = (value >> 16) & 0xFF;
    buffer[3] = (value >> 24) & 0xFF;
    buffer[4] = (value >> 32) & 0xFF;
    buffer[5] = (value >> 40) & 0xFF;
    buffer[6] = (value >> 48) & 0xFF;
    buffer[7] = (value >> 56) & 0xFF;
}

/**
 * Read a 16-bit value in little-endian format from buffer
 */
static inline uint16_t read_u16_le(const unsigned char* buffer) {
    return (uint16_t)buffer[0] | ((uint16_t)buffer[1] << 8);
}

/**
 * Read a 32-bit value in little-endian format from buffer
 */
static inline uint32_t read_u32_le(const unsigned char* buffer) {
    return (uint32_t)buffer[0] | ((uint32_t)buffer[1] << 8) |
           ((uint32_t)buffer[2] << 16) | ((uint32_t)buffer[3] << 24);
}

/**
 * Read a 64-bit value in little-endian format from buffer
 */
static inline uint64_t read_u64_le(const unsigned char* buffer) {
    return (uint64_t)buffer[0] | ((uint64_t)buffer[1] << 8) |
           ((uint64_t)buffer[2] << 16) | ((uint64_t)buffer[3] << 24) |
           ((uint64_t)buffer[4] << 32) | ((uint64_t)buffer[5] << 40) |
           ((uint64_t)buffer[6] << 48) | ((uint64_t)buffer[7] << 56);
}

/**
 * Create a complete UCI packet with header and payload
 * Returns dynamically allocated packet - caller must free()
 */
unsigned char* create_uci_packet(
    unsigned char mt, 
    unsigned char pbf, 
    unsigned char gid, 
    unsigned char oid,
    const unsigned char* payload,
    size_t payload_len,
    size_t* packet_len
);

/**
 * Create a UCI header with proper field packing
 */
void create_uci_header(
    struct uci_packet_header* header,
    unsigned char mt,
    unsigned char pbf,
    unsigned char gid,
    unsigned char oid,
    unsigned char payload_len
);

/**
 * Convenience functions for common UCI packet types
 */

/**
 * Create a session init packet
 */
unsigned char* create_session_init_packet(
    uint32_t session_id,
    unsigned char session_type,
    size_t* packet_len
);

/**
 * Create a session deinit packet
 */
unsigned char* create_session_deinit_packet(
    uint32_t session_id,
    size_t* packet_len
);

/**
 * Create a session start packet
 */
unsigned char* create_session_start_packet(
    uint32_t session_id,
    size_t* packet_len
);

/**
 * Create a session stop packet
 */
unsigned char* create_session_stop_packet(
    uint32_t session_id,
    size_t* packet_len
);

/**
 * Create a get session state packet
 */
unsigned char* create_get_session_state_packet(
    uint32_t session_id,
    size_t* packet_len
);

#endif // UCI_PACKET_UTILS_H