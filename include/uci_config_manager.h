#ifndef UCI_CONFIG_MANAGER_H
#define UCI_CONFIG_MANAGER_H

#include <stddef.h>
#include <stdint.h>
#include "uci_pdl.h"

// Configuration parameter information structure
typedef struct {
    AppConfigTlvType cfg_id;
    const char* name;
    const char* description;
    uint64_t min_value;
    uint64_t max_value;
    uint64_t default_value;
    size_t value_len;
    const char* unit;
} config_param_info_t;

// Device configuration parameter information structure
typedef struct {
    DeviceConfigId cfg_id;
    const char* name;
    const char* description;
    uint64_t min_value;
    uint64_t max_value;
    uint64_t default_value;
    size_t value_len;
    const char* unit;
} device_config_param_info_t;

// Configuration manager functions
int uci_config_init();
void uci_config_cleanup();

// Application configuration management
int uci_config_set_app_param(AppConfigTlvType cfg_id, const unsigned char* value, size_t value_len);
int uci_config_get_app_param(AppConfigTlvType cfg_id, unsigned char* value, size_t* value_len);
int uci_config_list_app_params();
const char* uci_config_get_app_param_name(AppConfigTlvType cfg_id);
const char* uci_config_get_app_param_desc(AppConfigTlvType cfg_id);
uint64_t uci_config_get_app_param_default(AppConfigTlvType cfg_id);
int uci_config_get_app_param_range(AppConfigTlvType cfg_id, uint64_t* min_val, uint64_t* max_val);
size_t uci_config_get_app_param_count(void);
const config_param_info_t* uci_config_get_app_param_info(AppConfigTlvType cfg_id);
const config_param_info_t* uci_config_get_app_param_info_at(size_t index);

// Device configuration management
int uci_config_set_device_param(DeviceConfigId cfg_id, const unsigned char* value, size_t value_len);
int uci_config_get_device_param(DeviceConfigId cfg_id, unsigned char* value, size_t* value_len);
int uci_config_list_device_params();
const char* uci_config_get_device_param_name(DeviceConfigId cfg_id);
const char* uci_config_get_device_param_desc(DeviceConfigId cfg_id);
uint64_t uci_config_get_device_param_default(DeviceConfigId cfg_id);
int uci_config_get_device_param_range(DeviceConfigId cfg_id, uint64_t* min_val, uint64_t* max_val);
size_t uci_config_get_device_param_length(DeviceConfigId cfg_id);
const device_config_param_info_t* uci_config_get_device_param_info(DeviceConfigId cfg_id);
size_t uci_config_get_device_param_count(void);
const device_config_param_info_t* uci_config_get_device_param_info_at(size_t index);
int uci_config_lookup_device_param(const char* name, DeviceConfigId* cfg_id,
                                   const device_config_param_info_t** info_out);

// Helper functions for parameter parsing
int uci_config_parse_app_param_name(const char* name, AppConfigTlvType* cfg_id);
int uci_config_lookup_app_param(const char* name, AppConfigTlvType* cfg_id,
                                const config_param_info_t** info_out);
int uci_config_parse_device_param_name(const char* name, DeviceConfigId* cfg_id);
int uci_config_parse_device_value(DeviceConfigId cfg_id, const char* value_str,
                                  unsigned char* value, size_t* value_len);
int uci_config_parse_app_value(AppConfigTlvType cfg_id, const char* value_str,
                               unsigned char* value, size_t* value_len);
int uci_config_parse_hex_value(const char* hex_str, unsigned char* value, size_t* value_len);
int uci_config_format_hex_value(const unsigned char* value, size_t value_len, char* output, size_t output_len);

// User-friendly interface functions
int uci_config_show_app_param_help(AppConfigTlvType cfg_id);
int uci_config_show_device_param_help(DeviceConfigId cfg_id);
int uci_config_show_all_app_params();
int uci_config_show_all_device_params();

#endif // UCI_CONFIG_MANAGER_H
