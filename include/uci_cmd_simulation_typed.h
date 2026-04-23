#ifndef UCI_CMD_SIMULATION_TYPED_H
#define UCI_CMD_SIMULATION_TYPED_H

#include "uci_command_framework.h"

int handle_simulate_notification_command_typed(const uci_cmd_dispatch_context_t* dispatch_ctx, const char* cmd_name, int argc, char** argv,
                                               const uci_param_def_t* params, int param_count);
int handle_simulate_session_status_command_typed(const uci_cmd_dispatch_context_t* dispatch_ctx, const char* cmd_name, int argc, char** argv,
                                                 const uci_param_def_t* params, int param_count);
int handle_simulate_data_credit_command_typed(const uci_cmd_dispatch_context_t* dispatch_ctx, const char* cmd_name, int argc, char** argv,
                                              const uci_param_def_t* params, int param_count);
int handle_demo_session_flow_command_typed(const uci_cmd_dispatch_context_t* dispatch_ctx, const char* cmd_name, int argc, char** argv,
                                           const uci_param_def_t* params, int param_count);
int handle_simulate_ranging_command_typed(const uci_cmd_dispatch_context_t* dispatch_ctx, const char* cmd_name, int argc, char** argv,
                                          const uci_param_def_t* params, int param_count);
int handle_simulate_multi_target_ranging_command_typed(const uci_cmd_dispatch_context_t* dispatch_ctx, const char* cmd_name, int argc, char** argv,
                                                       const uci_param_def_t* params, int param_count);
int handle_simulate_qm_sdk_vendor_command_typed(const uci_cmd_dispatch_context_t* dispatch_ctx, const char* cmd_name, int argc, char** argv,
                                                const uci_param_def_t* params, int param_count);

#endif
