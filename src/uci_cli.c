#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "../include/uci_cli.h"
#include "../include/uci_ui.h"

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

#include "../include/uci_globals.h"

int cli_tokenize(char* line, char** argv, int max_tokens) {
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

const cli_command_t* cli_find_command(const char* name) {
    if (!name) {
        return NULL;
    }

    for (int i = 0; i < g_cli_commands_count; i++) {
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

int cli_dispatch(int argc, char** argv) {
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

typedef struct {
    cli_command_group_t group;
    const char* title;
    const char* summary;
} cli_group_info_t;

static const cli_group_info_t k_group_info[] = {
    { CLI_GROUP_GENERAL, "General", "Shell basics and global utilities" },
    { CLI_GROUP_HARDWARE, "Hardware", "Switch transports and manage device connections" },
    { CLI_GROUP_DEVICE, "Device", "Core device management and configuration" },
    { CLI_GROUP_SESSION, "Session", "Session lifecycle and data exchange" },
    { CLI_GROUP_SESSION_CONFIG, "Session Config", "Extended session configuration commands" },
    { CLI_GROUP_ANALYSIS, "Analysis", "Packet analyzers and debugging helpers" },
    { CLI_GROUP_SIMULATION, "Simulation", "Demo and simulator-oriented commands" },
};

static size_t build_command_label(const cli_command_t* cmd, char* buffer, size_t buffer_len) {
    size_t written = 0;
    if (buffer_len > 0) {
        buffer[0] = '\0';
    }

    int result = snprintf(buffer, buffer_len, "%s", cmd->name);
    if (result < 0) {
        return 0;
    }
    written = (size_t)result;

    bool has_alias = false;
    for (size_t alias_idx = 0; alias_idx < ARRAY_SIZE(cmd->aliases); alias_idx++) {
        if (cmd->aliases[alias_idx] != NULL) {
            has_alias = true;
            break;
        }
    }

    if (!has_alias) {
        return written;
    }

    result = snprintf(buffer + written, (written < buffer_len) ? buffer_len - written : 0,
                      " (aliases: ");
    if (result > 0) {
        written += (size_t)result;
    }

    bool first_alias = true;
    for (size_t alias_idx = 0; alias_idx < ARRAY_SIZE(cmd->aliases); alias_idx++) {
        const char* alias = cmd->aliases[alias_idx];
        if (alias == NULL) {
            break;
        }

        result = snprintf(buffer + written, (written < buffer_len) ? buffer_len - written : 0,
                          "%s%s", first_alias ? "" : ", ", alias);
        if (result > 0) {
            written += (size_t)result;
        }
        first_alias = false;
    }

    result = snprintf(buffer + written, (written < buffer_len) ? buffer_len - written : 0, ")");
    if (result > 0) {
        written += (size_t)result;
    }

    return written;
}

void cli_print_help(void) {
    enum { kLabelBufferSize = 256 };
    char label_buffer[kLabelBufferSize];

    printf("UCI Shell Commands\n");
    printf("===================\n");
    printf("Invoke as \"uci-shell <command> [args]\" for one-shot usage, or run without arguments to enter the interactive shell.\n\n");

    bool first_section = true;
    for (size_t group_idx = 0; group_idx < ARRAY_SIZE(k_group_info); group_idx++) {
        const cli_group_info_t* group = &k_group_info[group_idx];

        size_t max_label_width = 0;
        bool group_has_commands = false;

        for (int cmd_idx = 0; cmd_idx < g_cli_commands_count; cmd_idx++) {
            const cli_command_t* cmd = &g_cli_commands[cmd_idx];
            if (cmd->group != group->group) {
                continue;
            }

            group_has_commands = true;
            build_command_label(cmd, label_buffer, sizeof(label_buffer));
            size_t label_len = strlen(label_buffer);
            if (label_len > max_label_width) {
                max_label_width = label_len;
            }
        }

        if (!group_has_commands) {
            continue;
        }

        if (!first_section) {
            printf("\n");
        }
        first_section = false;

        printf("%s\n", group->title);
        for (size_t dash = 0; dash < strlen(group->title); dash++) {
            putchar('-');
        }
        putchar('\n');
        if (group->summary && group->summary[0] != '\0') {
            printf("  %s\n", group->summary);
        }

        for (int cmd_idx = 0; cmd_idx < g_cli_commands_count; cmd_idx++) {
            const cli_command_t* cmd = &g_cli_commands[cmd_idx];
            if (cmd->group != group->group) {
                continue;
            }

            build_command_label(cmd, label_buffer, sizeof(label_buffer));

            printf("  %-*s  %s", (int)max_label_width, label_buffer,
                   (cmd->description && cmd->description[0] != '\0') ? cmd->description : "-");
            if (cmd->flags & CLI_CMD_FLAG_REQUIRES_HW_MODE) {
                printf(" [requires hardware mode]");
            }
            printf("\n");
        }
    }
}
