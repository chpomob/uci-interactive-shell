#ifndef UCI_STANDARDIZED_ERROR_HANDLING_H
#define UCI_STANDARDIZED_ERROR_HANDLING_H

#include "uci_utils.h"
#include "uci.h"  // Include uci.h to get MAX_SESSIONS definition
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

// Standardized error handling macros and functions for UCI Interactive Shell

// Log error with context
static inline void uci_log_error(const char* function, const char* context, uci_error_t error) {
    const char* error_str;
    switch (error) {
        case UCI_SUCCESS: error_str = "SUCCESS"; break;
        case UCI_ERROR_INVALID_PARAM: error_str = "INVALID_PARAM"; break;
        case UCI_ERROR_BUFFER_OVERFLOW: error_str = "BUFFER_OVERFLOW"; break;
        case UCI_ERROR_OUT_OF_MEMORY: error_str = "OUT_OF_MEMORY"; break;
        case UCI_ERROR_MALFORMED_PACKET: error_str = "MALFORMED_PACKET"; break;
        case UCI_ERROR_UNSUPPORTED_OPERATION: error_str = "UNSUPPORTED_OPERATION"; break;
        case UCI_ERROR_RESOURCE_EXHAUSTED: error_str = "RESOURCE_EXHAUSTED"; break;
        default: error_str = "UNKNOWN_ERROR"; break;
    }
    
    fprintf(stderr, "[ERROR] %s: %s (%s)\n", function, context, error_str);
}

// Macro for logging errors with context
#define UCI_LOG_ERROR(context, error) uci_log_error(__func__, context, error)

// Safe string copy with bounds checking using standardized error codes
static inline uci_error_t uci_safe_strcpy(char* dest, size_t dest_size, const char* src) {
    if (dest == NULL || dest_size == 0 || src == NULL) {
        return UCI_ERROR_INVALID_PARAM;
    }
    
    size_t src_len = strlen(src);
    if (src_len >= dest_size) {
        return UCI_ERROR_BUFFER_OVERFLOW;
    }
    
    strcpy(dest, src);
    return UCI_SUCCESS;
}

// Safe string copy with truncation using standardized error codes
static inline uci_error_t uci_safe_strcpy_trunc(char* dest, size_t dest_size, const char* src) {
    if (dest == NULL || dest_size == 0 || src == NULL) {
        return UCI_ERROR_INVALID_PARAM;
    }
    
    strncpy(dest, src, dest_size - 1);
    dest[dest_size - 1] = '\0';  // Ensure null termination
    return UCI_SUCCESS;
}

// Safe memory copy with bounds checking using standardized error codes
static inline uci_error_t uci_safe_memcpy(void* dest, size_t dest_size, const void* src, size_t src_size) {
    if (dest == NULL || src == NULL) {
        return UCI_ERROR_INVALID_PARAM;
    }
    
    if (src_size > dest_size) {
        return UCI_ERROR_BUFFER_OVERFLOW;
    }
    
    memcpy(dest, src, src_size);
    return UCI_SUCCESS;
}

// Safe memory copy with automatic limiting using standardized error codes
static inline uci_error_t uci_safe_memcpy_safe(void* dest, size_t dest_size, const void* src, size_t src_size) {
    if (dest == NULL || src == NULL) {
        return UCI_ERROR_INVALID_PARAM;
    }
    
    size_t copy_size = (src_size < dest_size) ? src_size : dest_size;
    memcpy(dest, src, copy_size);
    return UCI_SUCCESS;
}

// Check and validate session ID
static inline uci_error_t uci_validate_session_id(unsigned int session_id) {
    if (session_id >= MAX_SESSIONS) {
        return UCI_ERROR_INVALID_PARAM;
    }
    return UCI_SUCCESS;
}

// Safe command handler wrapper that returns standardized error codes
static inline uci_error_t uci_command_handler_result(int result) {
    if (result < 0) {
        return UCI_ERROR_INVALID_PARAM;
    }
    return UCI_SUCCESS;
}

#endif // UCI_STANDARDIZED_ERROR_HANDLING_H