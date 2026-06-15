#include "../include/uci_cli_completion.h"
#include "../include/uci_command_framework.h"
#include "../include/uci_cli.h"
#include "../include/uci_globals.h"
#include "../include/uci_cmd_framework_bridge.h"
#include "../include/uci_ui.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_READLINE
#include <readline/readline.h>
#include <readline/history.h>
#endif


typedef struct {
    const uci_command_def_t** entries;
    int count;
} cli_command_cache_t;

static void cli_cache_fill_callback(const uci_command_def_t* def, void* ctx) {
    cli_command_cache_t* cache = ctx;
    cache->entries[cache->count++] = def;
}

static const uci_command_def_t** cli_get_command_cache(int* count_out) {
    static const uci_command_def_t** cache = NULL;
    static int cache_count = 0;

    if (!cache) {
        cache_count = uci_cmd_framework_total_command_count();
        if (cache_count <= 0) {
            cache_count = 0;
            return NULL;
        }

        cache = calloc((size_t)cache_count, sizeof(*cache));
        if (!cache) {
            cache_count = 0;
            return NULL;
        }

        cli_command_cache_t ctx = {
            .entries = (const uci_command_def_t**)cache,
            .count = 0,
        };
        uci_cmd_framework_for_each_command(cli_cache_fill_callback, &ctx);
        cache_count = ctx.count;
    }

    if (count_out) {
        *count_out = cache_count;
    }
    return cache;
}

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

    int cache_count = 0;
    const uci_command_def_t** cache = cli_get_command_cache(&cache_count);
    if (!cache) {
        return;
    }

    for (int i = 0; i < cache_count; i++) {
        const uci_command_def_t* cmd = cache[i];
        if (!cmd) {
            continue;
        }

        if (strncmp(input, cmd->name, strlen(input)) == 0) {
            printf("  %s - %s\n", cmd->name, cmd->description);
            continue;
        }

        for (size_t alias_idx = 0; alias_idx < ARRAY_SIZE(cmd->aliases); alias_idx++) {
            const char* alias = cmd->aliases[alias_idx];
            if (!alias) {
                break;
            }
            if (strncmp(input, alias, strlen(input)) == 0) {
                printf("  %s (alias for %s) - %s\n", alias, cmd->name, cmd->description);
                break;
            }
        }
    }
}

#ifdef HAVE_READLINE
// Generator function for readline completion
char** cli_completion_generator(const char* text, int start, int end) {
    (void)start;
    (void)end;
    
    char** matches = NULL;
    
    if (text) {
        matches = rl_completion_matches(text, cli_command_generator);
    }
    
    return matches;
}

// Command generator function for readline
char* cli_command_generator(const char* text, int state) {
    static int list_index, len;
    int cache_count = 0;
    const uci_command_def_t** cache = cli_get_command_cache(&cache_count);

    if (!text || !cache) {
        return NULL;
    }

    if (state == 0) {
        list_index = 0;
        len = (int)strlen(text);
    }

    while (list_index < cache_count) {
        const uci_command_def_t* cmd = cache[list_index++];
        if (!cmd) {
            continue;
        }

        if (strncmp(cmd->name, text, (size_t)len) == 0) {
            char* result = malloc(strlen(cmd->name) + 1);
            if (result) {
                strcpy(result, cmd->name);
            }
            return result;
        }

        for (size_t alias_idx = 0; alias_idx < ARRAY_SIZE(cmd->aliases); alias_idx++) {
            const char* alias = cmd->aliases[alias_idx];
            if (!alias) {
                break;
            }
            if (strncmp(alias, text, (size_t)len) == 0) {
                char* result = malloc(strlen(alias) + 1);
                if (result) {
                    strcpy(result, alias);
                }
                return result;
            }
        }
    }

    return NULL;
}
#endif
