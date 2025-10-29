#ifndef UCI_UTILS_H
#define UCI_UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

// Forward declaration of uci_packet_header to break circular dependency
struct uci_packet_header;

// Safe memory allocation with error checking
static inline void* safe_malloc(size_t size) {
    if (size == 0) {
        return NULL;
    }
    
    void* ptr = malloc(size);
    if (ptr == NULL) {
        fprintf(stderr, "Error: Failed to allocate %zu bytes of memory\n", size);
        return NULL;
    }
    return ptr;
}

static inline void* safe_calloc(size_t count, size_t size) {
    if (count == 0 || size == 0) {
        return NULL;
    }
    
    void* ptr = calloc(count, size);
    if (ptr == NULL) {
        fprintf(stderr, "Error: Failed to allocate %zu elements of %zu bytes each\n", count, size);
        return NULL;
    }
    return ptr;
}

static inline void* safe_realloc(void* ptr, size_t size) {
    if (size == 0) {
        if (ptr != NULL) {
            free(ptr);
        }
        return NULL;
    }
    
    void* new_ptr = realloc(ptr, size);
    if (new_ptr == NULL) {
        fprintf(stderr, "Error: Failed to reallocate %zu bytes of memory\n", size);
        if (ptr != NULL) {
            free(ptr);  // Free original pointer if realloc failed
        }
        return NULL;
    }
    return new_ptr;
}

// Safe string copy with bounds checking
static inline int safe_strcpy(char* dest, size_t dest_size, const char* src) {
    if (dest == NULL || dest_size == 0 || src == NULL) {
        return -1;  // Error: invalid parameters
    }
    
    size_t src_len = strlen(src);
    if (src_len >= dest_size) {
        // String is too long to fit in destination
        return -1;
    }
    
    strncpy(dest, src, dest_size - 1);
    dest[dest_size - 1] = '\0';  // Ensure null termination
    return 0;  // Success
}

// Safe string copy with truncation if needed
static inline int safe_strcpy_trunc(char* dest, size_t dest_size, const char* src) {
    if (dest == NULL || dest_size == 0 || src == NULL) {
        return -1;  // Error: invalid parameters
    }
    
    strncpy(dest, src, dest_size - 1);
    dest[dest_size - 1] = '\0';  // Ensure null termination
    return 0;  // Success
}

// Safe memory copy with bounds checking
static inline int safe_memcpy(void* dest, size_t dest_size, const void* src, size_t src_size) {
    if (dest == NULL || src == NULL) {
        return -1;  // Error: invalid parameters
    }
    
    if (src_size > dest_size) {
        return -1;  // Error: source too large for destination
    }
    
    memcpy(dest, src, src_size);
    return 0;  // Success
}

// Safe memory copy that ensures no buffer overflow
static inline int safe_memcpy_safe(void* dest, size_t dest_size, const void* src, size_t src_size) {
    if (dest == NULL || src == NULL) {
        return -1;  // Error: invalid parameters
    }
    
    size_t copy_size = (src_size < dest_size) ? src_size : dest_size;
    memcpy(dest, src, copy_size);
    return 0;  // Success
}

// Safe read from packet with bounds checking
static inline int safe_read_bytes(const unsigned char* src, size_t src_len, size_t offset, 
                                  unsigned char* dest, size_t count) {
    if (!src || !dest || offset > src_len || count > src_len - offset) {
        return -1;  // Error: invalid parameters or buffer overflow
    }
    
    memcpy(dest, src + offset, count);
    return 0;  // Success
}

// Safe little-endian integer reading with bounds checking
static inline int safe_read_u16_le(const unsigned char* buffer, size_t buffer_len, size_t offset, uint16_t* result) {
    if (!buffer || !result || offset + sizeof(uint16_t) > buffer_len) {
        return -1;  // Error: invalid parameters or buffer overflow
    }
    
    *result = (uint16_t)buffer[offset] | ((uint16_t)buffer[offset + 1] << 8);
    return 0;  // Success
}

static inline int safe_read_u32_le(const unsigned char* buffer, size_t buffer_len, size_t offset, uint32_t* result) {
    if (!buffer || !result || offset + sizeof(uint32_t) > buffer_len) {
        return -1;  // Error: invalid parameters or buffer overflow
    }
    
    *result = (uint32_t)buffer[offset] | 
              ((uint32_t)buffer[offset + 1] << 8) |
              ((uint32_t)buffer[offset + 2] << 16) | 
              ((uint32_t)buffer[offset + 3] << 24);
    return 0;  // Success
}

static inline int safe_read_u64_le(const unsigned char* buffer, size_t buffer_len, size_t offset, uint64_t* result) {
    if (!buffer || !result || offset + sizeof(uint64_t) > buffer_len) {
        return -1;  // Error: invalid parameters or buffer overflow
    }
    
    *result = (uint64_t)buffer[offset] | 
              ((uint64_t)buffer[offset + 1] << 8) |
              ((uint64_t)buffer[offset + 2] << 16) | 
              ((uint64_t)buffer[offset + 3] << 24) |
              ((uint64_t)buffer[offset + 4] << 32) | 
              ((uint64_t)buffer[offset + 5] << 40) |
              ((uint64_t)buffer[offset + 6] << 48) | 
              ((uint64_t)buffer[offset + 7] << 56);
    return 0;  // Success
}

// Safe little-endian integer writing with bounds checking
static inline int safe_write_u16_le(unsigned char* buffer, size_t buffer_len, size_t offset, uint16_t value) {
    if (!buffer || offset + sizeof(uint16_t) > buffer_len) {
        return -1;  // Error: invalid parameters or buffer overflow
    }
    
    buffer[offset] = value & 0xFF;
    buffer[offset + 1] = (value >> 8) & 0xFF;
    return 0;  // Success
}

static inline int safe_write_u32_le(unsigned char* buffer, size_t buffer_len, size_t offset, uint32_t value) {
    if (!buffer || offset + sizeof(uint32_t) > buffer_len) {
        return -1;  // Error: invalid parameters or buffer overflow
    }
    
    buffer[offset] = value & 0xFF;
    buffer[offset + 1] = (value >> 8) & 0xFF;
    buffer[offset + 2] = (value >> 16) & 0xFF;
    buffer[offset + 3] = (value >> 24) & 0xFF;
    return 0;  // Success
}

static inline int safe_write_u64_le(unsigned char* buffer, size_t buffer_len, size_t offset, uint64_t value) {
    if (!buffer || offset + sizeof(uint64_t) > buffer_len) {
        return -1;  // Error: invalid parameters or buffer overflow
    }
    
    for (int i = 0; i < 8; i++) {
        buffer[offset + i] = (value >> (i * 8)) & 0xFF;
    }
    return 0;  // Success
}

// UCI error codes for consistent error handling
typedef enum {
    UCI_SUCCESS = 0,
    UCI_ERROR_INVALID_PARAM,
    UCI_ERROR_BUFFER_OVERFLOW,
    UCI_ERROR_OUT_OF_MEMORY,
    UCI_ERROR_MALFORMED_PACKET,
    UCI_ERROR_UNSUPPORTED_OPERATION,
    UCI_ERROR_RESOURCE_EXHAUSTED
} uci_error_t;

// Validation functions
static inline uci_error_t validate_uci_packet(const unsigned char* packet, size_t len) {
    // Size of uci_packet_header is 4 bytes
    const size_t header_size = 4;
    
    if (!packet || len < header_size) {
        return UCI_ERROR_INVALID_PARAM;
    }
    
    // Extract header information
    uint8_t mt = (packet[0] >> 5) & 0x7;
    uint8_t gid = packet[0] & 0xF;
    uint8_t payload_len = packet[3];
    
    // Validate header fields
    if (mt > 3 || gid > 15) {
        return UCI_ERROR_MALFORMED_PACKET;
    }
    
    // Validate payload length
    if (payload_len > (len - header_size)) {
        return UCI_ERROR_MALFORMED_PACKET;
    }
    
    return UCI_SUCCESS;
}

#endif // UCI_UTILS_H