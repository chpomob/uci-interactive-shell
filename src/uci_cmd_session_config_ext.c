/**
 * @file uci_cmd_session_config_ext.c
 * @brief Extended session configuration command handlers implementation
 *
 * This file implements additional UCI session configuration commands that extend
 * the basic functionality with hybrid controller/controlee configurations and
 * data size querying capabilities.
 */

#include "../include/uci_cmd_session_config.h"
#include "../include/uci.h"
#include "../include/uci_functions.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_PAYLOAD_LENGTH 255

/**
 * @brief Handle session_set_hybrid_controller_config command
 * @param session_id_str String representation of session ID
 * @param config_data Configuration data for hybrid controller
 * @param config_len Length of configuration data
 * @return 0 on success, -1 on error
 */
int handle_session_set_hybrid_controller_config_command(char* session_id_str, unsigned char* config_data, int config_len) {
    if (!session_id_str) {
        printf("Usage: session_set_hybrid_controller_config <session_id> [config_data]\n");
        printf("  Examples:\n");
        printf("    session_set_hybrid_controller_config 1\n");
        printf("    session_set_hybrid_controller_config 1 01020304\n");
        return -1;
    }

    unsigned int session_id = (unsigned int)strtoul(session_id_str, NULL, 10);
    
    // Validate session_id
    if (session_id == 0) {
        printf("Error: Invalid session ID. Must be a positive integer.\n");
        return -1;
    }

    return handle_session_set_hybrid_controller_config_command_value(
        session_id, (const char*)config_data, (size_t)config_len);
}

/**
 * @brief Handle session_set_hybrid_controlee_config command
 * @param session_id_str String representation of session ID
 * @param config_data Configuration data for hybrid controlee
 * @param config_len Length of configuration data
 * @return 0 on success, -1 on error
 */
int handle_session_set_hybrid_controlee_config_command(char* session_id_str, unsigned char* config_data, int config_len) {
    if (!session_id_str) {
        printf("Usage: session_set_hybrid_controlee_config <session_id> [config_data]\n");
        printf("  Examples:\n");
        printf("    session_set_hybrid_controlee_config 1\n");
        printf("    session_set_hybrid_controlee_config 1 01020304\n");
        return -1;
    }

    unsigned int session_id = (unsigned int)strtoul(session_id_str, NULL, 10);
    
    // Validate session_id
    if (session_id == 0) {
        printf("Error: Invalid session ID. Must be a positive integer.\n");
        return -1;
    }

    return handle_session_set_hybrid_controlee_config_command_value(
        session_id, (const char*)config_data, (size_t)config_len);
}

/**
 * @brief Handle session_query_data_size_in_ranging command
 * @param session_id_str String representation of session ID
 * @return 0 on success, -1 on error
 */
int handle_session_query_data_size_in_ranging_command(char* session_id_str) {
    if (!session_id_str) {
        printf("Usage: session_query_data_size_in_ranging <session_id>\n");
        printf("  Example: session_query_data_size_in_ranging 1\n");
        return -1;
    }

    unsigned int session_id = (unsigned int)strtoul(session_id_str, NULL, 10);
    
    // Validate session_id
    if (session_id == 0) {
        printf("Error: Invalid session ID. Must be a positive integer.\n");
        return -1;
    }

    return handle_session_query_data_size_in_ranging_command_value(session_id);
}

int handle_session_query_data_size_in_ranging_command_value(uint32_t session_id) {
    unsigned char payload[4];

    payload[0] = session_id & 0xFF;
    payload[1] = (session_id >> 8) & 0xFF;
    payload[2] = (session_id >> 16) & 0xFF;
    payload[3] = (session_id >> 24) & 0xFF;

    send_uci_command(COMMAND, 0, SESSION_CONFIG, SESSION_QUERY_DATA_SIZE_IN_RANGING, payload, sizeof(payload));
    return 0;
}

int handle_session_set_hybrid_controller_config_command_value(uint32_t session_id,
                                                              const char* config_data,
                                                              size_t config_len) {
    unsigned char payload[MAX_PAYLOAD_LENGTH];
    size_t payload_len = 4;

    payload[0] = session_id & 0xFF;
    payload[1] = (session_id >> 8) & 0xFF;
    payload[2] = (session_id >> 16) & 0xFF;
    payload[3] = (session_id >> 24) & 0xFF;

    if (config_data && config_len > 0) {
        if (config_len > MAX_PAYLOAD_LENGTH - 4) {
            printf("Error: Configuration data too long. Maximum %d bytes allowed.\n", MAX_PAYLOAD_LENGTH - 4);
            return -1;
        }
        memcpy(&payload[4], config_data, config_len);
        payload_len += config_len;
    }

    send_uci_command(COMMAND, 0, SESSION_CONFIG, SESSION_SET_HYBRID_CONTROLLER_CONFIG,
                     payload, (int)payload_len);
    return 0;
}

int handle_session_set_hybrid_controlee_config_command_value(uint32_t session_id,
                                                             const char* config_data,
                                                             size_t config_len) {
    unsigned char payload[MAX_PAYLOAD_LENGTH];
    size_t payload_len = 4;

    payload[0] = session_id & 0xFF;
    payload[1] = (session_id >> 8) & 0xFF;
    payload[2] = (session_id >> 16) & 0xFF;
    payload[3] = (session_id >> 24) & 0xFF;

    if (config_data && config_len > 0) {
        if (config_len > MAX_PAYLOAD_LENGTH - 4) {
            printf("Error: Configuration data too long. Maximum %d bytes allowed.\n", MAX_PAYLOAD_LENGTH - 4);
            return -1;
        }
        memcpy(&payload[4], config_data, config_len);
        payload_len += config_len;
    }

    send_uci_command(COMMAND, 0, SESSION_CONFIG, SESSION_SET_HYBRID_CONTROLEE_CONFIG,
                     payload, (int)payload_len);
    return 0;
}
