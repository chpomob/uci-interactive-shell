#include "../include/uci_cli_completion.h"
#include "../include/uci_cli.h"
#include "../include/uci_globals.h"
#include "../include/uci_ui.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_READLINE
#include <readline/readline.h>
#include <readline/history.h>
#endif

void cli_initialize_readline(void) {
#ifdef HAVE_READLINE
    // Initialize readline with tab completion
    rl_attempted_completion_function = cli_completion_generator;
#endif
}

void cli_print_completion_suggestions(const char* input) {
    if (!input) {
        return;
    }

    printf("Available commands matching '%s':\n", input);
    
    for (int i = 0; i < g_cli_commands_count; i++) {
        const cli_command_t* cmd = &g_cli_commands[i];
        if (strncmp(input, cmd->name, strlen(input)) == 0) {
            printf("  %s - %s\n", cmd->name, cmd->description);
        } else {
            // Check aliases
            for (int j = 0; cmd->aliases[j] != NULL; j++) {
                if (strncmp(input, cmd->aliases[j], strlen(input)) == 0) {
                    printf("  %s (alias for %s) - %s\n", cmd->aliases[j], cmd->name, cmd->description);
                    break;
                }
            }
        }
    }
}

#ifdef HAVE_READLINE
// Generator function for readline completion
char* cli_completion_generator(const char* text, int state) {
    static int list_index, len;
    const char* name;

    if (!text) {
        return NULL;
    }

    if (state == 0) {
        // Initialize for new completion attempt
        list_index = 0;
        len = strlen(text);
    }

    // Look for commands that match
    while (list_index < g_cli_commands_count) {
        name = g_cli_commands[list_index].name;
        list_index++;

        if (strncmp(name, text, len) == 0) {
            return strdup(name);
        }

        // Check aliases
        const char** aliases = g_cli_commands[list_index - 1].aliases;
        for (int i = 0; aliases[i] != NULL; i++) {
            if (strncmp(aliases[i], text, len) == 0) {
                return strdup(aliases[i]);
            }
        }
    }

    return NULL;
}
#endif