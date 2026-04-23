#ifndef UCI_CONFIG_METADATA_H
#define UCI_CONFIG_METADATA_H

#include "uci_config_manager.h"

const config_param_info_t* uci_config_metadata_find_app_param(AppConfigTlvType cfg_id);
const config_param_info_t* uci_config_metadata_find_app_param_by_name(const char* name);
size_t uci_config_metadata_get_app_param_count(void);
const config_param_info_t* uci_config_metadata_get_app_param_info_at(size_t index);
int uci_config_metadata_lookup_app_param_alias(const char* name, AppConfigTlvType* cfg_id);
int uci_config_metadata_lookup_app_value(AppConfigTlvType cfg_id, const char* token, uint64_t* value);

const device_config_param_info_t* uci_config_metadata_find_device_param(DeviceConfigId cfg_id);
const device_config_param_info_t* uci_config_metadata_find_device_param_by_name(const char* name);
size_t uci_config_metadata_get_device_param_count(void);
const device_config_param_info_t* uci_config_metadata_get_device_param_info_at(size_t index);

#endif // UCI_CONFIG_METADATA_H
