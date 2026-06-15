#include "../include/uci_cli.h"
#include "../include/uci_cli_completion.h"
#include "../include/uci_cmd_hardware.h"
#include "../include/uci_config_manager.h"
#include "../include/uci_functions.h"
#include "../include/uci_globals.h"
#include "../include/uci_logging.h"
#include "../include/uci_shell_runtime.h"
#include "../include/uci_ui.h"
#include "../include/uci_ui.h"

int main(int argc, char** argv) {
    // Initialize logging system with info level and color output
    uci_log_init(UCI_LOG_LEVEL_INFO, 1);
    
    if (argc > 1) {
        // Non-interactive mode - process command line arguments directly
        cli_dispatch(argc - 1, &argv[1]);
        return 0;
    }
    
    if (uci_config_init() != 0) {
        UCI_LOG_WARNING("Failed to initialize configuration manager");
    }

    uci_cmd_hardware_init(&g_hardware_mode, &g_uwb_chardev);
    ui_print_welcome_message();

    cli_initialize_readline();

    return uci_shell_run_interactive();
}
