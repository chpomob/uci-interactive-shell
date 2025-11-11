#ifndef UCI_CMD_FRAMEWORK_BRIDGE_H
#define UCI_CMD_FRAMEWORK_BRIDGE_H

#include "uci_command_framework.h"

extern const uci_command_def_t g_uci_command_defs[];
extern const int g_uci_command_defs_count;

const uci_command_def_t* uci_cmd_framework_find(const char* name);
int uci_cmd_framework_handler(int argc, char** argv);
typedef void (*uci_cmd_framework_iter_cb)(const uci_command_def_t* def, void* ctx);
void uci_cmd_framework_for_each_command(uci_cmd_framework_iter_cb cb, void* ctx);
int uci_cmd_framework_total_command_count(void);

#endif // UCI_CMD_FRAMEWORK_BRIDGE_H
