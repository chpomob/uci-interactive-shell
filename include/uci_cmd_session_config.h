#ifndef UCI_CMD_SESSION_CONFIG_H
#define UCI_CMD_SESSION_CONFIG_H

#include <stddef.h>
#include <stdint.h>

/**
 * @file uci_cmd_session_config.h
 * @brief Session configuration command handlers
 *
 * This module provides handlers for UCI session configuration commands including
 * application parameter configuration and multicast list management.
 */

int handle_set_app_config_command_value(uint32_t session_id, const char* config_name, const char* value_str);

int handle_get_app_config_command_value(uint32_t session_id, const char* config_name);

int handle_update_multicast_list_command_values(uint32_t session_id,
                                                const char* action_str,
                                                unsigned short short_address,
                                                uint32_t subsession_id);

int handle_session_query_data_size_in_ranging_command_value(uint32_t session_id);

int handle_session_update_dt_tag_rounds_command_values(uint32_t session_id,
                                                       const unsigned char* round_bytes,
                                                       size_t round_count);

int handle_session_data_transfer_phase_config_command_values(uint32_t session_id,
                                                             unsigned char repetition,
                                                             unsigned char control,
                                                             unsigned char size,
                                                             const unsigned char* payload,
                                                             size_t payload_len);

#endif /* UCI_CMD_SESSION_CONFIG_H */
