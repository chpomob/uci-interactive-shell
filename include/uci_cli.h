#ifndef UCI_CLI_H
#define UCI_CLI_H

#include <stddef.h>

#define CLI_MAX_LINE_LENGTH 256

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

int cli_tokenize(char* line, char** argv, int max_tokens);
int cli_dispatch(int argc, char** argv);
void cli_print_help(void);
const cli_command_t* cli_find_command(const char* name);

#endif // UCI_CLI_H