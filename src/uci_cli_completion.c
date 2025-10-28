#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <readline/readline.h>
#include <readline/history.h>

#include "../include/uci_cli_completion.h"
#include "../include/uci_cli.h"

#define COMPLETION_BUFFER_SIZE 512

static const char* const g_cli_commands[] = {
    "quit", "hw_init", "hw_send", "hw_send_raw", "hw_info", "hw_connect",
    "hw_get_device_info", "hw_device_info", "hw_device_reset", "hw_get_caps_info",
    "hw_set_config", "hw_get_config", "hw_get_device_state", "hw_set_device_active",
    "hw_set_device_ready", "hw_device_suspend", "hw_session_init", "hw_session_new",
    "hw_session_deinit", "hw_session_close", "hw_session_start", "hw_start_ranging",
    "hw_session_stop", "hw_stop_ranging", "hw_get_session_state", "hw_session_status",
    "hw_set_app_config", "hw_get_app_config",
    "get_device_info", "device_info", "device_reset", "get_caps_info", "set_config", "get_config",
    "get_device_state", "set_device_active", "set_device_ready", "device_suspend",
    "session_init", "session_new", "session_deinit", "session_close", "session_start", "start_ranging",
    "session_stop", "stop_ranging", "session_send_data", "send_data",
    "session_update_multicast_list", "session_update_dt_tag_rounds",
    "session_data_transfer_phase_config", "session_query_data_size_in_ranging",
    "get_session_state", "session_status",
    "set_app_config", "get_app_config", "simulate_notification", "simulate_session_status",
    "simulate_data_credit", "simulate_ranging", "simulate_multi_target_ranging", "demo_session_flow",
    "set_power", "device_on", "device_off", "analyze_packet", "complete", "history", "alias", "unalias",
    NULL
};

static size_t cli_command_count(void) {
    // Exclude terminating NULL
    return (sizeof(g_cli_commands) / sizeof(g_cli_commands[0])) - 1;
}

static void cli_print_matching_commands(const char* prefix) {
    if (!prefix) {
        prefix = "";
    }

    size_t prefix_len = strlen(prefix);
    int first = 1;

    for (size_t i = 0; i < cli_command_count(); i++) {
        const char* name = g_cli_commands[i];
        if (strncmp(prefix, name, prefix_len) == 0) {
            if (!first) {
                printf(" ");
            }
            printf("%s", name);
            first = 0;
        }
    }
}

void cli_print_completion_suggestions(const char* input) {
    if (input == NULL) {
        return;
    }

    char temp_input[COMPLETION_BUFFER_SIZE];
    strncpy(temp_input, input, sizeof(temp_input) - 1);
    temp_input[sizeof(temp_input) - 1] = '\0';

    char* tok = strtok(temp_input, " ");
    if (!tok) {
        printf("\n");
        return;
    }

    char* cmd_part = tok;
    char* next_part = strtok(NULL, " ");

    if (strcmp(cmd_part, "set_app_config") == 0) {
        if (next_part == NULL) {
            printf("device_type ranging_usage ranging_round_usage sts_config multi_node_mode channel channel_number device_role aoa_request aoa_result_req scheduled_mode");
        } else {
            char* config_name_part = strtok(NULL, " ");
            if (config_name_part == NULL) {
                printf("device_type ranging_usage ranging_round_usage sts_config multi_node_mode channel channel_number device_role aoa_request aoa_result_req scheduled_mode");
            } else if (strcmp(config_name_part, "device_type") == 0) {
                printf("responder initiator");
            } else if (strcmp(config_name_part, "ranging_usage") == 0 || strcmp(config_name_part, "ranging_round_usage") == 0) {
                printf("ranging data");
            } else if (strcmp(config_name_part, "sts_config") == 0) {
                printf("static dynamic");
            } else if (strcmp(config_name_part, "multi_node_mode") == 0) {
                printf("unicast anycast multicast");
            } else if (strcmp(config_name_part, "device_role") == 0) {
                printf("controller controlee");
            } else if (strcmp(config_name_part, "aoa_request") == 0 || strcmp(config_name_part, "aoa_result_req") == 0) {
                printf("enable on disable off");
            } else if (strcmp(config_name_part, "scheduled_mode") == 0) {
                printf("cont continuous scheduled");
            } else {
                printf("device_type ranging_usage ranging_round_usage sts_config multi_node_mode channel channel_number device_role aoa_request aoa_result_req scheduled_mode");
            }
        }
    } else if (strcmp(cmd_part, "set_config") == 0) {
        if (next_part == NULL) {
            printf("device_state low_power_mode");
        } else {
            char* config_name = next_part;
            char* value_part = strtok(NULL, " ");
            if (value_part == NULL) {
                if (strcmp(config_name, "device_state") == 0) {
                    printf("active ready");
                } else if (strcmp(config_name, "low_power_mode") == 0) {
                    printf("on off");
                } else {
                    printf("device_state low_power_mode");
                }
            } else if (strcmp(config_name, "device_state") == 0) {
                printf("active ready");
            } else if (strcmp(config_name, "low_power_mode") == 0) {
                printf("on off");
            }
        }
    } else if (strcmp(cmd_part, "session_init") == 0 || strcmp(cmd_part, "session_new") == 0) {
        if (next_part == NULL) {
            printf("fira_ranging ranging");
        } else {
            char* type_part = strtok(NULL, " ");
            if (type_part == NULL) {
                printf("fira_ranging ranging");
            }
        }
    } else if (strcmp(cmd_part, "history") == 0) {
        printf("history");
    } else if (strcmp(cmd_part, "alias") == 0) {
        printf("Show or create aliases. Use: alias <name> <command>");
    } else if (strcmp(cmd_part, "unalias") == 0) {
        cli_alias_print_names();
    } else {
        cli_print_matching_commands(input);
    }

    printf("\n");
}

static char* cli_command_generator(const char* text, int state) {
    static size_t list_index;
    static size_t len;

    if (!state) {
        list_index = 0;
        len = strlen(text);
    }

    while (g_cli_commands[list_index] != NULL) {
        const char* name = g_cli_commands[list_index++];
        if (strncmp(name, text, len) == 0) {
            char* result = malloc(strlen(name) + 1);
            if (result != NULL) {
                strcpy(result, name);
            }
            return result;
        }
    }

    return NULL;
}

// Parameter generator functions for different contexts
static char* parameter_generator_session_types(const char* text, int state) {
    static int list_index;
    static size_t len;
    static const char* const session_types[] = {
        "fira_ranging",
        "ranging", 
        NULL
    };

    if (!state) {
        list_index = 0;
        len = strlen(text);
    }

    while (session_types[list_index] != NULL) {
        const char* name = session_types[list_index++];
        if (strncmp(name, text, len) == 0) {
            char* result = malloc(strlen(name) + 1);
            if (result != NULL) {
                strcpy(result, name);
            }
            return result;
        }
    }

    return NULL;
}

static char* parameter_generator_session_id(const char* text, int state) {
    static int list_index;
    static size_t len;
    // Provide example session IDs - in a real system, these might come from active sessions
    static const char* const session_ids[] = {
        "1", "2", "3", "4", "5", "id1", "id2", "session1", "session2",
        NULL
    };

    if (!state) {
        list_index = 0;
        len = strlen(text);
    }

    while (session_ids[list_index] != NULL) {
        const char* name = session_ids[list_index++];
        if (strncmp(name, text, len) == 0) {
            char* result = malloc(strlen(name) + 1);
            if (result != NULL) {
                strcpy(result, name);
            }
            return result;
        }
    }

    return NULL;
}

static char* parameter_generator_config_name(const char* text, int state) {
    static int list_index;
    static size_t len;
    static const char* const config_names[] = {
        "device_state",
        "low_power_mode",
        NULL
    };

    if (!state) {
        list_index = 0;
        len = strlen(text);
    }

    while (config_names[list_index] != NULL) {
        const char* name = config_names[list_index++];
        if (strncmp(name, text, len) == 0) {
            char* result = malloc(strlen(name) + 1);
            if (result != NULL) {
                strcpy(result, name);
            }
            return result;
        }
    }

    return NULL;
}

static char* parameter_generator_app_config_name(const char* text, int state) {
    static int list_index;
    static size_t len;
    static const char* const app_config_names[] = {
        "device_type",
        "ranging_usage", 
        "ranging_round_usage",
        "sts_config",
        "multi_node_mode",
        "channel",
        "channel_number",
        "device_role",
        "aoa_request",
        "aoa_result_req",
        "scheduled_mode",
        NULL
    };

    if (!state) {
        list_index = 0;
        len = strlen(text);
    }

    while (app_config_names[list_index] != NULL) {
        const char* name = app_config_names[list_index++];
        if (strncmp(name, text, len) == 0) {
            char* result = malloc(strlen(name) + 1);
            if (result != NULL) {
                strcpy(result, name);
            }
            return result;
        }
    }

    return NULL;
}

static char* parameter_generator_device_state(const char* text, int state) {
    static int list_index;
    static size_t len;
    static const char* const device_states[] = {
        "active",
        "ready",
        NULL
    };

    if (!state) {
        list_index = 0;
        len = strlen(text);
    }

    while (device_states[list_index] != NULL) {
        const char* name = device_states[list_index++];
        if (strncmp(name, text, len) == 0) {
            char* result = malloc(strlen(name) + 1);
            if (result != NULL) {
                strcpy(result, name);
            }
            return result;
        }
    }

    return NULL;
}

static char* parameter_generator_bool_values(const char* text, int state) {
    static int list_index;
    static size_t len;
    static const char* const bool_values[] = {
        "on",
        "off",
        NULL
    };

    if (!state) {
        list_index = 0;
        len = strlen(text);
    }

    while (bool_values[list_index] != NULL) {
        const char* name = bool_values[list_index++];
        if (strncmp(name, text, len) == 0) {
            char* result = malloc(strlen(name) + 1);
            if (result != NULL) {
                strcpy(result, name);
            }
            return result;
        }
    }

    return NULL;
}

static char** cli_completion(const char* text, int start, int end) {
    (void)end;
    
    // Get the entire line being edited
    char* line = rl_line_buffer;
    
    if (!line) {
        return NULL;
    }
    
    // Count number of words before the current position to determine context
    int word_count = 0;
    int i = 0;
    int in_word = 0;
    
    // Parse the line up to the cursor position to find current word position
    for (i = 0; i < start; i++) {
        if (line[i] == ' ') {
            if (in_word) {
                word_count++;
                in_word = 0;
            }
        } else if (line[i] != '\t') {
            in_word = 1;
        }
    }
    
    // If we're at the first word, complete commands
    if (word_count == 0 && start == 0) {
        return rl_completion_matches(text, cli_command_generator);
    }
    
    // For subsequent words, provide parameter-specific completion
    if (word_count >= 1) {
        // Extract the command part (first word) to determine context
        char command[256];
        int j = 0;
        
        // Find the first word (command)
        while (j < start && line[j] != ' ') {
            if ((size_t)j < sizeof(command) - 1) {
                command[j] = line[j];
            }
            j++;
        }
        command[(size_t)j < sizeof(command) ? j : (int)sizeof(command) - 1] = '\0';
        
        // Check if this is a command that we should provide parameter completion for
        if (strcmp(command, "session_init") == 0 || strcmp(command, "session_new") == 0 || 
            strcmp(command, "hw_session_init") == 0 || strcmp(command, "hw_session_new") == 0) {
            // For session_init commands, provide session type completion after the session ID
            if (word_count == 1) {
                // After session ID, suggest session types using the generator function
                return rl_completion_matches(text, parameter_generator_session_types);
            }
        }
        else if (strcmp(command, "session_start") == 0 || strcmp(command, "start_ranging") == 0 ||
                 strcmp(command, "session_stop") == 0 || strcmp(command, "stop_ranging") == 0 ||
                 strcmp(command, "get_session_state") == 0 || strcmp(command, "session_status") == 0 ||
                 strcmp(command, "session_deinit") == 0 || strcmp(command, "session_close") == 0 ||
                 strcmp(command, "hw_session_start") == 0 || strcmp(command, "hw_start_ranging") == 0 ||
                 strcmp(command, "hw_session_stop") == 0 || strcmp(command, "hw_stop_ranging") == 0 ||
                 strcmp(command, "hw_get_session_state") == 0 || strcmp(command, "hw_session_status") == 0 ||
                 strcmp(command, "hw_session_deinit") == 0 || strcmp(command, "hw_session_close") == 0) {
            if (word_count == 1) {
                // For session commands that expect a session ID, provide dummy completion
                return rl_completion_matches(text, parameter_generator_session_id);
            }
        }
        else if (strcmp(command, "set_config") == 0 || strcmp(command, "hw_set_config") == 0) {
            if (word_count == 1) {
                // For the config parameter name
                return rl_completion_matches(text, parameter_generator_config_name);
            } else if (word_count == 2) {
                // For the value - extract parameter name to suggest appropriate values
                char param_name[256];
                int pos = 0;
                int word_idx = 0;
                
                // Skip command
                while (line[pos] != ' ' && line[pos] != '\0') pos++;
                // Skip space
                while (line[pos] == ' ') pos++;
                // Get parameter name
                int start_param = pos;
                while (line[pos] != ' ' && line[pos] != '\0' && word_idx == 0) {
                    if (line[pos+1] == ' ' || line[pos+1] == '\0') word_idx = 1; // end of param name
                    pos++;
                }
                int len = pos - start_param < (int)sizeof(param_name) ? pos - start_param : (int)sizeof(param_name) - 1;
                strncpy(param_name, line + start_param, len);
                param_name[len] = '\0';
                
                if (strcmp(param_name, "device_state") == 0) {
                    return rl_completion_matches(text, parameter_generator_device_state);
                } else if (strcmp(param_name, "low_power_mode") == 0) {
                    return rl_completion_matches(text, parameter_generator_bool_values);
                }
            }
        }
        else if (strcmp(command, "set_app_config") == 0 || strcmp(command, "hw_set_app_config") == 0) {
            if (word_count == 1) {
                // For session ID
                return rl_completion_matches(text, parameter_generator_session_id);
            } else if (word_count == 2) {
                // For config parameter name
                return rl_completion_matches(text, parameter_generator_app_config_name);
            }
            // We could extend this for value completion as well
        }
        else if (strcmp(command, "get_config") == 0 || strcmp(command, "hw_get_config") == 0) {
            if (word_count == 1) {
                // For the config parameter name
                return rl_completion_matches(text, parameter_generator_config_name);
            }
        }
        else if (strcmp(command, "get_app_config") == 0 || strcmp(command, "hw_get_app_config") == 0) {
            if (word_count == 1) {
                // For session ID
                return rl_completion_matches(text, parameter_generator_session_id);
            } else if (word_count == 2) {
                // For config parameter name
                return rl_completion_matches(text, parameter_generator_app_config_name);
            }
        }
    }
    
    // For other contexts, return NULL to disable completion so it doesn't default to file completion
    return NULL;
}

void cli_initialize_readline(void) {
    rl_readline_name = "uci-shell";
    rl_attempted_completion_function = cli_completion;

    using_history();
    stifle_history(100);
}
