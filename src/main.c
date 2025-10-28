#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> // For sleep
#include <ctype.h>  // For character handling

// Readline includes for tab completion
#include <readline/readline.h>
#include <readline/history.h>

// Define _GNU_SOURCE to get strdup declaration
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
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
#include "../include/uci_ui_packet_decoder.h"
#include "../include/uci_cmd_hardware.h"
#include "../include/uci_cmd_core.h"
#include "../include/uci_cmd_session.h"
#include "../include/uci_cmd_session_config.h"
#include "../include/uci_cmd_analysis.h"

#define MAX_PAYLOAD_LENGTH 255

// Global variables for hardware mode
int g_hardware_mode = 0;  // Flag to track if hardware mode is enabled
uci_hw_chardev_t g_uwb_chardev;  // Character device interface for UWB communication

int main() {
    char line[CLI_MAX_LINE_LENGTH];

    // Initialize configuration manager
    if (uci_config_init() != 0) {
        printf("Warning: Failed to initialize configuration manager\n");
    }

    // Initialize hardware command module
    uci_cmd_hardware_init(&g_hardware_mode, &g_uwb_chardev);

    // Print enhanced welcome message with colors
    ui_print_welcome_message();

    // Initialize readline for tab completion
    cli_initialize_readline();

    while (1) {
        char* input_line = readline("> ");
        if (input_line == NULL) {
            printf("\n");
            break;
        }

        // Remove trailing newline if present
        input_line[strcspn(input_line, "\r\n")] = 0;

        // Add non-empty lines to history
        if (strlen(input_line) > 0) {
            cli_history_add(input_line);
            add_history(input_line); // Also add to readline history
        }

        // Copy to our line buffer for compatibility with existing code
        strncpy(line, input_line, sizeof(line) - 1);
        line[sizeof(line) - 1] = '\0';
        free(input_line);

        if (strlen(line) == 0) {
            continue;
        }

        if (strcmp(line, "quit") == 0) {
            break;
        }

        // Handle history command
        if (strcmp(line, "history") == 0) {
            cli_history_print();
            continue;
        }

        // Handle alias commands
        char* command = strtok(line, " ");
        
        // Check if this command is an alias
        const char* real_command = cli_alias_lookup(command);
        if (real_command != NULL) {
            // Replace the original line with the aliased command
            char expanded_line[CLI_MAX_LINE_LENGTH];
            snprintf(expanded_line, sizeof(expanded_line), "%s%s", real_command, line + strlen(command));
            strcpy(line, expanded_line);
            command = strtok(line, " "); // Re-tokenize with the expanded command
        }

        if (strcmp(command, "complete") == 0) {
            char* full_input = strtok(NULL, "\n");
            cli_print_completion_suggestions(full_input);
        } else if (strcmp(command, "alias") == 0) {
            char* alias_name = strtok(NULL, " ");
            char* alias_cmd = strtok(NULL, "\n");
            
            if (!alias_name) {
                // No arguments provided, list all aliases
                cli_alias_print_all();
            } else if (!alias_cmd) {
                // Only alias name provided, show specific alias
                const char* real_cmd = cli_alias_lookup(alias_name);
                if (real_cmd) {
                    printf("%s -> %s\n", alias_name, real_cmd);
                } else {
                    printf("Alias '%s' not found\n", alias_name);
                }
            } else {
                // Both alias name and command provided, create new alias
                cli_alias_result_t result = cli_alias_add(alias_name, alias_cmd);
                if (result == CLI_ALIAS_FULL) {
                    ui_print_error("Maximum number of aliases reached");
                } else if (result == CLI_ALIAS_UPDATED) {
                    printf("Alias '%s' updated to '%s'\n", alias_name, alias_cmd);
                } else if (result == CLI_ALIAS_SUCCESS) {
                    printf("Alias '%s' added for '%s'\n", alias_name, alias_cmd);
                } else {
                    printf("Failed to add alias '%s'\n", alias_name);
                }
            }
        } else if (strcmp(command, "unalias") == 0) {
            char* alias_name = strtok(NULL, " ");
            if (!alias_name) {
                printf("Usage: unalias <alias_name>\n");
            } else {
                cli_alias_result_t result = cli_alias_remove(alias_name);
                if (result == CLI_ALIAS_SUCCESS) {
                    printf("Alias '%s' removed.\n", alias_name);
                } else {
                    printf("Alias '%s' not found.\n", alias_name);
                }
            }
        } else if (strcmp(command, "hw_init") == 0) {
            char* device_path = strtok(NULL, " ");
            handle_hw_init_command(device_path);
        } else if (strcmp(command, "hw_send") == 0 && g_hardware_mode) {
            char* mt_str = strtok(NULL, " ");
            char* pbf_str = strtok(NULL, " ");
            char* gid_str = strtok(NULL, " ");
            char* oid_str = strtok(NULL, " ");
            handle_hw_send_command(mt_str, pbf_str, gid_str, oid_str);
        } else if (strcmp(command, "mode_sim") == 0 || strcmp(command, "sim_mode") == 0) {
            handle_mode_sim_command();
        } else if (strcmp(command, "mode_hw") == 0 || strcmp(command, "hw_mode") == 0) {
            char* device_path = strtok(NULL, " ");
            handle_mode_hw_command(device_path);
        } else if (strcmp(command, "mode_info") == 0 || strcmp(command, "current_mode") == 0) {
            handle_mode_info_command();
        } else if (strcmp(command, "get_device_info") == 0 || strcmp(command, "device_info") == 0) {
            handle_get_device_info_command();
        } else if (strcmp(command, "device_reset") == 0) {
            handle_device_reset_command();
        } else if (strcmp(command, "set_power") == 0) {
            char* power_state = strtok(NULL, " ");
            handle_set_power_command(power_state);
        } else if (strcmp(command, "device_on") == 0) {
            handle_device_on_command();
        } else if (strcmp(command, "device_off") == 0) {
            handle_device_off_command();
        } else if (strcmp(command, "get_caps_info") == 0) {
            handle_get_caps_info_command();
        } else if (strcmp(command, "get_config") == 0) {
            char* config_name = strtok(NULL, " ");
            handle_get_config_command(config_name);
        } else if (strcmp(command, "get_device_state") == 0) {
            handle_get_device_state_command();
        } else if (strcmp(command, "set_device_active") == 0) {
            handle_set_device_active_command();
        } else if (strcmp(command, "set_device_ready") == 0) {
            handle_set_device_ready_command();
        } else if (strcmp(command, "set_config") == 0) {
            char* config_name = strtok(NULL, " ");
            char* value_str = strtok(NULL, " ");
            handle_set_config_command(config_name, value_str);
        } else if (strcmp(command, "device_suspend") == 0) {
            handle_device_suspend_command();
        } else if (strcmp(command, "query_timestamp") == 0) {
            handle_query_timestamp_command();
        } else if (strcmp(command, "session_init") == 0 || strcmp(command, "session_new") == 0) {
            char* session_id_str = strtok(NULL, " ");
            char* session_type_str = strtok(NULL, " ");
            handle_session_init_command(session_id_str, session_type_str);
        } else if (strcmp(command, "session_deinit") == 0 || strcmp(command, "session_close") == 0) {
            char* session_id_str = strtok(NULL, " ");
            handle_session_deinit_command(session_id_str);
        } else if (strcmp(command, "session_start") == 0 || strcmp(command, "start_ranging") == 0) {
            char* session_id_str = strtok(NULL, " ");
            handle_session_start_command(session_id_str);
        } else if (strcmp(command, "session_stop") == 0 || strcmp(command, "stop_ranging") == 0) {
            char* session_id_str = strtok(NULL, " ");
            handle_session_stop_command(session_id_str);
        } else if (strcmp(command, "session_send_data") == 0 || strcmp(command, "send_data") == 0) {
            char* session_id_str = strtok(NULL, " ");
            char* destination_str = strtok(NULL, " ");
            char* sequence_str = strtok(NULL, " ");
            char* payload_str = strtok(NULL, " ");
            handle_session_send_data_command(session_id_str, destination_str,
                                             sequence_str, payload_str);
        } else if (strcmp(command, "get_session_state") == 0 || strcmp(command, "session_status") == 0) {
            char* session_id_str = strtok(NULL, " ");
            handle_get_session_state_command(session_id_str);
        } else if (strcmp(command, "set_app_config") == 0) {
            char* session_id_str = strtok(NULL, " ");
            char* config_name = strtok(NULL, " ");
            char* value_str = strtok(NULL, " ");
            handle_set_app_config_command(session_id_str, config_name, value_str);
        } else if (strcmp(command, "get_app_config") == 0) {
            char* session_id_str = strtok(NULL, " ");
            char* config_name = strtok(NULL, " ");
            handle_get_app_config_command(session_id_str, config_name);
        } else if (strcmp(command, "simulate_notification") == 0) {
            char* type_str = strtok(NULL, " ");
            char* value_str = strtok(NULL, " ");
            if (!type_str || !value_str) {
                printf("Usage: simulate_notification <type> <value>\n");
                printf("  Example: simulate_notification device_status active\n");
                continue;
            }

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
                    continue;
                }
                unsigned char notification_packet[sizeof(struct uci_packet_header) + 1];
                struct uci_packet_header* ntf_header = (struct uci_packet_header*)notification_packet;
                set_header_values_safe(ntf_header, NOTIFICATION, COMPLETE, CORE, CORE_DEVICE_STATUS_NTF, 1);
                notification_packet[sizeof(struct uci_packet_header)] = device_state;
                parse_uci_packet(notification_packet, sizeof(struct uci_packet_header) + 1);
            } else {
                printf("Unknown notification type: %s\n", type_str);
                continue;
            }
        } else if (strcmp(command, "simulate_session_status") == 0) {
            char* session_id_str = strtok(NULL, " ");
            char* state_str = strtok(NULL, " ");
            char* reason_str = strtok(NULL, " ");
            if (!session_id_str || !state_str || !reason_str) {
                printf("Usage: simulate_session_status <session_id> <state> <reason>\n");
                printf("  Example: simulate_session_status 1 active mgmt_cmd\n");
                continue;
            }

            unsigned int session_id = (unsigned int)strtoul(session_id_str, NULL, 10);
            unsigned char session_state;
            unsigned char reason_code;

            if (strcmp(state_str, "init") == 0) session_state = SESSION_STATE_INIT;
            else if (strcmp(state_str, "deinit") == 0) session_state = SESSION_STATE_DEINIT;
            else if (strcmp(state_str, "active") == 0) session_state = SESSION_STATE_ACTIVE;
            else if (strcmp(state_str, "idle") == 0) session_state = SESSION_STATE_IDLE;
            else { printf("Invalid state\n"); continue; }

            if (strcmp(reason_str, "mgmt_cmd") == 0) reason_code = STATE_CHANGE_WITH_SESSION_MANAGEMENT_COMMANDS;
            else { printf("Invalid reason\n"); continue; }

            unsigned char notification_packet[sizeof(struct uci_packet_header) + 6];
            struct uci_packet_header* ntf_header = (struct uci_packet_header*)notification_packet;
            set_header_values_safe(ntf_header, NOTIFICATION, COMPLETE, SESSION_CONFIG, SESSION_STATUS_NTF, 6);
            // Store session_id in little-endian format to match UCI spec and read_u32_le parsing
            notification_packet[sizeof(struct uci_packet_header)] = session_id & 0xFF;           // LSB first
            notification_packet[sizeof(struct uci_packet_header) + 1] = (session_id >> 8) & 0xFF;
            notification_packet[sizeof(struct uci_packet_header) + 2] = (session_id >> 16) & 0xFF;
            notification_packet[sizeof(struct uci_packet_header) + 3] = (session_id >> 24) & 0xFF;   // MSB last
            notification_packet[sizeof(struct uci_packet_header) + 3] = session_id & 0xFF;
            notification_packet[sizeof(struct uci_packet_header) + 4] = session_state;
            notification_packet[sizeof(struct uci_packet_header) + 5] = reason_code;
            parse_uci_packet(notification_packet, sizeof(struct uci_packet_header) + 6);
        } else if (strcmp(command, "simulate_data_credit") == 0) {
            // Simulate a data credit notification
            unsigned char notification_packet[sizeof(struct uci_packet_header) + 5];
            struct uci_packet_header* ntf_header = (struct uci_packet_header*)notification_packet;
            set_header_values_safe(ntf_header, NOTIFICATION, COMPLETE, SESSION_CONTROL, SESSION_DATA_CREDIT_NTF, 5);
            notification_packet[sizeof(struct uci_packet_header)] = 0x01;
            notification_packet[sizeof(struct uci_packet_header) + 1] = 0x02;
            notification_packet[sizeof(struct uci_packet_header) + 2] = 0x03;
            notification_packet[sizeof(struct uci_packet_header) + 3] = 0x04;
            notification_packet[sizeof(struct uci_packet_header) + 4] = 0x01; // credit available
            parse_uci_packet(notification_packet, sizeof(struct uci_packet_header) + 5);
        } else if (strcmp(command, "session_update_multicast_list") == 0 || strcmp(command, "update_multicast_list") == 0) {
            char* session_id_str = strtok(NULL, " ");
            char* action_str = strtok(NULL, " ");
            char* short_address_str = strtok(NULL, " ");
            handle_update_multicast_list_command(session_id_str, action_str, short_address_str);
        } else if (strcmp(command, "session_set_hybrid_controller_config") == 0) {
            char* session_id_str = strtok(NULL, " ");
            char* config_data_str = strtok(NULL, " ");
            handle_session_set_hybrid_controller_config_command(session_id_str, 
                                                               config_data_str ? (unsigned char*)config_data_str : NULL, 
                                                               config_data_str ? strlen(config_data_str) : 0);
        } else if (strcmp(command, "session_set_hybrid_controlee_config") == 0) {
            char* session_id_str = strtok(NULL, " ");
            char* config_data_str = strtok(NULL, " ");
            handle_session_set_hybrid_controlee_config_command(session_id_str, 
                                                             config_data_str ? (unsigned char*)config_data_str : NULL, 
                                                             config_data_str ? strlen(config_data_str) : 0);
        } else if (strcmp(command, "session_query_data_size_in_ranging") == 0) {
            char* session_id_str = strtok(NULL, " ");
            handle_session_query_data_size_in_ranging_command(session_id_str);
        } else if (strcmp(command, "demo_session_flow") == 0) {
            printf("=== UCI Session Flow Demonstration ===\n");
            
            // 1. Initialize session
            printf("\n1. Initializing session...\n");
            unsigned char init_payload[] = {0x01, 0x02, 0x03, 0x04, FIRA_RANGING_SESSION};
            send_uci_command(COMMAND, 0, SESSION_CONFIG, SESSION_INIT, init_payload, sizeof(init_payload));
            
            // 2. Simulate session status notification after init
            printf("\n2. Session initialization complete - received status notification:\n");
            unsigned char ntf_packet1[sizeof(struct uci_packet_header) + 6];
            struct uci_packet_header* ntf_header1 = (struct uci_packet_header*)ntf_packet1;
            set_header_values_safe(ntf_header1, NOTIFICATION, COMPLETE, SESSION_CONFIG, SESSION_STATUS_NTF, 6);
            ntf_packet1[sizeof(struct uci_packet_header)] = 0x01; // session token
            ntf_packet1[sizeof(struct uci_packet_header) + 1] = 0x02;
            ntf_packet1[sizeof(struct uci_packet_header) + 2] = 0x03;
            ntf_packet1[sizeof(struct uci_packet_header) + 3] = 0x04;
            ntf_packet1[sizeof(struct uci_packet_header) + 4] = SESSION_STATE_INIT; // state
            ntf_packet1[sizeof(struct uci_packet_header) + 5] = STATE_CHANGE_WITH_SESSION_MANAGEMENT_COMMANDS;
            parse_uci_packet(ntf_packet1, sizeof(struct uci_packet_header) + 6);
            
            // 3. Start session
            printf("\n3. Starting session...\n");
            unsigned char start_payload[] = {0x01, 0x02, 0x03, 0x04};
            send_uci_command(COMMAND, 0, SESSION_CONTROL, SESSION_START, start_payload, sizeof(start_payload));
            
            // 4. Simulate session status notification after start
            printf("\n4. Session started - received status notification:\n");
            unsigned char ntf_packet2[sizeof(struct uci_packet_header) + 6];
            struct uci_packet_header* ntf_header2 = (struct uci_packet_header*)ntf_packet2;
            set_header_values_safe(ntf_header2, NOTIFICATION, COMPLETE, SESSION_CONFIG, SESSION_STATUS_NTF, 6);
            ntf_packet2[sizeof(struct uci_packet_header)] = 0x01; // session token
            ntf_packet2[sizeof(struct uci_packet_header) + 1] = 0x02;
            ntf_packet2[sizeof(struct uci_packet_header) + 2] = 0x03;
            ntf_packet2[sizeof(struct uci_packet_header) + 3] = 0x04;
            ntf_packet2[sizeof(struct uci_packet_header) + 4] = SESSION_STATE_ACTIVE; // state
            ntf_packet2[sizeof(struct uci_packet_header) + 5] = STATE_CHANGE_WITH_SESSION_MANAGEMENT_COMMANDS;
            parse_uci_packet(ntf_packet2, sizeof(struct uci_packet_header) + 6);
            
            // 5. Simulate data credit notification
            printf("\n5. Data credit available - received notification:\n");
            unsigned char ntf_packet3[sizeof(struct uci_packet_header) + 5];
            struct uci_packet_header* ntf_header3 = (struct uci_packet_header*)ntf_packet3;
            set_header_values_safe(ntf_header3, NOTIFICATION, COMPLETE, SESSION_CONTROL, SESSION_DATA_CREDIT_NTF, 5);
            ntf_packet3[sizeof(struct uci_packet_header)] = 0x01; // session token
            ntf_packet3[sizeof(struct uci_packet_header) + 1] = 0x02;
            ntf_packet3[sizeof(struct uci_packet_header) + 2] = 0x03;
            ntf_packet3[sizeof(struct uci_packet_header) + 3] = 0x04;
            ntf_packet3[sizeof(struct uci_packet_header) + 4] = 0x01; // credit available
            parse_uci_packet(ntf_packet3, sizeof(struct uci_packet_header) + 5);
            
            printf("\n=== Session Flow Demonstration Complete ===\n");
        } else if (strcmp(command, "simulate_ranging") == 0) {
            if (ui_color_enabled) {
                printf("%s%s%s=== Simulating UWB Ranging Notification ===%s\n", 
                       ANSI_COLOR_BRIGHT_CYAN, ANSI_BOLD, ANSI_BG_BLUE, ANSI_RESET);
            } else {
                printf("=== Simulating UWB Ranging Notification ===\n");
            }
            
            // Create a simulated ranging data notification following Qorvo UCI specification
            // Using RANGING_DATA group with RANGE_DATA_NTF_OPCODE as per real logs
            unsigned char ranging_ntf_payload[] = {
                // RangingData structure:
                0x09, 0x00, 0x00, 0x00,  // Sequence Counter: 9
                0x2a, 0x00, 0x00, 0x00,  // Session Handle: 0x0000002a (42)
                0x00,                    // RFU
                0xe8, 0x03, 0x00, 0x00,  // Ranging interval: 1000ms (0x000003e8) in units of 1200 RSTU
                0x01,                    // Ranging measurement type: TWR (0x01)
                0x00,                    // RFU
                0x00,                    // MAC addressing mode: 0=short address (2 bytes)
                0x00, 0x00, 0x00, 0x00,  // Primary Session ID: 0x00000000
                0x00, 0x00, 0x00, 0x00,  // RFU
                0x01,                    // Number of measurements: 1
                
                // First measurement - TWR type with short address (20 bytes total for short addresses)
                0x12, 0x34,              // MAC Address: 0x3412 (little endian for 0x1234)
                0x00,                    // Status: OK
                0x00,                    // NLOS byte
                0x64, 0x00,              // Distance: 100 cm (0x0064 little endian)
                0x14, 0x00,              // AoA Azimuth: 20 degrees
                0x08,                    // AoA Azimuth FoM: 8
                0x05, 0x00,              // AoA Elevation: 5 degrees 
                0x07,                    // AoA Elevation FoM: 7
                0x10, 0x00,              // Destination AoA Azimuth: 16 degrees
                0x06,                    // Destination AoA Azimuth FoM: 6
                0x03, 0x00,              // Destination AoA Elevation: 3 degrees
                0x09,                    // Destination AoA Elevation FoM: 9
                0x02,                    // Slot Index: 2
                0xE0                     // RSSI: -32 dBm (0xE0 as signed int8_t)
                // 11 more RFU bytes for short address measurements (for TWR)
                , 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
            };
            
            unsigned char notification_packet[sizeof(struct uci_packet_header) + sizeof(ranging_ntf_payload)];
            struct uci_packet_header* ntf_header = (struct uci_packet_header*)notification_packet;
            set_header_values_safe(ntf_header, NOTIFICATION, COMPLETE, RANGING_DATA, RANGE_DATA_NTF_OPCODE, sizeof(ranging_ntf_payload));
            memcpy(notification_packet + sizeof(struct uci_packet_header), ranging_ntf_payload, sizeof(ranging_ntf_payload));
            
            if (ui_color_enabled) {
                printf("%s%s→ Sending simulated ranging notification packet%s\n", 
                       ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
            } else {
                printf("→ Sending simulated ranging notification packet\n");
            }
            
            // Use UI-enhanced decoder directly for better visualization
            if (ui_color_enabled) {
                printf("%s%sReceived UCI packet:%s\n", ANSI_COLOR_BRIGHT_CYAN, ANSI_BOLD, ANSI_RESET);
                printf("  %sMT:%s 0x%01X\n", ANSI_COLOR_BRIGHT_YELLOW, ANSI_RESET, NOTIFICATION);
                printf("  %sPBF:%s 0x%01X\n", ANSI_COLOR_BRIGHT_YELLOW, ANSI_RESET, COMPLETE);
                printf("  %sGID:%s 0x%01X\n", ANSI_COLOR_BRIGHT_YELLOW, ANSI_RESET, RANGING_DATA);
                printf("  %sOpcode:%s 0x%02X\n", ANSI_COLOR_BRIGHT_YELLOW, ANSI_RESET, RANGE_DATA_NTF_OPCODE);
                printf("  %sPayload Length:%s %zu\n", ANSI_COLOR_BRIGHT_YELLOW, ANSI_RESET, sizeof(ranging_ntf_payload));
                printf("  %sPayload:%s ", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET);
                for (size_t i = 0; i < sizeof(ranging_ntf_payload) && i < 32; i++) {
                    printf("%02X ", ranging_ntf_payload[i]);
                }
                if (sizeof(ranging_ntf_payload) > 32) {
                    printf("... (and %zu more bytes)", sizeof(ranging_ntf_payload) - 32);
                }
                printf("\n");
                
                // Call the UI-enhanced decoder directly
                ui_decode_range_data_ntf(ranging_ntf_payload, sizeof(ranging_ntf_payload));
            } else {
                parse_uci_packet(notification_packet, sizeof(struct uci_packet_header) + sizeof(ranging_ntf_payload));
            }
            
            if (ui_color_enabled) {
                printf("%s%s%s=== Ranging Simulation Complete ===%s\n", 
                       ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_BG_GREEN, ANSI_RESET);
            } else {
                printf("=== Ranging Simulation Complete ===\n");
            }
        } else if (strcmp(command, "simulate_multi_target_ranging") == 0) {
            printf("=== Simulating Multi-Target UWB Ranging Notification ===\n");
            
            // Create a simulated two-way ranging measurement for multiple devices
            unsigned char multi_ranging_ntf_payload[] = {
                // Header fields (24 bytes total)
                0x00, 0x00, 0x00, 0x02,  // Sequence number: 2
                0x01, 0x02, 0x03, 0x04,  // Session token: 0x01020304
                0x01,                    // RCR indicator
                0x00, 0x00, 0x00, 0x64,  // Current ranging interval: 100ms
                0x01,                    // Ranging measurement type: TWO_WAY (0x01)
                0x00,                    // Reserved
                0x00,                    // MAC address indicator: SHORT_ADDRESS (0x00)
                0x00, 0x00, 0x00, 0x00,  // HUS primary session ID: 0x00000000
                
                // Two-Way measurement data
                0x03,                    // Number of measurements: 3
                
                // First measurement
                0x12, 0x34,              // MAC Address: 0x1234
                0x00,                    // Status: OK
                0x00,                    // NLOS: NO
                0x00, 0x64,              // Distance: 100 cm (1 meter)
                0x00, 0x14,              // AoA Azimuth: 20 degrees
                0x08,                    // AoA Azimuth FoM: 8
                0x00, 0x05,              // AoA Elevation: 5 degrees
                0x07,                    // AoA Elevation FoM: 7
                0x00, 0x10,              // Destination AoA Azimuth: 16 degrees
                0x06,                    // Destination AoA Azimuth FoM: 6
                0x00, 0x03,              // Destination AoA Elevation: 3 degrees
                0x09,                    // Destination AoA Elevation FoM: 9
                0x02,                    // Slot Index: 2
                0xE0,                    // RSSI: -32 dBm
                
                // Second measurement
                0x56, 0x78,              // MAC Address: 0x5678
                0x00,                    // Status: OK
                0x01,                    // NLOS: YES
                0x01, 0x90,              // Distance: 400 cm (4 meters)
                0x00, 0xA0,              // AoA Azimuth: 160 degrees
                0x04,                    // AoA Azimuth FoM: 4 (low confidence)
                0x00, 0x0A,              // AoA Elevation: 10 degrees
                0x05,                    // AoA Elevation FoM: 5 (medium-low confidence)
                0x00, 0x95,              // Destination AoA Azimuth: 149 degrees
                0x03,                    // Destination AoA Azimuth FoM: 3 (low confidence)
                0x00, 0x07,              // Destination AoA Elevation: 7 degrees
                0x04,                    // Destination AoA Elevation FoM: 4 (low confidence)
                0x05,                    // Slot Index: 5
                0xD0,                    // RSSI: -48 dBm (weaker signal)
                
                // Third measurement
                0x9A, 0xBC,              // MAC Address: 0x9ABC
                0x00,                    // Status: OK
                0x00,                    // NLOS: NO
                0x02, 0x58,              // Distance: 600 cm (6 meters)
                0x01, 0x04,              // AoA Azimuth: 260 degrees
                0x09,                    // AoA Azimuth FoM: 9 (very high confidence)
                0x00, 0xF6,              // AoA Elevation: -10 degrees
                0x08,                    // AoA Elevation FoM: 8 (high confidence)
                0x00, 0xFA,              // Destination AoA Azimuth: 250 degrees
                0x07,                    // Destination AoA Azimuth FoM: 7 (high confidence)
                0x00, 0x05,              // Destination AoA Elevation: 5 degrees
                0x06,                    // Destination AoA Elevation FoM: 6 (medium-high confidence)
                0x08,                    // Slot Index: 8
                0xC0                     // RSSI: -64 dBm (weak signal)
            };
            
            unsigned char notification_packet[sizeof(struct uci_packet_header) + sizeof(multi_ranging_ntf_payload)];
            struct uci_packet_header* ntf_header = (struct uci_packet_header*)notification_packet;
            set_header_values_safe(ntf_header, NOTIFICATION, COMPLETE, SESSION_CONTROL, SESSION_INFO_NTF, sizeof(multi_ranging_ntf_payload));
            memcpy(notification_packet + sizeof(struct uci_packet_header), multi_ranging_ntf_payload, sizeof(multi_ranging_ntf_payload));
            parse_uci_packet(notification_packet, sizeof(struct uci_packet_header) + sizeof(multi_ranging_ntf_payload));
            
            printf("=== Multi-Target Ranging Simulation Complete ===\n");
        } else if (strcmp(command, "analyze_packet") == 0) {
            // Collect all remaining arguments
            char* args[256];
            int arg_count = 0;
            
            // Parse all remaining arguments
            char* arg = strtok(NULL, " ");
            while (arg && arg_count < 255) {
                args[arg_count++] = arg;
                arg = strtok(NULL, " ");
            }
            args[arg_count] = NULL;
            
            // Call enhanced analysis command handler to leverage existing functionality
            handle_analyze_command(arg_count, args);
        } else if (strcmp(command, "help") == 0) {
            // Print comprehensive help information with UI enhancements
            ui_print_header("UCI Interactive Shell - Enhanced UI");
            
            if (ui_color_enabled) {
                printf("%s%sAvailable Commands:%s\n", ANSI_COLOR_BRIGHT_CYAN, ANSI_BOLD, ANSI_RESET);
                printf("%s------------------%s\n", ANSI_COLOR_BRIGHT_CYAN, ANSI_RESET);
            } else {
                printf("Available Commands:\n");
                printf("------------------\n");
            }
            if (ui_color_enabled) {
                printf("%s%sGeneral Commands:%s\n", ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET);
            } else {
                printf("General Commands:\n");
            }
            if (ui_color_enabled) {
                printf("  %s%s%s                      - %s%sShow this help information%s\n", 
                       ANSI_BOLD, ANSI_COLOR_BRIGHT_GREEN, "help", ANSI_COLOR_WHITE, ANSI_RESET, ANSI_RESET);
                printf("  %s%s%s                      - %s%sExit the shell%s\n", 
                       ANSI_BOLD, ANSI_COLOR_BRIGHT_GREEN, "quit", ANSI_COLOR_WHITE, ANSI_RESET, ANSI_RESET);
                printf("  %s%s%s                   - %s%sShow command history%s\n", 
                       ANSI_BOLD, ANSI_COLOR_BRIGHT_GREEN, "history", ANSI_COLOR_WHITE, ANSI_RESET, ANSI_RESET);
                printf("  %s%s%s <prefix>         - %s%sAutocomplete a command or parameter%s\n", 
                       ANSI_BOLD, ANSI_COLOR_BRIGHT_GREEN, "complete", ANSI_COLOR_WHITE, ANSI_RESET, ANSI_RESET);
                printf("  %s%s%s <name> [command]    - %s%sCreate or list command aliases%s\n", 
                       ANSI_BOLD, ANSI_COLOR_BRIGHT_GREEN, "alias", ANSI_COLOR_WHITE, ANSI_RESET, ANSI_RESET);
                printf("  %s%s%s <name>            - %s%sRemove an alias%s\n", 
                       ANSI_BOLD, ANSI_COLOR_BRIGHT_GREEN, "unalias", ANSI_COLOR_WHITE, ANSI_RESET, ANSI_RESET);
            } else {
                printf("  help                      - Show this help information\n");
                printf("  quit                      - Exit the shell\n");
                printf("  history                   - Show command history\n");
                printf("  complete <prefix>         - Autocomplete a command or parameter\n");
                printf("  alias <name> [command]    - Create or list command aliases\n");
                printf("  unalias <name>            - Remove an alias\n");
            }
            printf("\n");
            
            if (ui_color_enabled) {
                printf("%s%sDevice Management Commands:%s\n", ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET);
            } else {
                printf("Device Management Commands:\n");
            }
            if (ui_color_enabled) {
                printf("  %s%s%s, %s%s     - %s%sGet device information%s\n", 
                       ANSI_BOLD, ANSI_COLOR_BRIGHT_GREEN, "get_device_info", ANSI_COLOR_BRIGHT_GREEN, "device_info", ANSI_COLOR_WHITE, ANSI_RESET, ANSI_RESET);
                printf("  %s%s%s                     - %s%sReset the device%s\n", 
                       ANSI_BOLD, ANSI_COLOR_BRIGHT_GREEN, "device_reset", ANSI_COLOR_WHITE, ANSI_RESET, ANSI_RESET);
                printf("  %s%s%s                    - %s%sGet device capabilities information%s\n", 
                       ANSI_BOLD, ANSI_COLOR_BRIGHT_GREEN, "get_caps_info", ANSI_COLOR_WHITE, ANSI_RESET, ANSI_RESET);
                printf("  %s%s%s <param> <value>       - %s%sSet device configuration%s\n", 
                       ANSI_BOLD, ANSI_COLOR_BRIGHT_GREEN, "set_config", ANSI_COLOR_WHITE, ANSI_RESET, ANSI_RESET);
                printf("  %s%s%s <param>               - %s%sGet device configuration%s\n", 
                       ANSI_BOLD, ANSI_COLOR_BRIGHT_GREEN, "get_config", ANSI_COLOR_WHITE, ANSI_RESET, ANSI_RESET);
                printf("  %s%s%s                 - %s%sGet current device state%s\n", 
                       ANSI_BOLD, ANSI_COLOR_BRIGHT_GREEN, "get_device_state", ANSI_COLOR_WHITE, ANSI_RESET, ANSI_RESET);
                printf("  %s%s%s                - %s%sSet device to ACTIVE state%s\n", 
                       ANSI_BOLD, ANSI_COLOR_BRIGHT_GREEN, "set_device_active", ANSI_COLOR_WHITE, ANSI_RESET, ANSI_RESET);
                printf("  %s%s%s                 - %s%sSet device to READY state%s\n", 
                       ANSI_BOLD, ANSI_COLOR_BRIGHT_GREEN, "set_device_ready", ANSI_COLOR_WHITE, ANSI_RESET, ANSI_RESET);
                printf("  %s%s%s                   - %s%sSuspend the device%s\n", 
                       ANSI_BOLD, ANSI_COLOR_BRIGHT_GREEN, "device_suspend", ANSI_COLOR_WHITE, ANSI_RESET, ANSI_RESET);
                printf("  %s%s%s <state>                - %s%sSet device power state%s\n", 
                       ANSI_BOLD, ANSI_COLOR_BRIGHT_GREEN, "set_power", ANSI_COLOR_WHITE, ANSI_RESET, ANSI_RESET);
                printf("  %s%s%s                        - %s%sTurn device on (ACTIVE state)%s\n", 
                       ANSI_BOLD, ANSI_COLOR_BRIGHT_GREEN, "device_on", ANSI_COLOR_WHITE, ANSI_RESET, ANSI_RESET);
                printf("  %s%s%s                       - %s%sTurn device off (READY state)%s\n", 
                       ANSI_BOLD, ANSI_COLOR_BRIGHT_GREEN, "device_off", ANSI_COLOR_WHITE, ANSI_RESET, ANSI_RESET);
            } else {
                printf("  get_device_info, device_info     - Get device information\n");
                printf("  device_reset                     - Reset the device\n");
                printf("  get_caps_info                    - Get device capabilities information\n");
                printf("  set_config <param> <value>       - Set device configuration\n");
                printf("  get_config <param>               - Get device configuration\n");
                printf("  get_device_state                 - Get current device state\n");
                printf("  set_device_active                - Set device to ACTIVE state\n");
                printf("  set_device_ready                 - Set device to READY state\n");
                printf("  device_suspend                   - Suspend the device\n");
                printf("  query_timestamp                  - Query UWBS timestamp\n");
                printf("  mode_sim, sim_mode               - Switch to simulation mode\n");
                printf("  mode_hw <device_path>            - Switch to hardware mode\n");
                printf("  mode_info, current_mode          - Show current mode information\n");
                printf("  set_power <state>                - Set device power state\n");
                printf("  device_on                        - Turn device on (ACTIVE state)\n");
                printf("  device_off                       - Turn device off (READY state)\n");
            }
            printf("\n");
            
            printf("Session Management Commands:\n");
            if (ui_color_enabled) {
                printf("%s%sSession Management Commands:%s\n", ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET);
            } else {
                printf("Session Management Commands:\n");
            }
            if (ui_color_enabled) {
                printf("  %s%s%s <id> <type>         - %s%sInitialize a ranging session%s\n", 
                       ANSI_BOLD, ANSI_COLOR_BRIGHT_GREEN, "session_init", ANSI_COLOR_WHITE, ANSI_RESET, ANSI_RESET);
                printf("  %s%s%s <id> <type>          - %s%sAlias for session_init%s\n", 
                       ANSI_BOLD, ANSI_COLOR_BRIGHT_GREEN, "session_new", ANSI_COLOR_WHITE, ANSI_RESET, ANSI_RESET);
                printf("  %s%s%s <id>              - %s%sDeinitialize a ranging session%s\n", 
                       ANSI_BOLD, ANSI_COLOR_BRIGHT_GREEN, "session_deinit", ANSI_COLOR_WHITE, ANSI_RESET, ANSI_RESET);
                printf("  %s%s%s <id>               - %s%sAlias for session_deinit%s\n", 
                       ANSI_BOLD, ANSI_COLOR_BRIGHT_GREEN, "session_close", ANSI_COLOR_WHITE, ANSI_RESET, ANSI_RESET);
                printf("  %s%s%s <id>               - %s%sStart a ranging session%s\n", 
                       ANSI_BOLD, ANSI_COLOR_BRIGHT_GREEN, "session_start", ANSI_COLOR_WHITE, ANSI_RESET, ANSI_RESET);
                printf("  %s%s%s <id>               - %s%sAlias for session_start%s\n", 
                       ANSI_BOLD, ANSI_COLOR_BRIGHT_GREEN, "start_ranging", ANSI_COLOR_WHITE, ANSI_RESET, ANSI_RESET);
                printf("  %s%s%s <id>                - %s%sStop a ranging session%s\n", 
                       ANSI_BOLD, ANSI_COLOR_BRIGHT_GREEN, "session_stop", ANSI_COLOR_WHITE, ANSI_RESET, ANSI_RESET);
                printf("  %s%s%s <id>                - %s%sAlias for session_stop%s\n", 
                       ANSI_BOLD, ANSI_COLOR_BRIGHT_GREEN, "stop_ranging", ANSI_COLOR_WHITE, ANSI_RESET, ANSI_RESET);
                printf("  %s%s%s <id>           - %s%sGet session state%s\n", 
                       ANSI_BOLD, ANSI_COLOR_BRIGHT_GREEN, "get_session_state", ANSI_COLOR_WHITE, ANSI_RESET, ANSI_RESET);
                printf("  %s%s%s <id>              - %s%sAlias for get_session_state%s\n", 
                       ANSI_BOLD, ANSI_COLOR_BRIGHT_GREEN, "session_status", ANSI_COLOR_WHITE, ANSI_RESET, ANSI_RESET);
                printf("  %s%s%s                - %s%sGet number of active sessions%s\n", 
                       ANSI_BOLD, ANSI_COLOR_BRIGHT_GREEN, "get_session_count", ANSI_COLOR_WHITE, ANSI_RESET, ANSI_RESET);
                printf("  %s%s%s <id>              - %s%sQuery maximum data size in ranging%s\n", 
                       ANSI_BOLD, ANSI_COLOR_BRIGHT_GREEN, "session_query_data_size_in_ranging", ANSI_COLOR_WHITE, ANSI_RESET, ANSI_RESET);
                printf("  %s%s%s <id> [config_data] - %s%sSet hybrid controller configuration%s\n", 
                       ANSI_BOLD, ANSI_COLOR_BRIGHT_GREEN, "session_set_hybrid_controller_config", ANSI_COLOR_WHITE, ANSI_RESET, ANSI_RESET);
                printf("  %s%s%s <id> [config_data] - %s%sSet hybrid controlee configuration%s\n", 
                       ANSI_BOLD, ANSI_COLOR_BRIGHT_GREEN, "session_set_hybrid_controlee_config", ANSI_COLOR_WHITE, ANSI_RESET, ANSI_RESET);
                printf("  %s%s%s <id> <dest> <seq> <payload> - %s%sSend DATA_MESSAGE_SND payload%s\n",
                       ANSI_BOLD, ANSI_COLOR_BRIGHT_GREEN, "session_send_data", ANSI_COLOR_WHITE, ANSI_RESET, ANSI_RESET);
                printf("  %s%s%s <id> <dest> <seq> <payload> - %s%sAlias for session_send_data%s\n",
                       ANSI_BOLD, ANSI_COLOR_BRIGHT_GREEN, "send_data", ANSI_COLOR_WHITE, ANSI_RESET, ANSI_RESET);
            } else {
                printf("  session_init <id> <type>         - Initialize a ranging session\n");
                printf("    Supported session types:\n");
                printf("      - fira_ranging / ranging\n");
                printf("      - fira_ranging_and_data / ranging_and_data\n");
                printf("      - fira_data_transfer / data_transfer\n");
                printf("      - fira_ranging_only / ranging_only\n");
                printf("      - fira_in_band_data / in_band_data\n");
                printf("      - fira_ranging_with_data / ranging_with_data\n");
                printf("  session_new <id> <type>          - Alias for session_init\n");
                printf("  session_deinit <id>              - Deinitialize a ranging session\n");
                printf("  session_close <id>               - Alias for session_deinit\n");
                printf("  session_start <id>               - Start a ranging session\n");
                printf("  start_ranging <id>               - Alias for session_start\n");
                printf("  session_stop <id>                - Stop a ranging session\n");
                printf("  stop_ranging <id>                - Alias for session_stop\n");
                printf("  get_session_state <id>           - Get session state\n");
                printf("  session_status <id>              - Alias for get_session_state\n");
                printf("  session_send_data <id> <dest> <seq> <payload> - Send DATA_MESSAGE_SND payload\n");
                printf("  send_data <id> <dest> <seq> <payload>        - Alias for session_send_data\n");
                printf("  get_session_count                - Get number of active sessions\n");
                printf("  session_query_data_size_in_ranging <id>              - Query maximum data size in ranging\n");
                printf("  session_set_hybrid_controller_config <id> [config_data] - Set hybrid controller configuration\n");
                printf("  session_set_hybrid_controlee_config <id> [config_data] - Set hybrid controlee configuration\n");
            }
            printf("\n");
            
            if (ui_color_enabled) {
                printf("%s%sApplication Configuration Commands:%s\n", ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET);
            } else {
                printf("Application Configuration Commands:\n");
            }
            if (ui_color_enabled) {
                printf("  %s%s%s <id> <param> <value>  - %s%sSet session app configuration%s\n", 
                       ANSI_BOLD, ANSI_COLOR_BRIGHT_GREEN, "set_app_config", ANSI_COLOR_WHITE, ANSI_RESET, ANSI_RESET);
                printf("  %s%s%s <id> <param>          - %s%sGet session app configuration%s\n", 
                       ANSI_BOLD, ANSI_COLOR_BRIGHT_GREEN, "get_app_config", ANSI_COLOR_WHITE, ANSI_RESET, ANSI_RESET);
            } else {
                printf("  set_app_config <id> <param> <value>  - Set session app configuration\n");
                printf("  get_app_config <id> <param>          - Get session app configuration\n");
            }
            printf("\n");
            
            if (ui_color_enabled) {
                printf("%s%sHardware Mode Commands:%s\n", ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET);
            } else {
                printf("Hardware Mode Commands:\n");
            }
            if (ui_color_enabled) {
                printf("  %s%s%s <device_path>            - %s%sInitialize hardware mode%s\n", 
                       ANSI_BOLD, ANSI_COLOR_BRIGHT_GREEN, "hw_init", ANSI_COLOR_WHITE, ANSI_RESET, ANSI_RESET);
                printf("  %s%s%s <device_path>         - %s%sConnect to hardware device%s\n", 
                       ANSI_BOLD, ANSI_COLOR_BRIGHT_GREEN, "hw_connect", ANSI_COLOR_WHITE, ANSI_RESET, ANSI_RESET);
                printf("  %s%s%s                      - %s%sGet hardware device information%s\n", 
                       ANSI_BOLD, ANSI_COLOR_BRIGHT_GREEN, "hw_info", ANSI_COLOR_WHITE, ANSI_RESET, ANSI_RESET);
                printf("  %s%s%s <mt> <pbf> <gid> <oid> [payload...]  - %s%sSend raw command%s\n", 
                       ANSI_BOLD, ANSI_COLOR_BRIGHT_GREEN, "hw_send", ANSI_COLOR_WHITE, ANSI_RESET, ANSI_RESET);
                printf("  %s%s%s <bytes...>           - %s%sSend raw hex bytes%s\n", 
                       ANSI_BOLD, ANSI_COLOR_BRIGHT_GREEN, "hw_send_raw", ANSI_COLOR_WHITE, ANSI_RESET, ANSI_RESET);
            } else {
                printf("  hw_init <device_path>            - Initialize hardware mode\n");
                printf("  hw_connect <device_path>         - Connect to hardware device\n");
                printf("  hw_info                      - Get hardware device information\n");
                printf("  hw_send <mt> <pbf> <gid> <oid> [payload...]  - Send raw command\n");
                printf("  hw_send_raw <bytes...>           - Send raw hex bytes\n");
            }
            printf("\n");
            
            if (ui_color_enabled) {
                printf("%s%sSimulation & Testing Commands:%s\n", ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET);
            } else {
                printf("Simulation & Testing Commands:\n");
            }
            if (ui_color_enabled) {
                printf("  %s%s%s            - %s%sSimulate a device status notification%s\n", 
                       ANSI_BOLD, ANSI_COLOR_BRIGHT_GREEN, "simulate_notification", ANSI_COLOR_WHITE, ANSI_RESET, ANSI_RESET);
                printf("  %s%s%s          - %s%sSimulate a session status notification%s\n", 
                       ANSI_BOLD, ANSI_COLOR_BRIGHT_GREEN, "simulate_session_status", ANSI_COLOR_WHITE, ANSI_RESET, ANSI_RESET);
                printf("  %s%s%s             - %s%sSimulate a data credit notification%s\n", 
                       ANSI_BOLD, ANSI_COLOR_BRIGHT_GREEN, "simulate_data_credit", ANSI_COLOR_WHITE, ANSI_RESET, ANSI_RESET);
                printf("  %s%s%s                 - %s%sSimulate ranging measurements%s\n", 
                       ANSI_BOLD, ANSI_COLOR_BRIGHT_GREEN, "simulate_ranging", ANSI_COLOR_WHITE, ANSI_RESET, ANSI_RESET);
                printf("  %s%s%s    - %s%sSimulate multi-target ranging%s\n", 
                       ANSI_BOLD, ANSI_COLOR_BRIGHT_GREEN, "simulate_multi_target_ranging", ANSI_COLOR_WHITE, ANSI_RESET, ANSI_RESET);
                printf("  %s%s%s                - %s%sDemonstrate a complete session flow%s\n", 
                       ANSI_BOLD, ANSI_COLOR_BRIGHT_GREEN, "demo_session_flow", ANSI_COLOR_WHITE, ANSI_RESET, ANSI_RESET);
                printf("  %s%s%s <bytes...>        - %s%sAnalyze hex packet bytes with enhanced analysis (supports -v, -t, -c, -e, -h flags)%s\n", 
                       ANSI_BOLD, ANSI_COLOR_BRIGHT_GREEN, "analyze_packet", ANSI_COLOR_WHITE, ANSI_RESET, ANSI_RESET);
            } else {
                printf("  simulate_notification            - Simulate a device status notification\n");
                printf("  simulate_session_status          - Simulate a session status notification\n");
                printf("  simulate_data_credit             - Simulate a data credit notification\n");
                printf("  simulate_ranging                 - Simulate ranging measurements\n");
                printf("  simulate_multi_target_ranging    - Simulate multi-target ranging\n");
                printf("  demo_session_flow                - Demonstrate a complete session flow\n");
                printf("  analyze_packet <bytes...>        - Analyze hex packet bytes with enhanced analysis (supports -v, -t, -c, -e, -h flags)\n");
            }
            printf("\n");
            
            if (ui_color_enabled) {
                printf("%s%sExtended Session Configuration Commands:%s\n", ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET);
            } else {
                printf("Extended Session Configuration Commands:\n");
            }
            if (ui_color_enabled) {
                printf("  %s%s%s <id> [config_data] - %s%sSet hybrid controller configuration%s\n", 
                       ANSI_BOLD, ANSI_COLOR_BRIGHT_GREEN, "session_set_hybrid_controller_config", ANSI_COLOR_WHITE, ANSI_RESET, ANSI_RESET);
                printf("  %s%s%s <id> [config_data] - %s%sSet hybrid controlee configuration%s\n", 
                       ANSI_BOLD, ANSI_COLOR_BRIGHT_GREEN, "session_set_hybrid_controlee_config", ANSI_COLOR_WHITE, ANSI_RESET, ANSI_RESET);
                printf("  %s%s%s <id>           - %s%sQuery data size in ranging%s\n", 
                       ANSI_BOLD, ANSI_COLOR_BRIGHT_GREEN, "session_query_data_size_in_ranging", ANSI_COLOR_WHITE, ANSI_RESET, ANSI_RESET);
            } else {
                printf("  session_set_hybrid_controller_config <id> [config_data] - Set hybrid controller configuration\n");
                printf("  session_set_hybrid_controlee_config <id> [config_data] - Set hybrid controlee configuration\n");
                printf("  session_query_data_size_in_ranging <id>           - Query data size in ranging\n");
            }
            printf("\n");
            
            if (ui_color_enabled) {
                printf("%s%sFor complete documentation, see:%s\n", ANSI_COLOR_BRIGHT_BLUE, ANSI_BOLD, ANSI_RESET);
            } else {
                printf("For complete documentation, see:\n");
            }
            if (ui_color_enabled) {
                printf("  - %s%sREADME.md%s - %s%sProject overview and usage%s\n", 
                       ANSI_BOLD, ANSI_COLOR_BRIGHT_CYAN, ANSI_RESET, ANSI_COLOR_WHITE, ANSI_RESET, ANSI_RESET);
                printf("  - %s%sFINAL_SUMMARY.md%s - %s%sComplete feature summary and technical details%s\n", 
                       ANSI_BOLD, ANSI_COLOR_BRIGHT_CYAN, ANSI_RESET, ANSI_COLOR_WHITE, ANSI_RESET, ANSI_RESET);
                printf("  - %s%subi_analysis/%s - %s%sDetailed UCI protocol analysis based on Android UWB specification%s\n", 
                       ANSI_BOLD, ANSI_COLOR_BRIGHT_CYAN, ANSI_RESET, ANSI_COLOR_WHITE, ANSI_RESET, ANSI_RESET);
            } else {
                printf("  - README.md - Project overview and usage\n");
                printf("  - FINAL_SUMMARY.md - Complete feature summary and technical details\n");
                printf("  - uci_analysis/ - Detailed UCI protocol analysis based on Android UWB specification\n");
            }
        } else {
            ui_print_command_not_found(command);
        }

        // Note: In a real implementation, notifications would arrive asynchronously from the UWB device
        // Here we're simulating synchronous processing, so we don't need periodic dummy notifications
    }

    return 0;
}
