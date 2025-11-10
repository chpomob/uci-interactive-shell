#ifndef UCI_CMD_FRAMEWORK_BRIDGE_H
#define UCI_CMD_FRAMEWORK_BRIDGE_H

#include "uci_command_framework.h"

extern const uci_command_def_t g_uci_command_defs[];
extern const int g_uci_command_defs_count;

const uci_command_def_t* uci_cmd_framework_find(const char* name);
int uci_cmd_framework_handler(int argc, char** argv);

#endif // UCI_CMD_FRAMEWORK_BRIDGE_H
