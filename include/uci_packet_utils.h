#ifndef UCI_PACKET_UTILS_H
#define UCI_PACKET_UTILS_H

#include <stdint.h>
#include <stddef.h>
#include <string.h>
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

struct uci_payload_builder {
    unsigned char *buffer;
    size_t capacity;
    size_t length;
};

static inline void uci_payload_builder_init(struct uci_payload_builder *builder,
                                            unsigned char *buffer,
                                            size_t capacity) {
    builder->buffer = buffer;
    builder->capacity = capacity;
    builder->length = 0;
}

static inline size_t uci_payload_builder_length(const struct uci_payload_builder *builder) {
    return builder->length;
}

static inline int uci_payload_builder_reserve(struct uci_payload_builder *builder, size_t len) {
    if (builder->length + len > builder->capacity) {
        return -1;
    }
    return 0;
}

static inline int uci_payload_builder_put_u8(struct uci_payload_builder *builder, unsigned char value) {
    if (uci_payload_builder_reserve(builder, 1) < 0) {
        return -1;
    }
    builder->buffer[builder->length++] = value;
    return 0;
}

static inline int uci_payload_builder_put_mem(struct uci_payload_builder *builder,
                                              const unsigned char *data,
                                              size_t len) {
    if (uci_payload_builder_reserve(builder, len) < 0) {
        return -1;
    }
    if (len > 0) {
        memcpy(&builder->buffer[builder->length], data, len);
    }
    builder->length += len;
    return 0;
}

static inline int uci_payload_builder_put_u16_le(struct uci_payload_builder *builder, uint16_t value) {
    if (uci_payload_builder_reserve(builder, 2) < 0) {
        return -1;
    }
    write_u16_le(&builder->buffer[builder->length], value);
    builder->length += 2;
    return 0;
}

static inline int uci_payload_builder_put_u32_le(struct uci_payload_builder *builder, uint32_t value) {
    if (uci_payload_builder_reserve(builder, 4) < 0) {
        return -1;
    }
    write_u32_le(&builder->buffer[builder->length], value);
    builder->length += 4;
    return 0;
}

static inline int uci_payload_builder_put_u64_le(struct uci_payload_builder *builder, uint64_t value) {
    if (uci_payload_builder_reserve(builder, 8) < 0) {
        return -1;
    }
    write_u64_le(&builder->buffer[builder->length], value);
    builder->length += 8;
    return 0;
}

struct uci_tlv_reader {
    const unsigned char *buffer;
    size_t length;
    size_t offset;
};

static inline void uci_tlv_reader_init(struct uci_tlv_reader *reader,
                                       const unsigned char *buffer,
                                       size_t length) {
    reader->buffer = buffer;
    reader->length = length;
    reader->offset = 0;
}

static inline int uci_tlv_reader_next(struct uci_tlv_reader *reader,
                                      unsigned char *type,
                                      const unsigned char **value,
                                      unsigned char *len) {
    if (reader->offset >= reader->length) {
        return 0;
    }

    if (reader->length - reader->offset < 2) {
        return -1;
    }

    unsigned char tlv_type = reader->buffer[reader->offset++];
    unsigned char tlv_len = reader->buffer[reader->offset++];

    if (reader->length - reader->offset < tlv_len) {
        return -1;
    }

    if (type) {
        *type = tlv_type;
    }
    if (len) {
        *len = tlv_len;
    }
    if (value) {
        *value = &reader->buffer[reader->offset];
    }

    reader->offset += tlv_len;
    return 1;
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

size_t uci_build_data_message_snd_payload(unsigned char *buffer,
                                          size_t capacity,
                                          uint32_t session_identifier,
                                          uint64_t destination_address,
                                          uint16_t sequence_number,
                                          const unsigned char *app_data,
                                          size_t app_data_len);

const char* uci_device_state_to_string(unsigned char device_state);
const char* uci_status_to_string(unsigned char status);
const char* uci_status_description(unsigned char status);
const char* uci_session_state_to_string(unsigned char session_state);
const char* uci_session_reason_to_string(unsigned char reason_code);
const char* uci_session_type_to_string(unsigned char session_type);

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

/**
 * Create a get device info packet
 */
unsigned char* create_get_device_info_packet(
    size_t* packet_len
);

/**
 * Create a device reset packet
 */
unsigned char* create_device_reset_packet(
    uint8_t reset_config,
    size_t* packet_len
);

/**
 * Create a get caps info packet
 */
unsigned char* create_get_caps_info_packet(
    size_t* packet_len
);

/**
 * Create a set config packet
 */
unsigned char* create_set_config_packet(
    uint8_t num_configs,
    const unsigned char* configs,
    size_t configs_len,
    size_t* packet_len
);

/**
 * Create a get config packet
 */
unsigned char* create_get_config_packet(
    uint8_t num_configs,
    const unsigned char* config_ids,
    size_t config_ids_len,
    size_t* packet_len
);

#endif // UCI_PACKET_UTILS_H
