#ifndef UCI_CMD_SESSION_CONFIG_H
#define UCI_CMD_SESSION_CONFIG_H

#include <stddef.h>
#include <stdint.h>

#include <stdint.h>

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
int handle_set_app_config_command_value(uint32_t session_id, const char* config_name, const char* value_str);

/**
 * @brief Handle get_app_config command to retrieve session parameters
 * @param session_id_str String representation of session ID
 * @param config_names Array of configuration parameter names
 * @param config_count Number of configuration names provided
 * @return 0 on success, -1 on error
 */
int handle_get_app_config_command(char* session_id_str, char** config_names, int config_count);
int handle_get_app_config_command_value(uint32_t session_id, const char* config_name);

/**
 * @brief Handle session_update_multicast_list command
 * @param session_id_str String representation of session ID
 * @param action_str Action to perform ("add" or "remove")
 * @param address_str Short address to add/remove
 * @param subsession_id_str Sub-session identifier string
 * @return 0 on success, -1 on error
 */
int handle_update_multicast_list_command(char* session_id_str,
                                         char* action_str,
                                         char* address_str,
                                         char* subsession_id_str);
int handle_update_multicast_list_command_values(uint32_t session_id,
                                                const char* action_str,
                                                unsigned short short_address,
                                                uint32_t subsession_id);

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
int handle_session_query_data_size_in_ranging_command_value(uint32_t session_id);

/**
 * @brief Handle session_update_dt_tag_rounds command to configure DT-Tag active rounds
 * @param session_id_str String representation of session ID
 * @param round_values Array of string arguments representing round indices
 * @param round_count Number of provided round indices
 * @return 0 on success, -1 on error
 */
int handle_session_update_dt_tag_rounds_command(char* session_id_str,
                                               char** round_values,
                                               int round_count);
int handle_session_update_dt_tag_rounds_command_values(uint32_t session_id,
                                                       const unsigned char* round_bytes,
                                                       size_t round_count);

/**
 * @brief Handle session_data_transfer_phase_config command to configure DTP settings
 * @param session_id_str String representation of session ID
 * @param repetition_str Repetition count argument
 * @param control_str Control flags argument
 * @param size_str Declared payload size argument
 * @param payload_values Optional payload byte arguments
 * @param payload_count Number of payload byte arguments
 * @return 0 on success, -1 on error
 */
int handle_session_data_transfer_phase_config_command(char* session_id_str,
                                                      char* repetition_str,
                                                      char* control_str,
                                                      char* size_str,
                                                      char** payload_values,
                                                      int payload_count);
int handle_session_data_transfer_phase_config_command_values(uint32_t session_id,
                                                             unsigned char repetition,
                                                             unsigned char control,
                                                             unsigned char size,
                                                             const unsigned char* payload,
                                                             size_t payload_len);

int handle_session_set_hybrid_controller_config_command_value(uint32_t session_id,
                                                              const char* config_data,
                                                              size_t config_len);
int handle_session_set_hybrid_controlee_config_command_value(uint32_t session_id,
                                                             const char* config_data,
                                                             size_t config_len);

#endif /* UCI_CMD_SESSION_CONFIG_H */
