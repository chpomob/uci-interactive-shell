#ifndef UCI_COMMAND_FRAMEWORK_H
#define UCI_COMMAND_FRAMEWORK_H

#include "uci_cli.h"
#include "uci.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Parameter type definitions
typedef enum {
    PARAM_TYPE_UINT8 = 0,
    PARAM_TYPE_UINT16,
    PARAM_TYPE_UINT32,
    PARAM_TYPE_UINT64,
    PARAM_TYPE_HEX_STRING,
    PARAM_TYPE_STRING,
    PARAM_TYPE_SESSION_ID,
    PARAM_TYPE_DEVICE_STATE,
    PARAM_TYPE_SESSION_TYPE,
    PARAM_TYPE_HEX_BYTE,
} uci_param_type_t;

// Parameter validation flags
typedef enum {
    PARAM_FLAG_NONE = 0,
    PARAM_FLAG_REQUIRED = 1u << 0,
    PARAM_FLAG_OPTIONAL = 1u << 1,
} uci_param_flags_t;

// Parameter definition structure
typedef struct {
    const char* name;           // Parameter name for help
    uci_param_type_t type;      // Parameter data type
    uci_param_flags_t flags;    // Validation flags
    size_t max_len;            // Maximum length for string types
    unsigned int min_value;    // Minimum value for integer types
    unsigned int max_value;    // Maximum value for integer types
    const char* description;    // Description for help
} uci_param_def_t;

// Command definition structure
typedef struct {
    const char* name;
    const char* aliases[4];
    cli_command_group_t group;
    unsigned int flags;
    const char* description;
    const uci_param_def_t* params;  // Array of parameter definitions
    int param_count;               // Number of parameters
    int (*handler)(const char* cmd_name, int argc, char** argv, const uci_param_def_t* params, int param_count);
} uci_command_def_t;

// Command execution context
typedef struct {
    unsigned char mt;
    unsigned char pbf;
    unsigned char gid;
    unsigned char oid;
    unsigned char* payload;
    size_t payload_len;
    size_t max_payload_len;
} uci_command_context_t;

// Common parameter validation and conversion functions
int uci_cmd_validate_uint8(const char* str, unsigned char* value, unsigned char min_val, unsigned char max_val);
int uci_cmd_validate_uint16(const char* str, unsigned short* value, unsigned short min_val, unsigned short max_val);
int uci_cmd_validate_uint32(const char* str, unsigned int* value, unsigned int min_val, unsigned int max_val);
int uci_cmd_validate_session_id(const char* str, unsigned int* session_id);
int uci_cmd_validate_hex_string(const char* str, unsigned char* buffer, size_t* len, size_t max_len);
int uci_cmd_validate_device_state(const char* str, unsigned char* state);
int uci_cmd_validate_session_type(const char* str, unsigned char* type);

// Helper functions for payload construction
int uci_cmd_add_byte_to_payload(uci_command_context_t* ctx, unsigned char value);
int uci_cmd_add_uint16_to_payload(uci_command_context_t* ctx, unsigned short value);
int uci_cmd_add_uint32_to_payload(uci_command_context_t* ctx, unsigned int value);
int uci_cmd_add_bytes_to_payload(uci_command_context_t* ctx, const unsigned char* data, size_t len);

// Unified command execution function
int uci_cmd_execute_unified(uci_command_context_t* ctx);

// Command dispatch function
int uci_cmd_dispatch(const uci_command_def_t* cmd_def, int argc, char** argv);

#endif // UCI_COMMAND_FRAMEWORK_H