#ifndef UCI_GLOBALS_H
#define UCI_GLOBALS_H

#include "uci_cli.h"
#include "uci_hw_chardev.h"

extern const cli_command_t g_cli_commands[];
extern const int g_cli_commands_count;
extern int g_hardware_mode;
extern uci_hw_chardev_t g_uwb_chardev;

#endif // UCI_GLOBALS_H
