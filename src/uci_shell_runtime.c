#include "../include/uci_shell_runtime.h"

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <unistd.h>

#ifdef HAVE_READLINE
#include <readline/history.h>
#include <readline/readline.h>
#endif

#include "../include/uci.h"
#include "../include/uci_cli.h"
#include "../include/uci_functions.h"
#include "../include/uci_globals.h"
#include "../include/uci_hw_chardev.h"
#include "../include/uci_tcp_transport.h"

#define CLI_MAX_TOKENS 64

static int g_should_exit = 0;

static void process_command(char* line) {
    char* argv_tokens[CLI_MAX_TOKENS];
    int argc;

    if (!line || line[0] == '\0') {
        return;
    }

    if (strcmp(line, "quit") == 0) {
        g_should_exit = 1;
        return;
    }

    argc = cli_tokenize(line, argv_tokens, CLI_MAX_TOKENS);
    if (argc == 0) {
        return;
    }

    cli_dispatch(argc, argv_tokens);
}

static int get_active_hardware_fd(void) {
    if (uci_get_transport_mode() == UCI_TRANSPORT_MODE_HARDWARE && g_uwb_chardev.fd >= 0) {
        return g_uwb_chardev.fd;
    }

    return -1;
}

static int get_active_tcp_fd(void) {
    if (uci_get_transport_mode() == UCI_TRANSPORT_MODE_TCP) {
        return uci_tcp_transport_get_fd();
    }

    return -1;
}

static void service_hardware_transport(void) {
    unsigned char buffer[1024];
    int len = uci_hw_chardev_receive(&g_uwb_chardev, buffer, sizeof(buffer), 0);

    if (len > 0) {
        parse_uci_packet(buffer, (size_t)len);
    }
}

static void service_tcp_transport(void) {
    (void)uci_receive_tcp_packets(0);
}

#ifdef HAVE_READLINE
static void refresh_readline_prompt(void) {
    if (g_should_exit) {
        return;
    }

    rl_on_new_line();
    rl_forced_update_display();
}

static void handle_readline_input(char* line) {
    if (!line) {
        printf("\n");
        g_should_exit = 1;
        return;
    }

    if (line[0] != '\0') {
        add_history(line);
        process_command(line);
    }

    free(line);
}
#else
static void print_prompt(void) {
    if (!g_should_exit) {
        printf("> ");
        fflush(stdout);
    }
}
#endif

int uci_shell_run_interactive(void) {
    fd_set read_fds;
    int max_fd;

    g_should_exit = 0;

#ifdef HAVE_READLINE
    rl_callback_handler_install("> ", handle_readline_input);
#else
    static char line_buffer[CLI_MAX_LINE_LENGTH];
    static int line_buffer_len = 0;

    line_buffer_len = 0;
    print_prompt();
#endif

    while (!g_should_exit) {
        int hardware_fd;
        int tcp_fd;
        int activity;

        FD_ZERO(&read_fds);
        FD_SET(STDIN_FILENO, &read_fds);
        max_fd = STDIN_FILENO;

        hardware_fd = get_active_hardware_fd();
        tcp_fd = get_active_tcp_fd();

        if (hardware_fd >= 0) {
            FD_SET(hardware_fd, &read_fds);
            if (hardware_fd > max_fd) {
                max_fd = hardware_fd;
            }
        } else if (tcp_fd >= 0) {
            FD_SET(tcp_fd, &read_fds);
            if (tcp_fd > max_fd) {
                max_fd = tcp_fd;
            }
        }

        activity = select(max_fd + 1, &read_fds, NULL, NULL, NULL);
        if (activity < 0) {
            if (errno == EINTR) {
                continue;
            }

            perror("select error");
            break;
        }

        if (hardware_fd >= 0 && FD_ISSET(hardware_fd, &read_fds)) {
            service_hardware_transport();
#ifdef HAVE_READLINE
            refresh_readline_prompt();
#else
            print_prompt();
#endif
        } else if (tcp_fd >= 0 && FD_ISSET(tcp_fd, &read_fds)) {
            service_tcp_transport();
#ifdef HAVE_READLINE
            refresh_readline_prompt();
#else
            print_prompt();
#endif
        }

        if (FD_ISSET(STDIN_FILENO, &read_fds)) {
#ifdef HAVE_READLINE
            rl_callback_read_char();
#else
            int nread = read(STDIN_FILENO,
                             &line_buffer[line_buffer_len],
                             sizeof(line_buffer) - (size_t)line_buffer_len - 1);
            if (nread > 0) {
                line_buffer_len += nread;
                if (line_buffer[line_buffer_len - 1] == '\n') {
                    line_buffer[line_buffer_len - 1] = '\0';
                    process_command(line_buffer);
                    line_buffer_len = 0;
                    print_prompt();
                }
            } else {
                printf("\n");
                g_should_exit = 1;
            }
#endif
        }
    }

#ifdef HAVE_READLINE
    rl_callback_handler_remove();
#endif

    return 0;
}
