#ifndef UCI_CMD_CORE_TYPED_H
#define UCI_CMD_CORE_TYPED_H

#include "uci_command_framework.h"

int handle_get_device_info_command_typed(const char* cmd_name, int argc, char** argv,
                                         const uci_param_def_t* params, int param_count);
int handle_device_reset_command_typed(const char* cmd_name, int argc, char** argv,
                                      const uci_param_def_t* params, int param_count);
int handle_set_power_command_typed(const char* cmd_name, int argc, char** argv,
                                   const uci_param_def_t* params, int param_count);
int handle_device_on_command_typed(const char* cmd_name, int argc, char** argv,
                                   const uci_param_def_t* params, int param_count);
int handle_device_off_command_typed(const char* cmd_name, int argc, char** argv,
                                    const uci_param_def_t* params, int param_count);
int handle_get_config_command_typed(const char* cmd_name, int argc, char** argv,
                                    const uci_param_def_t* params, int param_count);
int handle_get_caps_info_command_typed(const char* cmd_name, int argc, char** argv,
                                       const uci_param_def_t* params, int param_count);
int handle_show_device_configs_command_typed(const char* cmd_name, int argc, char** argv,
                                             const uci_param_def_t* params, int param_count);
int handle_show_app_configs_command_typed(const char* cmd_name, int argc, char** argv,
                                          const uci_param_def_t* params, int param_count);
int handle_get_device_state_command_typed(const char* cmd_name, int argc, char** argv,
                                          const uci_param_def_t* params, int param_count);
int handle_set_device_active_command_typed(const char* cmd_name, int argc, char** argv,
                                           const uci_param_def_t* params, int param_count);
int handle_set_device_ready_command_typed(const char* cmd_name, int argc, char** argv,
                                          const uci_param_def_t* params, int param_count);
int handle_set_config_command_typed(const char* cmd_name, int argc, char** argv,
                                    const uci_param_def_t* params, int param_count);
int handle_device_suspend_command_typed(const char* cmd_name, int argc, char** argv,
                                        const uci_param_def_t* params, int param_count);
int handle_query_timestamp_command_typed(const char* cmd_name, int argc, char** argv,
                                         const uci_param_def_t* params, int param_count);
int handle_validate_arguments_command_typed(const char* cmd_name, int argc, char** argv,
                                            const uci_param_def_t* params, int param_count);

#endif // UCI_CMD_CORE_TYPED_H
