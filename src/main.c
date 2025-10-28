#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <stdbool.h>

// Readline includes for tab completion
#include <readline/readline.h>
#include <readline/history.h>

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "../include/uci.h"
#include "../include/uci_functions.h"
#include "../include/uci_cli.h"
#include "../include/uci_cli_completion.h"
#include "../include/uci_config_manager.h"
#include "../include/uci_hw.h"
#include "../include/uci_hw_interface.h"
#include "../include/uci_hw_chardev.h"
#include "../include/uci_ui.h"
#include "../include/uci_ui_main_patch.h"
#include "../include/uci_ui_packet_decoder.h"
#include "../include/uci_cmd_hardware.h"
#include "../include/uci_cmd_core.h"
#include "../include/uci_cmd_session.h"
#include "../include/uci_cmd_session_config.h"
#include "../include/uci_cmd_analysis.h"

#define MAX_PAYLOAD_LENGTH 255
#define CLI_MAX_TOKENS 64

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

enum {
    CLI_CMD_FLAG_NONE = 0,
    CLI_CMD_FLAG_REQUIRES_HW_MODE = 1u << 0
};

typedef enum {
    CLI_GROUP_GENERAL = 0,
    CLI_GROUP_HARDWARE,
    CLI_GROUP_DEVICE,
    CLI_GROUP_SESSION,
    CLI_GROUP_SESSION_CONFIG,
    CLI_GROUP_ANALYSIS,
    CLI_GROUP_SIMULATION,
} cli_command_group_t;

typedef int (*cli_command_handler_t)(int argc, char** argv);

typedef struct {
    const char* name;
    const char* aliases[4];
    cli_command_group_t group;
    unsigned int flags;
    const char* description;
    cli_command_handler_t handler;
} cli_command_t;

// Global variables for hardware mode
int g_hardware_mode = 0;
uci_hw_chardev_t g_uwb_chardev;

// Forward declarations for command handlers
static int cmd_complete(int argc, char** argv);
static int cmd_alias(int argc, char** argv);
static int cmd_unalias(int argc, char** argv);
static int cmd_hw_init(int argc, char** argv);
static int cmd_hw_send(int argc, char** argv);
static int cmd_mode_sim(int argc, char** argv);
static int cmd_mode_hw(int argc, char** argv);
static int cmd_mode_info(int argc, char** argv);
static int cmd_get_device_info(int argc, char** argv);
static int cmd_device_reset(int argc, char** argv);
static int cmd_set_power(int argc, char** argv);
static int cmd_device_on(int argc, char** argv);
static int cmd_device_off(int argc, char** argv);
static int cmd_get_caps_info(int argc, char** argv);
static int cmd_get_config(int argc, char** argv);
static int cmd_get_device_state(int argc, char** argv);
static int cmd_set_device_active(int argc, char** argv);
static int cmd_set_device_ready(int argc, char** argv);
static int cmd_set_config(int argc, char** argv);
static int cmd_device_suspend(int argc, char** argv);
static int cmd_query_timestamp(int argc, char** argv);
static int cmd_session_init(int argc, char** argv);
static int cmd_session_deinit(int argc, char** argv);
static int cmd_session_start(int argc, char** argv);
static int cmd_session_stop(int argc, char** argv);
static int cmd_session_send_data(int argc, char** argv);
static int cmd_session_logical_link_create(int argc, char** argv);
static int cmd_session_logical_link_close(int argc, char** argv);
static int cmd_session_logical_link_get_param(int argc, char** argv);
static int cmd_get_session_state(int argc, char** argv);
static int cmd_set_app_config(int argc, char** argv);
static int cmd_get_app_config(int argc, char** argv);
static int cmd_session_update_multicast_list(int argc, char** argv);
static int cmd_session_update_dt_tag_rounds(int argc, char** argv);
static int cmd_session_data_transfer_phase_config(int argc, char** argv);
static int cmd_session_set_hybrid_controller_config(int argc, char** argv);
static int cmd_session_set_hybrid_controlee_config(int argc, char** argv);
static int cmd_session_query_data_size_in_ranging(int argc, char** argv);
static int cmd_simulate_notification(int argc, char** argv);
static int cmd_simulate_session_status(int argc, char** argv);
static int cmd_simulate_data_credit(int argc, char** argv);
static int cmd_simulate_ranging(int argc, char** argv);
static int cmd_simulate_multi_target_ranging(int argc, char** argv);
static int cmd_demo_session_flow(int argc, char** argv);
static int cmd_analyze_packet(int argc, char** argv);
static int cmd_help(int argc, char** argv);

static const cli_command_t g_cli_commands[] = {
    { "help", { NULL }, CLI_GROUP_GENERAL, CLI_CMD_FLAG_NONE, "Show this help message", cmd_help },
    { "complete", { NULL }, CLI_GROUP_GENERAL, CLI_CMD_FLAG_NONE, "Display completion suggestions", cmd_complete },
    { "alias", { NULL }, CLI_GROUP_GENERAL, CLI_CMD_FLAG_NONE, "Create or list command aliases", cmd_alias },
    { "unalias", { NULL }, CLI_GROUP_GENERAL, CLI_CMD_FLAG_NONE, "Remove a command alias", cmd_unalias },

    { "mode_sim", { "sim_mode", NULL }, CLI_GROUP_HARDWARE, CLI_CMD_FLAG_NONE, "Switch to simulation mode", cmd_mode_sim },
    { "mode_hw", { "hw_mode", NULL }, CLI_GROUP_HARDWARE, CLI_CMD_FLAG_NONE, "Switch to hardware mode", cmd_mode_hw },
    { "mode_info", { "current_mode", NULL }, CLI_GROUP_HARDWARE, CLI_CMD_FLAG_NONE, "Display current mode", cmd_mode_info },
    { "hw_init", { "hw_connect", NULL }, CLI_GROUP_HARDWARE, CLI_CMD_FLAG_NONE, "Initialize hardware mode and connect", cmd_hw_init },
    { "hw_send", { NULL }, CLI_GROUP_HARDWARE, CLI_CMD_FLAG_REQUIRES_HW_MODE, "Send a raw packet to hardware", cmd_hw_send },

    { "get_device_info", { "device_info", NULL }, CLI_GROUP_DEVICE, CLI_CMD_FLAG_NONE, "Query device information", cmd_get_device_info },
    { "device_reset", { NULL }, CLI_GROUP_DEVICE, CLI_CMD_FLAG_NONE, "Reset the connected device", cmd_device_reset },
    { "set_power", { NULL }, CLI_GROUP_DEVICE, CLI_CMD_FLAG_NONE, "Set device power state", cmd_set_power },
    { "device_on", { NULL }, CLI_GROUP_DEVICE, CLI_CMD_FLAG_NONE, "Power on the device", cmd_device_on },
    { "device_off", { NULL }, CLI_GROUP_DEVICE, CLI_CMD_FLAG_NONE, "Power off the device", cmd_device_off },
    { "get_caps_info", { NULL }, CLI_GROUP_DEVICE, CLI_CMD_FLAG_NONE, "Query capability information", cmd_get_caps_info },
    { "get_config", { NULL }, CLI_GROUP_DEVICE, CLI_CMD_FLAG_NONE, "Read a device configuration parameter", cmd_get_config },
    { "get_device_state", { NULL }, CLI_GROUP_DEVICE, CLI_CMD_FLAG_NONE, "Report current device state", cmd_get_device_state },
    { "set_device_active", { NULL }, CLI_GROUP_DEVICE, CLI_CMD_FLAG_NONE, "Force device Active state", cmd_set_device_active },
    { "set_device_ready", { NULL }, CLI_GROUP_DEVICE, CLI_CMD_FLAG_NONE, "Force device Ready state", cmd_set_device_ready },
    { "set_config", { NULL }, CLI_GROUP_DEVICE, CLI_CMD_FLAG_NONE, "Update a device configuration parameter", cmd_set_config },
    { "device_suspend", { NULL }, CLI_GROUP_DEVICE, CLI_CMD_FLAG_NONE, "Suspend device operation", cmd_device_suspend },
    { "query_timestamp", { NULL }, CLI_GROUP_DEVICE, CLI_CMD_FLAG_NONE, "Query device timestamp", cmd_query_timestamp },

    { "session_init", { "session_new", NULL }, CLI_GROUP_SESSION, CLI_CMD_FLAG_NONE, "Initialize a ranging session", cmd_session_init },
    { "session_deinit", { "session_close", NULL }, CLI_GROUP_SESSION, CLI_CMD_FLAG_NONE, "Deinitialize a session", cmd_session_deinit },
    { "session_start", { "start_ranging", NULL }, CLI_GROUP_SESSION, CLI_CMD_FLAG_NONE, "Start a ranging session", cmd_session_start },
    { "session_stop", { "stop_ranging", NULL }, CLI_GROUP_SESSION, CLI_CMD_FLAG_NONE, "Stop a ranging session", cmd_session_stop },
    { "session_send_data", { "send_data", NULL }, CLI_GROUP_SESSION, CLI_CMD_FLAG_NONE, "Send DATA_MESSAGE_SND payload", cmd_session_send_data },
    { "session_logical_link_create", { NULL }, CLI_GROUP_SESSION, CLI_CMD_FLAG_NONE, "Create a logical link", cmd_session_logical_link_create },
    { "session_logical_link_close", { NULL }, CLI_GROUP_SESSION, CLI_CMD_FLAG_NONE, "Close a logical link", cmd_session_logical_link_close },
    { "session_logical_link_get_param", { NULL }, CLI_GROUP_SESSION, CLI_CMD_FLAG_NONE, "Query logical link parameters", cmd_session_logical_link_get_param },
    { "get_session_state", { "session_status", NULL }, CLI_GROUP_SESSION, CLI_CMD_FLAG_NONE, "Report session state", cmd_get_session_state },

    { "set_app_config", { NULL }, CLI_GROUP_SESSION_CONFIG, CLI_CMD_FLAG_NONE, "Configure session application parameters", cmd_set_app_config },
    { "get_app_config", { NULL }, CLI_GROUP_SESSION_CONFIG, CLI_CMD_FLAG_NONE, "Read session application parameters", cmd_get_app_config },
    { "session_update_multicast_list", { "update_multicast_list", NULL }, CLI_GROUP_SESSION_CONFIG, CLI_CMD_FLAG_NONE, "Maintain multicast list entries", cmd_session_update_multicast_list },
    { "session_update_dt_tag_rounds", { NULL }, CLI_GROUP_SESSION_CONFIG, CLI_CMD_FLAG_NONE, "Configure DT-Tag active rounds", cmd_session_update_dt_tag_rounds },
    { "session_data_transfer_phase_config", { NULL }, CLI_GROUP_SESSION_CONFIG, CLI_CMD_FLAG_NONE, "Configure data transfer phase", cmd_session_data_transfer_phase_config },
    { "session_set_hybrid_controller_config", { NULL }, CLI_GROUP_SESSION_CONFIG, CLI_CMD_FLAG_NONE, "Set hybrid controller configuration", cmd_session_set_hybrid_controller_config },
    { "session_set_hybrid_controlee_config", { NULL }, CLI_GROUP_SESSION_CONFIG, CLI_CMD_FLAG_NONE, "Set hybrid controlee configuration", cmd_session_set_hybrid_controlee_config },
    { "session_query_data_size_in_ranging", { NULL }, CLI_GROUP_SESSION_CONFIG, CLI_CMD_FLAG_NONE, "Query data size in ranging", cmd_session_query_data_size_in_ranging },

    { "analyze_packet", { NULL }, CLI_GROUP_ANALYSIS, CLI_CMD_FLAG_NONE, "Analyze packet bytes with enhanced decoder", cmd_analyze_packet },

    { "simulate_notification", { NULL }, CLI_GROUP_SIMULATION, CLI_CMD_FLAG_NONE, "Simulate device notification", cmd_simulate_notification },
    { "simulate_session_status", { NULL }, CLI_GROUP_SIMULATION, CLI_CMD_FLAG_NONE, "Simulate session status notification", cmd_simulate_session_status },
    { "simulate_data_credit", { NULL }, CLI_GROUP_SIMULATION, CLI_CMD_FLAG_NONE, "Simulate session data credit notification", cmd_simulate_data_credit },
    { "simulate_ranging", { NULL }, CLI_GROUP_SIMULATION, CLI_CMD_FLAG_NONE, "Simulate single-target ranging notification", cmd_simulate_ranging },
    { "simulate_multi_target_ranging", { NULL }, CLI_GROUP_SIMULATION, CLI_CMD_FLAG_NONE, "Simulate multi-target ranging notification", cmd_simulate_multi_target_ranging },
    { "demo_session_flow", { NULL }, CLI_GROUP_SIMULATION, CLI_CMD_FLAG_NONE, "Demonstrate session flow", cmd_demo_session_flow },
};

static void cli_expand_alias(char* line, size_t capacity) {
    if (!line || capacity == 0) {
        return;
    }

    for (int depth = 0; depth < 4; depth++) {
        const char* cursor = line;
        while (*cursor == ' ' || *cursor == '\t') {
            cursor++;
        }
        if (*cursor == '\0') {
            return;
        }

        size_t command_len = strcspn(cursor, " \t");
        if (command_len == 0 || command_len >= CLI_MAX_LINE_LENGTH) {
            return;
        }

        char command[CLI_MAX_LINE_LENGTH];
        memcpy(command, cursor, command_len);
        command[command_len] = '\0';

        const char* expansion = cli_alias_lookup(command);
        if (!expansion) {
            return;
        }

        const char* remainder = cursor + command_len;
        char expanded[CLI_MAX_LINE_LENGTH];
        int written = snprintf(expanded, sizeof(expanded), "%s%s", expansion, remainder);
        if (written < 0 || (size_t)written >= sizeof(expanded)) {
            printf("Error: Expanded alias is too long.\n");
            return;
        }

        strncpy(line, expanded, capacity - 1);
        line[capacity - 1] = '\0';
    }
}

static int cli_tokenize(char* line, char** argv, int max_tokens) {
    if (!line || !argv || max_tokens <= 0) {
        return 0;
    }

    int argc = 0;
    char* cursor = line;

    while (*cursor != '\0' && argc < max_tokens) {
        while (*cursor == ' ' || *cursor == '\t') {
            cursor++;
        }
        if (*cursor == '\0') {
            break;
        }

        argv[argc++] = cursor;
        while (*cursor != '\0' && *cursor != ' ' && *cursor != '\t') {
            cursor++;
        }
        if (*cursor == '\0') {
            break;
        }
        *cursor++ = '\0';
    }

    return argc;
}

static int cli_join_arguments(int argc, char** argv, char* buffer, size_t buffer_size) {
    if (!buffer || buffer_size == 0) {
        return -1;
    }

    buffer[0] = '\0';
    size_t used = 0;

    for (int i = 0; i < argc; i++) {
        const char* arg = argv[i];
        if (!arg) {
            continue;
        }

        if (i > 0) {
            if (used + 1 >= buffer_size) {
                return -1;
            }
            buffer[used++] = ' ';
        }

        size_t len = strlen(arg);
        if (used + len >= buffer_size) {
            size_t available = buffer_size - used - 1;
            memcpy(buffer + used, arg, available);
            used += available;
            buffer[used] = '\0';
            return -1;
        }

        memcpy(buffer + used, arg, len);
        used += len;
        buffer[used] = '\0';
    }

    return 0;
}

static const cli_command_t* cli_find_command(const char* name) {
    if (!name) {
        return NULL;
    }

    for (size_t i = 0; i < ARRAY_SIZE(g_cli_commands); i++) {
        const cli_command_t* cmd = &g_cli_commands[i];
        if (strcmp(name, cmd->name) == 0) {
            return cmd;
        }
        for (size_t alias_idx = 0; alias_idx < ARRAY_SIZE(cmd->aliases); alias_idx++) {
            const char* alias = cmd->aliases[alias_idx];
            if (alias == NULL) {
                break;
            }
            if (strcmp(name, alias) == 0) {
                return cmd;
            }
        }
    }

    return NULL;
}

static void cli_format_command_name(const cli_command_t* cmd, char* buffer, size_t buffer_size) {
    buffer[0] = '\0';
    if (!cmd || buffer_size == 0) {
        return;
    }

    strncat(buffer, cmd->name, buffer_size - 1);

    bool first_alias = true;
    for (size_t i = 0; i < ARRAY_SIZE(cmd->aliases); i++) {
        const char* alias = cmd->aliases[i];
        if (!alias) {
            break;
        }
        if (first_alias) {
            strncat(buffer, " (", buffer_size - strlen(buffer) - 1);
            first_alias = false;
        } else {
            strncat(buffer, ", ", buffer_size - strlen(buffer) - 1);
        }
        strncat(buffer, alias, buffer_size - strlen(buffer) - 1);
    }

    if (!first_alias) {
        strncat(buffer, ")", buffer_size - strlen(buffer) - 1);
    }
}

static const char* cli_group_title(cli_command_group_t group) {
    switch (group) {
        case CLI_GROUP_GENERAL:
            return "General Commands";
        case CLI_GROUP_HARDWARE:
            return "Mode & Hardware Commands";
        case CLI_GROUP_DEVICE:
            return "Device Management Commands";
        case CLI_GROUP_SESSION:
            return "Session Commands";
        case CLI_GROUP_SESSION_CONFIG:
            return "Advanced Session Configuration";
        case CLI_GROUP_ANALYSIS:
            return "Analysis Commands";
        case CLI_GROUP_SIMULATION:
            return "Simulation Utilities";
        default:
            return "Commands";
    }
}

static void cli_print_command_help_line(const cli_command_t* cmd) {
    char name_buffer[128];
    cli_format_command_name(cmd, name_buffer, sizeof(name_buffer));

    if (ui_color_enabled) {
        printf("  %s%s%s%s - %s%s%s\n",
               ANSI_BOLD, ANSI_COLOR_BRIGHT_GREEN, name_buffer, ANSI_RESET,
               ANSI_COLOR_WHITE, cmd->description, ANSI_RESET);
    } else {
        printf("  %s - %s\n", name_buffer, cmd->description);
    }
}

static void cli_print_group_header(const char* title) {
    if (ui_color_enabled) {
        printf("%s%s%s:%s\n", ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, title, ANSI_RESET);
    } else {
        printf("%s:\n", title);
    }
}

static void cli_print_help(void) {
    ui_print_header("UCI Interactive Shell - Enhanced UI");

    static const cli_command_group_t group_order[] = {
        CLI_GROUP_GENERAL,
        CLI_GROUP_HARDWARE,
        CLI_GROUP_DEVICE,
        CLI_GROUP_SESSION,
        CLI_GROUP_SESSION_CONFIG,
        CLI_GROUP_ANALYSIS,
        CLI_GROUP_SIMULATION
    };

    for (size_t g = 0; g < ARRAY_SIZE(group_order); g++) {
        cli_command_group_t group = group_order[g];
        bool printed_group = false;
        for (size_t i = 0; i < ARRAY_SIZE(g_cli_commands); i++) {
            const cli_command_t* cmd = &g_cli_commands[i];
            if (cmd->group != group) {
                continue;
            }
            if (!printed_group) {
                cli_print_group_header(cli_group_title(group));
                printed_group = true;
            }
            cli_print_command_help_line(cmd);
        }
        if (printed_group) {
            printf("\n");
        }
    }

    if (ui_color_enabled) {
        printf("%s%sKey Resources:%s\n", ANSI_COLOR_BRIGHT_BLUE, ANSI_BOLD, ANSI_RESET);
        printf("  - %sREADME.md%s – Project overview and usage\n", ANSI_COLOR_BRIGHT_CYAN, ANSI_RESET);
        printf("  - %sFINAL_SUMMARY.md%s – Feature matrix and technical details\n", ANSI_COLOR_BRIGHT_CYAN, ANSI_RESET);
        printf("  - %suci_analysis/%s – Detailed UCI protocol analysis\n", ANSI_COLOR_BRIGHT_CYAN, ANSI_RESET);
    } else {
        printf("Key Resources:\n");
        printf("  - README.md – Project overview and usage\n");
        printf("  - FINAL_SUMMARY.md – Feature matrix and technical details\n");
        printf("  - uci_analysis/ – Detailed UCI protocol analysis\n");
    }
}

static int cli_dispatch(int argc, char** argv) {
    if (argc == 0) {
        return 0;
    }

    const cli_command_t* command = cli_find_command(argv[0]);
    if (!command) {
        ui_print_command_not_found(argv[0]);
        return -1;
    }

    if ((command->flags & CLI_CMD_FLAG_REQUIRES_HW_MODE) && !g_hardware_mode) {
        ui_print_error("Command requires hardware mode. Run hw_init or mode_hw first.");
        return -1;
    }

    return command->handler(argc, argv);
}

static int cmd_complete(int argc, char** argv) {
    char buffer[CLI_MAX_LINE_LENGTH];
    const char* input = "";

    if (argc > 1) {
        if (cli_join_arguments(argc - 1, &argv[1], buffer, sizeof(buffer)) == 0) {
            input = buffer;
        } else {
            input = buffer;
        }
    }

    cli_print_completion_suggestions(input);
    return 0;
}

static int cmd_alias(int argc, char** argv) {
    if (argc == 1) {
        cli_alias_print_all();
        return 0;
    }

    const char* alias_name = argv[1];
    if (!alias_name) {
        printf("Usage: alias <name> [command]\n");
        return -1;
    }

    if (argc == 2) {
        const char* real_cmd = cli_alias_lookup(alias_name);
        if (real_cmd) {
            printf("%s -> %s\n", alias_name, real_cmd);
        } else {
            printf("Alias '%s' not found\n", alias_name);
        }
        return 0;
    }

    char alias_value[CLI_MAX_ALIAS_TEXT];
    if (cli_join_arguments(argc - 2, &argv[2], alias_value, sizeof(alias_value)) != 0) {
        printf("Alias definition too long.\n");
        return -1;
    }

    cli_alias_result_t result = cli_alias_add(alias_name, alias_value);
    if (result == CLI_ALIAS_FULL) {
        ui_print_error("Maximum number of aliases reached");
        return -1;
    } else if (result == CLI_ALIAS_UPDATED) {
        printf("Alias '%s' updated to '%s'\n", alias_name, alias_value);
    } else if (result == CLI_ALIAS_SUCCESS) {
        printf("Alias '%s' added for '%s'\n", alias_name, alias_value);
    } else {
        printf("Failed to add alias '%s'\n", alias_name);
        return -1;
    }

    return 0;
}

static int cmd_unalias(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: unalias <alias_name>\n");
        return -1;
    }

    cli_alias_result_t result = cli_alias_remove(argv[1]);
    if (result == CLI_ALIAS_SUCCESS) {
        printf("Alias '%s' removed.\n", argv[1]);
        return 0;
    }

    printf("Alias '%s' not found.\n", argv[1]);
    return -1;
}

static int cmd_hw_init(int argc, char** argv) {
    char* device_path = (argc > 1) ? argv[1] : NULL;
    return handle_hw_init_command(device_path);
}

static int cmd_hw_send(int argc, char** argv) {
    char** payload_tokens = (argc > 5) ? &argv[5] : NULL;
    int payload_count = (argc > 5) ? (argc - 5) : 0;

    return handle_hw_send_command(
        (argc > 1) ? argv[1] : NULL,
        (argc > 2) ? argv[2] : NULL,
        (argc > 3) ? argv[3] : NULL,
        (argc > 4) ? argv[4] : NULL,
        payload_tokens,
        payload_count);
}

static int cmd_mode_sim(int argc, char** argv) {
    (void)argc;
    (void)argv;
    handle_mode_sim_command();
    return 0;
}

static int cmd_mode_hw(int argc, char** argv) {
    char* device_path = (argc > 1) ? argv[1] : NULL;
    return handle_mode_hw_command(device_path);
}

static int cmd_mode_info(int argc, char** argv) {
    (void)argc;
    (void)argv;
    handle_mode_info_command();
    return 0;
}

static int cmd_get_device_info(int argc, char** argv) {
    (void)argc;
    (void)argv;
    handle_get_device_info_command();
    return 0;
}

static int cmd_device_reset(int argc, char** argv) {
    (void)argc;
    (void)argv;
    handle_device_reset_command();
    return 0;
}

static int cmd_set_power(int argc, char** argv) {
    char* power_state = (argc > 1) ? argv[1] : NULL;
    return handle_set_power_command(power_state);
}

static int cmd_device_on(int argc, char** argv) {
    (void)argc;
    (void)argv;
    handle_device_on_command();
    return 0;
}

static int cmd_device_off(int argc, char** argv) {
    (void)argc;
    (void)argv;
    handle_device_off_command();
    return 0;
}

static int cmd_get_caps_info(int argc, char** argv) {
    (void)argc;
    (void)argv;
    handle_get_caps_info_command();
    return 0;
}

static int cmd_get_config(int argc, char** argv) {
    char* config_name = (argc > 1) ? argv[1] : NULL;
    return handle_get_config_command(config_name);
}

static int cmd_get_device_state(int argc, char** argv) {
    (void)argc;
    (void)argv;
    handle_get_device_state_command();
    return 0;
}

static int cmd_set_device_active(int argc, char** argv) {
    (void)argc;
    (void)argv;
    handle_set_device_active_command();
    return 0;
}

static int cmd_set_device_ready(int argc, char** argv) {
    (void)argc;
    (void)argv;
    handle_set_device_ready_command();
    return 0;
}

static int cmd_set_config(int argc, char** argv) {
    char* config_name = (argc > 1) ? argv[1] : NULL;
    char* value_str = (argc > 2) ? argv[2] : NULL;
    return handle_set_config_command(config_name, value_str);
}

static int cmd_device_suspend(int argc, char** argv) {
    (void)argc;
    (void)argv;
    handle_device_suspend_command();
    return 0;
}

static int cmd_query_timestamp(int argc, char** argv) {
    (void)argc;
    (void)argv;
    handle_query_timestamp_command();
    return 0;
}

static int cmd_session_init(int argc, char** argv) {
    char* session_id_str = (argc > 1) ? argv[1] : NULL;
    char* session_type_str = (argc > 2) ? argv[2] : NULL;
    return handle_session_init_command(session_id_str, session_type_str);
}

static int cmd_session_deinit(int argc, char** argv) {
    char* session_id_str = (argc > 1) ? argv[1] : NULL;
    return handle_session_deinit_command(session_id_str);
}

static int cmd_session_start(int argc, char** argv) {
    char* session_id_str = (argc > 1) ? argv[1] : NULL;
    return handle_session_start_command(session_id_str);
}

static int cmd_session_stop(int argc, char** argv) {
    char* session_id_str = (argc > 1) ? argv[1] : NULL;
    return handle_session_stop_command(session_id_str);
}

static int cmd_session_send_data(int argc, char** argv) {
    char* session_id_str = (argc > 1) ? argv[1] : NULL;
    char* destination_str = (argc > 2) ? argv[2] : NULL;
    char* sequence_str = (argc > 3) ? argv[3] : NULL;
    char* payload_str = (argc > 4) ? argv[4] : NULL;
    return handle_session_send_data_command(session_id_str, destination_str, sequence_str, payload_str);
}

static int cmd_session_logical_link_create(int argc, char** argv) {
    char* session_id_str = (argc > 1) ? argv[1] : NULL;
    char* link_id_str = (argc > 2) ? argv[2] : NULL;
    char* mode_str = (argc > 3) ? argv[3] : NULL;
    char* credit_str = (argc > 4) ? argv[4] : NULL;
    return handle_session_logical_link_create_command(session_id_str, link_id_str, mode_str, credit_str);
}

static int cmd_session_logical_link_close(int argc, char** argv) {
    char* session_id_str = (argc > 1) ? argv[1] : NULL;
    char* link_id_str = (argc > 2) ? argv[2] : NULL;
    return handle_session_logical_link_close_command(session_id_str, link_id_str);
}

static int cmd_session_logical_link_get_param(int argc, char** argv) {
    char* session_id_str = (argc > 1) ? argv[1] : NULL;
    char* link_id_str = (argc > 2) ? argv[2] : NULL;
    return handle_session_logical_link_get_param_command(session_id_str, link_id_str);
}

static int cmd_get_session_state(int argc, char** argv) {
    char* session_id_str = (argc > 1) ? argv[1] : NULL;
    return handle_get_session_state_command(session_id_str);
}

static int cmd_set_app_config(int argc, char** argv) {
    char* session_id_str = (argc > 1) ? argv[1] : NULL;
    char* config_name = (argc > 2) ? argv[2] : NULL;
    char* value_str = (argc > 3) ? argv[3] : NULL;
    return handle_set_app_config_command(session_id_str, config_name, value_str);
}

static int cmd_get_app_config(int argc, char** argv) {
    char* session_id_str = (argc > 1) ? argv[1] : NULL;
    char** config_names = (argc > 2) ? &argv[2] : NULL;
    int config_count = (argc > 2) ? (argc - 2) : 0;
    return handle_get_app_config_command(session_id_str, config_names, config_count);
}

static int cmd_session_update_multicast_list(int argc, char** argv) {
    char* session_id_str = (argc > 1) ? argv[1] : NULL;
    char* action_str = (argc > 2) ? argv[2] : NULL;
    char* short_address_str = (argc > 3) ? argv[3] : NULL;
    char* subsession_id_str = (argc > 4) ? argv[4] : NULL;
    return handle_update_multicast_list_command(session_id_str, action_str, short_address_str, subsession_id_str);
}

static int cmd_session_update_dt_tag_rounds(int argc, char** argv) {
    char* session_id_str = (argc > 1) ? argv[1] : NULL;
    char** round_values = (argc > 2) ? &argv[2] : NULL;
    int round_count = (argc > 2) ? (argc - 2) : 0;
    return handle_session_update_dt_tag_rounds_command(session_id_str, round_values, round_count);
}

static int cmd_session_data_transfer_phase_config(int argc, char** argv) {
    char* session_id_str = (argc > 1) ? argv[1] : NULL;
    char* repetition_str = (argc > 2) ? argv[2] : NULL;
    char* control_str = (argc > 3) ? argv[3] : NULL;
    char* size_str = (argc > 4) ? argv[4] : NULL;
    char** payload_values = (argc > 5) ? &argv[5] : NULL;
    int payload_count = (argc > 5) ? (argc - 5) : 0;
    return handle_session_data_transfer_phase_config_command(session_id_str,
                                                             repetition_str,
                                                             control_str,
                                                             size_str,
                                                             payload_values,
                                                             payload_count);
}

static int cmd_session_set_hybrid_controller_config(int argc, char** argv) {
    char* session_id_str = (argc > 1) ? argv[1] : NULL;
    char* config_data_str = (argc > 2) ? argv[2] : NULL;
    int config_len = config_data_str ? (int)strlen(config_data_str) : 0;
    return handle_session_set_hybrid_controller_config_command(session_id_str,
                                                               (unsigned char*)config_data_str,
                                                               config_len);
}

static int cmd_session_set_hybrid_controlee_config(int argc, char** argv) {
    char* session_id_str = (argc > 1) ? argv[1] : NULL;
    char* config_data_str = (argc > 2) ? argv[2] : NULL;
    int config_len = config_data_str ? (int)strlen(config_data_str) : 0;
    return handle_session_set_hybrid_controlee_config_command(session_id_str,
                                                              (unsigned char*)config_data_str,
                                                              config_len);
}

static int cmd_session_query_data_size_in_ranging(int argc, char** argv) {
    char* session_id_str = (argc > 1) ? argv[1] : NULL;
    return handle_session_query_data_size_in_ranging_command(session_id_str);
}

static int cmd_simulate_notification(int argc, char** argv) {
    if (argc < 3) {
        printf("Usage: simulate_notification <type> <value>\n");
        printf("  Example: simulate_notification device_status active\n");
        return -1;
    }

    const char* type_str = argv[1];
    const char* value_str = argv[2];

    if (strcmp(type_str, "device_status") == 0) {
        unsigned char device_state;
        if (strcmp(value_str, "active") == 0) {
            device_state = DEVICE_STATE_ACTIVE;
        } else if (strcmp(value_str, "ready") == 0) {
            device_state = DEVICE_STATE_READY;
        } else if (strcmp(value_str, "error") == 0) {
            device_state = DEVICE_STATE_ERROR;
        } else {
            printf("Invalid value for device_status. Use 'active', 'ready', or 'error'.\n");
            return -1;
        }

        unsigned char notification_packet[sizeof(struct uci_packet_header) + 1];
        struct uci_packet_header* ntf_header = (struct uci_packet_header*)notification_packet;
        set_header_values_safe(ntf_header, NOTIFICATION, COMPLETE, CORE, CORE_DEVICE_STATUS_NTF, 1);
        notification_packet[sizeof(struct uci_packet_header)] = device_state;
        parse_uci_packet(notification_packet, sizeof(struct uci_packet_header) + 1);
        return 0;
    }

    printf("Unknown notification type: %s\n", type_str);
    return -1;
}

static int cmd_simulate_session_status(int argc, char** argv) {
    if (argc < 4) {
        printf("Usage: simulate_session_status <session_id> <state> <reason>\n");
        printf("  Example: simulate_session_status 1 active mgmt_cmd\n");
        return -1;
    }

    unsigned int session_id = (unsigned int)strtoul(argv[1], NULL, 10);
    const char* state_str = argv[2];
    const char* reason_str = argv[3];

    unsigned char session_state;
    unsigned char reason_code;

    if (strcmp(state_str, "init") == 0) session_state = SESSION_STATE_INIT;
    else if (strcmp(state_str, "deinit") == 0) session_state = SESSION_STATE_DEINIT;
    else if (strcmp(state_str, "active") == 0) session_state = SESSION_STATE_ACTIVE;
    else if (strcmp(state_str, "idle") == 0) session_state = SESSION_STATE_IDLE;
    else {
        printf("Invalid session state '%s'. Use init, deinit, active, or idle.\n", state_str);
        return -1;
    }

    if (strcmp(reason_str, "mgmt_cmd") == 0 || strcmp(reason_str, "command") == 0) {
        reason_code = STATE_CHANGE_WITH_SESSION_MANAGEMENT_COMMANDS;
    } else {
        printf("Invalid reason '%s'. Use mgmt_cmd.\n", reason_str);
        return -1;
    }

    unsigned char notification_packet[sizeof(struct uci_packet_header) + 6];
    struct uci_packet_header* ntf_header = (struct uci_packet_header*)notification_packet;
    set_header_values_safe(ntf_header, NOTIFICATION, COMPLETE, SESSION_CONFIG, SESSION_STATUS_NTF, 6);
    notification_packet[sizeof(struct uci_packet_header)] = (unsigned char)(session_id & 0xFF);
    notification_packet[sizeof(struct uci_packet_header) + 1] = (unsigned char)((session_id >> 8) & 0xFF);
    notification_packet[sizeof(struct uci_packet_header) + 2] = (unsigned char)((session_id >> 16) & 0xFF);
    notification_packet[sizeof(struct uci_packet_header) + 3] = (unsigned char)((session_id >> 24) & 0xFF);
    notification_packet[sizeof(struct uci_packet_header) + 4] = session_state;
    notification_packet[sizeof(struct uci_packet_header) + 5] = reason_code;
    parse_uci_packet(notification_packet, sizeof(struct uci_packet_header) + 6);
    return 0;
}

static int cmd_simulate_data_credit(int argc, char** argv) {
    (void)argc;
    (void)argv;

    unsigned char notification_packet[sizeof(struct uci_packet_header) + 5];
    struct uci_packet_header* ntf_header = (struct uci_packet_header*)notification_packet;
    set_header_values_safe(ntf_header, NOTIFICATION, COMPLETE, SESSION_CONTROL, SESSION_DATA_CREDIT_NTF, 5);
    notification_packet[sizeof(struct uci_packet_header)] = 0x01;
    notification_packet[sizeof(struct uci_packet_header) + 1] = 0x02;
    notification_packet[sizeof(struct uci_packet_header) + 2] = 0x03;
    notification_packet[sizeof(struct uci_packet_header) + 3] = 0x04;
    notification_packet[sizeof(struct uci_packet_header) + 4] = 0x01;
    parse_uci_packet(notification_packet, sizeof(struct uci_packet_header) + 5);
    return 0;
}

static int cmd_demo_session_flow(int argc, char** argv) {
    (void)argc;
    (void)argv;

    printf("=== UCI Session Flow Demonstration ===\n");

    printf("\n1. Initializing session...\n");
    unsigned char init_payload[] = {0x01, 0x02, 0x03, 0x04, FIRA_RANGING_SESSION};
    send_uci_command(COMMAND, 0, SESSION_CONFIG, SESSION_INIT, init_payload, sizeof(init_payload));

    printf("\n2. Session initialization complete - received status notification:\n");
    unsigned char ntf_packet1[sizeof(struct uci_packet_header) + 6];
    struct uci_packet_header* ntf_header1 = (struct uci_packet_header*)ntf_packet1;
    set_header_values_safe(ntf_header1, NOTIFICATION, COMPLETE, SESSION_CONFIG, SESSION_STATUS_NTF, 6);
    ntf_packet1[sizeof(struct uci_packet_header)] = 0x01;
    ntf_packet1[sizeof(struct uci_packet_header) + 1] = 0x02;
    ntf_packet1[sizeof(struct uci_packet_header) + 2] = 0x03;
    ntf_packet1[sizeof(struct uci_packet_header) + 3] = 0x04;
    ntf_packet1[sizeof(struct uci_packet_header) + 4] = SESSION_STATE_INIT;
    ntf_packet1[sizeof(struct uci_packet_header) + 5] = STATE_CHANGE_WITH_SESSION_MANAGEMENT_COMMANDS;
    parse_uci_packet(ntf_packet1, sizeof(struct uci_packet_header) + 6);

    printf("\n3. Starting session...\n");
    unsigned char start_payload[] = {0x01, 0x02, 0x03, 0x04};
    send_uci_command(COMMAND, 0, SESSION_CONTROL, SESSION_START, start_payload, sizeof(start_payload));

    printf("\n4. Session started - received status notification:\n");
    unsigned char ntf_packet2[sizeof(struct uci_packet_header) + 6];
    struct uci_packet_header* ntf_header2 = (struct uci_packet_header*)ntf_packet2;
    set_header_values_safe(ntf_header2, NOTIFICATION, COMPLETE, SESSION_CONFIG, SESSION_STATUS_NTF, 6);
    ntf_packet2[sizeof(struct uci_packet_header)] = 0x01;
    ntf_packet2[sizeof(struct uci_packet_header) + 1] = 0x02;
    ntf_packet2[sizeof(struct uci_packet_header) + 2] = 0x03;
    ntf_packet2[sizeof(struct uci_packet_header) + 3] = 0x04;
    ntf_packet2[sizeof(struct uci_packet_header) + 4] = SESSION_STATE_ACTIVE;
    ntf_packet2[sizeof(struct uci_packet_header) + 5] = STATE_CHANGE_WITH_SESSION_MANAGEMENT_COMMANDS;
    parse_uci_packet(ntf_packet2, sizeof(struct uci_packet_header) + 6);

    printf("\n5. Data credit available - received notification:\n");
    unsigned char ntf_packet3[sizeof(struct uci_packet_header) + 5];
    struct uci_packet_header* ntf_header3 = (struct uci_packet_header*)ntf_packet3;
    set_header_values_safe(ntf_header3, NOTIFICATION, COMPLETE, SESSION_CONTROL, SESSION_DATA_CREDIT_NTF, 5);
    ntf_packet3[sizeof(struct uci_packet_header)] = 0x01;
    ntf_packet3[sizeof(struct uci_packet_header) + 1] = 0x02;
    ntf_packet3[sizeof(struct uci_packet_header) + 2] = 0x03;
    ntf_packet3[sizeof(struct uci_packet_header) + 3] = 0x04;
    ntf_packet3[sizeof(struct uci_packet_header) + 4] = 0x01;
    parse_uci_packet(ntf_packet3, sizeof(struct uci_packet_header) + 5);

    printf("\n=== Session Flow Demonstration Complete ===\n");
    return 0;
}

static int cmd_simulate_ranging(int argc, char** argv) {
    (void)argc;
    (void)argv;

    if (ui_color_enabled) {
        printf("%s%s%s=== Simulating UWB Ranging Notification ===%s\n",
               ANSI_COLOR_BRIGHT_CYAN, ANSI_BOLD, ANSI_BG_BLUE, ANSI_RESET);
    } else {
        printf("=== Simulating UWB Ranging Notification ===\n");
    }

    unsigned char ranging_ntf_payload[] = {
        0x09, 0x00, 0x00, 0x00,
        0x2a, 0x00, 0x00, 0x00,
        0x00,
        0xe8, 0x03, 0x00, 0x00,
        0x01,
        0x00,
        0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x01,
        0x12, 0x34,
        0x00,
        0x00,
        0x64, 0x00,
        0x14, 0x00,
        0x08,
        0x05, 0x00,
        0x07,
        0x10, 0x00,
        0x06,
        0x03, 0x00,
        0x09,
        0x02,
        0xE0,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };

    unsigned char notification_packet[sizeof(struct uci_packet_header) + sizeof(ranging_ntf_payload)];
    struct uci_packet_header* ntf_header = (struct uci_packet_header*)notification_packet;
    set_header_values_safe(ntf_header, NOTIFICATION, COMPLETE, RANGING_DATA, RANGE_DATA_NTF_OPCODE, sizeof(ranging_ntf_payload));
    memcpy(notification_packet + sizeof(struct uci_packet_header), ranging_ntf_payload, sizeof(ranging_ntf_payload));

    if (ui_color_enabled) {
        printf("%s%s→ Sending simulated ranging notification packet%s\n",
               ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("→ Sending simulated ranging notification packet\n");
    }

    if (ui_color_enabled) {
        printf("%s%sReceived UCI packet:%s\n", ANSI_COLOR_BRIGHT_CYAN, ANSI_BOLD, ANSI_RESET);
        printf("  %sMT:%s 0x%01X\n", ANSI_COLOR_BRIGHT_YELLOW, ANSI_RESET, NOTIFICATION);
        printf("  %sPBF:%s 0x%01X\n", ANSI_COLOR_BRIGHT_YELLOW, ANSI_RESET, COMPLETE);
        printf("  %sGID:%s 0x%01X\n", ANSI_COLOR_BRIGHT_YELLOW, ANSI_RESET, RANGING_DATA);
        printf("  %sOpcode:%s 0x%02X\n", ANSI_COLOR_BRIGHT_YELLOW, ANSI_RESET, RANGE_DATA_NTF_OPCODE);
    } else {
        printf("Received UCI packet:\n");
        printf("  MT: 0x%01X\n", NOTIFICATION);
        printf("  PBF: 0x%01X\n", COMPLETE);
        printf("  GID: 0x%01X\n", RANGING_DATA);
        printf("  Opcode: 0x%02X\n", RANGE_DATA_NTF_OPCODE);
    }

    parse_uci_packet(notification_packet, sizeof(notification_packet));
    return 0;
}

static int cmd_simulate_multi_target_ranging(int argc, char** argv) {
    (void)argc;
    (void)argv;

    unsigned char multi_ranging_ntf_payload[] = {
        0x0A, 0x00, 0x00, 0x00,
        0x2B, 0x00, 0x00, 0x00,
        0x00,
        0x20, 0x03, 0x00, 0x00,
        0x02,
        0x00,
        0x00,
        0xAA, 0xBB, 0xCC, 0xDD,
        0x00, 0x00, 0x00, 0x00,
        0x02,
        0x22, 0x11,
        0x00,
        0x01,
        0x32, 0x00,
        0x0F, 0x00,
        0x08,
        0x02, 0x00,
        0x06,
        0x0E, 0x00,
        0x05,
        0x03, 0x00,
        0x07,
        0x01,
        0xD8,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x34, 0x12,
        0x00,
        0x00,
        0x96, 0x00,
        0x20, 0x00,
        0x09,
        0x06, 0x00,
        0x05,
        0x12, 0x00,
        0x07,
        0x05, 0x00,
        0x08,
        0x03,
        0xC0,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };

    unsigned char notification_packet[sizeof(struct uci_packet_header) + sizeof(multi_ranging_ntf_payload)];
    struct uci_packet_header* ntf_header = (struct uci_packet_header*)notification_packet;
    set_header_values_safe(ntf_header, NOTIFICATION, COMPLETE, SESSION_CONTROL, SESSION_INFO_NTF, sizeof(multi_ranging_ntf_payload));
    memcpy(notification_packet + sizeof(struct uci_packet_header), multi_ranging_ntf_payload, sizeof(multi_ranging_ntf_payload));
    parse_uci_packet(notification_packet, sizeof(notification_packet));

    printf("=== Multi-Target Ranging Simulation Complete ===\n");
    return 0;
}

static int cmd_analyze_packet(int argc, char** argv) {
    if (argc <= 1) {
        handle_analyze_command(0, NULL);
    } else {
        handle_analyze_command(argc - 1, &argv[1]);
    }
    return 0;
}

static int cmd_help(int argc, char** argv) {
    (void)argc;
    (void)argv;
    cli_print_help();
    return 0;
}

int main(void) {
    char line[CLI_MAX_LINE_LENGTH];

    if (uci_config_init() != 0) {
        printf("Warning: Failed to initialize configuration manager\n");
    }

    uci_cmd_hardware_init(&g_hardware_mode, &g_uwb_chardev);
    ui_print_welcome_message();
    cli_initialize_readline();

    while (1) {
        char* input_line = readline("> ");
        if (input_line == NULL) {
            printf("\n");
            break;
        }

        input_line[strcspn(input_line, "\r\n")] = '\0';

        if (strlen(input_line) > 0) {
            cli_history_add(input_line);
            add_history(input_line);
        }

        strncpy(line, input_line, sizeof(line) - 1);
        line[sizeof(line) - 1] = '\0';
        free(input_line);

        if (line[0] == '\0') {
            continue;
        }

        if (strcmp(line, "quit") == 0) {
            break;
        }

        if (strcmp(line, "history") == 0) {
            cli_history_print();
            continue;
        }

        cli_expand_alias(line, sizeof(line));

        char* argv_tokens[CLI_MAX_TOKENS];
        int argc = cli_tokenize(line, argv_tokens, CLI_MAX_TOKENS);
        if (argc == 0) {
            continue;
        }

        cli_dispatch(argc, argv_tokens);
    }

    return 0;
}
