#ifndef UCI_CMD_HANDLERS_H
#define UCI_CMD_HANDLERS_H

int cmd_session_init(int argc, char** argv);
int cmd_session_deinit(int argc, char** argv);
int cmd_session_start(int argc, char** argv);
int cmd_session_stop(int argc, char** argv);
int cmd_session_send_data(int argc, char** argv);
int cmd_session_logical_link_create(int argc, char** argv);
int cmd_session_logical_link_close(int argc, char** argv);
int cmd_session_logical_link_get_param(int argc, char** argv);
int cmd_get_session_state(int argc, char** argv);
int cmd_set_app_config(int argc, char** argv);
int cmd_get_app_config(int argc, char** argv);
int cmd_session_update_multicast_list(int argc, char** argv);
int cmd_session_update_dt_tag_rounds(int argc, char** argv);
int cmd_session_data_transfer_phase_config(int argc, char** argv);
int cmd_session_set_hybrid_controller_config(int argc, char** argv);
int cmd_session_set_hybrid_controlee_config(int argc, char** argv);
int cmd_session_query_data_size_in_ranging(int argc, char** argv);

#endif // UCI_CMD_HANDLERS_H
