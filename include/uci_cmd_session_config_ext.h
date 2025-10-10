#ifndef UCI_CMD_SESSION_CONFIG_EXT_H
#define UCI_CMD_SESSION_CONFIG_EXT_H

/**
 * @file uci_cmd_session_config_ext.h
 * @brief Extended session configuration command handlers
 *
 * This module provides handlers for extended UCI session configuration commands including
 * hybrid controller/controlee configurations and data size querying.
 */

/**
 * @brief Handle session_set_hybrid_controller_config command
 * @param session_id_str String representation of session ID
 * @param config_data Configuration data for hybrid controller
 * @param config_len Length of configuration data
 * @return 0 on success, -1 on error
 */
int handle_session_set_hybrid_controller_config_command(char* session_id_str, unsigned char* config_data, int config_len);

/**
 * @brief Handle session_set_hybrid_controlee_config command
 * @param session_id_str String representation of session ID
 * @param config_data Configuration data for hybrid controlee
 * @param config_len Length of configuration data
 * @return 0 on success, -1 on error
 */
int handle_session_set_hybrid_controlee_config_command(char* session_id_str, unsigned char* config_data, int config_len);

/**
 * @brief Handle session_query_data_size_in_ranging command
 * @param session_id_str String representation of session ID
 * @return 0 on success, -1 on error
 */
int handle_session_query_data_size_in_ranging_command(char* session_id_str);

#endif /* UCI_CMD_SESSION_CONFIG_EXT_H */