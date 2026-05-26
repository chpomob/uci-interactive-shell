#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "../include/uci.h"
#include "../include/uci_cli.h"
#include "../include/uci_command_utils.h"
#include "../include/uci_functions.h"
#include "../include/uci_packet_utils.h"
#include "../include/uci_pdl.h"
#include "../include/uci_cmd_simulation_typed.h"
#include "../include/uci_ui.h"
#include "../include/uci_command_framework.h"

static void print_simulation_usage(void) {
    printf("Simulation helpers:\n");
    printf("  simulate_notification device_status <active|ready|error>\n");
    printf("  simulate_session_status <session_id> <state> <reason>\n");
    printf("    state  : init, deinit, active, idle\n");
    printf("    reason : mgmt_cmd\n");
}

static int parse_device_state(const char* value_str, unsigned char* state_out) {
    if (!value_str || !state_out) {
        print_simulation_usage();
        return -1;
    }

    if (strcasecmp(value_str, "active") == 0) {
        *state_out = DEVICE_STATE_ACTIVE;
    } else if (strcasecmp(value_str, "ready") == 0) {
        *state_out = DEVICE_STATE_READY;
    } else if (strcasecmp(value_str, "error") == 0) {
        *state_out = DEVICE_STATE_ERROR;
    } else {
        printf("Invalid value for device_status '%s'. Use active, ready, or error.\n", value_str);
        return -1;
    }
    return 0;
}

static int parse_sim_session_state(const char* state_str, unsigned char* state_out) {
    if (!state_str || !state_out) {
        print_simulation_usage();
        return -1;
    }

    if (strcasecmp(state_str, "init") == 0) {
        *state_out = SESSION_STATE_INIT;
    } else if (strcasecmp(state_str, "deinit") == 0) {
        *state_out = SESSION_STATE_DEINIT;
    } else if (strcasecmp(state_str, "active") == 0) {
        *state_out = SESSION_STATE_ACTIVE;
    } else if (strcasecmp(state_str, "idle") == 0) {
        *state_out = SESSION_STATE_IDLE;
    } else {
        printf("Invalid session state '%s'. Use init, deinit, active, or idle.\n", state_str);
        return -1;
    }
    return 0;
}

static int parse_sim_session_reason(const char* reason_str, unsigned char* reason_out) {
    if (!reason_str || !reason_out) {
        print_simulation_usage();
        return -1;
    }

    if (strcasecmp(reason_str, "mgmt_cmd") == 0 || strcasecmp(reason_str, "command") == 0) {
        *reason_out = STATE_CHANGE_WITH_SESSION_MANAGEMENT_COMMANDS;
    } else {
        printf("Invalid reason '%s'. Use mgmt_cmd.\n", reason_str);
        return -1;
    }
    return 0;
}

int handle_simulate_notification_command_typed(const uci_cmd_dispatch_context_t* dispatch_ctx,
                                               const char* cmd_name,
                                               int argc,
                                               char** argv,
                                               const uci_param_def_t* params,
                                               int param_count) {
    (void)cmd_name;
    (void)argc;
    (void)argv;
    (void)params;
    (void)param_count;

    const uci_cmd_parsed_param_t* type_param = uci_cmd_get_parsed_param(dispatch_ctx, 0);
    const uci_cmd_parsed_param_t* value_param = uci_cmd_get_parsed_param(dispatch_ctx, 1);
    if (!type_param || !type_param->present || !type_param->raw_value ||
        !value_param || !value_param->present || !value_param->raw_value) {
        print_simulation_usage();
        return -1;
    }

    const char* type_str = type_param->raw_value;
    const char* value_str = value_param->raw_value;

    if (strcmp(type_str, "device_status") == 0) {
        unsigned char device_state = 0;
        if (parse_device_state(value_str, &device_state) != 0) {
            return -1;
        }

        size_t packet_len = 0;
        unsigned char* notification_packet =
            create_uci_packet(NOTIFICATION, COMPLETE, CORE, CORE_DEVICE_STATUS_NTF,
                              &device_state, 1, &packet_len);
        if (notification_packet) {
            parse_uci_packet(notification_packet, packet_len);
            free(notification_packet);
        }
        return 0;
    }

    printf("Unknown notification type: %s\n", type_str);
    print_simulation_usage();
    return -1;
}

int handle_simulate_session_status_command_typed(const uci_cmd_dispatch_context_t* dispatch_ctx,
                                                 const char* cmd_name,
                                                 int argc,
                                                 char** argv,
                                                 const uci_param_def_t* params,
                                                 int param_count) {
    (void)cmd_name;
    (void)argc;
    (void)argv;
    (void)params;
    (void)param_count;

    const uci_cmd_parsed_param_t* session_param = uci_cmd_get_parsed_param(dispatch_ctx, 0);
    const uci_cmd_parsed_param_t* state_param = uci_cmd_get_parsed_param(dispatch_ctx, 1);
    const uci_cmd_parsed_param_t* reason_param = uci_cmd_get_parsed_param(dispatch_ctx, 2);
    if (!session_param || !session_param->present ||
        !state_param || !state_param->present || !state_param->raw_value ||
        !reason_param || !reason_param->present || !reason_param->raw_value) {
        print_simulation_usage();
        return -1;
    }

    uint32_t session_id = session_param->value.session_id;
    unsigned char session_state = 0;
    if (parse_sim_session_state(state_param->raw_value, &session_state) != 0) {
        return -1;
    }

    unsigned char reason_code = 0;
    if (parse_sim_session_reason(reason_param->raw_value, &reason_code) != 0) {
        return -1;
    }

    unsigned char notification_payload[6];
    write_u32_le(notification_payload, session_id);
    notification_payload[4] = session_state;
    notification_payload[5] = reason_code;

    size_t packet_len = 0;
    unsigned char* notification_packet =
        create_uci_packet(NOTIFICATION, COMPLETE, SESSION_CONFIG, SESSION_STATUS_NTF,
                          notification_payload, sizeof(notification_payload), &packet_len);
    if (notification_packet) {
        parse_uci_packet(notification_packet, packet_len);
        free(notification_packet);
    }
    return 0;
}

int handle_simulate_data_credit_command_typed(const uci_cmd_dispatch_context_t* dispatch_ctx,
                                              const char* cmd_name,
                                              int argc,
                                              char** argv,
                                              const uci_param_def_t* params,
                                              int param_count) {
    (void)dispatch_ctx;
    (void)cmd_name;
    (void)argc;
    (void)argv;
    (void)params;
    (void)param_count;

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

int handle_demo_session_flow_command_typed(const uci_cmd_dispatch_context_t* dispatch_ctx,
                                           const char* cmd_name,
                                           int argc,
                                           char** argv,
                                           const uci_param_def_t* params,
                                           int param_count) {
    (void)dispatch_ctx;
    (void)cmd_name;
    (void)argc;
    (void)argv;
    (void)params;
    (void)param_count;

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

    printf("=== Session Flow Demonstration Complete ===\n");
    return 0;
}

int handle_simulate_ranging_command_typed(const uci_cmd_dispatch_context_t* dispatch_ctx,
                                          const char* cmd_name,
                                          int argc,
                                          char** argv,
                                          const uci_param_def_t* params,
                                          int param_count) {
    (void)dispatch_ctx;
    (void)cmd_name;
    (void)argc;
    (void)argv;
    (void)params;
    (void)param_count;

    if (ui_color_enabled) {
        printf("%s%s=== Simulating UWB Ranging Notification ===%s\n",
               ANSI_COLOR_BRIGHT_CYAN, ANSI_BOLD, ANSI_RESET);
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
    unsigned char* notification_packet = create_uci_packet(NOTIFICATION, COMPLETE, SESSION_CONTROL, SESSION_INFO_NTF_OPCODE,
                                                         ranging_ntf_payload, sizeof(ranging_ntf_payload), &packet_len);
    if (notification_packet) {
        if (ui_color_enabled) {
            printf("%s%s→ Sending simulated ranging notification packet%s\n",
                   ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
        }
        parse_uci_packet(notification_packet, packet_len);
        free(notification_packet);
    }

    return 0;
}

int handle_simulate_multi_target_ranging_command_typed(const uci_cmd_dispatch_context_t* dispatch_ctx,
                                                       const char* cmd_name,
                                                       int argc,
                                                       char** argv,
                                                       const uci_param_def_t* params,
                                                       int param_count) {
    (void)dispatch_ctx;
    (void)cmd_name;
    (void)argc;
    (void)argv;
    (void)params;
    (void)param_count;

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
    unsigned char* notification_packet = create_uci_packet(NOTIFICATION, COMPLETE, SESSION_CONTROL, SESSION_INFO_NTF_OPCODE,
                                                         multi_ranging_ntf_payload, sizeof(multi_ranging_ntf_payload), &packet_len);
    if (notification_packet) {
        parse_uci_packet(notification_packet, packet_len);
        free(notification_packet);
    }

    printf("=== Multi-Target Ranging Simulation Complete ===\n");
    return 0;
}

int handle_simulate_qm_sdk_vendor_command_typed(const uci_cmd_dispatch_context_t* dispatch_ctx,
                                                const char* cmd_name,
                                                int argc,
                                                char** argv,
                                                const uci_param_def_t* params,
                                                int param_count) {
    (void)cmd_name;
    (void)argc;
    (void)argv;
    (void)params;
    (void)param_count;

    const uci_cmd_parsed_param_t* opcode_param = uci_cmd_get_parsed_param(dispatch_ctx, 0);
    if (!opcode_param || !opcode_param->present) {
        if (ui_color_enabled) {
            printf("%s%sUsage:%s simulate_qm_sdk_vendor_command <opcode> [params...]\n", 
                   ANSI_COLOR_BRIGHT_RED, ANSI_BOLD, ANSI_RESET);
            printf("  Send a QM SDK vendor-specific command through GID 0x0B (QORVO_EXT2)\n");
            printf("\n");
            printf("%s%sAvailable opcodes:%s\n", ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET);
            printf("  0x00: QORVO_TEST_DEBUG - Debug/test command\n");
            printf("  0x01: QORVO_TEST_TX_CW - Continuous wave transmission test\n");
            printf("  0x02: QORVO_TEST_PLLRF - PLL status test\n");
            printf("  0x03: QORVO_FIRA_RANGE_DIAGNOSTICS - Ranging diagnostics\n");
            printf("  0x07: QORVO_SESSION_GET - Get session information\n");
            printf("  0x08: QORVO_FIRA_SET_ANT_FLEX_CONFIG - Set antenna flexibility configuration\n");
            printf("  0x09: QORVO_FIRA_GET_ANT_FLEX_CONFIG - Get antenna flexibility configuration\n");
            printf("  0x0A: QORVO_CCC_SET_ANT_FLEX_CONFIG - Set CCC antenna flexibility configuration\n");
            printf("  0x0B: QORVO_CCC_GET_ANT_FLEX_CONFIG - Get CCC antenna flexibility configuration\n");
            printf("  0x22: QORVO_CORE_PSDU_DUMP - Dump PSDU data\n");
            printf("  0x23: QORVO_CORE_GET_MEM_STATS - Get memory statistics\n");
            printf("  0x24: QORVO_CORE_GET_POWER_STATS - Get power statistics\n");
            printf("  0x25: QORVO_CORE_GET_CPU_STATS - Get CPU statistics\n");
            printf("  0x26: QORVO_CORE_RESET_CPU_STATS - Reset CPU statistics\n");
            printf("  0x27: QORVO_CORE_GET_DEVICE_STATS - Get device statistics\n");
            printf("  0x30: QORVO_CORE_ERASE_CERTS - Erase certificates\n");
            printf("  0x31: QORVO_CORE_DEVICE_BOOT - Device boot notification\n");
            printf("  0x35: QORVO_CORE_TOGGLE_GPIO_TIMESYNC - Toggle GPIO timesync\n");
            printf("  0x36: QORVO_CORE_QUERY_GPIO_TIMESTAMP - Query GPIO timestamp\n");
        } else {
            printf("Usage: simulate_qm_sdk_vendor_command <opcode> [params...]\n");
            printf("  Send a QM SDK vendor-specific command through GID 0x0B (QORVO_EXT2)\n");
            printf("\n");
            printf("Available opcodes:\n");
            printf("  0x00: QORVO_TEST_DEBUG - Debug/test command\n");
            printf("  0x01: QORVO_TEST_TX_CW - Continuous wave transmission test\n");
            printf("  0x02: QORVO_TEST_PLLRF - PLL status test\n");
            printf("  0x03: QORVO_FIRA_RANGE_DIAGNOSTICS - Ranging diagnostics\n");
            printf("  0x07: QORVO_SESSION_GET - Get session information\n");
            printf("  0x08: QORVO_FIRA_SET_ANT_FLEX_CONFIG - Set antenna flexibility configuration\n");
            printf("  0x09: QORVO_FIRA_GET_ANT_FLEX_CONFIG - Get antenna flexibility configuration\n");
            printf("  0x0A: QORVO_CCC_SET_ANT_FLEX_CONFIG - Set CCC antenna flexibility configuration\n");
            printf("  0x0B: QORVO_CCC_GET_ANT_FLEX_CONFIG - Get CCC antenna flexibility configuration\n");
            printf("  0x22: QORVO_CORE_PSDU_DUMP - Dump PSDU data\n");
            printf("  0x23: QORVO_CORE_GET_MEM_STATS - Get memory statistics\n");
            printf("  0x24: QORVO_CORE_GET_POWER_STATS - Get power statistics\n");
            printf("  0x25: QORVO_CORE_GET_CPU_STATS - Get CPU statistics\n");
            printf("  0x26: QORVO_CORE_RESET_CPU_STATS - Reset CPU statistics\n");
            printf("  0x27: QORVO_CORE_GET_DEVICE_STATS - Get device statistics\n");
            printf("  0x30: QORVO_CORE_ERASE_CERTS - Erase certificates\n");
            printf("  0x31: QORVO_CORE_DEVICE_BOOT - Device boot notification\n");
            printf("  0x35: QORVO_CORE_TOGGLE_GPIO_TIMESYNC - Toggle GPIO timesync\n");
            printf("  0x36: QORVO_CORE_QUERY_GPIO_TIMESTAMP - Query GPIO timestamp\n");
        }
        return 1;
    }

    unsigned char opcode = opcode_param->value.u8;
    size_t packet_len;
    unsigned char* vendor_payload = NULL;
    size_t vendor_payload_len = 0;
    
    // Handle opcode-specific payload construction
    switch(opcode) {
        case QORVO_TEST_DEBUG:  // 0x00
            vendor_payload = malloc(1);
            if (vendor_payload) {
                vendor_payload[0] = 0x00;
                vendor_payload_len = 1;
            }
            break;
            
        case QORVO_TEST_TX_CW:  // 0x01
        case QORVO_TEST_PLLRF:  // 0x02
        case QORVO_FIRA_RANGE_DIAGNOSTICS:  // 0x03
        case QORVO_SESSION_GET:  // 0x07
        case QORVO_FIRA_SET_ANT_FLEX_CONFIG:  // 0x08
        case QORVO_FIRA_GET_ANT_FLEX_CONFIG:  // 0x09
        case QORVO_CCC_SET_ANT_FLEX_CONFIG:  // 0x0A
        case QORVO_CCC_GET_ANT_FLEX_CONFIG:  // 0x0B
        default:
            vendor_payload = malloc(1);
            if (vendor_payload) {
                vendor_payload[0] = 0x00;
                vendor_payload_len = 1;
            }
            break;
    }
    
    if (!vendor_payload) {
        fprintf(stderr, "Error: Failed to allocate memory for vendor payload\n");
        return -1;
    }
    
    unsigned char* vendor_packet = create_uci_packet(COMMAND, COMPLETE, QORVO_EXT2, opcode,
                                                   vendor_payload, vendor_payload_len, &packet_len);
    free(vendor_payload);
    
    if (vendor_packet) {
        if (ui_color_enabled) {
            printf("%s%s→ Sending QM SDK vendor command packet through GID 0x0B (QORVO_EXT2)%s\n",
                   ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
        } else {
            printf("→ Sending QM SDK vendor command packet through GID 0x0B (QORVO_EXT2)\n");
        }
        
        parse_uci_packet(vendor_packet, packet_len);
        free(vendor_packet);
    }
    
    return 0;
}

int handle_get_calibration_command_typed(const uci_cmd_dispatch_context_t* dispatch_ctx,
                                         const char* cmd_name,
                                         int argc,
                                         char** argv,
                                         const uci_param_def_t* params,
                                         int param_count) {
    (void)dispatch_ctx;
    (void)cmd_name;
    (void)params;
    (void)param_count;

    int num_params = argc - 1;
    if (num_params <= 0) {
        printf("Usage: get_calibration <param_name> [param_name ...]\n");
        printf("Example: get_calibration ant0.port ant_set0.nb_rx_ants\n");
        return -1;
    }
    if (num_params > 255) {
        printf("Error: too many parameters (max 255)\n");
        return -1;
    }

    unsigned char payload[1024];
    size_t offset = 0;
    payload[offset++] = (unsigned char)num_params;
    for (int i = 1; i < argc; i++) {
        size_t name_len = strlen(argv[i]);
        if (name_len == 0 || name_len > 255) {
            printf("Error: invalid param name length\n");
            return -1;
        }
        payload[offset++] = (unsigned char)name_len;
        memcpy(&payload[offset], argv[i], name_len);
        offset += name_len;
    }

    send_uci_command(COMMAND, 0, 0x0E, 0x2B, payload, (int)offset);
    return 0;
}
