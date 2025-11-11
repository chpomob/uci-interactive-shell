#ifndef UCI_CLI_H
#define UCI_CLI_H

#include <stddef.h>
#include "uci_cmd_framework_bridge.h"

#define CLI_MAX_LINE_LENGTH 256

int cli_tokenize(char* line, char** argv, int max_tokens);
int cli_dispatch(int argc, char** argv);
void cli_print_help(void);
const uci_command_def_t* cli_find_command(const char* name);

#endif // UCI_CLI_H
