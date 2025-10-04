#include "../include/uci_config_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>  // For strcasecmp
#include <stdint.h>
#include <ctype.h>

// Application configuration parameter information
static const config_param_info_t app_config_params[] = {
    {DEVICE_TYPE, "DEVICE_TYPE", "Device type (initiator/responder)", 0, 1, 0, ""},
    {RANGING_ROUND_USAGE, "RANGING_ROUND_USAGE", "Ranging round usage", 0, 3, 0, ""},
    {STS_CONFIG, "STS_CONFIG", "Secure timestamp configuration", 0, 7, 0, ""},
    {MULTI_NODE_MODE, "MULTI_NODE_MODE", "Multi-node mode", 0, 3, 0, ""},
    {CHANNEL_NUMBER, "CHANNEL_NUMBER", "UWB channel number", 0, 15, 9, ""},
    {NO_OF_CONTROLEE, "NO_OF_CONTROLEE", "Number of controlees", 0, 8, 0, ""},
    {DEVICE_MAC_ADDRESS, "DEVICE_MAC_ADDRESS", "Device MAC address", 0, 0xFFFF, 0, ""},
    {DST_MAC_ADDRESS, "DST_MAC_ADDRESS", "Destination MAC address", 0, 0xFFFF, 0, ""},
    {SLOT_DURATION, "SLOT_DURATION", "Slot duration in RSTU", 0, 0xFFFF, 2400, "RSTU"},
    {RANGING_DURATION, "RANGING_DURATION", "Ranging duration in milliseconds", 0, 0xFFFFFFFF, 1000, "ms"},
    {STS_INDEX, "STS_INDEX", "STS index", 0, 0xFFFF, 0, ""},
    {MAC_FCS_TYPE, "MAC_FCS_TYPE", "MAC frame check sequence type", 0, 1, 0, ""},
    {RANGING_ROUND_CONTROL, "RANGING_ROUND_CONTROL", "Ranging round control", 0, 0xFF, 0, ""},
    {AOA_RESULT_REQ, "AOA_RESULT_REQ", "Angle-of-arrival result request", 0, 3, 0, ""},
    {RNG_DATA_NTF, "RNG_DATA_NTF", "Ranging data notification", 0, 3, 0, ""},
    {RNG_DATA_NTF_PROXIMITY_NEAR, "RNG_DATA_NTF_PROXIMITY_NEAR", "Ranging data notification proximity near", 0, 0xFFFF, 0, "cm"},
    {RNG_DATA_NTF_PROXIMITY_FAR, "RNG_DATA_NTF_PROXIMITY_FAR", "Ranging data notification proximity far", 0, 0xFFFF, 0, "cm"},
    {DEVICE_ROLE, "DEVICE_ROLE", "Device role", 0, 1, 0, ""},
    {RFRAME_CONFIG, "RFRAME_CONFIG", "Ranging frame configuration", 0, 7, 0, ""},
    {RSSI_REPORTING, "RSSI_REPORTING", "RSSI reporting", 0, 1, 0, ""},
    {PREAMBLE_CODE_INDEX, "PREAMBLE_CODE_INDEX", "Preamble code index", 0, 31, 10, ""},
    {SFD_ID, "SFD_ID", "Start of frame delimiter ID", 0, 3, 2, ""},
    {PSDU_DATA_RATE, "PSDU_DATA_RATE", "PSDU data rate", 0, 3, 0, ""},
    {PREAMBLE_DURATION, "PREAMBLE_DURATION", "Preamble duration", 0, 3, 1, ""},
    {LINK_LAYER_MODE, "LINK_LAYER_MODE", "Link layer mode", 0, 1, 0, ""},
    {DATA_REPETITION_COUNT, "DATA_REPETITION_COUNT", "Data repetition count", 0, 63, 0, ""},
    {RANGING_TIME_STRUCT, "RANGING_TIME_STRUCT", "Ranging time structure", 0, 3, 0, ""},
    {SLOTS_PER_RR, "SLOTS_PER_RR", "Slots per ranging round", 0, 0xFF, 1, ""},
    {TX_ADAPTIVE_PAYLOAD_POWER, "TX_ADAPTIVE_PAYLOAD_POWER", "Transmit adaptive payload power", 0, 1, 0, ""},
    {RNG_DATA_NTF_AOA_BOUND, "RNG_DATA_NTF_AOA_BOUND", "Ranging data notification AoA bound", 0, 0xFFFF, 0, "degrees"},
    {RESPONDER_SLOT_INDEX, "RESPONDER_SLOT_INDEX", "Responder slot index", 0, 0xFF, 0, ""},
    {PRF_MODE, "PRF_MODE", "Pulse repetition frequency mode", 0, 3, 0, ""},
    {CAP_SIZE_RANGE, "CAP_SIZE_RANGE", "Capability size range", 0, 0xFFFF, 0, ""},
    {TX_JITTER_WINDOW_SIZE, "TX_JITTER_WINDOW_SIZE", "Transmit jitter window size", 0, 0xFFFF, 0, "RSTU"},
    {SCHEDULED_MODE, "SCHEDULED_MODE", "Scheduled mode", 0, 1, 0, ""},
    {KEY_ROTATION, "KEY_ROTATION", "Key rotation", 0, 1, 0, ""},
    {KEY_ROTATION_RATE, "KEY_ROTATION_RATE", "Key rotation rate", 0, 0xFFFF, 0, ""},
    {SESSION_PRIORITY, "SESSION_PRIORITY", "Session priority", 0, 100, 50, ""},
    {MAC_ADDRESS_MODE, "MAC_ADDRESS_MODE", "MAC address mode", 0, 1, 0, ""},
    {VENDOR_ID, "VENDOR_ID", "Vendor ID", 0, 0xFFFF, 0, ""},
    {STATIC_STS_IV, "STATIC_STS_IV", "Static STS initialization vector", 0, 0xFFFFFFFFFFFFFFFFULL, 0, ""},
    {NUMBER_OF_STS_SEGMENTS, "NUMBER_OF_STS_SEGMENTS", "Number of STS segments", 0, 0xFF, 1, ""},
    {MAX_RR_RETRY, "MAX_RR_RETRY", "Maximum ranging round retry", 0, 0xFF, 3, ""},
    {UWB_INITIATION_TIME, "UWB_INITIATION_TIME", "UWB initiation time", 0, 0xFFFFFFFF, 0, "ms"},
    {HOPPING_MODE, "HOPPING_MODE", "Hopping mode", 0, 3, 0, ""},
    {BLOCK_STRIDE_LENGTH, "BLOCK_STRIDE_LENGTH", "Block stride length", 0, 0xFFFF, 0, ""},
    {RESULT_REPORT_CONFIG, "RESULT_REPORT_CONFIG", "Result report configuration", 0, 0xFF, 0, ""},
    {IN_BAND_TERMINATION_ATTEMPT_COUNT, "IN_BAND_TERMINATION_ATTEMPT_COUNT", "In-band termination attempt count", 0, 0xFF, 3, ""},
    {SUB_SESSION_ID, "SUB_SESSION_ID", "Sub-session ID", 0, 0xFFFFFFFF, 0, ""},
};

// Device configuration parameter information
static const device_config_param_info_t device_config_params[] = {
    {DEVICE_STATE, "DEVICE_STATE", "Device state", 0, 3, 0, ""},
    {LOW_POWER_MODE, "LOW_POWER_MODE", "Low power mode", 0, 1, 0, ""},
};

// Configuration storage arrays
static unsigned char app_config_values[256][256];  // Up to 256 config params with up to 256 bytes each
static size_t app_config_lengths[256] = {0};       // Length of each config param
static unsigned char device_config_values[256][256]; // Up to 256 device config params with up to 256 bytes each
static size_t device_config_lengths[256] = {0};     // Length of each device config param

// Verbose mode flag
static int g_verbose_mode = 0;

// Initialize configuration manager
int uci_config_init() {
    // Initialize all configuration values to defaults
    size_t num_app_params = sizeof(app_config_params) / sizeof(app_config_params[0]);
    for (size_t i = 0; i < num_app_params; i++) {
        const config_param_info_t* param = &app_config_params[i];
        app_config_values[param->cfg_id][0] = (unsigned char)(param->default_value & 0xFF);
        app_config_lengths[param->cfg_id] = 1;
        
        // Special handling for multi-byte values
        if (param->default_value > 0xFF) {
            if (param->default_value <= 0xFFFF) {
                app_config_values[param->cfg_id][0] = (unsigned char)(param->default_value & 0xFF);
                app_config_values[param->cfg_id][1] = (unsigned char)((param->default_value >> 8) & 0xFF);
                app_config_lengths[param->cfg_id] = 2;
            } else if (param->default_value <= 0xFFFFFFFF) {
                app_config_values[param->cfg_id][0] = (unsigned char)(param->default_value & 0xFF);
                app_config_values[param->cfg_id][1] = (unsigned char)((param->default_value >> 8) & 0xFF);
                app_config_values[param->cfg_id][2] = (unsigned char)((param->default_value >> 16) & 0xFF);
                app_config_values[param->cfg_id][3] = (unsigned char)((param->default_value >> 24) & 0xFF);
                app_config_lengths[param->cfg_id] = 4;
            } else if (param->default_value <= 0xFFFFFFFFFFFFFFFFULL) {
                app_config_values[param->cfg_id][0] = (unsigned char)(param->default_value & 0xFF);
                app_config_values[param->cfg_id][1] = (unsigned char)((param->default_value >> 8) & 0xFF);
                app_config_values[param->cfg_id][2] = (unsigned char)((param->default_value >> 16) & 0xFF);
                app_config_values[param->cfg_id][3] = (unsigned char)((param->default_value >> 24) & 0xFF);
                app_config_values[param->cfg_id][4] = (unsigned char)((param->default_value >> 32) & 0xFF);
                app_config_values[param->cfg_id][5] = (unsigned char)((param->default_value >> 40) & 0xFF);
                app_config_values[param->cfg_id][6] = (unsigned char)((param->default_value >> 48) & 0xFF);
                app_config_values[param->cfg_id][7] = (unsigned char)((param->default_value >> 56) & 0xFF);
                app_config_lengths[param->cfg_id] = 8;
            }
        }
    }
    
    // Initialize device configuration values to defaults
    size_t num_device_params = sizeof(device_config_params) / sizeof(device_config_params[0]);
    for (size_t i = 0; i < num_device_params; i++) {
        const device_config_param_info_t* param = &device_config_params[i];
        device_config_values[param->cfg_id][0] = (unsigned char)(param->default_value & 0xFF);
        device_config_lengths[param->cfg_id] = 1;
        
        // Special handling for multi-byte values
        if (param->default_value > 0xFF) {
            if (param->default_value <= 0xFFFF) {
                device_config_values[param->cfg_id][0] = (unsigned char)(param->default_value & 0xFF);
                device_config_values[param->cfg_id][1] = (unsigned char)((param->default_value >> 8) & 0xFF);
                device_config_lengths[param->cfg_id] = 2;
            } else if (param->default_value <= 0xFFFFFFFF) {
                device_config_values[param->cfg_id][0] = (unsigned char)(param->default_value & 0xFF);
                device_config_values[param->cfg_id][1] = (unsigned char)((param->default_value >> 8) & 0xFF);
                device_config_values[param->cfg_id][2] = (unsigned char)((param->default_value >> 16) & 0xFF);
                device_config_values[param->cfg_id][3] = (unsigned char)((param->default_value >> 24) & 0xFF);
                device_config_lengths[param->cfg_id] = 4;
            } else if (param->default_value <= 0xFFFFFFFFFFFFFFFFULL) {
                device_config_values[param->cfg_id][0] = (unsigned char)(param->default_value & 0xFF);
                device_config_values[param->cfg_id][1] = (unsigned char)((param->default_value >> 8) & 0xFF);
                device_config_values[param->cfg_id][2] = (unsigned char)((param->default_value >> 16) & 0xFF);
                device_config_values[param->cfg_id][3] = (unsigned char)((param->default_value >> 24) & 0xFF);
                device_config_values[param->cfg_id][4] = (unsigned char)((param->default_value >> 32) & 0xFF);
                device_config_values[param->cfg_id][5] = (unsigned char)((param->default_value >> 40) & 0xFF);
                device_config_values[param->cfg_id][6] = (unsigned char)((param->default_value >> 48) & 0xFF);
                device_config_values[param->cfg_id][7] = (unsigned char)((param->default_value >> 56) & 0xFF);
                device_config_lengths[param->cfg_id] = 8;
            }
        }
    }
    
    if (g_verbose_mode) {
        printf("UCI configuration manager initialized with %zu app params and %zu device params\n", 
               num_app_params, num_device_params);
    }
    
    return 0;
}

// Cleanup configuration manager
void uci_config_cleanup() {
    // Nothing to do for now
    if (g_verbose_mode) {
        printf("UCI configuration manager cleaned up\n");
    }
}

// Enable/disable verbose mode
void uci_config_set_verbose(int verbose) {
    g_verbose_mode = verbose ? 1 : 0;
}

// Set application configuration parameter
int uci_config_set_app_param(AppConfigTlvType cfg_id, const unsigned char* value, size_t value_len) {
    if (!value || value_len == 0 || value_len > 256) {
        if (g_verbose_mode) {
            printf("Error: Invalid parameters for uci_config_set_app_param\n");
        }
        return -1;
    }
    
    // Store the value
    memcpy(app_config_values[cfg_id], value, value_len);
    app_config_lengths[cfg_id] = value_len;
    
    if (g_verbose_mode) {
        printf("Stored app config %s (0x%02X) with %zu bytes:", 
               uci_config_get_app_param_name(cfg_id), cfg_id, value_len);
        for (size_t i = 0; i < value_len; i++) {
            printf(" %02X", value[i]);
        }
        printf("\n");
    }
    
    return 0;
}

// Get application configuration parameter
int uci_config_get_app_param(AppConfigTlvType cfg_id, unsigned char* value, size_t* value_len) {
    if (!value || !value_len || *value_len < app_config_lengths[cfg_id]) {
        if (g_verbose_mode) {
            printf("Error: Invalid parameters for uci_config_get_app_param\n");
        }
        return -1;
    }
    
    // Copy the value
    memcpy(value, app_config_values[cfg_id], app_config_lengths[cfg_id]);
    *value_len = app_config_lengths[cfg_id];
    
    if (g_verbose_mode) {
        printf("Retrieved app config %s (0x%02X) with %zu bytes:", 
               uci_config_get_app_param_name(cfg_id), cfg_id, *value_len);
        for (size_t i = 0; i < *value_len; i++) {
            printf(" %02X", value[i]);
        }
        printf("\n");
    }
    
    return 0;
}

// List application configuration parameters
int uci_config_list_app_params() {
    printf("Available Application Configuration Parameters:\n");
    printf("=============================================\n");
    
    size_t num_params = sizeof(app_config_params) / sizeof(app_config_params[0]);
    for (size_t i = 0; i < num_params; i++) {
        const config_param_info_t* param = &app_config_params[i];
        printf("  %-30s (0x%02X) - %s (default: %lu %s, range: %lu-%lu)\n", 
               param->name, param->cfg_id, param->description, 
               (unsigned long)param->default_value, param->unit, 
               (unsigned long)param->min_value, (unsigned long)param->max_value);
    }
    
    return 0;
}

// Get application configuration parameter name
const char* uci_config_get_app_param_name(AppConfigTlvType cfg_id) {
    size_t num_params = sizeof(app_config_params) / sizeof(app_config_params[0]);
    for (size_t i = 0; i < num_params; i++) {
        if (app_config_params[i].cfg_id == cfg_id) {
            return app_config_params[i].name;
        }
    }
    return "UNKNOWN";
}

// Get application configuration parameter description
const char* uci_config_get_app_param_desc(AppConfigTlvType cfg_id) {
    size_t num_params = sizeof(app_config_params) / sizeof(app_config_params[0]);
    for (size_t i = 0; i < num_params; i++) {
        if (app_config_params[i].cfg_id == cfg_id) {
            return app_config_params[i].description;
        }
    }
    return "Unknown configuration parameter";
}

// Get application configuration parameter default value
uint64_t uci_config_get_app_param_default(AppConfigTlvType cfg_id) {
    size_t num_params = sizeof(app_config_params) / sizeof(app_config_params[0]);
    for (size_t i = 0; i < num_params; i++) {
        if (app_config_params[i].cfg_id == cfg_id) {
            return app_config_params[i].default_value;
        }
    }
    return 0;
}

// Get application configuration parameter range
int uci_config_get_app_param_range(AppConfigTlvType cfg_id, uint64_t* min_val, uint64_t* max_val) {
    if (!min_val || !max_val) {
        return -1;
    }
    
    size_t num_params = sizeof(app_config_params) / sizeof(app_config_params[0]);
    for (size_t i = 0; i < num_params; i++) {
        if (app_config_params[i].cfg_id == cfg_id) {
            *min_val = app_config_params[i].min_value;
            *max_val = app_config_params[i].max_value;
            return 0;
        }
    }
    
    *min_val = 0;
    *max_val = 0;
    return -1;
}

// Set device configuration parameter
int uci_config_set_device_param(DeviceConfigId cfg_id, const unsigned char* value, size_t value_len) {
    if (!value || value_len == 0 || value_len > 256) {
        if (g_verbose_mode) {
            printf("Error: Invalid parameters for uci_config_set_device_param\n");
        }
        return -1;
    }
    
    // Store the value
    memcpy(device_config_values[cfg_id], value, value_len);
    device_config_lengths[cfg_id] = value_len;
    
    if (g_verbose_mode) {
        printf("Stored device config %s (0x%02X) with %zu bytes:", 
               uci_config_get_device_param_name(cfg_id), cfg_id, value_len);
        for (size_t i = 0; i < value_len; i++) {
            printf(" %02X", value[i]);
        }
        printf("\n");
    }
    
    return 0;
}

// Get device configuration parameter
int uci_config_get_device_param(DeviceConfigId cfg_id, unsigned char* value, size_t* value_len) {
    if (!value || !value_len || *value_len < device_config_lengths[cfg_id]) {
        if (g_verbose_mode) {
            printf("Error: Invalid parameters for uci_config_get_device_param\n");
        }
        return -1;
    }
    
    // Copy the value
    memcpy(value, device_config_values[cfg_id], device_config_lengths[cfg_id]);
    *value_len = device_config_lengths[cfg_id];
    
    if (g_verbose_mode) {
        printf("Retrieved device config %s (0x%02X) with %zu bytes:", 
               uci_config_get_device_param_name(cfg_id), cfg_id, *value_len);
        for (size_t i = 0; i < *value_len; i++) {
            printf(" %02X", value[i]);
        }
        printf("\n");
    }
    
    return 0;
}

// List device configuration parameters
int uci_config_list_device_params() {
    printf("Available Device Configuration Parameters:\n");
    printf("=========================================\n");
    
    size_t num_params = sizeof(device_config_params) / sizeof(device_config_params[0]);
    for (size_t i = 0; i < num_params; i++) {
        const device_config_param_info_t* param = &device_config_params[i];
        printf("  %-20s (0x%02X) - %s (default: %lu, range: %lu-%lu)\n", 
               param->name, param->cfg_id, param->description, 
               (unsigned long)param->default_value, 
               (unsigned long)param->min_value, (unsigned long)param->max_value);
    }
    
    return 0;
}

// Get device configuration parameter name
const char* uci_config_get_device_param_name(DeviceConfigId cfg_id) {
    size_t num_params = sizeof(device_config_params) / sizeof(device_config_params[0]);
    for (size_t i = 0; i < num_params; i++) {
        if (device_config_params[i].cfg_id == cfg_id) {
            return device_config_params[i].name;
        }
    }
    return "UNKNOWN";
}

// Get device configuration parameter description
const char* uci_config_get_device_param_desc(DeviceConfigId cfg_id) {
    size_t num_params = sizeof(device_config_params) / sizeof(device_config_params[0]);
    for (size_t i = 0; i < num_params; i++) {
        if (device_config_params[i].cfg_id == cfg_id) {
            return device_config_params[i].description;
        }
    }
    return "Unknown device configuration parameter";
}

// Get device configuration parameter default value
uint64_t uci_config_get_device_param_default(DeviceConfigId cfg_id) {
    size_t num_params = sizeof(device_config_params) / sizeof(device_config_params[0]);
    for (size_t i = 0; i < num_params; i++) {
        if (device_config_params[i].cfg_id == cfg_id) {
            return device_config_params[i].default_value;
        }
    }
    return 0;
}

// Get device configuration parameter range
int uci_config_get_device_param_range(DeviceConfigId cfg_id, uint64_t* min_val, uint64_t* max_val) {
    if (!min_val || !max_val) {
        return -1;
    }
    
    size_t num_params = sizeof(device_config_params) / sizeof(device_config_params[0]);
    for (size_t i = 0; i < num_params; i++) {
        if (device_config_params[i].cfg_id == cfg_id) {
            *min_val = device_config_params[i].min_value;
            *max_val = device_config_params[i].max_value;
            return 0;
        }
    }
    
    *min_val = 0;
    *max_val = 0;
    return -1;
}

// Parse application parameter name to ID
int uci_config_parse_app_param_name(const char* name, AppConfigTlvType* cfg_id) {
    if (!name || !cfg_id) {
        return -1;
    }
    
    size_t num_params = sizeof(app_config_params) / sizeof(app_config_params[0]);
    for (size_t i = 0; i < num_params; i++) {
        if (strcasecmp(app_config_params[i].name, name) == 0) {
            *cfg_id = app_config_params[i].cfg_id;
            return 0;
        }
    }
    
    // Try to parse as hex number
    char* endptr;
    unsigned long value = strtoul(name, &endptr, 0);
    if (*endptr == '\0' && value <= 0xFF) {
        *cfg_id = (AppConfigTlvType)value;
        return 0;
    }
    
    return -1;
}

// Parse device parameter name to ID
int uci_config_parse_device_param_name(const char* name, DeviceConfigId* cfg_id) {
    if (!name || !cfg_id) {
        return -1;
    }
    
    size_t num_params = sizeof(device_config_params) / sizeof(device_config_params[0]);
    for (size_t i = 0; i < num_params; i++) {
        if (strcasecmp(device_config_params[i].name, name) == 0) {
            *cfg_id = device_config_params[i].cfg_id;
            return 0;
        }
    }
    
    // Try to parse as hex number
    char* endptr;
    unsigned long value = strtoul(name, &endptr, 0);
    if (*endptr == '\0' && value <= 0xFF) {
        *cfg_id = (DeviceConfigId)value;
        return 0;
    }
    
    return -1;
}

// Parse hex value string to bytes
int uci_config_parse_hex_value(const char* hex_str, unsigned char* value, size_t* value_len) {
    if (!hex_str || !value || !value_len) {
        return -1;
    }
    
    size_t len = strlen(hex_str);
    if (len % 2 != 0) {
        return -1; // Must be even number of hex digits
    }
    
    size_t byte_count = len / 2;
    if (byte_count > *value_len) {
        return -1; // Buffer too small
    }
    
    for (size_t i = 0; i < byte_count; i++) {
        char hex_byte[3] = {hex_str[i*2], hex_str[i*2+1], '\0'};
        char* endptr;
        unsigned long byte_val = strtoul(hex_byte, &endptr, 16);
        if (*endptr != '\0') {
            return -1; // Invalid hex digit
        }
        value[i] = (unsigned char)byte_val;
    }
    
    *value_len = byte_count;
    return 0;
}

// Format bytes as hex string
int uci_config_format_hex_value(const unsigned char* value, size_t value_len, char* output, size_t output_len) {
    if (!value || !output || output_len < (value_len * 3)) {
        return -1;
    }
    
    for (size_t i = 0; i < value_len; i++) {
        snprintf(output + i * 3, output_len - i * 3, "%02X ", value[i]);
    }
    
    // Remove trailing space
    if (value_len > 0) {
        output[value_len * 3 - 1] = '\0';
    } else {
        output[0] = '\0';
    }
    
    return 0;
}

// Show help for specific application parameter
int uci_config_show_app_param_help(AppConfigTlvType cfg_id) {
    size_t num_params = sizeof(app_config_params) / sizeof(app_config_params[0]);
    for (size_t i = 0; i < num_params; i++) {
        if (app_config_params[i].cfg_id == cfg_id) {
            const config_param_info_t* param = &app_config_params[i];
            printf("Application Configuration Parameter: %s (0x%02X)\n", param->name, param->cfg_id);
            printf("  Description: %s\n", param->description);
            printf("  Default Value: %lu %s\n", (unsigned long)param->default_value, param->unit);
            printf("  Valid Range: %lu to %lu\n", (unsigned long)param->min_value, (unsigned long)param->max_value);
            if (strlen(param->unit) > 0) {
                printf("  Unit: %s\n", param->unit);
            }
            return 0;
        }
    }
    
    printf("Unknown Application Configuration Parameter: 0x%02X\n", cfg_id);
    return -1;
}

// Show help for specific device parameter
int uci_config_show_device_param_help(DeviceConfigId cfg_id) {
    size_t num_params = sizeof(device_config_params) / sizeof(device_config_params[0]);
    for (size_t i = 0; i < num_params; i++) {
        if (device_config_params[i].cfg_id == cfg_id) {
            const device_config_param_info_t* param = &device_config_params[i];
            printf("Device Configuration Parameter: %s (0x%02X)\n", param->name, param->cfg_id);
            printf("  Description: %s\n", param->description);
            printf("  Default Value: %lu\n", (unsigned long)param->default_value);
            printf("  Valid Range: %lu to %lu\n", (unsigned long)param->min_value, (unsigned long)param->max_value);
            if (strlen(param->unit) > 0) {
                printf("  Unit: %s\n", param->unit);
            }
            return 0;
        }
    }
    
    printf("Unknown Device Configuration Parameter: 0x%02X\n", cfg_id);
    return -1;
}

// Show all application parameters
int uci_config_show_all_app_params() {
    printf("=== All Application Configuration Parameters ===\n");
    size_t num_params = sizeof(app_config_params) / sizeof(app_config_params[0]);
    for (size_t i = 0; i < num_params; i++) {
        const config_param_info_t* param = &app_config_params[i];
        printf("\n%s (0x%02X):\n", param->name, param->cfg_id);
        printf("  Description: %s\n", param->description);
        printf("  Default Value: %lu %s\n", (unsigned long)param->default_value, param->unit);
        printf("  Valid Range: %lu to %lu\n", (unsigned long)param->min_value, (unsigned long)param->max_value);
        if (strlen(param->unit) > 0) {
            printf("  Unit: %s\n", param->unit);
        }
        
        // Show current value
        unsigned char current_value[256];
        size_t current_len = sizeof(current_value);
        if (uci_config_get_app_param(param->cfg_id, current_value, &current_len) == 0) {
            printf("  Current Value: ");
            for (size_t j = 0; j < current_len; j++) {
                printf("%02X ", current_value[j]);
            }
            printf("(%zu bytes)\n", current_len);
        } else {
            printf("  Current Value: Unable to retrieve\n");
        }
    }
    printf("\nTotal: %zu parameters\n", num_params);
    return 0;
}

// Show all device parameters
int uci_config_show_all_device_params() {
    printf("=== All Device Configuration Parameters ===\n");
    size_t num_params = sizeof(device_config_params) / sizeof(device_config_params[0]);
    for (size_t i = 0; i < num_params; i++) {
        const device_config_param_info_t* param = &device_config_params[i];
        printf("\n%s (0x%02X):\n", param->name, param->cfg_id);
        printf("  Description: %s\n", param->description);
        printf("  Default Value: %lu\n", (unsigned long)param->default_value);
        printf("  Valid Range: %lu to %lu\n", (unsigned long)param->min_value, (unsigned long)param->max_value);
        if (strlen(param->unit) > 0) {
            printf("  Unit: %s\n", param->unit);
        }
        
        // Show current value
        unsigned char current_value[256];
        size_t current_len = sizeof(current_value);
        if (uci_config_get_device_param(param->cfg_id, current_value, &current_len) == 0) {
            printf("  Current Value: ");
            for (size_t j = 0; j < current_len; j++) {
                printf("%02X ", current_value[j]);
            }
            printf("(%zu bytes)\n", current_len);
        } else {
            printf("  Current Value: Unable to retrieve\n");
        }
    }
    printf("\nTotal: %zu parameters\n", num_params);
    return 0;
}