#ifndef UCI_CMD_CORE_H
#define UCI_CMD_CORE_H

// Core UCI command handlers
void handle_get_device_info_command(void);
void handle_device_reset_command(void);
void handle_get_caps_info_command(void);

// Power/state management commands
int handle_set_power_command(char* power_state);
int handle_set_power_state_from_value(unsigned char device_state_value);
void handle_device_on_command(void);
void handle_device_off_command(void);

// Device configuration commands
int handle_get_config_command(char* config_name);
void handle_get_device_state_command(void);
void handle_set_device_active_command(void);
void handle_set_device_ready_command(void);
int handle_set_config_command(char* config_name, char* value_str);

// Device suspend and timestamp commands
void handle_device_suspend_command(void);
void handle_query_timestamp_command(void);

// CLI helpers for configuration discovery
int cmd_show_device_configs(int argc, char** argv);
int cmd_show_app_configs(int argc, char** argv);
int show_device_configs_with_filters(const char* id_filter, const char* name_filter, int full);
int show_app_configs_with_filters(const char* id_filter, const char* name_filter, int full);

#endif // UCI_CMD_CORE_H
