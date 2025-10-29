#include "../include/uci_globals.h"

// Minimal version for tests that only includes global variables
// without the command table which references functions in main.c

// Global variables definitions
int g_hardware_mode = 0;
uci_hw_chardev_t g_uwb_chardev;

// Empty command table for tests to satisfy uci_cli.c dependencies
const cli_command_t g_cli_commands[] = {};
const int g_cli_commands_count = 0;