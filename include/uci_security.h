#ifndef UCI_SECURITY_H
#define UCI_SECURITY_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>

// Forward declaration of uci_packet_header
struct uci_packet_header {
    unsigned char first_byte;   // GID | (PBF << 4) | (MT << 5)
    unsigned char second_byte;  // Opcode in bits[5:0], reserved bits[7:6]
    unsigned char reserved2;    // Reserved
    unsigned char payload_len;  // Payload length
};

// Error codes for UCI security functions
typedef enum {
    UCI_SEC_SUCCESS = 0,
    UCI_SEC_ERROR_INVALID_PARAM,
    UCI_SEC_ERROR_BUFFER_OVERFLOW,
    UCI_SEC_ERROR_OUT_OF_MEMORY,
    UCI_SEC_ERROR_MALFORMED_PACKET,
    UCI_SEC_ERROR_UNSUPPORTED_OPERATION,
    UCI_SEC_ERROR_RESOURCE_EXHAUSTED,
    UCI_SEC_ERROR_ACCESS_DENIED,
    UCI_SEC_ERROR_CRYPTO_FAILURE
} uci_security_error_t;

// Secure memory allocation with bounds checking
static inline void* uci_sec_malloc(size_t size) {
    // Check for integer overflow
    if (size == 0 || size > SIZE_MAX / 2) {
        return NULL;
    }
    
    void* ptr = malloc(size);
    if (ptr == NULL) {
        fprintf(stderr, "Error: Failed to allocate %zu bytes of memory\n", size);
        return NULL;
    }
    
    // Zero-initialize memory for security
    memset(ptr, 0, size);
    return ptr;
}

static inline void* uci_sec_calloc(size_t count, size_t size) {
    // Check for integer overflow
    if (count == 0 || size == 0 || count > SIZE_MAX / size) {
        return NULL;
    }
    
    void* ptr = calloc(count, size);
    if (ptr == NULL) {
        fprintf(stderr, "Error: Failed to allocate %zu elements of %zu bytes each\n", count, size);
        return NULL;
    }
    return ptr;
}

static inline void* uci_sec_realloc(void* ptr, size_t size) {
    // Check for integer overflow
    if (size > SIZE_MAX / 2) {
        free(ptr);
        return NULL;
    }
    
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

// Secure memory deallocation
static inline void uci_sec_free(void** ptr) {
    if (ptr && *ptr) {
        // Securely wipe memory before freeing
        // Note: malloc_usable_size is platform-specific, so we'll skip it for portability
        // In a real implementation, you might use explicit size tracking
        volatile unsigned char* p = (volatile unsigned char*)(*ptr);
        // Without knowing the exact size, we'll zero what we can reasonably expect
        // This is a compromise for portability
        for (size_t i = 0; i < 4096; i++) {  // Zero up to 4KB
            p[i] = 0;
        }
        free(*ptr);
        *ptr = NULL;
    }
}

// Safe string copy with bounds checking
static inline uci_security_error_t uci_sec_strcpy(char* dest, size_t dest_size, const char* src) {
    if (!dest || dest_size == 0 || !src) {
        return UCI_SEC_ERROR_INVALID_PARAM;
    }
    
    size_t src_len = strlen(src);
    if (src_len >= dest_size) {
        // String is too long to fit in destination
        return UCI_SEC_ERROR_BUFFER_OVERFLOW;
    }
    
    strcpy(dest, src);
    return UCI_SEC_SUCCESS;
}

// Safe string copy with truncation if needed
static inline uci_security_error_t uci_sec_strcpy_trunc(char* dest, size_t dest_size, const char* src) {
    if (!dest || dest_size == 0 || !src) {
        return UCI_SEC_ERROR_INVALID_PARAM;
    }
    
    strncpy(dest, src, dest_size - 1);
    dest[dest_size - 1] = '\0';  // Ensure null termination
    return UCI_SEC_SUCCESS;
}

// Safe memory copy with bounds checking
static inline uci_security_error_t uci_sec_memcpy(void* dest, size_t dest_size, const void* src, size_t src_size) {
    if (!dest || !src) {
        return UCI_SEC_ERROR_INVALID_PARAM;
    }
    
    if (src_size > dest_size) {
        return UCI_SEC_ERROR_BUFFER_OVERFLOW;
    }
    
    memcpy(dest, src, src_size);
    return UCI_SEC_SUCCESS;
}

// Safe memory copy that ensures no buffer overflow
static inline uci_security_error_t uci_sec_memcpy_safe(void* dest, size_t dest_size, const void* src, size_t src_size) {
    if (!dest || !src) {
        return UCI_SEC_ERROR_INVALID_PARAM;
    }
    
    size_t copy_size = (src_size < dest_size) ? src_size : dest_size;
    memcpy(dest, src, copy_size);
    return UCI_SEC_SUCCESS;
}

// Secure memory comparison (constant time to prevent timing attacks)
static inline bool uci_sec_memcmp_consttime(const void* a, const void* b, size_t len) {
    if (!a || !b) {
        return false;
    }
    
    const unsigned char* pa = (const unsigned char*)a;
    const unsigned char* pb = (const unsigned char*)b;
    unsigned char result = 0;
    
    for (size_t i = 0; i < len; i++) {
        result |= pa[i] ^ pb[i];
    }
    
    return result == 0;
}

// Safe read from packet with bounds checking
static inline uci_security_error_t uci_sec_read_bytes(const unsigned char* src, size_t src_len, size_t offset, 
                                                       unsigned char* dest, size_t count) {
    if (!src || !dest) {
        return UCI_SEC_ERROR_INVALID_PARAM;
    }
    
    if (offset > src_len || count > src_len - offset) {
        return UCI_SEC_ERROR_BUFFER_OVERFLOW;
    }
    
    memcpy(dest, src + offset, count);
    return UCI_SEC_SUCCESS;
}

// Safe little-endian integer reading with bounds checking
static inline uci_security_error_t uci_sec_read_u16_le(const unsigned char* buffer, size_t buffer_len, size_t offset, uint16_t* result) {
    if (!buffer || !result) {
        return UCI_SEC_ERROR_INVALID_PARAM;
    }
    
    if (offset + sizeof(uint16_t) > buffer_len) {
        return UCI_SEC_ERROR_BUFFER_OVERFLOW;
    }
    
    *result = (uint16_t)buffer[offset] | ((uint16_t)buffer[offset + 1] << 8);
    return UCI_SEC_SUCCESS;
}

static inline uci_security_error_t uci_sec_read_u32_le(const unsigned char* buffer, size_t buffer_len, size_t offset, uint32_t* result) {
    if (!buffer || !result) {
        return UCI_SEC_ERROR_INVALID_PARAM;
    }
    
    if (offset + sizeof(uint32_t) > buffer_len) {
        return UCI_SEC_ERROR_BUFFER_OVERFLOW;
    }
    
    *result = (uint32_t)buffer[offset] | 
              ((uint32_t)buffer[offset + 1] << 8) |
              ((uint32_t)buffer[offset + 2] << 16) | 
              ((uint32_t)buffer[offset + 3] << 24);
    return UCI_SEC_SUCCESS;
}

static inline uci_security_error_t uci_sec_read_u64_le(const unsigned char* buffer, size_t buffer_len, size_t offset, uint64_t* result) {
    if (!buffer || !result) {
        return UCI_SEC_ERROR_INVALID_PARAM;
    }
    
    if (offset + sizeof(uint64_t) > buffer_len) {
        return UCI_SEC_ERROR_BUFFER_OVERFLOW;
    }
    
    *result = (uint64_t)buffer[offset] | 
              ((uint64_t)buffer[offset + 1] << 8) |
              ((uint64_t)buffer[offset + 2] << 16) | 
              ((uint64_t)buffer[offset + 3] << 24) |
              ((uint64_t)buffer[offset + 4] << 32) | 
              ((uint64_t)buffer[offset + 5] << 40) |
              ((uint64_t)buffer[offset + 6] << 48) | 
              ((uint64_t)buffer[offset + 7] << 56);
    return UCI_SEC_SUCCESS;
}

// Safe little-endian integer writing with bounds checking
static inline uci_security_error_t uci_sec_write_u16_le(unsigned char* buffer, size_t buffer_len, size_t offset, uint16_t value) {
    if (!buffer) {
        return UCI_SEC_ERROR_INVALID_PARAM;
    }
    
    if (offset + sizeof(uint16_t) > buffer_len) {
        return UCI_SEC_ERROR_BUFFER_OVERFLOW;
    }
    
    buffer[offset] = value & 0xFF;
    buffer[offset + 1] = (value >> 8) & 0xFF;
    return UCI_SEC_SUCCESS;
}

static inline uci_security_error_t uci_sec_write_u32_le(unsigned char* buffer, size_t buffer_len, size_t offset, uint32_t value) {
    if (!buffer) {
        return UCI_SEC_ERROR_INVALID_PARAM;
    }
    
    if (offset + sizeof(uint32_t) > buffer_len) {
        return UCI_SEC_ERROR_BUFFER_OVERFLOW;
    }
    
    buffer[offset] = value & 0xFF;
    buffer[offset + 1] = (value >> 8) & 0xFF;
    buffer[offset + 2] = (value >> 16) & 0xFF;
    buffer[offset + 3] = (value >> 24) & 0xFF;
    return UCI_SEC_SUCCESS;
}

static inline uci_security_error_t uci_sec_write_u64_le(unsigned char* buffer, size_t buffer_len, size_t offset, uint64_t value) {
    if (!buffer) {
        return UCI_SEC_ERROR_INVALID_PARAM;
    }
    
    if (offset + sizeof(uint64_t) > buffer_len) {
        return UCI_SEC_ERROR_BUFFER_OVERFLOW;
    }
    
    for (int i = 0; i < 8; i++) {
        buffer[offset + i] = (value >> (i * 8)) & 0xFF;
    }
    return UCI_SEC_SUCCESS;
}

// Packet validation functions
static inline uci_security_error_t uci_sec_validate_packet_header(const unsigned char* packet, size_t len) {
    if (!packet || len < sizeof(struct uci_packet_header)) {
        return UCI_SEC_ERROR_INVALID_PARAM;
    }
    
    // Extract header information
    uint8_t mt = (packet[0] >> 5) & 0x7;
    uint8_t gid = packet[0] & 0xF;
    uint8_t payload_len = packet[3];
    
    // Validate header fields
    if (mt > 3 || gid > 15) {
        return UCI_SEC_ERROR_MALFORMED_PACKET;
    }
    
    // Validate payload length
    if (payload_len > (len - sizeof(struct uci_packet_header))) {
        return UCI_SEC_ERROR_MALFORMED_PACKET;
    }
    
    return UCI_SEC_SUCCESS;
}

// Input sanitization functions
static inline uci_security_error_t uci_sec_sanitize_string_input(char* input, size_t max_len) {
    if (!input || max_len == 0) {
        return UCI_SEC_ERROR_INVALID_PARAM;
    }
    
    // Remove potentially dangerous characters
    for (size_t i = 0; i < max_len && input[i] != '\0'; i++) {
        switch (input[i]) {
            case ';':
            case '&':
            case '|':
            case '`':
            case '$':
            case '(':
            case ')':
            case '{':
            case '}':
            case '[':
            case ']':
            case '<':
            case '>':
            case '"':
            case '\'':
            case '\\':
                input[i] = '_'; // Replace with safe character
                break;
            default:
                break;
        }
    }
    
    return UCI_SEC_SUCCESS;
}

// Secure random number generation (placeholder - should use OS-specific secure RNG)
static inline uint32_t uci_sec_random_uint32(void) {
    // In a real implementation, this should use a cryptographically secure
    // random number generator like /dev/urandom on Linux or CryptGenRandom on Windows
    static uint32_t seed = 0;
    if (seed == 0) {
        seed = (uint32_t)time(NULL);
    }
    
    // Simple linear congruential generator (NOT cryptographically secure)
    // This is just a placeholder - DO NOT USE IN PRODUCTION
    seed = (seed * 1103515245 + 12345) & 0x7fffffff;
    return seed;
}

// Secure key derivation (placeholder - should use proper cryptographic functions)
static inline uci_security_error_t uci_sec_derive_key(const unsigned char* input, size_t input_len,
                                                       unsigned char* output, size_t output_len) {
    if (!input || !output || input_len == 0 || output_len == 0) {
        return UCI_SEC_ERROR_INVALID_PARAM;
    }
    
    // Simple hash-based key derivation (NOT cryptographically secure)
    // This is just a placeholder - DO NOT USE IN PRODUCTION
    uint32_t hash = 0;
    for (size_t i = 0; i < input_len; i++) {
        hash = hash * 31 + input[i];
    }
    
    // Fill output with derived key material
    for (size_t i = 0; i < output_len; i++) {
        output[i] = (hash >> ((i % 4) * 8)) & 0xFF;
    }
    
    return UCI_SEC_SUCCESS;
}

// Secure packet encryption (placeholder - should use proper cryptographic libraries)
static inline uci_security_error_t uci_sec_encrypt_packet(unsigned char* packet, size_t packet_len,
                                                          const unsigned char* key, size_t key_len) {
    if (!packet || !key || packet_len == 0 || key_len == 0) {
        return UCI_SEC_ERROR_INVALID_PARAM;
    }
    
    // Simple XOR cipher (NOT cryptographically secure)
    // This is just a placeholder - DO NOT USE IN PRODUCTION
    for (size_t i = 0; i < packet_len; i++) {
        packet[i] ^= key[i % key_len];
    }
    
    return UCI_SEC_SUCCESS;
}

// Secure packet decryption (placeholder - should use proper cryptographic libraries)
static inline uci_security_error_t uci_sec_decrypt_packet(unsigned char* packet, size_t packet_len,
                                                          const unsigned char* key, size_t key_len) {
    if (!packet || !key || packet_len == 0 || key_len == 0) {
        return UCI_SEC_ERROR_INVALID_PARAM;
    }
    
    // Simple XOR cipher (NOT cryptographically secure)
    // This is just a placeholder - DO NOT USE IN PRODUCTION
    for (size_t i = 0; i < packet_len; i++) {
        packet[i] ^= key[i % key_len];
    }
    
    return UCI_SEC_SUCCESS;
}

// Secure hash computation (placeholder - should use proper cryptographic hash functions)
static inline uci_security_error_t uci_sec_compute_hash(const unsigned char* data, size_t data_len,
                                                         unsigned char* hash, size_t hash_len) {
    if (!data || !hash || data_len == 0 || hash_len == 0) {
        return UCI_SEC_ERROR_INVALID_PARAM;
    }
    
    // Simple checksum (NOT cryptographically secure)
    // This is just a placeholder - DO NOT USE IN PRODUCTION
    uint32_t checksum = 0;
    for (size_t i = 0; i < data_len; i++) {
        checksum += data[i];
    }
    
    // Fill hash with checksum
    for (size_t i = 0; i < hash_len && i < sizeof(checksum); i++) {
        hash[i] = (checksum >> (i * 8)) & 0xFF;
    }
    
    return UCI_SEC_SUCCESS;
}

// Secure signature verification (placeholder - should use proper digital signatures)
static inline bool uci_sec_verify_signature(const unsigned char* data, size_t data_len,
                                            const unsigned char* signature, size_t sig_len,
                                            const unsigned char* public_key, size_t key_len) {
    if (!data || !signature || !public_key || data_len == 0 || sig_len == 0 || key_len == 0) {
        return false;
    }
    
    // Simple verification (NOT cryptographically secure)
    // This is just a placeholder - DO NOT USE IN PRODUCTION
    unsigned char computed_hash[32];
    if (uci_sec_compute_hash(data, data_len, computed_hash, sizeof(computed_hash)) != UCI_SEC_SUCCESS) {
        return false;
    }
    
    // Compare with provided signature
    return uci_sec_memcmp_consttime(signature, computed_hash, 
                                   sig_len < sizeof(computed_hash) ? sig_len : sizeof(computed_hash));
}

#endif // UCI_SECURITY_H