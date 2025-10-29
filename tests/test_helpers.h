#ifndef TEST_HELPERS_H
#define TEST_HELPERS_H

#include <stdint.h>

// Structure to hold the decoded SESSION_INIT command
typedef struct {
    uint32_t session_id;
    uint8_t session_type;
} decoded_session_init_cmd_t;

// Decodes a SESSION_INIT command payload for testing
int test_decode_session_init_cmd(const unsigned char* payload, int payload_len, decoded_session_init_cmd_t* decoded_cmd);

// Structure to hold the decoded SESSION_DEINIT command
typedef struct {
    uint32_t session_id;
} decoded_session_deinit_cmd_t;

// Decodes a SESSION_DEINIT command payload for testing
int test_decode_session_deinit_cmd(const unsigned char* payload, int payload_len, decoded_session_deinit_cmd_t* decoded_cmd);

// Structure to hold the decoded SESSION_START command
typedef struct {
    uint32_t session_id;
} decoded_session_start_cmd_t;

// Decodes a SESSION_START command payload for testing
int test_decode_session_start_cmd(const unsigned char* payload, int payload_len, decoded_session_start_cmd_t* decoded_cmd);

// Structure to hold the decoded SESSION_STOP command
typedef struct {
    uint32_t session_id;
} decoded_session_stop_cmd_t;

// Decodes a SESSION_STOP command payload for testing
int test_decode_session_stop_cmd(const unsigned char* payload, int payload_len, decoded_session_stop_cmd_t* decoded_cmd);

// Structure to hold the decoded GET_SESSION_STATE command
typedef struct {
    uint32_t session_id;
} decoded_get_session_state_cmd_t;

// Decodes a GET_SESSION_STATE command payload for testing
int test_decode_get_session_state_cmd(const unsigned char* payload, int payload_len, decoded_get_session_state_cmd_t* decoded_cmd);

// Structure to hold the decoded DEVICE_RESET command
typedef struct {
    uint8_t reset_config;
} decoded_device_reset_cmd_t;

// Decodes a DEVICE_RESET command payload for testing
int test_decode_device_reset_cmd(const unsigned char* payload, int payload_len, decoded_device_reset_cmd_t* decoded_cmd);

// Structure to hold the decoded SET_CONFIG command
typedef struct {
    uint8_t num_configs;
    uint8_t configs[255];
} decoded_set_config_cmd_t;

// Decodes a SET_CONFIG command payload for testing
int test_decode_set_config_cmd(const unsigned char* payload, int payload_len, decoded_set_config_cmd_t* decoded_cmd);

#endif // TEST_HELPERS_H
