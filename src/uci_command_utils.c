/**
 * @file uci_command_utils.c
 * @brief Implementation of command utility functions
 *
 * Provides standardized utilities for command argument validation,
 * error reporting, and consistent user messaging across the UCI shell.
 */

bool parse_hex_string_safe(const char* hex_str, unsigned char* bytes, size_t max_len, size_t* out_len) {
    if (!hex_str || !bytes || !out_len) {
        fprintf(stderr, "Error: Invalid parameters for hex string parsing\n");
        return false;
    }
    
    size_t hex_len = strlen(hex_str);
    
    // Check for buffer overflow risk
    if (hex_len > 1024) {
        fprintf(stderr, "Error: Hex string too long for parsing (max 1024 characters)\n");
        return false;
    }
    
    // Check if length is even (valid hex strings should have even length)
    if (hex_len % 2 != 0) {
        fprintf(stderr, "Error: Invalid hex string length for parsing - must be even\n");
        return false;
    }
    
    size_t byte_count = hex_len / 2;
    
    // Check if we have enough space in the output buffer
    if (byte_count > max_len) {
        fprintf(stderr, "Error: Insufficient buffer space for hex parsing - need %zu bytes, have %zu\n", 
                byte_count, max_len);
        return false;
    }
    
    // Parse the hex string
    for (size_t i = 0; i < byte_count; i++) {
        char hex_byte[3] = {hex_str[i*2], hex_str[i*2+1], '\0'};
        char* endptr;
        unsigned long val = strtoul(hex_byte, &endptr, 16);
        
        // Check for conversion errors
        if (*endptr != '\0' || val > 255) {
            fprintf(stderr, "Error: Invalid hex pair '%s' at position %zu\n", hex_byte, i);
            return false;
        }
        
        bytes[i] = (unsigned char)val;
    }
    
    *out_len = byte_count;
    return true;
}

bool validate_min_args(int argc, int min_args, const char* cmd_name, const char* usage_fmt, ...) {
    if (argc >= min_args) {
        return true;
    }
    
    // Print usage message
    if (cmd_name) {
        fprintf(stderr, "Error: Insufficient arguments for '%s'\n", cmd_name);
    } else {
        fprintf(stderr, "Error: Insufficient arguments\n");
    }
    
    // Print detailed usage if provided
    if (usage_fmt) {
        va_list args;
        va_start(args, usage_fmt);
        fprintf(stderr, "Usage: ");
        vfprintf(stderr, usage_fmt, args);
        fprintf(stderr, "\n");
        va_end(args);
    }
    
    return false;
}

bool validate_numeric_range(const char* value_str, long min_val, long max_val, 
                           const char* value_name, long* out_value) {
    if (!value_str) {
        fprintf(stderr, "Error: %s is required\n", value_name ? value_name : "Value");
        return false;
    }
    
    char* endptr;
    errno = 0;
    long value = strtol(value_str, &endptr, 0);
    
    if (errno != 0 || *endptr != '\0') {
        fprintf(stderr, "Error: Invalid %s '%s' - must be a valid number\n", 
                value_name ? value_name : "value", value_str);
        return false;
    }
    
    if (value < min_val || value > max_val) {
        fprintf(stderr, "Error: %s '%ld' out of range [%ld, %ld]\n", 
                value_name ? value_name : "Value", value, min_val, max_val);
        return false;
    }
    
    if (out_value) {
        *out_value = value;
    }
    
    return true;
}

bool validate_hex_string(const char* hex_str, size_t expected_len) {
    if (!hex_str) {
        fprintf(stderr, "Error: Hex string is required\n");
        return false;
    }
    
    size_t len = strlen(hex_str);
    
    // Check for buffer overflow risk
    if (len > 1024) {  // Reasonable limit for hex strings
        fprintf(stderr, "Error: Hex string too long (max 1024 characters)\n");
        return false;
    }
    
    // Check if length is even (valid hex strings should have even length)
    if (len % 2 != 0) {
        fprintf(stderr, "Error: Invalid hex string length - must be even\n");
        return false;
    }
    
    // If expected length is specified, check it
    if (expected_len > 0 && len != expected_len * 2) {
        fprintf(stderr, "Error: Hex string length mismatch - expected %zu bytes, got %zu\n", 
                expected_len, len / 2);
        return false;
    }
    
    // Check that all characters are valid hex digits
    for (size_t i = 0; i < len; i++) {
        if (!isxdigit(hex_str[i])) {
            fprintf(stderr, "Error: Invalid character '%c' in hex string at position %zu\n", 
                    hex_str[i], i);
            return false;
        }
    }
    
    return true;
}

void report_error(int error_code, const char* fmt, ...) {
    fprintf(stderr, "Error");
    if (error_code != 0) {
        fprintf(stderr, " [%d]", error_code);
    }
    fprintf(stderr, ": ");
    
    if (fmt) {
        va_list args;
        va_start(args, fmt);
        vfprintf(stderr, fmt, args);
        va_end(args);
    }
    
    fprintf(stderr, "\n");
}

bool validate_session_id(const char* session_id_str, unsigned int* out_session_id) {
    long session_id;
    if (!validate_numeric_range(session_id_str, 0, 0xFFFFFFFF, "Session ID", &session_id)) {
        return false;
    }
    
    if (out_session_id) {
        *out_session_id = (unsigned int)session_id;
    }
    
    return true;
}