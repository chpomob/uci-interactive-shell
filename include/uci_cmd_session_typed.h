#ifndef UCI_CMD_SESSION_TYPED_H
#define UCI_CMD_SESSION_TYPED_H

#include "uci_command_framework.h"

int handle_session_init_command_typed(const uci_cmd_dispatch_context_t* dispatch_ctx, const char* cmd_name, int argc, char** argv,
                                      const uci_param_def_t* params, int param_count);
int handle_session_deinit_command_typed(const uci_cmd_dispatch_context_t* dispatch_ctx, const char* cmd_name, int argc, char** argv,
                                        const uci_param_def_t* params, int param_count);
int handle_session_start_command_typed(const uci_cmd_dispatch_context_t* dispatch_ctx, const char* cmd_name, int argc, char** argv,
                                       const uci_param_def_t* params, int param_count);
int handle_session_stop_command_typed(const uci_cmd_dispatch_context_t* dispatch_ctx, const char* cmd_name, int argc, char** argv,
                                      const uci_param_def_t* params, int param_count);
int handle_session_send_data_command_typed(const uci_cmd_dispatch_context_t* dispatch_ctx, const char* cmd_name, int argc, char** argv,
                                           const uci_param_def_t* params, int param_count);
int handle_session_logical_link_create_command_typed(const uci_cmd_dispatch_context_t* dispatch_ctx, const char* cmd_name, int argc, char** argv,
                                                     const uci_param_def_t* params, int param_count);
int handle_session_logical_link_close_command_typed(const uci_cmd_dispatch_context_t* dispatch_ctx, const char* cmd_name, int argc, char** argv,
                                                    const uci_param_def_t* params, int param_count);
int handle_session_logical_link_get_param_command_typed(const uci_cmd_dispatch_context_t* dispatch_ctx, const char* cmd_name, int argc, char** argv,
                                                        const uci_param_def_t* params, int param_count);
int handle_get_session_state_command_typed(const uci_cmd_dispatch_context_t* dispatch_ctx, const char* cmd_name, int argc, char** argv,
                                           const uci_param_def_t* params, int param_count);
int handle_set_app_config_command_typed(const uci_cmd_dispatch_context_t* dispatch_ctx, const char* cmd_name, int argc, char** argv,
                                        const uci_param_def_t* params, int param_count);
int handle_get_app_config_command_typed(const uci_cmd_dispatch_context_t* dispatch_ctx, const char* cmd_name, int argc, char** argv,
                                        const uci_param_def_t* params, int param_count);
int handle_session_update_multicast_list_command_typed(const uci_cmd_dispatch_context_t* dispatch_ctx, const char* cmd_name, int argc, char** argv,
                                                       const uci_param_def_t* params, int param_count);
int handle_session_update_dt_tag_rounds_command_typed(const uci_cmd_dispatch_context_t* dispatch_ctx, const char* cmd_name, int argc, char** argv,
                                                      const uci_param_def_t* params, int param_count);
int handle_session_data_transfer_phase_config_command_typed(const uci_cmd_dispatch_context_t* dispatch_ctx, const char* cmd_name, int argc, char** argv,
                                                            const uci_param_def_t* params, int param_count);
int handle_session_set_hybrid_controller_config_command_typed(const uci_cmd_dispatch_context_t* dispatch_ctx, const char* cmd_name, int argc, char** argv,
                                                              const uci_param_def_t* params, int param_count);
int handle_session_set_hybrid_controlee_config_command_typed(const uci_cmd_dispatch_context_t* dispatch_ctx, const char* cmd_name, int argc, char** argv,
                                                             const uci_param_def_t* params, int param_count);
int handle_session_query_data_size_in_ranging_command_typed(const uci_cmd_dispatch_context_t* dispatch_ctx, const char* cmd_name, int argc, char** argv,
                                                            const uci_param_def_t* params, int param_count);

#endif
