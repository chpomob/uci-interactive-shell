#include "../include/uci_command_framework.h"
#include "../include/uci_cli.h"
#include <stdio.h>
#include <string.h>

// Array of all command definitions using the new framework
extern const uci_command_def_t g_uci_command_defs[];
extern const int g_uci_command_defs_count;

// Global lookup table to connect legacy command handlers to new framework commands
// Commented out unused variable to prevent compiler warning
// static struct {
//     const char* cmd_name;
//     const uci_command_def_t* def;
// } command_lookup_table[] = {
//     // This would be populated with actual command associations
// };

// Generic handler that bridges the old CLI system to the new framework
int uci_cmd_framework_handler(int argc, char** argv) {
    if (argc == 0 || !argv[0]) {
        return -1;
    }

    // Find the command definition that matches this command name
    const uci_command_def_t* cmd_def = NULL;
    
    // Look for the command in our framework definitions
    for (int i = 0; i < g_uci_command_defs_count; i++) {
        if (strcmp(g_uci_command_defs[i].name, argv[0]) == 0) {
            cmd_def = &g_uci_command_defs[i];
            break;
        }
        
        // Also check aliases
        for (int j = 0; j < 4; j++) {
            if (g_uci_command_defs[i].aliases[j] && 
                strcmp(g_uci_command_defs[i].aliases[j], argv[0]) == 0) {
                cmd_def = &g_uci_command_defs[i];
                break;
            }
        }
        
        if (cmd_def) break;
    }

    if (!cmd_def) {
        printf("Command not found in framework: %s\n", argv[0]);
        return -1;
    }

    return uci_cmd_dispatch(cmd_def, argc, argv);
}