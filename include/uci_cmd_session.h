#ifndef UCI_CMD_SESSION_H
#define UCI_CMD_SESSION_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "uci.h"

// Session management command handlers
int handle_session_init_command_values(uint32_t session_id, SessionType session_type);
int handle_session_deinit_command_value(uint32_t session_id);
int handle_session_start_command_value(uint32_t session_id);
int handle_session_stop_command_value(uint32_t session_id);
int handle_get_session_state_command_value(uint32_t session_id);
int handle_session_send_data_command_values(uint32_t session_id,
                                            uint64_t destination,
                                            uint16_t sequence,
                                            const unsigned char* payload,
                                            size_t payload_len);

int handle_session_logical_link_create_command_values(uint32_t session_id,
                                                      unsigned char link_id,
                                                      bool mode_present,
                                                      unsigned char mode,
                                                      bool credit_present,
                                                      unsigned char credit);

int handle_session_logical_link_close_command_value(uint32_t session_id,
                                                    unsigned char link_id);

int handle_session_logical_link_get_param_command_value(uint32_t session_id,
                                                        unsigned char link_id);

#endif // UCI_CMD_SESSION_H
