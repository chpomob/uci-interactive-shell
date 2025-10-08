#ifndef UCI_CMD_SESSION_H
#define UCI_CMD_SESSION_H

// Session management command handlers
int handle_session_init_command(char* session_id_str, char* session_type_str);
int handle_session_deinit_command(char* session_id_str);
int handle_session_start_command(char* session_id_str);
int handle_session_stop_command(char* session_id_str);
int handle_get_session_state_command(char* session_id_str);

#endif // UCI_CMD_SESSION_H
