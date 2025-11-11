#include "uci_cli.h"
#include "uci_cmd_framework_bridge.h"

#define CLI_COMMAND_ENTRY(name, group, flags, desc, a0, a1, a2, a3) \
    { name, { a0, a1, a2, a3 }, group, flags, desc, uci_cmd_framework_handler },

const cli_command_t g_cli_commands[] = {
#include "cli_commands_general.inc"
#include "cli_commands_hardware.inc"
#include "cli_commands_device.inc"
#include "cli_commands_session.inc"
#include "cli_commands_session_config.inc"
#include "cli_commands_analysis.inc"
#include "cli_commands_simulation.inc"
};

#undef CLI_COMMAND_ENTRY

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
const int g_cli_commands_count = (int)ARRAY_SIZE(g_cli_commands);
