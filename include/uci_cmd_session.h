#ifndef UCI_CMD_SESSION_H
#define UCI_CMD_SESSION_H

#include <stdint.h>
#include "uci.h"

// Session management command handlers
int handle_session_init_command(char* session_id_str, char* session_type_str);
int handle_session_init_command_values(uint32_t session_id, SessionType session_type);
int handle_session_deinit_command(char* session_id_str);
int handle_session_start_command(char* session_id_str);
int handle_session_stop_command(char* session_id_str);
int handle_get_session_state_command(char* session_id_str);
int handle_session_send_data_command(char* session_id_str,
                                    char* destination_str,
                                    char* sequence_str,
                                    char* payload_str);

int handle_session_logical_link_create_command(char* session_id_str,
                                               char* link_id_str,
                                               char* mode_str,
                                               char* credit_str);

int handle_session_logical_link_close_command(char* session_id_str,
                                              char* link_id_str);

int handle_session_logical_link_get_param_command(char* session_id_str,
                                                  char* link_id_str);

#endif // UCI_CMD_SESSION_H
