#ifndef UCI_TYPES_H
#define UCI_TYPES_H

#include <stdint.h>

// Standardized fixed-width integer types for UCI Interactive Shell
// This ensures consistent type usage across the codebase

// Basic types
typedef uint8_t uci_uint8;   // replacing unsigned char
typedef uint16_t uci_uint16; // replacing unsigned short
typedef uint32_t uci_uint32; // replacing unsigned int where appropriate
typedef uint64_t uci_uint64; // replacing unsigned long long where appropriate

// UCI-specific types
typedef uint8_t uci_message_type_t;
typedef uint8_t uci_packet_boundary_flag_t;
typedef uint8_t uci_group_id_t;
typedef uint8_t uci_opcode_id_t;
typedef uint8_t uci_status_t;
typedef uint8_t uci_session_state_t;
typedef uint8_t uci_device_state_t;
typedef uint16_t uci_session_handle_t;
typedef uint32_t uci_session_id_t;

// Length and count types
typedef uint8_t uci_length_t;
typedef uint8_t uci_count_t;
typedef uint8_t uci_config_id_t;

// Utility function to convert standard types to fixed-width types
static inline uci_uint8 uci_from_uchar(unsigned char value) {
    return (uci_uint8)value;
}

static inline uci_uint16 uci_from_ushort(unsigned short value) {
    return (uci_uint16)value;
}

static inline uci_uint32 uci_from_uint(unsigned int value) {
    return (uci_uint32)value;
}

static inline unsigned char uci_to_uchar(uci_uint8 value) {
    return (unsigned char)value;
}

static inline unsigned short uci_to_ushort(uci_uint16 value) {
    return (unsigned short)value;
}

static inline unsigned int uci_to_uint(uci_uint32 value) {
    return (unsigned int)value;
}

// Common error reporting function
static inline void uci_report_error(const char* function_name, const char* operation, uci_error_t error) {
    if (error != UCI_SUCCESS) {
        const char* error_msg;
        switch (error) {
            case UCI_ERROR_INVALID_PARAM: error_msg = "Invalid parameter"; break;
            case UCI_ERROR_BUFFER_OVERFLOW: error_msg = "Buffer overflow"; break;
            case UCI_ERROR_OUT_OF_MEMORY: error_msg = "Out of memory"; break;
            case UCI_ERROR_MALFORMED_PACKET: error_msg = "Malformed packet"; break;
            case UCI_ERROR_UNSUPPORTED_OPERATION: error_msg = "Unsupported operation"; break;
            case UCI_ERROR_RESOURCE_EXHAUSTED: error_msg = "Resource exhausted"; break;
            default: error_msg = "Unknown error"; break;
        }
        fprintf(stderr, "[ERROR] %s: %s failed - %s\n", function_name, operation, error_msg);
    }
}

#endif // UCI_TYPES_H