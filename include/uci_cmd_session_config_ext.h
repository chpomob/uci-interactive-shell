#ifndef UCI_CMD_SESSION_CONFIG_EXT_H
#define UCI_CMD_SESSION_CONFIG_EXT_H

#include <stddef.h>
#include <stdint.h>

/**
 * @file uci_cmd_session_config_ext.h
 * @brief Extended session configuration command handlers
 *
 * This module provides handlers for extended UCI session configuration commands including
 * hybrid controller/controlee configurations and data size querying.
 */

int handle_session_set_hybrid_controller_config_command_value(uint32_t session_id,
                                                              const char* config_data,
                                                              size_t config_len);
int handle_session_set_hybrid_controlee_config_command_value(uint32_t session_id,
                                                             const char* config_data,
                                                             size_t config_len);
int handle_session_query_data_size_in_ranging_command_value(uint32_t session_id);

#endif /* UCI_CMD_SESSION_CONFIG_EXT_H */
