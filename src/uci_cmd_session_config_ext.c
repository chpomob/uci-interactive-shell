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
#include <string.h>

#define MAX_PAYLOAD_LENGTH 255

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
