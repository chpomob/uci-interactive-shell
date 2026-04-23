#include "../include/uci_config_manager.h"
#include "../include/uci_config_metadata.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>  // For strcasecmp
#include <stdint.h>
#include <errno.h>
#include <ctype.h>

static int parse_unsigned_value(const char* value_str, uint64_t* out_value) {
    if (!value_str || !out_value) {
        return -1;
    }

    errno = 0;
    char* endptr = NULL;
    unsigned long long parsed = strtoull(value_str, &endptr, 0);
    if (errno != 0 || !value_str[0] || (endptr && *endptr != '\0')) {
        return -1;
    }

    *out_value = (uint64_t)parsed;
    return 0;
}

static int parse_boolean_value(const char* value_str, unsigned char* out_value) {
    if (!value_str || !out_value) {
        return -1;
    }

    if (strcasecmp(value_str, "on") == 0 ||
        strcasecmp(value_str, "enable") == 0 ||
        strcasecmp(value_str, "enabled") == 0 ||
        strcasecmp(value_str, "true") == 0 ||
        strcasecmp(value_str, "yes") == 0) {
        *out_value = 1;
        return 0;
    }

    if (strcasecmp(value_str, "off") == 0 ||
        strcasecmp(value_str, "disable") == 0 ||
        strcasecmp(value_str, "disabled") == 0 ||
        strcasecmp(value_str, "false") == 0 ||
        strcasecmp(value_str, "no") == 0) {
        *out_value = 0;
        return 0;
    }

    uint64_t numeric = 0;
    if (parse_unsigned_value(value_str, &numeric) == 0 && numeric <= 1) {
        *out_value = (unsigned char)numeric;
        return 0;
    }

    return -1;
}

static void trim_ascii_whitespace(char** start, char** end) {
    while (*start < *end && isspace((unsigned char)**start)) {
        (*start)++;
    }
    while (*end > *start && isspace((unsigned char)*((*end) - 1))) {
        (*end)--;
    }
}

static int parse_dst_mac_address_list(const char* value_str, unsigned char* value, size_t* value_len) {
    char buffer[256];
    char* cursor = NULL;
    char* token = NULL;
    size_t count = 0;

    if (!value_str || !value || !value_len) {
        return -1;
    }
    if (strlen(value_str) >= sizeof(buffer)) {
        return -1;
    }

    strcpy(buffer, value_str);
    cursor = buffer;
    while ((token = strsep(&cursor, ",")) != NULL) {
        char* start = token;
        char* end = token + strlen(token);
        uint64_t numeric = 0;

        trim_ascii_whitespace(&start, &end);
        if (start == end) {
            return -1;
        }

        *end = '\0';
        if (parse_unsigned_value(start, &numeric) != 0 || numeric > 0xFFFFU) {
            return -1;
        }
        if ((count + 1U) * 2U > *value_len) {
            return -1;
        }

        value[count * 2U] = (unsigned char)(numeric & 0xFFU);
        value[count * 2U + 1U] = (unsigned char)((numeric >> 8) & 0xFFU);
        count++;
    }

    if (count == 0U) {
        return -1;
    }

    *value_len = count * 2U;
    return 0;
}


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
    size_t num_app_params = uci_config_get_app_param_count();
    for (size_t i = 0; i < num_app_params; i++) {
        const config_param_info_t* param = uci_config_get_app_param_info_at(i);
        if (!param) {
            continue;
        }
        size_t value_len = param->value_len > 0 ? param->value_len : 1;
        if (value_len > sizeof(app_config_values[param->cfg_id])) {
            value_len = sizeof(app_config_values[param->cfg_id]);
        }
        memset(app_config_values[param->cfg_id], 0, value_len);
        app_config_lengths[param->cfg_id] = value_len;
        for (size_t byte = 0; byte < value_len && byte < sizeof(param->default_value); byte++) {
            app_config_values[param->cfg_id][byte] =
                (unsigned char)((param->default_value >> (8 * byte)) & 0xFF);
        }
    }
    
    // Initialize device configuration values to defaults
    size_t num_device_params = uci_config_get_device_param_count();
    for (size_t i = 0; i < num_device_params; i++) {
        const device_config_param_info_t* param = uci_config_get_device_param_info_at(i);
        if (!param) {
            continue;
        }
        size_t value_len = param->value_len > 0 ? param->value_len : 1;
        if (value_len > sizeof(device_config_values[param->cfg_id])) {
            value_len = sizeof(device_config_values[param->cfg_id]);
        }
        device_config_lengths[param->cfg_id] = value_len;
        for (size_t byte = 0; byte < value_len; byte++) {
            device_config_values[param->cfg_id][byte] =
                (unsigned char)((param->default_value >> (8 * byte)) & 0xFF);
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
        const char* param_name = uci_config_get_app_param_name(cfg_id);
        printf("Stored app config %s (0x%02X) with %zu bytes:", 
               param_name ? param_name : "unknown", cfg_id, value_len);
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
        const char* param_name = uci_config_get_app_param_name(cfg_id);
        printf("Retrieved app config %s (0x%02X) with %zu bytes:", 
               param_name ? param_name : "unknown", cfg_id, *value_len);
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
    
    size_t count = uci_config_get_app_param_count();
    for (size_t i = 0; i < count; i++) {
        const config_param_info_t* param = uci_config_get_app_param_info_at(i);
        if (!param) {
            continue;
        }
        printf("  %-30s (0x%02X) - %s (default: %lu %s, range: %lu-%lu)\n", 
               param->name, param->cfg_id, param->description, 
               (unsigned long)param->default_value, param->unit, 
               (unsigned long)param->min_value, (unsigned long)param->max_value);
    }
    
    return 0;
}

// Get application configuration parameter name
const char* uci_config_get_app_param_name(AppConfigTlvType cfg_id) {
    const config_param_info_t* info = uci_config_metadata_find_app_param(cfg_id);
    return info ? info->name : NULL;
}

// Get application configuration parameter description
const char* uci_config_get_app_param_desc(AppConfigTlvType cfg_id) {
    const config_param_info_t* info = uci_config_metadata_find_app_param(cfg_id);
    return info ? info->description : "Unknown configuration parameter";
}

size_t uci_config_get_app_param_count(void) {
    return uci_config_metadata_get_app_param_count();
}

const config_param_info_t* uci_config_get_app_param_info(AppConfigTlvType cfg_id) {
    return uci_config_metadata_find_app_param(cfg_id);
}

const config_param_info_t* uci_config_get_app_param_info_at(size_t index) {
    return uci_config_metadata_get_app_param_info_at(index);
}

// Get application configuration parameter default value
uint64_t uci_config_get_app_param_default(AppConfigTlvType cfg_id) {
    const config_param_info_t* info = uci_config_metadata_find_app_param(cfg_id);
    return info ? info->default_value : 0;
}

// Get application configuration parameter range
int uci_config_get_app_param_range(AppConfigTlvType cfg_id, uint64_t* min_val, uint64_t* max_val) {
    if (!min_val || !max_val) {
        return -1;
    }
    
    const config_param_info_t* info = uci_config_metadata_find_app_param(cfg_id);
    if (info) {
        *min_val = info->min_value;
        *max_val = info->max_value;
        return 0;
    }
    
    *min_val = 0;
    *max_val = 0;
    return -1;
}

// Set device configuration parameter
int uci_config_set_device_param(DeviceConfigId cfg_id, const unsigned char* value, size_t value_len) {
    const device_config_param_info_t* info = uci_config_get_device_param_info(cfg_id);
    if (!info) {
        if (g_verbose_mode) {
            printf("Error: Unknown device config ID 0x%02X\n", cfg_id);
        }
        return -1;
    }

    if (!value || value_len == 0 || value_len > 256) {
        if (g_verbose_mode) {
            printf("Error: Invalid parameters for uci_config_set_device_param\n");
        }
        return -1;
    }

    size_t expected_len = info->value_len > 0 ? info->value_len : value_len;
    if (expected_len > 0 && value_len != expected_len) {
        if (g_verbose_mode) {
            printf("Error: Device config %s expects %zu bytes but got %zu\n",
                   info->name, expected_len, value_len);
        }
        return -1;
    }
    
    // Store the value
    memcpy(device_config_values[cfg_id], value, value_len);
    device_config_lengths[cfg_id] = value_len;
    
    if (g_verbose_mode) {
        const char* param_name = uci_config_get_device_param_name(cfg_id);
        printf("Stored device config %s (0x%02X) with %zu bytes:", 
               param_name ? param_name : "unknown", cfg_id, value_len);
        for (size_t i = 0; i < value_len; i++) {
            printf(" %02X", value[i]);
        }
        printf("\n");
    }
    
    return 0;
}

// Get device configuration parameter
int uci_config_get_device_param(DeviceConfigId cfg_id, unsigned char* value, size_t* value_len) {
    const device_config_param_info_t* info = uci_config_get_device_param_info(cfg_id);
    if (!info) {
        if (g_verbose_mode) {
            printf("Error: Unknown device config ID 0x%02X\n", cfg_id);
        }
        return -1;
    }

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
        const char* param_name = uci_config_get_device_param_name(cfg_id);
        printf("Retrieved device config %s (0x%02X) with %zu bytes:", 
               param_name ? param_name : "unknown", cfg_id, *value_len);
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
    
    size_t count = uci_config_get_device_param_count();
    for (size_t i = 0; i < count; i++) {
        const device_config_param_info_t* param = uci_config_get_device_param_info_at(i);
        if (!param) {
            continue;
        }
        printf("  %-28s (0x%02X) - %s (default: %lu, range: %lu-%lu, size: %zu byte%s)\n", 
               param->name, param->cfg_id, param->description, 
               (unsigned long)param->default_value, 
               (unsigned long)param->min_value, (unsigned long)param->max_value,
               param->value_len,
               param->value_len == 1 ? "" : "s");
    }
    
    return 0;
}

// Get device configuration parameter name
const char* uci_config_get_device_param_name(DeviceConfigId cfg_id) {
    const device_config_param_info_t* info = uci_config_get_device_param_info(cfg_id);
    return info ? info->name : NULL;
}

// Get device configuration parameter description
const char* uci_config_get_device_param_desc(DeviceConfigId cfg_id) {
    const device_config_param_info_t* info = uci_config_get_device_param_info(cfg_id);
    return info ? info->description : "Unknown device configuration parameter";
}

// Get device configuration parameter default value
uint64_t uci_config_get_device_param_default(DeviceConfigId cfg_id) {
    const device_config_param_info_t* info = uci_config_get_device_param_info(cfg_id);
    return info ? info->default_value : 0;
}

// Get device configuration parameter range
int uci_config_get_device_param_range(DeviceConfigId cfg_id, uint64_t* min_val, uint64_t* max_val) {
    if (!min_val || !max_val) {
        return -1;
    }
    
    const device_config_param_info_t* info = uci_config_get_device_param_info(cfg_id);
    if (!info) {
        *min_val = 0;
        *max_val = 0;
        return -1;
    }

    *min_val = info->min_value;
    *max_val = info->max_value;
    return 0;
}

size_t uci_config_get_device_param_length(DeviceConfigId cfg_id) {
    const device_config_param_info_t* info = uci_config_get_device_param_info(cfg_id);
    return info ? info->value_len : 0;
}

const device_config_param_info_t* uci_config_get_device_param_info(DeviceConfigId cfg_id) {
    return uci_config_metadata_find_device_param(cfg_id);
}

size_t uci_config_get_device_param_count(void) {
    return uci_config_metadata_get_device_param_count();
}

const device_config_param_info_t* uci_config_get_device_param_info_at(size_t index) {
    return uci_config_metadata_get_device_param_info_at(index);
}

int uci_config_lookup_device_param(const char* name, DeviceConfigId* cfg_id,
                                   const device_config_param_info_t** info_out) {
    if (!name || !cfg_id) {
        return -1;
    }

    const device_config_param_info_t* info = uci_config_metadata_find_device_param_by_name(name);
    if (info) {
        *cfg_id = info->cfg_id;
        if (info_out) {
            *info_out = info;
        }
        return 0;
    }

    errno = 0;
    char* endptr = NULL;
    unsigned long value = strtoul(name, &endptr, 0);
    if (errno == 0 && endptr && *endptr == '\0' && value <= 0xFF) {
        *cfg_id = (DeviceConfigId)value;
        if (info_out) {
            *info_out = uci_config_metadata_find_device_param((DeviceConfigId)value);
        }
        return 0;
    }

    return -1;
}

// Parse application parameter name to ID
int uci_config_parse_app_param_name(const char* name, AppConfigTlvType* cfg_id) {
    if (!name || !cfg_id) {
        return -1;
    }
    
    const config_param_info_t* info = uci_config_metadata_find_app_param_by_name(name);
    if (info) {
        *cfg_id = info->cfg_id;
        return 0;
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

int uci_config_lookup_app_param(const char* name, AppConfigTlvType* cfg_id,
                                const config_param_info_t** info_out) {
    if (!name || !cfg_id) {
        return -1;
    }

    const config_param_info_t* info = uci_config_metadata_find_app_param_by_name(name);
    if (!info) {
        AppConfigTlvType alias_cfg_id = 0;
        if (uci_config_metadata_lookup_app_param_alias(name, &alias_cfg_id) == 0) {
            info = uci_config_metadata_find_app_param(alias_cfg_id);
        }
    }

    if (!info) {
        errno = 0;
        char* endptr = NULL;
        unsigned long value = strtoul(name, &endptr, 0);
        if (errno == 0 && endptr && *endptr == '\0' && value <= 0xFF) {
            info = uci_config_metadata_find_app_param((AppConfigTlvType)value);
        }
    }

    if (!info) {
        return -1;
    }

    *cfg_id = info->cfg_id;
    if (info_out) {
        *info_out = info;
    }
    return 0;
}

// Parse device parameter name to ID
int uci_config_parse_device_param_name(const char* name, DeviceConfigId* cfg_id) {
    if (!name || !cfg_id) {
        return -1;
    }

    return uci_config_lookup_device_param(name, cfg_id, NULL);
}

int uci_config_parse_device_value(DeviceConfigId cfg_id, const char* value_str,
                                  unsigned char* value, size_t* value_len) {
    if (!value_str || !value || !value_len) {
        return -1;
    }

    const device_config_param_info_t* info = uci_config_get_device_param_info(cfg_id);
    if (!info) {
        return -1;
    }

    size_t expected_len = info->value_len > 0 ? info->value_len : 1;
    if (*value_len < expected_len) {
        return -1;
    }

    memset(value, 0, expected_len);

    if (cfg_id == DEVICE_STATE) {
        unsigned char state_value = 0;
        if (strcasecmp(value_str, "ready") == 0) {
            state_value = DEVICE_STATE_READY;
        } else if (strcasecmp(value_str, "active") == 0) {
            state_value = DEVICE_STATE_ACTIVE;
        } else if (strcasecmp(value_str, "error") == 0) {
            state_value = DEVICE_STATE_ERROR;
        } else {
            uint64_t numeric = 0;
            if (parse_unsigned_value(value_str, &numeric) != 0 || numeric > 0xFF) {
                return -1;
            }
            state_value = (unsigned char)numeric;
        }

        value[0] = state_value;
        *value_len = 1;
        return 0;
    }

    if (cfg_id == LOW_POWER_MODE ||
        cfg_id == DEVICE_PAN_COORD ||
        cfg_id == DEVICE_PROMISCUOUS) {
        unsigned char bool_value = 0;
        if (parse_boolean_value(value_str, &bool_value) != 0) {
            return -1;
        }
        value[0] = bool_value;
        *value_len = 1;
        return 0;
    }

    uint64_t numeric_value = 0;
    if (parse_unsigned_value(value_str, &numeric_value) != 0) {
        return -1;
    }

    if (numeric_value < info->min_value || numeric_value > info->max_value) {
        return -1;
    }

    for (size_t i = 0; i < expected_len; i++) {
        value[i] = (unsigned char)((numeric_value >> (8 * i)) & 0xFF);
    }

    *value_len = expected_len;
    return 0;
}

int uci_config_parse_app_value(AppConfigTlvType cfg_id, const char* value_str,
                               unsigned char* value, size_t* value_len) {
    if (!value_str || !value || !value_len) {
        return -1;
    }

    const config_param_info_t* info = uci_config_get_app_param_info(cfg_id);
    if (!info) {
        return -1;
    }

    size_t expected_len = info->value_len;
    if (expected_len == 0) {
        expected_len = 1;
    }

    if (*value_len < expected_len) {
        return -1;
    }

    if (cfg_id == SESSION_KEY || cfg_id == SUBSESSION_KEY) {
        const char* hex_str = value_str;
        size_t parsed_len = *value_len;
        if (strncmp(hex_str, "0x", 2) == 0 || strncmp(hex_str, "0X", 2) == 0) {
            hex_str += 2;
        }
        if (uci_config_parse_hex_value(hex_str, value, &parsed_len) != 0) {
            return -1;
        }
        if (parsed_len != 16 && parsed_len != 32) {
            return -1;
        }
        *value_len = parsed_len;
        return 0;
    }

    if (cfg_id == SESSION_TIME_BASE) {
        const char* hex_str = value_str;
        size_t parsed_len = *value_len;
        if (strncmp(hex_str, "0x", 2) == 0 || strncmp(hex_str, "0X", 2) == 0) {
            hex_str += 2;
        }
        if (uci_config_parse_hex_value(hex_str, value, &parsed_len) != 0) {
            return -1;
        }
        if (parsed_len != 9) {
            return -1;
        }
        *value_len = parsed_len;
        return 0;
    }

    if (cfg_id == DST_MAC_ADDRESS) {
        size_t parsed_len = *value_len;
        if (parse_dst_mac_address_list(value_str, value, &parsed_len) != 0) {
            return -1;
        }
        if (parsed_len == 0 || (parsed_len % 2) != 0 || parsed_len > 16) {
            return -1;
        }
        *value_len = parsed_len;
        return 0;
    }

    uint64_t numeric_value = 0;
    int have_numeric = 0;
    int mapped_value = 0;

    if (uci_config_metadata_lookup_app_value(cfg_id, value_str, &numeric_value) == 0) {
        have_numeric = 1;
        mapped_value = 1;
    }

    if (!have_numeric && expected_len == 1) {
        unsigned char bool_value = 0;
        if (parse_boolean_value(value_str, &bool_value) == 0) {
            numeric_value = bool_value;
            have_numeric = 1;
        }
    }

    if (!have_numeric) {
        if (parse_unsigned_value(value_str, &numeric_value) != 0) {
            return -1;
        }
    }

    if (!mapped_value) {
        if (numeric_value < info->min_value || numeric_value > info->max_value) {
            return -1;
        }
    }

    memset(value, 0, expected_len);
    for (size_t i = 0; i < expected_len; i++) {
        value[i] = (unsigned char)((numeric_value >> (8 * i)) & 0xFF);
    }

    *value_len = expected_len;
    return 0;
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
    const config_param_info_t* param = uci_config_metadata_find_app_param(cfg_id);
    if (param) {
        printf("Application Configuration Parameter: %s (0x%02X)\n", param->name, param->cfg_id);
        printf("  Description: %s\n", param->description);
        printf("  Default Value: %lu %s\n", (unsigned long)param->default_value, param->unit);
        printf("  Valid Range: %lu to %lu\n", (unsigned long)param->min_value, (unsigned long)param->max_value);
        if (strlen(param->unit) > 0) {
            printf("  Unit: %s\n", param->unit);
        }
        return 0;
    }
    
    printf("Unknown Application Configuration Parameter: 0x%02X\n", cfg_id);
    return -1;
}

// Show help for specific device parameter
int uci_config_show_device_param_help(DeviceConfigId cfg_id) {
    const device_config_param_info_t* param = uci_config_get_device_param_info(cfg_id);
    if (!param) {
        printf("Unknown Device Configuration Parameter: 0x%02X\n", cfg_id);
        return -1;
    }

    printf("Device Configuration Parameter: %s (0x%02X)\n", param->name, param->cfg_id);
    printf("  Description: %s\n", param->description);
    printf("  Default Value: %lu\n", (unsigned long)param->default_value);
    printf("  Valid Range: %lu to %lu\n", (unsigned long)param->min_value, (unsigned long)param->max_value);
    printf("  Value Length: %zu byte%s\n", param->value_len, param->value_len == 1 ? "" : "s");
    if (strlen(param->unit) > 0) {
        printf("  Unit: %s\n", param->unit);
    }
    return 0;
}

// Show all application parameters
int uci_config_show_all_app_params() {
    printf("=== All Application Configuration Parameters ===\n");
    size_t count = uci_config_get_app_param_count();
    for (size_t i = 0; i < count; i++) {
        const config_param_info_t* param = uci_config_get_app_param_info_at(i);
        if (!param) {
            continue;
        }
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
    printf("\nTotal: %zu parameters\n", count);
    return 0;
}

// Show all device parameters
int uci_config_show_all_device_params() {
    printf("=== All Device Configuration Parameters ===\n");
    size_t count = uci_config_get_device_param_count();
    for (size_t i = 0; i < count; i++) {
        const device_config_param_info_t* param = uci_config_get_device_param_info_at(i);
        if (!param) {
            continue;
        }
        printf("\n%s (0x%02X):\n", param->name, param->cfg_id);
        printf("  Description: %s\n", param->description);
        printf("  Default Value: %lu\n", (unsigned long)param->default_value);
        printf("  Valid Range: %lu to %lu\n", (unsigned long)param->min_value, (unsigned long)param->max_value);
        printf("  Value Length: %zu byte%s\n", param->value_len, param->value_len == 1 ? "" : "s");
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
    printf("\nTotal: %zu parameters\n", count);
    return 0;
}
