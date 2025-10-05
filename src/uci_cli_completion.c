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
    "session_stop", "stop_ranging", "get_session_state", "session_status",
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

static char** cli_completion(const char* text, int start, int end) {
    (void)end;
    if (start == 0) {
        return rl_completion_matches(text, cli_command_generator);
    }
    return NULL;
}

void cli_initialize_readline(void) {
    rl_readline_name = "uci-shell";
    rl_attempted_completion_function = cli_completion;

    using_history();
    stifle_history(100);
}
