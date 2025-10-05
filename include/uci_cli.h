#ifndef UCI_CLI_H
#define UCI_CLI_H

#include <stddef.h>

#define CLI_MAX_LINE_LENGTH 256
#define CLI_MAX_HISTORY_SIZE 100
#define CLI_MAX_ALIASES 50
#define CLI_MAX_ALIAS_TEXT 128

typedef enum {
    CLI_ALIAS_SUCCESS = 0,
    CLI_ALIAS_UPDATED,
    CLI_ALIAS_FULL,
    CLI_ALIAS_NOT_FOUND
} cli_alias_result_t;

void cli_history_add(const char* command);
void cli_history_print(void);

cli_alias_result_t cli_alias_add(const char* alias, const char* command);
const char* cli_alias_lookup(const char* alias);
void cli_alias_print_all(void);
cli_alias_result_t cli_alias_remove(const char* alias);
void cli_alias_print_names(void);

#endif // UCI_CLI_H
