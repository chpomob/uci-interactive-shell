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
#include "../include/uci_command_framework.h"
#include "../include/uci_standardized_error_handling.h"
#include "../include/uci_types.h"

#define MAX_PAYLOAD_LENGTH 255
#define CLI_MAX_TOKENS 64

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))





int cmd_hw_init(int argc, char** argv);
int cmd_hw_send(int argc, char** argv);
int cmd_mode_sim(int argc, char** argv);
int cmd_mode_hw(int argc, char** argv);
int cmd_mode_info(int argc, char** argv);
int cmd_get_device_info(int argc, char** argv);
int cmd_device_reset(int argc, char** argv);
int cmd_set_power(int argc, char** argv);
int cmd_device_on(int argc, char** argv);
int cmd_device_off(int argc, char** argv);
int cmd_get_caps_info(int argc, char** argv);
int cmd_get_config(int argc, char** argv);
int cmd_get_device_state(int argc, char** argv);
int cmd_set_device_active(int argc, char** argv);
int cmd_set_device_ready(int argc, char** argv);
int cmd_set_config(int argc, char** argv);
int cmd_device_suspend(int argc, char** argv);
int cmd_query_timestamp(int argc, char** argv);
int cmd_session_init(int argc, char** argv);
int cmd_session_deinit(int argc, char** argv);
int cmd_session_start(int argc, char** argv);
int cmd_session_stop(int argc, char** argv);
int cmd_session_send_data(int argc, char** argv);
int cmd_session_logical_link_create(int argc, char** argv);
int cmd_session_logical_link_close(int argc, char** argv);
int cmd_session_logical_link_get_param(int argc, char** argv);
int cmd_get_session_state(int argc, char** argv);
int cmd_set_app_config(int argc, char** argv);
int cmd_get_app_config(int argc, char** argv);
int cmd_session_update_multicast_list(int argc, char** argv);
int cmd_session_update_dt_tag_rounds(int argc, char** argv);
int cmd_session_data_transfer_phase_config(int argc, char** argv);
int cmd_session_set_hybrid_controller_config(int argc, char** argv);
int cmd_session_set_hybrid_controlee_config(int argc, char** argv);
int cmd_session_query_data_size_in_ranging(int argc, char** argv);
int cmd_simulate_notification(int argc, char** argv);
int cmd_simulate_session_status(int argc, char** argv);
int cmd_simulate_data_credit(int argc, char** argv);
int cmd_simulate_ranging(int argc, char** argv);
int cmd_simulate_multi_target_ranging(int argc, char** argv);
int cmd_demo_session_flow(int argc, char** argv);
int cmd_analyze_packet(int argc, char** argv);
int cmd_help(int argc, char** argv);






int cmd_hw_init(int argc, char** argv) {
    char* device_path = (argc > 1) ? argv[1] : NULL;
    return handle_hw_init_command(device_path);
}

int cmd_hw_send(int argc, char** argv) {
    char** payload_tokens = (argc > 5) ? &argv[5] : NULL;
    int payload_count = (argc > 5) ? (argc - 5) : 0;

    return handle_hw_send_command(
        (argc > 1) ? argv[1] : NULL,
        (argc > 2) ? argv[2] : NULL,
        (argc > 3) ? argv[3] : NULL,
        (argc > 4) ? argv[4] : NULL,
        payload_tokens,
        payload_count);
}

int cmd_mode_sim(int argc, char** argv) {
    (void)argc;
    (void)argv;
    handle_mode_sim_command();
    return 0;
}

int cmd_mode_hw(int argc, char** argv) {
    char* device_path = (argc > 1) ? argv[1] : NULL;
    return handle_mode_hw_command(device_path);
}

int cmd_mode_info(int argc, char** argv) {
    (void)argc;
    (void)argv;
    handle_mode_info_command();
    return 0;
}

int cmd_get_device_info(int argc, char** argv) {
    (void)argc;
    (void)argv;
    handle_get_device_info_command();
    return 0;
}

int cmd_device_reset(int argc, char** argv) {
    (void)argc;
    (void)argv;
    handle_device_reset_command();
    return 0;
}

int cmd_set_power(int argc, char** argv) {
    char* power_state = (argc > 1) ? argv[1] : NULL;
    return handle_set_power_command(power_state);
}

int cmd_device_on(int argc, char** argv) {
    (void)argc;
    (void)argv;
    handle_device_on_command();
    return 0;
}

int cmd_device_off(int argc, char** argv) {
    (void)argc;
    (void)argv;
    handle_device_off_command();
    return 0;
}

int cmd_get_caps_info(int argc, char** argv) {
    (void)argc;
    (void)argv;
    handle_get_caps_info_command();
    return 0;
}

int cmd_get_config(int argc, char** argv) {
    char* config_name = (argc > 1) ? argv[1] : NULL;
    return handle_get_config_command(config_name);
}

int cmd_get_device_state(int argc, char** argv) {
    (void)argc;
    (void)argv;
    handle_get_device_state_command();
    return 0;
}

int cmd_set_device_active(int argc, char** argv) {
    (void)argc;
    (void)argv;
    handle_set_device_active_command();
    return 0;
}

int cmd_set_device_ready(int argc, char** argv) {
    (void)argc;
    (void)argv;
    handle_set_device_ready_command();
    return 0;
}

int cmd_set_config(int argc, char** argv) {
    char* config_name = (argc > 1) ? argv[1] : NULL;
    char* value_str = (argc > 2) ? argv[2] : NULL;
    return handle_set_config_command(config_name, value_str);
}

int cmd_device_suspend(int argc, char** argv) {
    (void)argc;
    (void)argv;
    handle_device_suspend_command();
    return 0;
}

int cmd_query_timestamp(int argc, char** argv) {
    (void)argc;
    (void)argv;
    handle_query_timestamp_command();
    return 0;
}

int cmd_session_init(int argc, char** argv) {
    char* session_id_str = (argc > 1) ? argv[1] : NULL;
    char* session_type_str = (argc > 2) ? argv[2] : NULL;
    return handle_session_init_command(session_id_str, session_type_str);
}

int cmd_session_deinit(int argc, char** argv) {
    char* session_id_str = (argc > 1) ? argv[1] : NULL;
    return handle_session_deinit_command(session_id_str);
}

int cmd_session_start(int argc, char** argv) {
    char* session_id_str = (argc > 1) ? argv[1] : NULL;
    return handle_session_start_command(session_id_str);
}

int cmd_session_stop(int argc, char** argv) {
    char* session_id_str = (argc > 1) ? argv[1] : NULL;
    return handle_session_stop_command(session_id_str);
}

int cmd_session_send_data(int argc, char** argv) {
    char* session_id_str = (argc > 1) ? argv[1] : NULL;
    char* destination_str = (argc > 2) ? argv[2] : NULL;
    char* sequence_str = (argc > 3) ? argv[3] : NULL;
    char* payload_str = (argc > 4) ? argv[4] : NULL;
    return handle_session_send_data_command(session_id_str, destination_str, sequence_str, payload_str);
}

int cmd_session_logical_link_create(int argc, char** argv) {
    char* session_id_str = (argc > 1) ? argv[1] : NULL;
    char* link_id_str = (argc > 2) ? argv[2] : NULL;
    char* mode_str = (argc > 3) ? argv[3] : NULL;
    char* credit_str = (argc > 4) ? argv[4] : NULL;
    return handle_session_logical_link_create_command(session_id_str, link_id_str, mode_str, credit_str);
}

int cmd_session_logical_link_close(int argc, char** argv) {
    char* session_id_str = (argc > 1) ? argv[1] : NULL;
    char* link_id_str = (argc > 2) ? argv[2] : NULL;
    return handle_session_logical_link_close_command(session_id_str, link_id_str);
}

int cmd_session_logical_link_get_param(int argc, char** argv) {
    char* session_id_str = (argc > 1) ? argv[1] : NULL;
    char* link_id_str = (argc > 2) ? argv[2] : NULL;
    return handle_session_logical_link_get_param_command(session_id_str, link_id_str);
}

int cmd_get_session_state(int argc, char** argv) {
    char* session_id_str = (argc > 1) ? argv[1] : NULL;
    return handle_get_session_state_command(session_id_str);
}

int cmd_set_app_config(int argc, char** argv) {
    char* session_id_str = (argc > 1) ? argv[1] : NULL;
    char* config_name = (argc > 2) ? argv[2] : NULL;
    char* value_str = (argc > 3) ? argv[3] : NULL;
    return handle_set_app_config_command(session_id_str, config_name, value_str);
}

int cmd_get_app_config(int argc, char** argv) {
    char* session_id_str = (argc > 1) ? argv[1] : NULL;
    char** config_names = (argc > 2) ? &argv[2] : NULL;
    int config_count = (argc > 2) ? (argc - 2) : 0;
    return handle_get_app_config_command(session_id_str, config_names, config_count);
}

int cmd_session_update_multicast_list(int argc, char** argv) {
    char* session_id_str = (argc > 1) ? argv[1] : NULL;
    char* action_str = (argc > 2) ? argv[2] : NULL;
    char* short_address_str = (argc > 3) ? argv[3] : NULL;
    char* subsession_id_str = (argc > 4) ? argv[4] : NULL;
    return handle_update_multicast_list_command(session_id_str, action_str, short_address_str, subsession_id_str);
}

int cmd_session_update_dt_tag_rounds(int argc, char** argv) {
    char* session_id_str = (argc > 1) ? argv[1] : NULL;
    char** round_values = (argc > 2) ? &argv[2] : NULL;
    int round_count = (argc > 2) ? (argc - 2) : 0;
    return handle_session_update_dt_tag_rounds_command(session_id_str, round_values, round_count);
}

int cmd_session_data_transfer_phase_config(int argc, char** argv) {
    char* session_id_str = (argc > 1) ? argv[1] : NULL;
    char* repetition_str = (argc > 2) ? argv[2] : NULL;
    char* control_str = (argc > 3) ? argv[3] : NULL;
    char* size_str = (argc > 4) ? argv[4] : NULL;
    char** payload_values = (argc > 5) ? &argv[5] : NULL;
    int payload_count = (argc > 5) ? (argc - 5) : 0;
    return handle_session_data_transfer_phase_config_command(session_id_str,
                                                             repetition_str,
                                                             control_str,
                                                             size_str,
                                                             payload_values,
                                                             payload_count);
}

int cmd_session_set_hybrid_controller_config(int argc, char** argv) {
    char* session_id_str = (argc > 1) ? argv[1] : NULL;
    char* config_data_str = (argc > 2) ? argv[2] : NULL;
    int config_len = config_data_str ? (int)strlen(config_data_str) : 0;
    return handle_session_set_hybrid_controller_config_command(session_id_str,
                                                               (unsigned char*)config_data_str,
                                                               config_len);
}

int cmd_session_set_hybrid_controlee_config(int argc, char** argv) {
    char* session_id_str = (argc > 1) ? argv[1] : NULL;
    char* config_data_str = (argc > 2) ? argv[2] : NULL;
    int config_len = config_data_str ? (int)strlen(config_data_str) : 0;
    return handle_session_set_hybrid_controlee_config_command(session_id_str,
                                                              (unsigned char*)config_data_str,
                                                              config_len);
}

int cmd_session_query_data_size_in_ranging(int argc, char** argv) {
    char* session_id_str = (argc > 1) ? argv[1] : NULL;
    return handle_session_query_data_size_in_ranging_command(session_id_str);
}

int cmd_simulate_notification(int argc, char** argv) {
    if (argc < 3) {
        printf("Usage: simulate_notification <type> <value>\n");
        printf("  Example: simulate_notification device_status active\n");
        return -1;
    }

    const char* type_str = argv[1];
    const char* value_str = argv[2];

    if (strcmp(type_str, "device_status") == 0) {
        unsigned char device_state;
        if (strcmp(value_str, "active") == 0) {
            device_state = DEVICE_STATE_ACTIVE;
        } else if (strcmp(value_str, "ready") == 0) {
            device_state = DEVICE_STATE_READY;
        } else if (strcmp(value_str, "error") == 0) {
            device_state = DEVICE_STATE_ERROR;
        } else {
            printf("Invalid value for device_status. Use 'active', 'ready', or 'error'.\n");
            return -1;
        }

        size_t packet_len;
        unsigned char* notification_packet = create_uci_packet(NOTIFICATION, COMPLETE, CORE, CORE_DEVICE_STATUS_NTF,
                                                             &device_state, 1, &packet_len);
        if (notification_packet) {
            parse_uci_packet(notification_packet, packet_len);
            free(notification_packet);
        }
        return 0;
    }

    printf("Unknown notification type: %s\n", type_str);
    return -1;
}

int cmd_simulate_session_status(int argc, char** argv) {
    if (argc < 4) {
        printf("Usage: simulate_session_status <session_id> <state> <reason>\n");
        printf("  Example: simulate_session_status 1 active mgmt_cmd\n");
        return -1;
    }

    unsigned int session_id = (unsigned int)strtoul(argv[1], NULL, 10);
    const char* state_str = argv[2];
    const char* reason_str = argv[3];

    unsigned char session_state;
    unsigned char reason_code;

    if (strcmp(state_str, "init") == 0) session_state = SESSION_STATE_INIT;
    else if (strcmp(state_str, "deinit") == 0) session_state = SESSION_STATE_DEINIT;
    else if (strcmp(state_str, "active") == 0) session_state = SESSION_STATE_ACTIVE;
    else if (strcmp(state_str, "idle") == 0) session_state = SESSION_STATE_IDLE;
    else {
        printf("Invalid session state '%s'. Use init, deinit, active, or idle.\n", state_str);
        return -1;
    }

    if (strcmp(reason_str, "mgmt_cmd") == 0 || strcmp(reason_str, "command") == 0) {
        reason_code = STATE_CHANGE_WITH_SESSION_MANAGEMENT_COMMANDS;
    } else {
        printf("Invalid reason '%s'. Use mgmt_cmd.\n", reason_str);
        return -1;
    }

    unsigned char notification_payload[6];
    write_u32_le(notification_payload, session_id);
    notification_payload[4] = session_state;
    notification_payload[5] = reason_code;

    size_t packet_len;
    unsigned char* notification_packet = create_uci_packet(NOTIFICATION, COMPLETE, SESSION_CONFIG, SESSION_STATUS_NTF,
                                                         notification_payload, sizeof(notification_payload), &packet_len);
    if (notification_packet) {
        parse_uci_packet(notification_packet, packet_len);
        free(notification_packet);
    }
    return 0;
}

int cmd_simulate_data_credit(int argc, char** argv) {
    (void)argc;
    (void)argv;

    unsigned char notification_payload[5] = {0x01, 0x02, 0x03, 0x04, 0x01};

    size_t packet_len;
    unsigned char* notification_packet = create_uci_packet(NOTIFICATION, COMPLETE, SESSION_CONTROL, SESSION_DATA_CREDIT_NTF,
                                                         notification_payload, sizeof(notification_payload), &packet_len);
    if (notification_packet) {
        parse_uci_packet(notification_packet, packet_len);
        free(notification_packet);
    }
    return 0;
}

int cmd_demo_session_flow(int argc, char** argv) {
    (void)argc;
    (void)argv;

    printf("=== UCI Session Flow Demonstration ===\n");

    printf("\n1. Initializing session...\n");
    unsigned char init_payload[] = {0x01, 0x02, 0x03, 0x04, FIRA_RANGING_SESSION};
    send_uci_command(COMMAND, 0, SESSION_CONFIG, SESSION_INIT, init_payload, sizeof(init_payload));

    printf("\n2. Session initialization complete - received status notification:\n");
    unsigned char ntf_payload1[6] = {0x01, 0x02, 0x03, 0x04, SESSION_STATE_INIT, STATE_CHANGE_WITH_SESSION_MANAGEMENT_COMMANDS};
    size_t packet_len1;
    unsigned char* ntf_packet1 = create_uci_packet(NOTIFICATION, COMPLETE, SESSION_CONFIG, SESSION_STATUS_NTF,
                                                   ntf_payload1, sizeof(ntf_payload1), &packet_len1);
    if (ntf_packet1) {
        parse_uci_packet(ntf_packet1, packet_len1);
        free(ntf_packet1);
    }

    printf("\n3. Starting session...\n");
    unsigned char start_payload[] = {0x01, 0x02, 0x03, 0x04};
    send_uci_command(COMMAND, 0, SESSION_CONTROL, SESSION_START, start_payload, sizeof(start_payload));

    printf("\n4. Session started - received status notification:\n");
    unsigned char ntf_payload2[6] = {0x01, 0x02, 0x03, 0x04, SESSION_STATE_ACTIVE, STATE_CHANGE_WITH_SESSION_MANAGEMENT_COMMANDS};
    size_t packet_len2;
    unsigned char* ntf_packet2 = create_uci_packet(NOTIFICATION, COMPLETE, SESSION_CONFIG, SESSION_STATUS_NTF,
                                                   ntf_payload2, sizeof(ntf_payload2), &packet_len2);
    if (ntf_packet2) {
        parse_uci_packet(ntf_packet2, packet_len2);
        free(ntf_packet2);
    }

    printf("\n5. Data credit available - received notification:\n");
    unsigned char ntf_payload3[5] = {0x01, 0x02, 0x03, 0x04, 0x01};
    size_t packet_len3;
    unsigned char* ntf_packet3 = create_uci_packet(NOTIFICATION, COMPLETE, SESSION_CONTROL, SESSION_DATA_CREDIT_NTF,
                                                   ntf_payload3, sizeof(ntf_payload3), &packet_len3);
    if (ntf_packet3) {
        parse_uci_packet(ntf_packet3, packet_len3);
        free(ntf_packet3);
    }

    printf("\n=== Session Flow Demonstration Complete ===\n");
    return 0;
}

int cmd_simulate_ranging(int argc, char** argv) {
    (void)argc;
    (void)argv;

    if (ui_color_enabled) {
        printf("%s%s%s=== Simulating UWB Ranging Notification ===%s\n",
               ANSI_COLOR_BRIGHT_CYAN, ANSI_BOLD, ANSI_BG_BLUE, ANSI_RESET);
    } else {
        printf("=== Simulating UWB Ranging Notification ===\n");
    }

    unsigned char ranging_ntf_payload[] = {
        0x09, 0x00, 0x00, 0x00,
        0x2a, 0x00, 0x00, 0x00,
        0x00,
        0xe8, 0x03, 0x00, 0x00,
        0x01,
        0x00,
        0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x01,
        0x12, 0x34,
        0x00,
        0x00,
        0x64, 0x00,
        0x14, 0x00,
        0x08,
        0x05, 0x00,
        0x07,
        0x10, 0x00,
        0x06,
        0x03, 0x00,
        0x09,
        0x02,
        0xE0,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };

    size_t packet_len;
    unsigned char* notification_packet = create_uci_packet(NOTIFICATION, COMPLETE, RANGING_DATA, RANGE_DATA_NTF_OPCODE,
                                                         ranging_ntf_payload, sizeof(ranging_ntf_payload), &packet_len);
    if (notification_packet) {
        if (ui_color_enabled) {
            printf("%s%s→ Sending simulated ranging notification packet%s\n",
                   ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
        } else {
            printf("→ Sending simulated ranging notification packet\n");
        }

        parse_uci_packet(notification_packet, packet_len);
        free(notification_packet);
    }
    return 0;
}

int cmd_simulate_multi_target_ranging(int argc, char** argv) {
    (void)argc;
    (void)argv;

    unsigned char multi_ranging_ntf_payload[] = {
        0x0A, 0x00, 0x00, 0x00,
        0x2B, 0x00, 0x00, 0x00,
        0x00,
        0x20, 0x03, 0x00, 0x00,
        0x02,
        0x00,
        0x00,
        0xAA, 0xBB, 0xCC, 0xDD,
        0x00, 0x00, 0x00, 0x00,
        0x02,
        0x22, 0x11,
        0x00,
        0x01,
        0x32, 0x00,
        0x0F, 0x00,
        0x08,
        0x02, 0x00,
        0x06,
        0x0E, 0x00,
        0x05,
        0x03, 0x00,
        0x07,
        0x01,
        0xD8,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x34, 0x12,
        0x00,
        0x00,
        0x96, 0x00,
        0x20, 0x00,
        0x09,
        0x06, 0x00,
        0x05,
        0x12, 0x00,
        0x07,
        0x05, 0x00,
        0x08,
        0x03,
        0xC0,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };

    size_t packet_len;
    unsigned char* notification_packet = create_uci_packet(NOTIFICATION, COMPLETE, SESSION_CONTROL, SESSION_INFO_NTF,
                                                         multi_ranging_ntf_payload, sizeof(multi_ranging_ntf_payload), &packet_len);
    if (notification_packet) {
        parse_uci_packet(notification_packet, packet_len);
        free(notification_packet);
    }

    printf("=== Multi-Target Ranging Simulation Complete ===\n");
    return 0;
}

int cmd_analyze_packet(int argc, char** argv) {
    if (argc <= 1) {
        handle_analyze_command(0, NULL);
    } else {
        handle_analyze_command(argc - 1, &argv[1]);
    }
    return 0;
}

int cmd_help(int argc, char** argv) {
    (void)argc;
    (void)argv;
    cli_print_help();
    return 0;
}

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
    if (argc > 1) {
        // Non-interactive mode - process command line arguments directly
        cli_dispatch(argc - 1, &argv[1]);
        return 0;
    }
    
    if (uci_config_init() != 0) {
        printf("Warning: Failed to initialize configuration manager\n");
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
