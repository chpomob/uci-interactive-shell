#include <stdio.h>
#include <string.h>
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

void cli_print_help(void) {
    printf("Available commands:\n");
    for (int i = 0; i < g_cli_commands_count; i++) {
        printf("  %s\n", g_cli_commands[i].name);
    }
}
