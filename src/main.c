#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <stdbool.h>
#include <sys/select.h>
#include <sys/time.h>


#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#ifdef HAVE_READLINE
#include <readline/readline.h>
#include <readline/history.h>
#endif

#include "../include/uci.h"
#include "../include/uci_functions.h"
#include "../include/uci_cli.h"
#include "../include/uci_cli_completion.h"
#include "../include/uci_config_manager.h"
#include "../include/uci_hw.h"
#include "../include/uci_hw_interface.h"
#include "../include/uci_hw_chardev.h"
#include "../include/uci_ui.h"
#include "../include/uci_ui_main_patch.h"
#include "../include/uci_packet_utils.h"
#include "../include/uci_cmd_hardware.h"
#include "../include/uci_cmd_core.h"
#include "../include/uci_cmd_session.h"
#include "../include/uci_cmd_session_config.h"
#include "../include/uci_cmd_analysis.h"
#include "../include/uci_globals.h"
#include "../include/uci_standardized_error_handling.h"
#include "../include/uci_types.h"

#define MAX_PAYLOAD_LENGTH 255
#define CLI_MAX_TOKENS 64

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

void process_command(char *line) {
    if (line[0] == '\0') {
        return;
    }

    if (strcmp(line, "quit") == 0) {
        exit(0);
    }

    char* argv_tokens[CLI_MAX_TOKENS];
    int argc = cli_tokenize(line, argv_tokens, CLI_MAX_TOKENS);
    if (argc == 0) {
        return;
    }

    cli_dispatch(argc, argv_tokens);
}

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
    
    // Initialize readline completion
    cli_initialize_readline();

#ifdef HAVE_READLINE
    // Use readline for input
    char* line;
    while ((line = readline("> ")) != NULL) {
        if (strlen(line) > 0) {
            add_history(line);
            process_command(line);
        }
        free(line);
    }
    printf("\n");
#else
    // Fallback to simple line reading
    fd_set read_fds;
    int max_fd;

    static char line_buffer[CLI_MAX_LINE_LENGTH];
    static int line_buffer_len = 0;

    while (1) {
        FD_ZERO(&read_fds);
        FD_SET(STDIN_FILENO, &read_fds);
        max_fd = STDIN_FILENO;

        if (g_hardware_mode && g_uwb_chardev.fd >= 0) {
            FD_SET(g_uwb_chardev.fd, &read_fds);
            if (g_uwb_chardev.fd > max_fd) {
                max_fd = g_uwb_chardev.fd;
            }
        }
        
        printf("> ");
        fflush(stdout);

        int activity = select(max_fd + 1, &read_fds, NULL, NULL, NULL);

        if (activity < 0) {
            perror("select error");
            break;
        }

        if (FD_ISSET(STDIN_FILENO, &read_fds)) {
            int nread = read(STDIN_FILENO, &line_buffer[line_buffer_len], sizeof(line_buffer) - line_buffer_len -1);
            if (nread > 0) {
                line_buffer_len += nread;
                if (line_buffer[line_buffer_len - 1] == '\n') {
                    line_buffer[line_buffer_len - 1] = '\0';
                    process_command(line_buffer);
                    line_buffer_len = 0;
                }
            } else {
                // EOF
                printf("\n");
                break;
            }
        }

        if (g_hardware_mode && g_uwb_chardev.fd >= 0 && FD_ISSET(g_uwb_chardev.fd, &read_fds)) {
            unsigned char buffer[1024];
            int len = uci_hw_chardev_receive(&g_uwb_chardev, buffer, sizeof(buffer), 0);
            if (len > 0) {
                parse_uci_packet(buffer, len);
            }
        }
    }
#endif

    return 0;
}
