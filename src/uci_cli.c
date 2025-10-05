#include <stdio.h>
#include <string.h>

#include "../include/uci_cli.h"

static char g_history[CLI_MAX_HISTORY_SIZE][CLI_MAX_LINE_LENGTH];
static int g_history_count = 0;

static char g_aliases[CLI_MAX_ALIASES][2][CLI_MAX_ALIAS_TEXT];
static int g_alias_count = 0;

void cli_history_add(const char* command) {
    if (command == NULL || command[0] == '\0') {
        return;
    }

    if (g_history_count > 0) {
        const char* last_entry = g_history[(g_history_count - 1) % CLI_MAX_HISTORY_SIZE];
        if (strncmp(last_entry, command, CLI_MAX_LINE_LENGTH) == 0) {
            return;
        }
    }

    strncpy(g_history[g_history_count % CLI_MAX_HISTORY_SIZE], command, CLI_MAX_LINE_LENGTH - 1);
    g_history[g_history_count % CLI_MAX_HISTORY_SIZE][CLI_MAX_LINE_LENGTH - 1] = '\0';
    g_history_count++;
}

void cli_history_print(void) {
    int start = (g_history_count > CLI_MAX_HISTORY_SIZE)
                    ? (g_history_count - CLI_MAX_HISTORY_SIZE)
                    : 0;

    for (int i = start; i < g_history_count; i++) {
        int index = i % CLI_MAX_HISTORY_SIZE;
        printf("%4d  %s\n", i + 1, g_history[index]);
    }
}

cli_alias_result_t cli_alias_add(const char* alias, const char* command) {
    if (alias == NULL || alias[0] == '\0' || command == NULL || command[0] == '\0') {
        return CLI_ALIAS_NOT_FOUND;
    }

    for (int i = 0; i < g_alias_count; i++) {
        if (strcmp(g_aliases[i][0], alias) == 0) {
            strncpy(g_aliases[i][1], command, CLI_MAX_ALIAS_TEXT - 1);
            g_aliases[i][1][CLI_MAX_ALIAS_TEXT - 1] = '\0';
            return CLI_ALIAS_UPDATED;
        }
    }

    if (g_alias_count >= CLI_MAX_ALIASES) {
        return CLI_ALIAS_FULL;
    }

    strncpy(g_aliases[g_alias_count][0], alias, CLI_MAX_ALIAS_TEXT - 1);
    g_aliases[g_alias_count][0][CLI_MAX_ALIAS_TEXT - 1] = '\0';
    strncpy(g_aliases[g_alias_count][1], command, CLI_MAX_ALIAS_TEXT - 1);
    g_aliases[g_alias_count][1][CLI_MAX_ALIAS_TEXT - 1] = '\0';
    g_alias_count++;
    return CLI_ALIAS_SUCCESS;
}

const char* cli_alias_lookup(const char* alias) {
    if (alias == NULL) {
        return NULL;
    }

    for (int i = 0; i < g_alias_count; i++) {
        if (strcmp(g_aliases[i][0], alias) == 0) {
            return g_aliases[i][1];
        }
    }
    return NULL;
}

void cli_alias_print_all(void) {
    if (g_alias_count == 0) {
        printf("No aliases defined.\n");
        return;
    }

    printf("Defined aliases:\n");
    for (int i = 0; i < g_alias_count; i++) {
        printf("  %s -> %s\n", g_aliases[i][0], g_aliases[i][1]);
    }
}

cli_alias_result_t cli_alias_remove(const char* alias) {
    if (alias == NULL) {
        return CLI_ALIAS_NOT_FOUND;
    }

    for (int i = 0; i < g_alias_count; i++) {
        if (strcmp(g_aliases[i][0], alias) == 0) {
            for (int j = i; j < g_alias_count - 1; j++) {
                memcpy(g_aliases[j], g_aliases[j + 1], sizeof(g_aliases[j]));
            }
            g_alias_count--;
            return CLI_ALIAS_SUCCESS;
        }
    }
    return CLI_ALIAS_NOT_FOUND;
}

void cli_alias_print_names(void) {
    for (int i = 0; i < g_alias_count; i++) {
        printf("%s ", g_aliases[i][0]);
    }
}
