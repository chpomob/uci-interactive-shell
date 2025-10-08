#ifndef UCI_CMD_SESSION_CONFIG_H
#define UCI_CMD_SESSION_CONFIG_H

/**
 * @file uci_cmd_session_config.h
 * @brief Session configuration command handlers
 *
 * This module provides handlers for UCI session configuration commands including
 * application parameter configuration and multicast list management.
 */

/**
 * @brief Handle set_app_config command to configure session parameters
 * @param session_id_str String representation of session ID
 * @param config_name Configuration parameter name (e.g., "device_type", "channel")
 * @param value_str String representation of value
 * @return 0 on success, -1 on error
 */
int handle_set_app_config_command(char* session_id_str, char* config_name, char* value_str);

/**
 * @brief Handle get_app_config command to retrieve session parameters
 * @param session_id_str String representation of session ID
 * @param config_name Configuration parameter name
 * @return 0 on success, -1 on error
 */
int handle_get_app_config_command(char* session_id_str, char* config_name);

/**
 * @brief Handle session_update_multicast_list command
 * @param session_id_str String representation of session ID
 * @param action_str Action to perform ("add" or "remove")
 * @param address_str Short address to add/remove
 * @return 0 on success, -1 on error
 */
int handle_update_multicast_list_command(char* session_id_str, char* action_str, char* address_str);

#endif /* UCI_CMD_SESSION_CONFIG_H */
