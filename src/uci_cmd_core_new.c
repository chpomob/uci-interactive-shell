#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include "../include/uci_cmd_core_new.h"
#include "../include/uci_cmd_core.h"
#include "../include/uci_command_utils.h"
#include "../include/uci_ui.h"

static const char* get_raw_param_value(int index) {
    const uci_cmd_parsed_param_t* param = uci_cmd_get_parsed_param(index);
    if (!param || !param->present || !param->raw_value || param->raw_value[0] == '\0') {
        return NULL;
    }
    return param->raw_value;
}

static int parse_detail_option(const char* value, int* detail_full) {
    if (!value || value[0] == '\0' || strcasecmp(value, "summary") == 0) {
        *detail_full = 0;
        return 0;
    }
    if (strcasecmp(value, "full") == 0 || strcasecmp(value, "detailed") == 0 || strcasecmp(value, "details") == 0) {
        *detail_full = 1;
        return 0;
    }
    char buffer[128];
    snprintf(buffer, sizeof(buffer), "Invalid detail option '%s' (expected summary or full)", value);
    ui_print_error(buffer);
    return -1;
}

static int parse_config_list_options(int param_count,
                                     const char** id_filter,
                                     const char** name_filter,
                                     int* detail_full) {
    for (int i = 0; i < param_count; i++) {
        const uci_cmd_parsed_param_t* param = uci_cmd_get_parsed_param(i);
        if (!param || !param->present || !param->raw_value) {
            continue;
        }

        const char* raw = param->raw_value;
        if (!raw || raw[0] == '\0') {
            continue;
        }

        if (strcmp(raw, "--full") == 0 || strcmp(raw, "-f") == 0) {
            *detail_full = 1;
            continue;
        }

        if (strcmp(raw, "--summary") == 0) {
            *detail_full = 0;
            continue;
        }

        if (strcmp(raw, "--filter") == 0) {
            const char* value = get_raw_param_value(i + 1);
            if (!value) {
                ui_print_error("Missing value for --filter");
                return -1;
            }
            *name_filter = value;
            i++;
            continue;
        }

        if (strcmp(raw, "--id") == 0) {
            const char* value = get_raw_param_value(i + 1);
            if (!value) {
                ui_print_error("Missing value for --id");
                return -1;
            }
            *id_filter = value;
            i++;
            continue;
        }

        const char* equals = strchr(raw, '=');
        if (equals) {
            size_t key_len = (size_t)(equals - raw);
            if (key_len == 0) {
                ui_print_error("Invalid option syntax (expected key=value)");
                return -1;
            }
            const char* value = equals + 1;
            if (key_len == 6 && strncasecmp(raw, "filter", key_len) == 0) {
                *name_filter = (value && value[0] != '\0') ? value : NULL;
                continue;
            }
            if (key_len == 2 && strncasecmp(raw, "id", key_len) == 0) {
                if (!value || value[0] == '\0') {
                    ui_print_error("Missing id value");
                    return -1;
                }
                *id_filter = value;
                continue;
            }
            if (key_len == 6 && strncasecmp(raw, "detail", key_len) == 0) {
                if (parse_detail_option(value, detail_full) != 0) {
                    return -1;
                }
                continue;
            }
            char buffer[128];
            snprintf(buffer, sizeof(buffer), "Unknown option key '%.*s'", (int)key_len, raw);
            ui_print_error(buffer);
            return -1;
        }

        if (raw[0] == '-') {
            char buffer[128];
            snprintf(buffer, sizeof(buffer), "Unknown flag '%s'", raw);
            ui_print_error(buffer);
            return -1;
        }

        if (!*name_filter) {
            *name_filter = raw;
            continue;
        }

        if (!*id_filter) {
            *id_filter = raw;
            continue;
        }

        ui_print_error("Too many positional arguments. Use up to three options (filter, id, detail).");
        return -1;
    }
    return 0;
}

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

int handle_show_device_configs_command_new(const char* cmd_name,
                                           int argc,
                                           char** argv,
                                           const uci_param_def_t* params,
                                           int param_count) {
    (void)cmd_name;
    (void)argc;
    (void)argv;
    (void)params;

    const char* id_filter = NULL;
    const char* name_filter = NULL;
    int full = 0;

    if (parse_config_list_options(param_count, &id_filter, &name_filter, &full) != 0) {
        return -1;
    }
    return show_device_configs_with_filters(id_filter, name_filter, full);
}

int handle_show_app_configs_command_new(const char* cmd_name,
                                        int argc,
                                        char** argv,
                                        const uci_param_def_t* params,
                                        int param_count) {
    (void)cmd_name;
    (void)argc;
    (void)argv;
    (void)params;

    const char* id_filter = NULL;
    const char* name_filter = NULL;
    int full = 0;

    if (parse_config_list_options(param_count, &id_filter, &name_filter, &full) != 0) {
        return -1;
    }
    return show_app_configs_with_filters(id_filter, name_filter, full);
}
