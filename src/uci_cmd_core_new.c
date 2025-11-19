#include <stdio.h>
#include <stdlib.h>
#include "../include/uci_cmd_core_new.h"
#include "../include/uci_cmd_core.h"
#include "../include/uci_command_utils.h"

// Handler for get_device_info command using framework
int handle_get_device_info_command_new(const char* cmd_name, int argc, char** argv, 
                                       const uci_param_def_t* params, int param_count) {
    // Unused parameters - prevent compiler warnings
    (void)cmd_name;
    (void)argc;
    (void)argv;
    (void)params;
    (void)param_count;
    
    handle_get_device_info_command();
    return 0;
}

// Handler for device_reset command using framework
int handle_device_reset_command_new(const char* cmd_name, int argc, char** argv, 
                                    const uci_param_def_t* params, int param_count) {
    // Unused parameters - prevent compiler warnings
    (void)cmd_name;
    (void)argc;
    (void)argv;
    (void)params;
    (void)param_count;
    
    handle_device_reset_command();
    return 0;
}

// Handler for set_power command using framework
int handle_set_power_command_new(const char* cmd_name, int argc, char** argv, 
                                 const uci_param_def_t* params, int param_count) {
    // Unused parameters - prevent compiler warnings
    (void)cmd_name;
    (void)argc;
    (void)argv;
    (void)params;
    (void)param_count;
    
    const uci_cmd_parsed_param_t* state_param = uci_cmd_get_parsed_param(0);
    if (state_param && state_param->present && state_param->type == PARAM_TYPE_DEVICE_STATE) {
        return handle_set_power_state_from_value(state_param->value.device_state);
    }

    if (argc < 2) {
        handle_set_power_command(NULL);
        return -1;
    }

    return handle_set_power_command(argv[1]);
}

// Handler for device_on command using framework
int handle_device_on_command_new(const char* cmd_name, int argc, char** argv, 
                                 const uci_param_def_t* params, int param_count) {
    // Unused parameters - prevent compiler warnings
    (void)cmd_name;
    (void)argc;
    (void)argv;
    (void)params;
    (void)param_count;
    handle_device_on_command();
    return 0;
}

// Handler for device_off command using framework
int handle_device_off_command_new(const char* cmd_name, int argc, char** argv, 
                                  const uci_param_def_t* params, int param_count) {
    // Unused parameters - prevent compiler warnings
    (void)cmd_name;
    (void)argc;
    (void)argv;
    (void)params;
    (void)param_count;
    handle_device_off_command();
    return 0;
}

// Handler for get_config command using framework
int handle_get_config_command_new(const char* cmd_name, int argc, char** argv, 
                                  const uci_param_def_t* params, int param_count) {
    // Unused parameters - prevent compiler warnings
    (void)cmd_name;
    (void)argc;
    (void)argv;
    (void)params;
    (void)param_count;
    
    if (argc < 2) {
        handle_get_config_command(NULL);
        return -1;
    }

    return handle_get_config_command(argv[1]);
}

int handle_get_caps_info_command_new(const char* cmd_name, int argc, char** argv,
                                     const uci_param_def_t* params, int param_count) {
    (void)cmd_name;
    (void)argc;
    (void)argv;
    (void)params;
    (void)param_count;
    handle_get_caps_info_command();
    return 0;
}

// Handler for get_device_state command using framework
int handle_get_device_state_command_new(const char* cmd_name, int argc, char** argv, 
                                        const uci_param_def_t* params, int param_count) {
    // Unused parameters - prevent compiler warnings
    (void)cmd_name;
    (void)argc;
    (void)argv;
    (void)params;
    (void)param_count;
    handle_get_device_state_command();
    return 0;
}

// Handler for set_device_active command using framework
int handle_set_device_active_command_new(const char* cmd_name, int argc, char** argv, 
                                         const uci_param_def_t* params, int param_count) {
    // Unused parameters - prevent compiler warnings
    (void)cmd_name;
    (void)argc;
    (void)argv;
    (void)params;
    (void)param_count;
    handle_set_device_active_command();
    return 0;
}

// Handler for set_device_ready command using framework
int handle_set_device_ready_command_new(const char* cmd_name, int argc, char** argv, 
                                        const uci_param_def_t* params, int param_count) {
    // Unused parameters - prevent compiler warnings
    (void)cmd_name;
    (void)argc;
    (void)argv;
    (void)params;
    (void)param_count;
    handle_set_device_ready_command();
    return 0;
}

// Handler for set_config command using framework
int handle_set_config_command_new(const char* cmd_name, int argc, char** argv, 
                                  const uci_param_def_t* params, int param_count) {
    // Unused parameters - prevent compiler warnings
    (void)cmd_name;
    (void)argc;
    (void)argv;
    (void)params;
    (void)param_count;
    
    if (argc < 3) {
        handle_set_config_command(NULL, NULL);
        return -1;
    }

    return handle_set_config_command(argv[1], argv[2]);
}

// Handler for device_suspend command using framework
int handle_device_suspend_command_new(const char* cmd_name, int argc, char** argv, 
                                      const uci_param_def_t* params, int param_count) {
    // Unused parameters - prevent compiler warnings
    (void)cmd_name;
    (void)argc;
    (void)argv;
    (void)params;
    (void)param_count;
    handle_device_suspend_command();
    return 0;
}

// Handler for query_timestamp command using framework
int handle_query_timestamp_command_new(const char* cmd_name, int argc, char** argv, 
                                       const uci_param_def_t* params, int param_count) {
    // Unused parameters - prevent compiler warnings
    (void)cmd_name;
    (void)argc;
    (void)argv;
    (void)params;
    (void)param_count;
    handle_query_timestamp_command();
    return 0;
}

/**
 * @brief Handler for validate_arguments_command using new framework
 * 
 * Demonstrates the use of the new argument validation and error reporting utilities.
 * Shows proper argument validation, numeric range validation, and consistent error reporting.
 * 
 * @param cmd_name Name of the command being executed
 * @param argc Argument count (including command name)
 * @param argv Argument values (argv[0] is command name)
 * @param params Parameter definitions for the command
 * @param param_count Number of parameters
 * @return 0 on success, -1 on error
 */
int handle_validate_arguments_command_new(const char* cmd_name, int argc, char** argv, 
                                          const uci_param_def_t* params, int param_count) {
    // Unused parameters - prevent compiler warnings
    (void)cmd_name;
    (void)argv;
    (void)params;
    (void)param_count;
    
    if (argc < 4) {
        fprintf(stderr, "Usage: validate_arguments <integer_value> <hex_string> <session_id>\n");
        fprintf(stderr, "  Examples:\n");
        fprintf(stderr, "    validate_arguments 42 AABBCCDD 1\n");
        fprintf(stderr, "    validate_arguments -10 FFEE 2\n");
        return -1;
    }

    const char* integer_str = argv[1];
    const char* hex_str = argv[2];
    const char* session_id_str = argv[3];
    
    // Validate integer value
    long integer_val;
    if (!validate_numeric_range(integer_str, -1000, 1000, "integer_value", &integer_val)) {
        return -1;
    }
    
    // Validate hex string
    if (!validate_hex_string(hex_str, 0)) {
        fprintf(stderr, "Error: Invalid hex string format for '%s'\n", hex_str);
        return -1;
    }
    
    // Validate session ID
    unsigned int session_id;
    if (uci_cmd_validate_session_id(session_id_str, &session_id) != 0) {
        return -1;
    }
    
    printf("Arguments validated successfully:\n");
    printf("  Integer Value: %ld\n", integer_val);
    printf("  Hex String: %s\n", hex_str);
    printf("  Session ID: %u\n", session_id);
    
    return 0;
}
