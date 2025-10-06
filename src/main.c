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

#define MAX_PAYLOAD_LENGTH 255

// Global variables for hardware mode
static int g_hardware_mode = 0;  // Flag to track if hardware mode is enabled
static uci_hw_chardev_t g_uwb_chardev;  // Character device interface for UWB communication

int main() {
    char line[CLI_MAX_LINE_LENGTH];

    // Initialize configuration manager
    if (uci_config_init() != 0) {
        printf("Warning: Failed to initialize configuration manager\n");
    }

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
            if (!device_path) {
                printf("Usage: hw_init <device_path>\n");
                continue;
            }
            
            // Initialize both the legacy hardware interface and the new character device interface
            if (uci_hw_interface_init(device_path) == 0) {
                g_hardware_mode = 1;
                ui_print_hardware_mode_initialized(device_path);
                
                // Also initialize the character device interface
                if (uci_hw_chardev_init(&g_uwb_chardev, device_path) == 0) {
                    if (uci_hw_chardev_open(&g_uwb_chardev) == 0) {
                        ui_print_success("Character device interface initialized successfully");
                    } else {
                        printf("Warning: Failed to open character device interface\n");
                    }
                } else {
                    printf("Warning: Failed to initialize character device interface\n");
                }
            } else {
                printf("Failed to initialize hardware mode\n");
            }
        } else if (strcmp(command, "hw_send") == 0 && g_hardware_mode) {
            if (!uci_hw_interface_is_connected()) {
                printf("Hardware not connected. Use 'hw_connect <device_path>' first.\n");
                continue;
            }
            
            char* mt_str = strtok(NULL, " ");
            char* pbf_str = strtok(NULL, " ");
            char* gid_str = strtok(NULL, " ");
            char* oid_str = strtok(NULL, " ");
            
            if (!mt_str || !pbf_str || !gid_str || !oid_str) {
                printf("Usage: hw_send <mt> <pbf> <gid> <oid> [payload_bytes...]\n");
                printf("  Example: hw_send 01 00 00 02 (send CORE_DEVICE_INFO command)\n");
                printf("  MT: 01=COMMAND, 02=RESPONSE, 03=NOTIFICATION\n");
                printf("  PBF: 00=COMPLETE, 01=START, 02=CONT, 03=END\n");
                printf("  GID: 00=CORE, 01=SESSION_CONFIG, 02=SESSION_CONTROL\n");
                printf("  OID: Command opcode (depends on GID)\n");
                continue;
            }
            
            unsigned char mt = (unsigned char)strtol(mt_str, NULL, 16);
            unsigned char pbf = (unsigned char)strtol(pbf_str, NULL, 16);
            unsigned char gid = (unsigned char)strtol(gid_str, NULL, 16);
            unsigned char oid = (unsigned char)strtol(oid_str, NULL, 16);
            
            unsigned char payload[MAX_PAYLOAD_LENGTH];
            int payload_len = 0;
            
            char* token;
            while ((token = strtok(NULL, " ")) != NULL && payload_len < MAX_PAYLOAD_LENGTH) {
                payload[payload_len++] = (unsigned char)strtol(token, NULL, 16);
            }
            
            // Send command to hardware using the character device interface
            if (uci_hw_interface_send_command(mt, pbf, gid, oid, payload, payload_len) == 0) {
                printf("Command sent to hardware successfully\n");
                
                // Try to receive response (with timeout)
                unsigned char response_buffer[1024];
                int response_len = uci_hw_interface_receive_response(response_buffer, sizeof(response_buffer), 1000);
                if (response_len > 0) {
                    printf("Received %d bytes from hardware:\n", response_len);
                    printf("  ");
                    for (int i = 0; i < response_len; i++) {
                        printf("%02X ", response_buffer[i]);
                    }
                    printf("\n");
                    
                    // Parse and display the response
                    parse_uci_packet(response_buffer, response_len);
                } else if (response_len == 0) {
                    printf("No response received from hardware (timeout)\n");
                } else {
                    printf("Error receiving response from hardware\n");
                }
            } else {
                printf("Failed to send command to hardware\n");
            }
        } else if (strcmp(command, "hw_get_device_info") == 0 || strcmp(command, "hw_device_info") == 0) {
            // Friendly alias for getting device info
            if (!g_hardware_mode || !uci_hw_interface_is_connected()) {
                printf("Hardware not connected. Use 'hw_connect <device_path>' first.\n");
                continue;
            }
            
            // Send CORE_DEVICE_INFO command (MT=0x01, PBF=0x00, GID=0x00, OID=0x02)
            if (uci_hw_interface_send_command(0x01, 0x00, 0x00, 0x02, NULL, 0) == 0) {
                printf("CORE_DEVICE_INFO command sent to hardware successfully\n");
                // Try to receive response
                unsigned char response_buffer[1024];
                int response_len = uci_hw_interface_receive_response(response_buffer, sizeof(response_buffer), 1000);
                if (response_len > 0) {
                    printf("Received %d bytes from hardware:\n  ", response_len);
                    for (int i = 0; i < response_len; i++) {
                        printf("%02X ", response_buffer[i]);
                    }
                    printf("\n");
                    parse_uci_packet(response_buffer, response_len);
                }
            } else {
                printf("Failed to send CORE_DEVICE_INFO command to hardware\n");
            }
        } else if (strcmp(command, "hw_device_reset") == 0) {
            // Friendly alias for device reset
            if (!g_hardware_mode || !uci_hw_interface_is_connected()) {
                printf("Hardware not connected. Use 'hw_connect <device_path>' first.\n");
                continue;
            }
            
            unsigned char payload[] = {UWBS_RESET};
            // Send CORE_DEVICE_RESET command (MT=0x01, PBF=0x00, GID=0x00, OID=0x00)
            if (uci_hw_interface_send_command(0x01, 0x00, 0x00, 0x00, payload, sizeof(payload)) == 0) {
                printf("CORE_DEVICE_RESET command sent to hardware successfully\n");
                // Try to receive response
                unsigned char response_buffer[1024];
                int response_len = uci_hw_interface_receive_response(response_buffer, sizeof(response_buffer), 1000);
                if (response_len > 0) {
                    printf("Received %d bytes from hardware:\n  ", response_len);
                    for (int i = 0; i < response_len; i++) {
                        printf("%02X ", response_buffer[i]);
                    }
                    printf("\n");
                    parse_uci_packet(response_buffer, response_len);
                }
            } else {
                printf("Failed to send CORE_DEVICE_RESET command to hardware\n");
            }
        } else if (strcmp(command, "hw_session_init") == 0 || strcmp(command, "hw_session_new") == 0) {
            // Friendly alias for session initialization
            if (!g_hardware_mode || !uci_hw_interface_is_connected()) {
                printf("Hardware not connected. Use 'hw_connect <device_path>' first.\n");
                continue;
            }
            
            char* session_id_str = strtok(NULL, " ");
            char* session_type_str = strtok(NULL, " ");
            if (!session_id_str || !session_type_str) {
                printf("Usage: hw_session_init <session_id> <session_type>\n");
                printf("  session_id: Numeric session identifier (e.g., 1, 2, 3)\n");
                printf("  session_type: Type of session to create\n");
                printf("    fira_ranging: FiRa ranging session\n");
                printf("    ranging: Generic ranging session\n");
                printf("Example: hw_session_init 1 fira_ranging\n");
                continue;
            }

            unsigned int session_id = (unsigned int)strtoul(session_id_str, NULL, 10);
            SessionType session_type;

            if (strcmp(session_type_str, "fira_ranging") == 0 || strcmp(session_type_str, "ranging") == 0) {
                session_type = FIRA_RANGING_SESSION;
            } else {
                printf("Unknown session_type: %s. Use 'fira_ranging' or 'ranging'.\n", session_type_str);
                continue;
            }

            unsigned char payload[5];
            payload[0] = (session_id >> 24) & 0xFF;
            payload[1] = (session_id >> 16) & 0xFF;
            payload[2] = (session_id >> 8) & 0xFF;
            payload[3] = session_id & 0xFF;
            payload[4] = session_type;
            
            // Send SESSION_INIT command (MT=0x01, PBF=0x00, GID=0x01, OID=0x00)
            if (uci_hw_interface_send_command(0x01, 0x00, 0x01, 0x00, payload, sizeof(payload)) == 0) {
                printf("SESSION_INIT command sent to hardware successfully\n");
                // Try to receive response
                unsigned char response_buffer[1024];
                int response_len = uci_hw_interface_receive_response(response_buffer, sizeof(response_buffer), 1000);
                if (response_len > 0) {
                    printf("Received %d bytes from hardware:\n  ", response_len);
                    for (int i = 0; i < response_len; i++) {
                        printf("%02X ", response_buffer[i]);
                    }
                    printf("\n");
                    parse_uci_packet(response_buffer, response_len);
                }
            } else {
                printf("Failed to send SESSION_INIT command to hardware\n");
            }
        } else if (strcmp(command, "hw_session_start") == 0 || strcmp(command, "hw_start_ranging") == 0) {
            // Friendly alias for starting a session
            if (!g_hardware_mode || !uci_hw_interface_is_connected()) {
                printf("Hardware not connected. Use 'hw_connect <device_path>' first.\n");
                continue;
            }
            
            char* session_id_str = strtok(NULL, " ");
            if (!session_id_str) {
                printf("Usage: hw_session_start <session_id>\n");
                printf("  session_id: Numeric session identifier (e.g., 1, 2, 3)\n");
                printf("Example: hw_session_start 1\n");
                continue;
            }

            unsigned int session_id = (unsigned int)strtoul(session_id_str, NULL, 10);
            unsigned char payload[4];
            payload[0] = (session_id >> 24) & 0xFF;
            payload[1] = (session_id >> 16) & 0xFF;
            payload[2] = (session_id >> 8) & 0xFF;
            payload[3] = session_id & 0xFF;
            
            // Send SESSION_START command (MT=0x01, PBF=0x00, GID=0x02, OID=0x00)
            if (uci_hw_interface_send_command(0x01, 0x00, 0x02, 0x00, payload, sizeof(payload)) == 0) {
                printf("SESSION_START command sent to hardware successfully\n");
                // Try to receive response
                unsigned char response_buffer[1024];
                int response_len = uci_hw_interface_receive_response(response_buffer, sizeof(response_buffer), 1000);
                if (response_len > 0) {
                    printf("Received %d bytes from hardware:\n  ", response_len);
                    for (int i = 0; i < response_len; i++) {
                        printf("%02X ", response_buffer[i]);
                    }
                    printf("\n");
                    parse_uci_packet(response_buffer, response_len);
                }
            } else {
                printf("Failed to send SESSION_START command to hardware\n");
            }
        } else if (strcmp(command, "hw_set_config") == 0) {
            // Friendly alias for setting device configuration
            if (!g_hardware_mode || !uci_hw_interface_is_connected()) {
                printf("Hardware not connected. Use 'hw_connect <device_path>' first.\n");
                continue;
            }
            
            char* config_name = strtok(NULL, " ");
            char* value_str = strtok(NULL, " ");
            if (!config_name || !value_str) {
                printf("Usage: hw_set_config <config_name> <value>\n");
                printf("  config_name: Configuration parameter to set\n");
                printf("    device_state: Device state (active, ready, sleep)\n");
                printf("    low_power_mode: Low power mode (on, off)\n");
                printf("  value: Value to set for the configuration\n");
                printf("Example: hw_set_config device_state active\n");
                printf("         hw_set_config low_power_mode on\n");
                continue;
            }

            DeviceConfigId cfg_id;
            unsigned char value;

            if (strcmp(config_name, "device_state") == 0) {
                cfg_id = DEVICE_STATE;
                if (strcmp(value_str, "active") == 0) {
                    value = DEVICE_STATE_ACTIVE;
                } else if (strcmp(value_str, "ready") == 0) {
                    value = DEVICE_STATE_READY;
                } else if (strcmp(value_str, "sleep") == 0 || strcmp(value_str, "suspend") == 0) {
                    // For sleep, we'll send the device suspend command
                    unsigned char suspend_payload[] = {0x00}; // Wakeup source
                    if (uci_hw_interface_send_command(0x01, 0x00, 0x00, CORE_DEVICE_SUSPEND, suspend_payload, sizeof(suspend_payload)) == 0) {
                        printf("CORE_DEVICE_SUSPEND command sent to hardware successfully\n");
                    }
                    continue;
                } else {
                    printf("Invalid value for device_state. Use 'active', 'ready', 'sleep', or 'suspend'.\n");
                    continue;
                }
            } else if (strcmp(config_name, "low_power_mode") == 0) {
                cfg_id = LOW_POWER_MODE;
                if (strcmp(value_str, "on") == 0) {
                    value = 0x01;
                } else if (strcmp(value_str, "off") == 0) {
                    value = 0x00;
                } else {
                    printf("Invalid value for low_power_mode. Use 'on' or 'off'.\n");
                    continue;
                }
            } else {
                printf("Unknown config_name: %s\n", config_name);
                printf("Supported config names: device_state, low_power_mode\n");
                continue;
            }

            unsigned char payload[] = {0x01, cfg_id, 0x01, value};
            // Send CORE_SET_CONFIG command (MT=0x01, PBF=0x00, GID=0x00, OID=0x04)
            if (uci_hw_interface_send_command(0x01, 0x00, 0x00, CORE_SET_CONFIG, payload, sizeof(payload)) == 0) {
                printf("CORE_SET_CONFIG command sent to hardware successfully\n");
                // Try to receive response
                unsigned char response_buffer[1024];
                int response_len = uci_hw_interface_receive_response(response_buffer, sizeof(response_buffer), 1000);
                if (response_len > 0) {
                    printf("Received %d bytes from hardware:\n  ", response_len);
                    for (int i = 0; i < response_len; i++) {
                        printf("%02X ", response_buffer[i]);
                    }
                    printf("\n");
                    parse_uci_packet(response_buffer, response_len);
                }
            } else {
                printf("Failed to send CORE_SET_CONFIG command to hardware\n");
            }
        } else if (strcmp(command, "hw_get_caps_info") == 0) {
            // Friendly alias for getting capabilities info
            if (!g_hardware_mode || !uci_hw_interface_is_connected()) {
                printf("Hardware not connected. Use 'hw_connect <device_path>' first.\n");
                continue;
            }
            
            // Send CORE_GET_CAPS_INFO command (MT=0x01, PBF=0x00, GID=0x00, OID=0x03)
            if (uci_hw_interface_send_command(0x01, 0x00, 0x00, 0x03, NULL, 0) == 0) {
                printf("CORE_GET_CAPS_INFO command sent to hardware successfully\n");
                // Try to receive response
                unsigned char response_buffer[1024];
                int response_len = uci_hw_interface_receive_response(response_buffer, sizeof(response_buffer), 1000);
                if (response_len > 0) {
                    printf("Received %d bytes from hardware:\n  ", response_len);
                    for (int i = 0; i < response_len; i++) {
                        printf("%02X ", response_buffer[i]);
                    }
                    printf("\n");
                    parse_uci_packet(response_buffer, response_len);
                }
            } else {
                printf("Failed to send CORE_GET_CAPS_INFO command to hardware\n");
            }
        } else if (strcmp(command, "hw_session_deinit") == 0 || strcmp(command, "hw_session_close") == 0) {
            // Friendly alias for session deinitialization
            if (!g_hardware_mode || !uci_hw_interface_is_connected()) {
                printf("Hardware not connected. Use 'hw_connect <device_path>' first.\n");
                continue;
            }
            
            char* session_id_str = strtok(NULL, " ");
            if (!session_id_str) {
                printf("Usage: hw_session_deinit <session_id>\n");
                printf("  session_id: Numeric session identifier (e.g., 1, 2, 3)\n");
                printf("Example: hw_session_deinit 1\n");
                continue;
            }

            unsigned int session_id = (unsigned int)strtoul(session_id_str, NULL, 10);
            unsigned char payload[4];
            payload[0] = (session_id >> 24) & 0xFF;
            payload[1] = (session_id >> 16) & 0xFF;
            payload[2] = (session_id >> 8) & 0xFF;
            payload[3] = session_id & 0xFF;
            
            // Send SESSION_DEINIT command (MT=0x01, PBF=0x00, GID=0x01, OID=0x01)
            if (uci_hw_interface_send_command(0x01, 0x00, 0x01, 0x01, payload, sizeof(payload)) == 0) {
                printf("SESSION_DEINIT command sent to hardware successfully\n");
                // Try to receive response
                unsigned char response_buffer[1024];
                int response_len = uci_hw_interface_receive_response(response_buffer, sizeof(response_buffer), 1000);
                if (response_len > 0) {
                    printf("Received %d bytes from hardware:\n  ", response_len);
                    for (int i = 0; i < response_len; i++) {
                        printf("%02X ", response_buffer[i]);
                    }
                    printf("\n");
                    parse_uci_packet(response_buffer, response_len);
                }
            } else {
                printf("Failed to send SESSION_DEINIT command to hardware\n");
            }
        } else if (strcmp(command, "hw_get_session_state") == 0 || strcmp(command, "hw_session_status") == 0) {
            // Friendly alias for getting session state
            if (!g_hardware_mode || !uci_hw_interface_is_connected()) {
                printf("Hardware not connected. Use 'hw_connect <device_path>' first.\n");
                continue;
            }
            
            char* session_id_str = strtok(NULL, " ");
            if (!session_id_str) {
                printf("Usage: hw_get_session_state <session_id>\n");
                printf("  session_id: Numeric session identifier (e.g., 1, 2, 3)\n");
                printf("Example: hw_get_session_state 1\n");
                continue;
            }

            unsigned int session_id = (unsigned int)strtoul(session_id_str, NULL, 10);
            unsigned char payload[4];
            payload[0] = (session_id >> 24) & 0xFF;
            payload[1] = (session_id >> 16) & 0xFF;
            payload[2] = (session_id >> 8) & 0xFF;
            payload[3] = session_id & 0xFF;
            
            // Send SESSION_GET_STATE command (MT=0x01, PBF=0x00, GID=0x01, OID=0x06)
            if (uci_hw_interface_send_command(0x01, 0x00, 0x01, 0x06, payload, sizeof(payload)) == 0) {
                printf("SESSION_GET_STATE command sent to hardware successfully\n");
                // Try to receive response
                unsigned char response_buffer[1024];
                int response_len = uci_hw_interface_receive_response(response_buffer, sizeof(response_buffer), 1000);
                if (response_len > 0) {
                    printf("Received %d bytes from hardware:\n  ", response_len);
                    for (int i = 0; i < response_len; i++) {
                        printf("%02X ", response_buffer[i]);
                    }
                    printf("\n");
                    parse_uci_packet(response_buffer, response_len);
                }
            } else {
                printf("Failed to send SESSION_GET_STATE command to hardware\n");
            }
        } else if (strcmp(command, "hw_set_app_config") == 0) {
            // Friendly alias for setting app configuration
            if (!g_hardware_mode || !uci_hw_interface_is_connected()) {
                printf("Hardware not connected. Use 'hw_connect <device_path>' first.\n");
                continue;
            }
            
            char* session_id_str = strtok(NULL, " ");
            char* config_name = strtok(NULL, " ");
            char* value_str = strtok(NULL, "");
            if (!session_id_str || !config_name || !value_str) {
                printf("Usage: hw_set_app_config <session_id> <config_name> <value>\n");
                printf("  session_id: Numeric session identifier (e.g., 1, 2, 3)\n");
                printf("  config_name: Application configuration parameter to set\n");
                printf("    device_type: Device role (responder, initiator)\n");
                printf("    ranging_usage: Ranging usage (ranging, data)\n");
                printf("    sts_config: STS configuration (static, dynamic)\n");
                printf("    multi_node_mode: Multi-node mode (unicast, anycast, multicast)\n");
                printf("    channel: Channel number (0-255)\n");
                printf("    device_role: Device role (controller, controlee)\n");
                printf("    aoa_request: Angle-of-Arrival request (enable/on, disable/off)\n");
                printf("    scheduled_mode: Scheduled mode (cont/continuous, scheduled)\n");
                printf("    device_mac_address: Device MAC address (8-byte hex)\n");
                printf("    dst_mac_address: Destination MAC address (8-byte hex)\n");
                printf("    static_sts_iv: Static STS initialization vector (6-byte hex)\n");
                printf("    uwb_initiation_time: UWB initiation time (4-byte value)\n");
                printf("  value: Value to set for the configuration\n");
                printf("    For single-byte values: Use name or numeric value\n");
                printf("    For multi-byte values: Use hex string (e.g., AABBCCDDEEFF0011)\n");
                printf("Example: hw_set_app_config 1 device_type responder\n");
                printf("         hw_set_app_config 1 ranging_usage ranging\n");
                printf("         hw_set_app_config 1 channel 5\n");
                printf("         hw_set_app_config 1 device_mac_address AABBCCDDEEFF0011\n");
                printf("         hw_set_app_config 1 static_sts_iv AABBCCDDEEFF\n");
                continue;
            }

            unsigned int session_id = (unsigned int)strtoul(session_id_str, NULL, 10);
            AppConfigTlvType cfg_id;
            
            // Buffer to hold the value (support up to 32 bytes for complex TLVs)
            unsigned char value_buffer[32];
            int value_length = 0;
            int is_valid = 1;

            // Determine config ID and parse value based on config name
            if (strcmp(config_name, "device_type") == 0) {
                cfg_id = DEVICE_TYPE;
                if (strcmp(value_str, "responder") == 0) {
                    value_buffer[0] = 0x01;
                } else if (strcmp(value_str, "initiator") == 0) {
                    value_buffer[0] = 0x02;
                } else {
                    // Try to parse as numeric value
                    char *endptr;
                    long val = strtol(value_str, &endptr, 10);
                    if (*endptr != '\0' || val < 0 || val > 255) {
                        printf("Invalid value for device_type. Use 'responder', 'initiator', or numeric value 1-2.\n");
                        continue;
                    }
                    value_buffer[0] = (unsigned char)val;
                }
                value_length = 1;
            } else if (strcmp(config_name, "ranging_usage") == 0 || strcmp(config_name, "ranging_round_usage") == 0) {
                cfg_id = RANGING_ROUND_USAGE;
                if (strcmp(value_str, "ranging") == 0) {
                    value_buffer[0] = 0x00;
                } else if (strcmp(value_str, "data") == 0) {
                    value_buffer[0] = 0x01;
                } else {
                    // Try to parse as numeric value
                    char *endptr;
                    long val = strtol(value_str, &endptr, 10);
                    if (*endptr != '\0' || val < 0 || val > 255) {
                        printf("Invalid value for ranging_usage. Use 'ranging', 'data', or numeric value 0-1.\n");
                        continue;
                    }
                    value_buffer[0] = (unsigned char)val;
                }
                value_length = 1;
            } else if (strcmp(config_name, "sts_config") == 0) {
                cfg_id = STS_CONFIG;
                if (strcmp(value_str, "static") == 0) {
                    value_buffer[0] = 0x00;
                } else if (strcmp(value_str, "dynamic") == 0) {
                    value_buffer[0] = 0x01;
                } else {
                    // Try to parse as numeric value
                    char *endptr;
                    long val = strtol(value_str, &endptr, 10);
                    if (*endptr != '\0' || val < 0 || val > 255) {
                        printf("Invalid value for sts_config. Use 'static', 'dynamic', or numeric value 0-1.\n");
                        continue;
                    }
                    value_buffer[0] = (unsigned char)val;
                }
                value_length = 1;
            } else if (strcmp(config_name, "multi_node_mode") == 0) {
                cfg_id = MULTI_NODE_MODE;
                if (strcmp(value_str, "unicast") == 0) {
                    value_buffer[0] = 0x00;
                } else if (strcmp(value_str, "anycast") == 0) {
                    value_buffer[0] = 0x01;
                } else if (strcmp(value_str, "multicast") == 0) {
                    value_buffer[0] = 0x02;
                } else {
                    // Try to parse as numeric value
                    char *endptr;
                    long val = strtol(value_str, &endptr, 10);
                    if (*endptr != '\0' || val < 0 || val > 255) {
                        printf("Invalid value for multi_node_mode. Use 'unicast', 'anycast', 'multicast', or numeric value 0-2.\n");
                        continue;
                    }
                    value_buffer[0] = (unsigned char)val;
                }
                value_length = 1;
            } else if (strcmp(config_name, "channel") == 0 || strcmp(config_name, "channel_number") == 0) {
                cfg_id = CHANNEL_NUMBER;
                // Allow numeric values for channel number
                char *endptr;
                long channel_val = strtol(value_str, &endptr, 10);
                if (*endptr != '\0') {
                    printf("Invalid value for channel. Use a numeric value.\n");
                    continue;
                }
                if (channel_val < 0 || channel_val > 255) {
                    printf("Invalid value for channel. Use 0-255.\n");
                    continue;
                }
                value_buffer[0] = (unsigned char)channel_val;
                value_length = 1;
            } else if (strcmp(config_name, "device_role") == 0) {
                cfg_id = DEVICE_ROLE;
                if (strcmp(value_str, "controller") == 0) {
                    value_buffer[0] = 0x00;
                } else if (strcmp(value_str, "controlee") == 0) {
                    value_buffer[0] = 0x01;
                } else {
                    // Try to parse as numeric value
                    char *endptr;
                    long val = strtol(value_str, &endptr, 10);
                    if (*endptr != '\0' || val < 0 || val > 255) {
                        printf("Invalid value for device_role. Use 'controller', 'controlee', or numeric value 0-1.\n");
                        continue;
                    }
                    value_buffer[0] = (unsigned char)val;
                }
                value_length = 1;
            } else if (strcmp(config_name, "aoa_request") == 0 || strcmp(config_name, "aoa_result_req") == 0) {
                cfg_id = AOA_RESULT_REQ;
                if (strcmp(value_str, "disable") == 0 || strcmp(value_str, "off") == 0) {
                    value_buffer[0] = 0x00;
                } else if (strcmp(value_str, "enable") == 0 || strcmp(value_str, "on") == 0) {
                    value_buffer[0] = 0x01;
                } else {
                    // Try to parse as numeric value
                    char *endptr;
                    long val = strtol(value_str, &endptr, 10);
                    if (*endptr != '\0' || val < 0 || val > 255) {
                        printf("Invalid value for aoa_request. Use 'enable'/'on', 'disable'/'off', or numeric value 0-1.\n");
                        continue;
                    }
                    value_buffer[0] = (unsigned char)val;
                }
                value_length = 1;
            } else if (strcmp(config_name, "scheduled_mode") == 0) {
                cfg_id = SCHEDULED_MODE;
                if (strcmp(value_str, "cont") == 0 || strcmp(value_str, "continuous") == 0) {
                    value_buffer[0] = 0x00;
                } else if (strcmp(value_str, "scheduled") == 0) {
                    value_buffer[0] = 0x01;
                } else {
                    // Try to parse as numeric value
                    char *endptr;
                    long val = strtol(value_str, &endptr, 10);
                    if (*endptr != '\0' || val < 0 || val > 255) {
                        printf("Invalid value for scheduled_mode. Use 'cont'/'continuous', 'scheduled', or numeric value 0-1.\n");
                        continue;
                    }
                    value_buffer[0] = (unsigned char)val;
                }
                value_length = 1;
            } else if (strcmp(config_name, "device_mac_address") == 0) {
                cfg_id = DEVICE_MAC_ADDRESS;
                // Parse 8-byte MAC address in hex format
                size_t len = strlen(value_str);
                if (len != 16) {
                    printf("Invalid value for device_mac_address. Use 16-character hex string (8 bytes).\n");
                    continue;
                }
                value_length = 8;
                for (int i = 0; i < value_length; i++) {
                    char hex_byte[3] = {value_str[i*2], value_str[i*2+1], '\0'};
                    char* endptr;
                    unsigned long byte_val = strtoul(hex_byte, &endptr, 16);
                    if (*endptr != '\0') {
                        printf("Invalid hex value for device_mac_address.\n");
                        is_valid = 0;
                        break;
                    }
                    value_buffer[i] = (unsigned char)byte_val;
                }
            } else if (strcmp(config_name, "dst_mac_address") == 0) {
                cfg_id = DST_MAC_ADDRESS;
                // Parse 8-byte destination MAC address in hex format
                size_t len = strlen(value_str);
                if (len != 16) {
                    printf("Invalid value for dst_mac_address. Use 16-character hex string (8 bytes).\n");
                    continue;
                }
                value_length = 8;
                for (int i = 0; i < value_length; i++) {
                    char hex_byte[3] = {value_str[i*2], value_str[i*2+1], '\0'};
                    char* endptr;
                    unsigned long byte_val = strtoul(hex_byte, &endptr, 16);
                    if (*endptr != '\0') {
                        printf("Invalid hex value for dst_mac_address.\n");
                        is_valid = 0;
                        break;
                    }
                    value_buffer[i] = (unsigned char)byte_val;
                }
            } else if (strcmp(config_name, "static_sts_iv") == 0) {
                cfg_id = STATIC_STS_IV;
                // Parse 6-byte static STS IV in hex format
                size_t len = strlen(value_str);
                if (len != 12) {
                    printf("Invalid value for static_sts_iv. Use 12-character hex string (6 bytes).\n");
                    continue;
                }
                value_length = 6;
                for (int i = 0; i < value_length; i++) {
                    char hex_byte[3] = {value_str[i*2], value_str[i*2+1], '\0'};
                    char* endptr;
                    unsigned long byte_val = strtoul(hex_byte, &endptr, 16);
                    if (*endptr != '\0') {
                        printf("Invalid hex value for static_sts_iv.\n");
                        is_valid = 0;
                        break;
                    }
                    value_buffer[i] = (unsigned char)byte_val;
                }
            } else if (strcmp(config_name, "uwb_initiation_time") == 0) {
                cfg_id = UWB_INITIATION_TIME;
                // Parse 4-byte initiation time as numeric value
                char *endptr;
                unsigned long long time_val = strtoull(value_str, &endptr, 10);
                if (*endptr != '\0') {
                    printf("Invalid value for uwb_initiation_time. Use numeric value.\n");
                    continue;
                }
                if (time_val > 0xFFFFFFFFULL) {
                    printf("Invalid value for uwb_initiation_time. Value too large.\n");
                    continue;
                }
                value_buffer[0] = (time_val >> 24) & 0xFF;
                value_buffer[1] = (time_val >> 16) & 0xFF;
                value_buffer[2] = (time_val >> 8) & 0xFF;
                value_buffer[3] = time_val & 0xFF;
                value_length = 4;
            } else {
                printf("Unknown config_name: %s\n", config_name);
                printf("Supported config names: device_type, ranging_usage, sts_config, multi_node_mode, channel, device_role, aoa_request, scheduled_mode, device_mac_address, dst_mac_address, static_sts_iv, uwb_initiation_time\n");
                continue;
            }
            
            if (!is_valid) {
                continue;
            }

            // Prepare payload with proper TLV format
            // Calculate payload size: session_id(4) + num_tlvs(1) + tlv_header(2) + value(value_length) + reserved(1)
            int payload_size = 4 + 1 + 2 + value_length + 1;
            unsigned char* payload = malloc(payload_size);
            if (!payload) {
                printf("Failed to allocate memory for payload\n");
                continue;
            }
            
            payload[0] = (session_id >> 24) & 0xFF;
            payload[1] = (session_id >> 16) & 0xFF;
            payload[2] = (session_id >> 8) & 0xFF;
            payload[3] = session_id & 0xFF;
            payload[4] = 1; // Number of TLVs
            payload[5] = cfg_id;
            payload[6] = value_length; // Length
            
            // Copy value
            memcpy(&payload[7], value_buffer, value_length);
            
            // Reserved byte
            payload[7 + value_length] = 0;
            
            // Send SESSION_SET_APP_CONFIG command (MT=0x01, PBF=0x00, GID=0x01, OID=0x03)
            if (uci_hw_interface_send_command(0x01, 0x00, 0x01, 0x03, payload, payload_size) == 0) {
                printf("SESSION_SET_APP_CONFIG command sent to hardware successfully\n");
                // Try to receive response
                unsigned char response_buffer[1024];
                int response_len = uci_hw_interface_receive_response(response_buffer, sizeof(response_buffer), 1000);
                if (response_len > 0) {
                    printf("Received %d bytes from hardware:\n  ", response_len);
                    for (int i = 0; i < response_len; i++) {
                        printf("%02X ", response_buffer[i]);
                    }
                    printf("\n");
                    parse_uci_packet(response_buffer, response_len);
                }
            } else {
                printf("Failed to send SESSION_SET_APP_CONFIG command to hardware\n");
            }
            
            free(payload);
        } else if (strcmp(command, "hw_get_app_config") == 0) {
            // Friendly alias for getting app configuration
            if (!g_hardware_mode || !uci_hw_interface_is_connected()) {
                printf("Hardware not connected. Use 'hw_connect <device_path>' first.\n");
                continue;
            }
            
            char* session_id_str = strtok(NULL, " ");
            char* config_name = strtok(NULL, " ");
            if (!session_id_str || !config_name) {
                printf("Usage: hw_get_app_config <session_id> <config_name>\n");
                printf("  session_id: Numeric session identifier (e.g., 1, 2, 3)\n");
                printf("  config_name: Application configuration parameter to get\n");
                printf("    device_type: Device role (responder, initiator)\n");
                printf("    ranging_usage: Ranging usage (ranging, data)\n");
                printf("    sts_config: STS configuration (static, dynamic)\n");
                printf("    multi_node_mode: Multi-node mode (unicast, anycast, multicast)\n");
                printf("    channel: Channel number\n");
                printf("    device_role: Device role (controller, controlee)\n");
                printf("    aoa_request: Angle-of-Arrival request status\n");
                printf("    scheduled_mode: Scheduled mode\n");
                printf("    device_mac_address: Device MAC address\n");
                printf("    dst_mac_address: Destination MAC address\n");
                printf("    static_sts_iv: Static STS initialization vector\n");
                printf("    uwb_initiation_time: UWB initiation time\n");
                printf("Example: hw_get_app_config 1 device_type\n");
                printf("         hw_get_app_config 1 channel\n");
                printf("         hw_get_app_config 1 device_mac_address\n");
                printf("         hw_get_app_config 1 static_sts_iv\n");
                continue;
            }

            unsigned int session_id = (unsigned int)strtoul(session_id_str, NULL, 10);
            AppConfigTlvType cfg_id;

            if (strcmp(config_name, "device_type") == 0) {
                cfg_id = DEVICE_TYPE;
            } else if (strcmp(config_name, "ranging_usage") == 0 || strcmp(config_name, "ranging_round_usage") == 0) {
                cfg_id = RANGING_ROUND_USAGE;
            } else if (strcmp(config_name, "sts_config") == 0) {
                cfg_id = STS_CONFIG;
            } else if (strcmp(config_name, "multi_node_mode") == 0) {
                cfg_id = MULTI_NODE_MODE;
            } else if (strcmp(config_name, "channel") == 0 || strcmp(config_name, "channel_number") == 0) {
                cfg_id = CHANNEL_NUMBER;
            } else if (strcmp(config_name, "device_role") == 0) {
                cfg_id = DEVICE_ROLE;
            } else if (strcmp(config_name, "aoa_request") == 0 || strcmp(config_name, "aoa_result_req") == 0) {
                cfg_id = AOA_RESULT_REQ;
            } else if (strcmp(config_name, "scheduled_mode") == 0) {
                cfg_id = SCHEDULED_MODE;
            } else if (strcmp(config_name, "device_mac_address") == 0) {
                cfg_id = DEVICE_MAC_ADDRESS;
            } else if (strcmp(config_name, "dst_mac_address") == 0) {
                cfg_id = DST_MAC_ADDRESS;
            } else if (strcmp(config_name, "static_sts_iv") == 0) {
                cfg_id = STATIC_STS_IV;
            } else if (strcmp(config_name, "uwb_initiation_time") == 0) {
                cfg_id = UWB_INITIATION_TIME;
            } else {
                printf("Unknown config_name: %s\n", config_name);
                printf("Supported config names: device_type, ranging_usage, sts_config, multi_node_mode, channel, device_role, aoa_request, scheduled_mode, device_mac_address, dst_mac_address, static_sts_iv, uwb_initiation_time\n");
                continue;
            }

            unsigned char payload[6];
            payload[0] = (session_id >> 24) & 0xFF;
            payload[1] = (session_id >> 16) & 0xFF;
            payload[2] = (session_id >> 8) & 0xFF;
            payload[3] = session_id & 0xFF;
            payload[4] = 1; // Number of configs
            payload[5] = cfg_id;
            
            // Send SESSION_GET_APP_CONFIG command (MT=0x01, PBF=0x00, GID=0x01, OID=0x04)
            if (uci_hw_interface_send_command(0x01, 0x00, 0x01, 0x04, payload, sizeof(payload)) == 0) {
                printf("SESSION_GET_APP_CONFIG command sent to hardware successfully\n");
                // Try to receive response
                unsigned char response_buffer[1024];
                int response_len = uci_hw_interface_receive_response(response_buffer, sizeof(response_buffer), 1000);
                if (response_len > 0) {
                    printf("Received %d bytes from hardware:\n  ", response_len);
                    for (int i = 0; i < response_len; i++) {
                        printf("%02X ", response_buffer[i]);
                    }
                    printf("\n");
                    parse_uci_packet(response_buffer, response_len);
                }
            } else {
                printf("Failed to send SESSION_GET_APP_CONFIG command to hardware\n");
            }
        } else if (strcmp(command, "hw_send_raw") == 0 && g_hardware_mode) {
            if (!uci_hw_interface_is_connected()) {
                printf("Hardware not connected. Use 'hw_init <device_path>' first.\n");
                continue;
            }
            
            char* hex_bytes_str = strtok(NULL, " ");
            if (!hex_bytes_str) {
                printf("Usage: hw_send_raw <hex_bytes...>\n");
                printf("  Example: hw_send_raw 20 08 00 00 (send CORE_DEVICE_INFO command)\n");
                printf("  Format: [GID|PBF|MT][Opcode|R][Reserved][Length][Payload...]\n");
                continue;
            }
            
            // Parse hex bytes
            unsigned char packet[256];
            int packet_len = 0;
            
            char* token = hex_bytes_str;
            do {
                if (packet_len >= (int)sizeof(packet)) {
                    ui_print_error("Packet too long");
                    break;
                }
                packet[packet_len++] = (unsigned char)strtol(token, NULL, 16);
            } while ((token = strtok(NULL, " ")) != NULL);
            
            if (packet_len == 0) {
                printf("Error: No bytes provided\n");
                continue;
            }
            
            printf("Sending raw UCI packet to hardware (%d bytes):\n  ", packet_len);
            for (int i = 0; i < packet_len; i++) {
                printf("%02X ", packet[i]);
            }
            printf("\n");
            
            // Send packet to hardware using character device interface
            int send_result = uci_hw_chardev_send(&g_uwb_chardev, packet, packet_len);
            if (send_result < 0) {
                printf("Failed to send raw UCI packet to hardware\n");
                continue;
            }
            
            printf("Raw UCI packet sent successfully\n");
            
            // Try to receive response (with timeout)
            unsigned char response_buffer[1024];
            int response_len = uci_hw_chardev_receive(&g_uwb_chardev, response_buffer, sizeof(response_buffer), 1000);
            if (response_len > 0) {
                printf("Received %d bytes from hardware:\n  ", response_len);
                for (int i = 0; i < response_len; i++) {
                    printf("%02X ", response_buffer[i]);
                }
                printf("\n");
                
                // Parse and display the response
                parse_uci_packet(response_buffer, response_len);
            } else if (response_len == 0) {
                printf("No response received from hardware (timeout)\n");
            } else {
                printf("Error receiving response from hardware\n");
            }
        } else if (strcmp(command, "hw_info") == 0) {
            if (!g_hardware_mode) {
                ui_print_hardware_mode_not_initialized();
            } else {
                printf("=== UCI Hardware Information ===\n");
                printf("Device: %s\n", uci_hw_interface_get_device_path());
                printf("Mode: %s\n", g_hardware_mode ? "ENABLED" : "DISABLED");
                printf("Connected: %s\n", uci_hw_interface_is_connected() ? "YES" : "NO");
                printf("===============================\n");
            }
        } else if (strcmp(command, "hw_connect") == 0) {
            char* device_path = strtok(NULL, " ");
            if (!device_path) {
                printf("Usage: hw_connect <device_path>\n");
                printf("  Example: hw_connect /dev/ttyUSB0\n");
                continue;
            }
            
            // Enable verbose mode for hardware communication
            uci_hw_chardev_set_verbose(&g_uwb_chardev, 1);
            uci_hw_interface_set_verbose(1);
            
            // Initialize hardware interface with the specified device
            if (uci_hw_interface_init(device_path) == 0) {
                g_hardware_mode = 1;
                ui_print_hardware_mode_initialized(device_path);
                printf("Ready to communicate with real UWB hardware!\n");
            } else {
                ui_print_error("Failed to initialize hardware mode with device");
            }
        } else if (strcmp(command, "get_device_info") == 0 || strcmp(command, "device_info") == 0) {
            send_uci_command(COMMAND, 0, CORE, CORE_DEVICE_INFO, NULL, 0);
        } else if (strcmp(command, "device_reset") == 0) {
            unsigned char payload[] = {UWBS_RESET};
            send_uci_command(COMMAND, 0, CORE, CORE_DEVICE_RESET, payload, sizeof(payload));
            // Simulate receiving a notification
            unsigned char dummy_notification_packet[] = {0x60, 0x01, 0x01, DEVICE_STATE_READY};
            parse_uci_packet(dummy_notification_packet, sizeof(dummy_notification_packet));
        } else if (strcmp(command, "set_power") == 0 || strcmp(command, "device_on") == 0 || strcmp(command, "device_off") == 0) {
            // Handle friendly power commands
            char* power_state = NULL;
            if (strcmp(command, "set_power") == 0) {
                power_state = strtok(NULL, " ");
                if (!power_state) {
                    printf("Usage: set_power <state> (active, ready, sleep)\n");
                    continue;
                }
            } else if (strcmp(command, "device_on") == 0) {
                power_state = "active";
            } else if (strcmp(command, "device_off") == 0) {
                power_state = "ready";
            }
            
            DeviceConfigId cfg_id = DEVICE_STATE;
            unsigned char value;
            
            if (strcmp(power_state, "active") == 0) {
                value = DEVICE_STATE_ACTIVE;
            } else if (strcmp(power_state, "ready") == 0) {
                value = DEVICE_STATE_READY;
            } else if (strcmp(power_state, "sleep") == 0 || strcmp(power_state, "suspend") == 0) {
                // For sleep, we'll send the device suspend command
                unsigned char suspend_payload[] = {0x00}; // Wakeup source
                send_uci_command(COMMAND, 0, CORE, CORE_DEVICE_SUSPEND, suspend_payload, sizeof(suspend_payload));
                continue;
            } else {
                printf("Invalid power state: %s. Use 'active', 'ready', 'sleep', or 'suspend'.\n", power_state);
                continue;
            }
            
            unsigned char payload[] = {0x01, cfg_id, 0x01, value};
            send_uci_command(COMMAND, 0, CORE, CORE_SET_CONFIG, payload, sizeof(payload));
        } else if (strcmp(command, "get_caps_info") == 0) {
            char* config_name = strtok(NULL, " ");
            char* value_name = strtok(NULL, " ");
            if (!config_name || !value_name) {
                printf("Usage: set_config <config_name> <value>\n");
                printf("  Examples:\n");
                printf("    set_config device_state active\n");
                printf("    set_config low_power_mode off\n");
                continue;
            }

            DeviceConfigId cfg_id;
            unsigned char value;

            if (strcmp(config_name, "device_state") == 0) {
                cfg_id = DEVICE_STATE;
                if (strcmp(value_name, "active") == 0) {
                    value = DEVICE_STATE_ACTIVE;
                } else if (strcmp(value_name, "ready") == 0) {
                    value = DEVICE_STATE_READY;
                } else {
                    printf("Invalid value for device_state. Use 'active' or 'ready'.\n");
                    continue;
                }
            } else if (strcmp(config_name, "low_power_mode") == 0) {
                cfg_id = LOW_POWER_MODE;
                if (strcmp(value_name, "on") == 0) {
                    value = 1;
                } else if (strcmp(value_name, "off") == 0) {
                    value = 0;
                } else {
                    printf("Invalid value for low_power_mode. Use 'on' or 'off'.\n");
                    continue;
                }
            } else {
                printf("Unknown config_name: %s\n", config_name);
                continue;
            }

            unsigned char payload[] = {0x01, cfg_id, 0x01, value};
            send_uci_command(COMMAND, 0, CORE, CORE_SET_CONFIG, payload, sizeof(payload));
        } else if (strcmp(command, "get_config") == 0) {
            unsigned char payload[] = {0x01, DEVICE_STATE}; // num_tlvs(1), cfg_id
            send_uci_command(COMMAND, 0, CORE, CORE_GET_CONFIG, payload, sizeof(payload));
        } else if (strcmp(command, "get_device_state") == 0) {
            unsigned char payload[] = {0x01, DEVICE_STATE}; // num_tlvs(1), cfg_id
            send_uci_command(COMMAND, 0, CORE, CORE_GET_CONFIG, payload, sizeof(payload));
        } else if (strcmp(command, "set_device_active") == 0) {
            unsigned char payload[] = {0x01, DEVICE_STATE, 0x01, DEVICE_STATE_ACTIVE}; // num_tlvs(1), cfg_id, length, value
            send_uci_command(COMMAND, 0, CORE, CORE_SET_CONFIG, payload, sizeof(payload));
        } else if (strcmp(command, "set_device_ready") == 0) {
            unsigned char payload[] = {0x01, DEVICE_STATE, 0x01, DEVICE_STATE_READY}; // num_tlvs(1), cfg_id, length, value
            send_uci_command(COMMAND, 0, CORE, CORE_SET_CONFIG, payload, sizeof(payload));
        } else if (strcmp(command, "device_suspend") == 0) {
            unsigned char payload[] = {0x00}; // Wakeup source
            send_uci_command(COMMAND, 0, CORE, CORE_DEVICE_SUSPEND, payload, sizeof(payload));
        } else if (strcmp(command, "session_init") == 0 || strcmp(command, "session_new") == 0) {
            char* session_id_str = strtok(NULL, " ");
            char* session_type_str = strtok(NULL, " ");
            if (!session_id_str || !session_type_str) {
                printf("Usage: session_init <session_id> <session_type>\n");
                printf("  Example: session_init 1 fira_ranging\n");
                printf("  Alternative: session_new 1 ranging\n");
                continue;
            }

            unsigned int session_id = (unsigned int)strtoul(session_id_str, NULL, 10);
            SessionType session_type;

            // Support both technical and friendly names for session types
            if (strcmp(session_type_str, "fira_ranging") == 0 || strcmp(session_type_str, "ranging") == 0) {
                session_type = FIRA_RANGING_SESSION;
            } else {
                printf("Unknown session_type: %s. Use 'fira_ranging' or 'ranging'.\n", session_type_str);
                continue;
            }

            unsigned char payload[5];
            payload[0] = (session_id >> 24) & 0xFF;
            payload[1] = (session_id >> 16) & 0xFF;
            payload[2] = (session_id >> 8) & 0xFF;
            payload[3] = session_id & 0xFF;
            payload[4] = session_type;
            send_uci_command(COMMAND, 0, SESSION_CONFIG, SESSION_INIT, payload, sizeof(payload));
        } else if (strcmp(command, "session_deinit") == 0 || strcmp(command, "session_close") == 0) {
            char* session_id_str = strtok(NULL, " ");
            if (!session_id_str) {
                printf("Usage: session_deinit <session_id>\n");
                continue;
            }
            unsigned int session_id = (unsigned int)strtoul(session_id_str, NULL, 10);
            unsigned char payload[4];
            payload[0] = (session_id >> 24) & 0xFF;
            payload[1] = (session_id >> 16) & 0xFF;
            payload[2] = (session_id >> 8) & 0xFF;
            payload[3] = session_id & 0xFF;
            send_uci_command(COMMAND, 0, SESSION_CONFIG, SESSION_DEINIT, payload, sizeof(payload));
        } else if (strcmp(command, "session_start") == 0 || strcmp(command, "start_ranging") == 0) {
            char* session_id_str = strtok(NULL, " ");
            if (!session_id_str) {
                printf("Usage: session_start <session_id>\n");
                printf("  Alternative: start_ranging <session_id>\n");
                continue;
            }
            unsigned int session_id = (unsigned int)strtoul(session_id_str, NULL, 10);
            unsigned char payload[4];
            payload[0] = (session_id >> 24) & 0xFF;
            payload[1] = (session_id >> 16) & 0xFF;
            payload[2] = (session_id >> 8) & 0xFF;
            payload[3] = session_id & 0xFF;
            send_uci_command(COMMAND, 0, SESSION_CONTROL, SESSION_START, payload, sizeof(payload));
        } else if (strcmp(command, "session_stop") == 0 || strcmp(command, "stop_ranging") == 0) {
            char* session_id_str = strtok(NULL, " ");
            if (!session_id_str) {
                printf("Usage: session_stop <session_id>\n");
                printf("  Alternative: stop_ranging <session_id>\n");
                continue;
            }
            unsigned int session_id = (unsigned int)strtoul(session_id_str, NULL, 10);
            unsigned char payload[4];
            payload[0] = (session_id >> 24) & 0xFF;
            payload[1] = (session_id >> 16) & 0xFF;
            payload[2] = (session_id >> 8) & 0xFF;
            payload[3] = session_id & 0xFF;
            send_uci_command(COMMAND, 0, SESSION_CONTROL, SESSION_STOP, payload, sizeof(payload));
        } else if (strcmp(command, "get_session_state") == 0 || strcmp(command, "session_status") == 0) {
            char* session_id_str = strtok(NULL, " ");
            if (!session_id_str) {
                printf("Usage: get_session_state <session_id>\n");
                printf("  Alternative: session_status <session_id>\n");
                continue;
            }
            unsigned int session_id = (unsigned int)strtoul(session_id_str, NULL, 10);
            unsigned char payload[4];
            payload[0] = (session_id >> 24) & 0xFF;
            payload[1] = (session_id >> 16) & 0xFF;
            payload[2] = (session_id >> 8) & 0xFF;
            payload[3] = session_id & 0xFF;
            send_uci_command(COMMAND, 0, SESSION_CONFIG, SESSION_GET_STATE, payload, sizeof(payload));
        } else if (strcmp(command, "set_app_config") == 0) {
            char* session_id_str = strtok(NULL, " ");
            char* config_name = strtok(NULL, " ");
            char* value_str = strtok(NULL, " ");
            if (!session_id_str || !config_name || !value_str) {
                printf("Usage: set_app_config <session_id> <config_name> <value>\n");
                printf("  Example: set_app_config 1 device_type responder\n");
                printf("  Examples: set_app_config 1 ranging_usage ranging\n");
                printf("            set_app_config 1 sts_config dynamic\n");
                printf("            set_app_config 1 multi_node_mode unicast\n");
                printf("            set_app_config 1 channel 5\n");
                printf("            set_app_config 1 device_role controller\n");
                continue;
            }

            unsigned int session_id = (unsigned int)strtoul(session_id_str, NULL, 10);
            AppConfigTlvType cfg_id;
            unsigned char value;
            int found = 0; // Flag to track if config name was found

            if (strcmp(config_name, "device_type") == 0) {
                cfg_id = DEVICE_TYPE;
                if (strcmp(value_str, "responder") == 0) {
                    value = 0x01;
                } else if (strcmp(value_str, "initiator") == 0) {
                    value = 0x02;
                } else {
                    printf("Invalid value for device_type. Use 'responder' or 'initiator'.\n");
                    continue;
                }
                found = 1;
            } else if (strcmp(config_name, "ranging_usage") == 0 || strcmp(config_name, "ranging_round_usage") == 0) {
                cfg_id = RANGING_ROUND_USAGE;
                if (strcmp(value_str, "ranging") == 0) {
                    value = 0x00;
                } else if (strcmp(value_str, "data") == 0) {
                    value = 0x01;
                } else {
                    printf("Invalid value for ranging_usage. Use 'ranging' or 'data'.\n");
                    continue;
                }
                found = 1;
            } else if (strcmp(config_name, "sts_config") == 0) {
                cfg_id = STS_CONFIG;
                if (strcmp(value_str, "static") == 0) {
                    value = 0x00;
                } else if (strcmp(value_str, "dynamic") == 0) {
                    value = 0x01;
                } else {
                    printf("Invalid value for sts_config. Use 'static' or 'dynamic'.\n");
                    continue;
                }
                found = 1;
            } else if (strcmp(config_name, "multi_node_mode") == 0) {
                cfg_id = MULTI_NODE_MODE;
                if (strcmp(value_str, "unicast") == 0) {
                    value = 0x00;
                } else if (strcmp(value_str, "anycast") == 0) {
                    value = 0x01;
                } else if (strcmp(value_str, "multicast") == 0) {
                    value = 0x02;
                } else {
                    printf("Invalid value for multi_node_mode. Use 'unicast', 'anycast', or 'multicast'.\n");
                    continue;
                }
                found = 1;
            } else if (strcmp(config_name, "channel") == 0 || strcmp(config_name, "channel_number") == 0) {
                cfg_id = CHANNEL_NUMBER;
                // Allow numeric values for channel number
                char *endptr;
                long channel_val = strtol(value_str, &endptr, 10);
                if (*endptr != '\0') {
                    printf("Invalid value for channel. Use a numeric value.\n");
                    continue;
                }
                if (channel_val < 0 || channel_val > 255) {
                    printf("Invalid value for channel. Use 0-255.\n");
                    continue;
                }
                value = (unsigned char)channel_val;
                found = 1;
            } else if (strcmp(config_name, "device_role") == 0) {
                cfg_id = DEVICE_ROLE;
                if (strcmp(value_str, "controller") == 0) {
                    value = 0x00;
                } else if (strcmp(value_str, "controlee") == 0) {
                    value = 0x01;
                } else {
                    printf("Invalid value for device_role. Use 'controller' or 'controlee'.\n");
                    continue;
                }
                found = 1;
            } else if (strcmp(config_name, "aoa_request") == 0 || strcmp(config_name, "aoa_result_req") == 0) {
                cfg_id = AOA_RESULT_REQ;
                if (strcmp(value_str, "disable") == 0 || strcmp(value_str, "off") == 0) {
                    value = 0x00;
                } else if (strcmp(value_str, "enable") == 0 || strcmp(value_str, "on") == 0) {
                    value = 0x01;
                } else {
                    printf("Invalid value for aoa_request. Use 'enable'/'on' or 'disable'/'off'.\n");
                    continue;
                }
                found = 1;
            } else if (strcmp(config_name, "scheduled_mode") == 0) {
                cfg_id = SCHEDULED_MODE;
                if (strcmp(value_str, "cont") == 0 || strcmp(value_str, "continuous") == 0) {
                    value = 0x00;
                } else if (strcmp(value_str, "scheduled") == 0) {
                    value = 0x01;
                } else {
                    printf("Invalid value for scheduled_mode. Use 'cont'/'continuous' or 'scheduled'.\n");
                    continue;
                }
                found = 1;
            }

            if (!found) {
                printf("Unknown config_name: %s\n", config_name);
                printf("Supported config names: device_type, ranging_usage, sts_config, multi_node_mode, channel, device_role, aoa_request, scheduled_mode\n");
                continue;
            }

            unsigned char payload[8];
            payload[0] = (session_id >> 24) & 0xFF;
            payload[1] = (session_id >> 16) & 0xFF;
            payload[2] = (session_id >> 8) & 0xFF;
            payload[3] = session_id & 0xFF;
            payload[4] = 1; // Number of TLVs
            payload[5] = cfg_id;
            payload[6] = 1; // Length
            payload[7] = value;
            send_uci_command(COMMAND, 0, SESSION_CONFIG, SESSION_SET_APP_CONFIG, payload, sizeof(payload));
        } else if (strcmp(command, "get_app_config") == 0) {
            char* session_id_str = strtok(NULL, " ");
            if (!session_id_str) {
                printf("Usage: get_app_config <session_id> <config_name_1> [config_name_2]...\n");
                continue;
            }
            unsigned int session_id = (unsigned int)strtoul(session_id_str, NULL, 10);

            unsigned char payload[MAX_PAYLOAD_LENGTH];
            payload[0] = (session_id >> 24) & 0xFF;
            payload[1] = (session_id >> 16) & 0xFF;
            payload[2] = (session_id >> 8) & 0xFF;
            payload[3] = session_id & 0xFF;

            int num_configs = 0;
            char* config_name;
            while ((config_name = strtok(NULL, " ")) != NULL) {
                int valid_config = 0;
                if (strcmp(config_name, "device_type") == 0) {
                    payload[5 + num_configs] = DEVICE_TYPE;
                    num_configs++;
                    valid_config = 1;
                } else if (strcmp(config_name, "ranging_usage") == 0 || strcmp(config_name, "ranging_round_usage") == 0) {
                    payload[5 + num_configs] = RANGING_ROUND_USAGE;
                    num_configs++;
                    valid_config = 1;
                } else if (strcmp(config_name, "sts_config") == 0) {
                    payload[5 + num_configs] = STS_CONFIG;
                    num_configs++;
                    valid_config = 1;
                } else if (strcmp(config_name, "multi_node_mode") == 0) {
                    payload[5 + num_configs] = MULTI_NODE_MODE;
                    num_configs++;
                    valid_config = 1;
                } else if (strcmp(config_name, "channel") == 0 || strcmp(config_name, "channel_number") == 0) {
                    payload[5 + num_configs] = CHANNEL_NUMBER;
                    num_configs++;
                    valid_config = 1;
                } else if (strcmp(config_name, "device_role") == 0) {
                    payload[5 + num_configs] = DEVICE_ROLE;
                    num_configs++;
                    valid_config = 1;
                } else if (strcmp(config_name, "aoa_request") == 0 || strcmp(config_name, "aoa_result_req") == 0) {
                    payload[5 + num_configs] = AOA_RESULT_REQ;
                    num_configs++;
                    valid_config = 1;
                } else if (strcmp(config_name, "scheduled_mode") == 0) {
                    payload[5 + num_configs] = SCHEDULED_MODE;
                    num_configs++;
                    valid_config = 1;
                }
                if (!valid_config) {
                    printf("Unknown config_name: %s\n", config_name);
                    printf("Supported config names: device_type, ranging_usage, sts_config, multi_node_mode, channel, device_role, aoa_request, scheduled_mode\n");
                }
            }

            if (num_configs == 0) {
                printf("No valid config names provided.\n");
                continue;
            }

            payload[4] = num_configs;
            send_uci_command(COMMAND, 0, SESSION_CONFIG, SESSION_GET_APP_CONFIG, payload, 5 + num_configs);
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
                set_header_values(ntf_header, NOTIFICATION, COMPLETE, CORE, CORE_DEVICE_STATUS_NTF, 1);
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
            set_header_values(ntf_header, NOTIFICATION, COMPLETE, SESSION_CONFIG, SESSION_STATUS_NTF, 6);
            notification_packet[sizeof(struct uci_packet_header)] = (session_id >> 24) & 0xFF;
            notification_packet[sizeof(struct uci_packet_header) + 1] = (session_id >> 16) & 0xFF;
            notification_packet[sizeof(struct uci_packet_header) + 2] = (session_id >> 8) & 0xFF;
            notification_packet[sizeof(struct uci_packet_header) + 3] = session_id & 0xFF;
            notification_packet[sizeof(struct uci_packet_header) + 4] = session_state;
            notification_packet[sizeof(struct uci_packet_header) + 5] = reason_code;
            parse_uci_packet(notification_packet, sizeof(struct uci_packet_header) + 6);
        } else if (strcmp(command, "simulate_data_credit") == 0) {
            // Simulate a data credit notification
            unsigned char notification_packet[sizeof(struct uci_packet_header) + 5];
            struct uci_packet_header* ntf_header = (struct uci_packet_header*)notification_packet;
            set_header_values(ntf_header, NOTIFICATION, COMPLETE, SESSION_CONTROL, SESSION_DATA_CREDIT_NTF, 5);
            notification_packet[sizeof(struct uci_packet_header)] = 0x01;
            notification_packet[sizeof(struct uci_packet_header) + 1] = 0x02;
            notification_packet[sizeof(struct uci_packet_header) + 2] = 0x03;
            notification_packet[sizeof(struct uci_packet_header) + 3] = 0x04;
            notification_packet[sizeof(struct uci_packet_header) + 4] = 0x01; // credit available
            parse_uci_packet(notification_packet, sizeof(struct uci_packet_header) + 5);
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
            set_header_values(ntf_header1, NOTIFICATION, COMPLETE, SESSION_CONFIG, SESSION_STATUS_NTF, 6);
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
            set_header_values(ntf_header2, NOTIFICATION, COMPLETE, SESSION_CONFIG, SESSION_STATUS_NTF, 6);
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
            set_header_values(ntf_header3, NOTIFICATION, COMPLETE, SESSION_CONTROL, SESSION_DATA_CREDIT_NTF, 5);
            ntf_packet3[sizeof(struct uci_packet_header)] = 0x01; // session token
            ntf_packet3[sizeof(struct uci_packet_header) + 1] = 0x02;
            ntf_packet3[sizeof(struct uci_packet_header) + 2] = 0x03;
            ntf_packet3[sizeof(struct uci_packet_header) + 3] = 0x04;
            ntf_packet3[sizeof(struct uci_packet_header) + 4] = 0x01; // credit available
            parse_uci_packet(ntf_packet3, sizeof(struct uci_packet_header) + 5);
            
            printf("\n=== Session Flow Demonstration Complete ===\n");
        } else if (strcmp(command, "simulate_ranging") == 0) {
            printf("=== Simulating UWB Ranging Notification ===\n");
            
            // Create a simulated two-way ranging measurement for a nearby device
            unsigned char ranging_ntf_payload[] = {
                // Header fields (24 bytes total)
                0x00, 0x00, 0x00, 0x01,  // Sequence number: 1
                0x01, 0x02, 0x03, 0x04,  // Session token: 0x01020304
                0x01,                    // RCR indicator
                0x00, 0x00, 0x00, 0x64,  // Current ranging interval: 100ms
                0x01,                    // Ranging measurement type: TWO_WAY (0x01)
                0x00,                    // Reserved
                0x00,                    // MAC address indicator: SHORT_ADDRESS (0x00)
                0x00, 0x00, 0x00, 0x00,  // HUS primary session ID: 0x00000000
                
                // Two-Way measurement data
                0x01,                    // Number of measurements: 1
                
                // First measurement (SHORT ADDRESS)
                0x12, 0x34,              // MAC Address: 0x1234
                0x00,                    // Status: OK
                0x00,                    // NLOS: NO
                0x00, 0x64,              // Distance: 100 cm (1 meter)
                0x00, 0x14,              // AoA Azimuth: 20 degrees
                0x08,                    // AoA Azimuth FoM: 8 (medium confidence)
                0x00, 0x05,              // AoA Elevation: 5 degrees
                0x07,                    // AoA Elevation FoM: 7 (high confidence)
                0x00, 0x10,              // Destination AoA Azimuth: 16 degrees
                0x06,                    // Destination AoA Azimuth FoM: 6 (medium-high confidence)
                0x00, 0x03,              // Destination AoA Elevation: 3 degrees
                0x09,                    // Destination AoA Elevation FoM: 9 (very high confidence)
                0x02,                    // Slot Index: 2
                0xE0                     // RSSI: -32 dBm (strong signal)
            };
            
            unsigned char notification_packet[sizeof(struct uci_packet_header) + sizeof(ranging_ntf_payload)];
            struct uci_packet_header* ntf_header = (struct uci_packet_header*)notification_packet;
            set_header_values(ntf_header, NOTIFICATION, COMPLETE, SESSION_CONTROL, SESSION_INFO_NTF, sizeof(ranging_ntf_payload));
            memcpy(notification_packet + sizeof(struct uci_packet_header), ranging_ntf_payload, sizeof(ranging_ntf_payload));
            parse_uci_packet(notification_packet, sizeof(struct uci_packet_header) + sizeof(ranging_ntf_payload));
            
            printf("=== Ranging Simulation Complete ===\n");
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
            set_header_values(ntf_header, NOTIFICATION, COMPLETE, SESSION_CONTROL, SESSION_INFO_NTF, sizeof(multi_ranging_ntf_payload));
            memcpy(notification_packet + sizeof(struct uci_packet_header), multi_ranging_ntf_payload, sizeof(multi_ranging_ntf_payload));
            parse_uci_packet(notification_packet, sizeof(struct uci_packet_header) + sizeof(multi_ranging_ntf_payload));
            
            printf("=== Multi-Target Ranging Simulation Complete ===\n");
        } else if (strcmp(command, "analyze_packet") == 0) {
            // Parse hex bytes from command line
            char* hex_byte_str = strtok(NULL, " ");
            if (!hex_byte_str) {
                printf("Usage: analyze_packet <hex_bytes...>\n");
                printf("Example: analyze_packet 20 08 00 00\n");
                printf("         analyze_packet 21 00 00 05 00 00 00 01 00\n");
                continue;
            }
            
            unsigned char packet[256];
            int packet_len = 0;
            
            // Parse hex bytes
            do {
                if (packet_len >= (int)sizeof(packet)) {
                    printf("Error: Packet too long (max %zu bytes)\n", sizeof(packet));
                    break;
                }
                char* endptr;
                unsigned long value = strtoul(hex_byte_str, &endptr, 16);
                if (*endptr != '\0' || value > 0xFF) {
                    printf("Error: Invalid hex byte '%s'\n", hex_byte_str);
                    break;
                }
                packet[packet_len++] = (unsigned char)value;
            } while ((hex_byte_str = strtok(NULL, " ")) != NULL);
            
            if (packet_len > 0) {
                printf("Analyzing UCI packet (%d bytes):\n", packet_len);
                for (int i = 0; i < packet_len; i++) {
                    printf("%02X ", packet[i]);
                }
                printf("\n\n");
                analyze_uci_packet(packet, packet_len);
            }
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
            printf("General Commands:\n");
            printf("  help                      - Show this help information\n");
            printf("  quit                      - Exit the shell\n");
            printf("  history                   - Show command history\n");
            printf("  complete <prefix>         - Autocomplete a command or parameter\n");
            printf("  alias <name> [command]    - Create or list command aliases\n");
            printf("  unalias <name>            - Remove an alias\n\n");
            
            printf("Device Management Commands:\n");
            printf("  get_device_info, device_info     - Get device information\n");
            printf("  device_reset                     - Reset the device\n");
            printf("  get_caps_info                    - Get device capabilities information\n");
            printf("  set_config <param> <value>       - Set device configuration\n");
            printf("  get_config <param>               - Get device configuration\n");
            printf("  get_device_state                 - Get current device state\n");
            printf("  set_device_active                - Set device to ACTIVE state\n");
            printf("  set_device_ready                 - Set device to READY state\n");
            printf("  device_suspend                   - Suspend the device\n");
            printf("  set_power <state>                - Set device power state\n");
            printf("  device_on                        - Turn device on (ACTIVE state)\n");
            printf("  device_off                       - Turn device off (READY state)\n\n");
            
            printf("Session Management Commands:\n");
            printf("  session_init <id> <type>         - Initialize a ranging session\n");
            printf("  session_new <id> <type>          - Alias for session_init\n");
            printf("  session_deinit <id>              - Deinitialize a ranging session\n");
            printf("  session_close <id>               - Alias for session_deinit\n");
            printf("  session_start <id>               - Start a ranging session\n");
            printf("  start_ranging <id>               - Alias for session_start\n");
            printf("  session_stop <id>                - Stop a ranging session\n");
            printf("  stop_ranging <id>                - Alias for session_stop\n");
            printf("  get_session_state <id>           - Get session state\n");
            printf("  session_status <id>              - Alias for get_session_state\n");
            printf("  get_session_count                - Get number of active sessions\n\n");
            
            printf("Application Configuration Commands:\n");
            printf("  set_app_config <id> <param> <value>  - Set session app configuration\n");
            printf("  get_app_config <id> <param>          - Get session app configuration\n\n");
            
            printf("Hardware Mode Commands:\n");
            printf("  hw_init <device_path>            - Initialize hardware mode\n");
            printf("  hw_connect <device_path>         - Connect to hardware device\n");
            printf("  hw_info                      - Get hardware device information\n");
            printf("  hw_send <mt> <pbf> <gid> <oid> [payload...]  - Send raw command\n");
            printf("  hw_send_raw <bytes...>           - Send raw hex bytes\n");
            printf("  hw_get_device_info, hw_device_info   - Get device info in hardware mode\n");
            printf("  hw_device_reset                  - Reset device in hardware mode\n");
            printf("  hw_get_caps_info                 - Get capabilities in hardware mode\n");
            printf("  hw_set_config <param> <value>    - Set config in hardware mode\n");
            printf("  hw_get_config <param>            - Get config in hardware mode\n");
            printf("  hw_get_device_state              - Get device state in hardware mode\n");
            printf("  hw_set_device_active             - Set device active in hardware mode\n");
            printf("  hw_set_device_ready              - Set device ready in hardware mode\n");
            printf("  hw_device_suspend                - Suspend device in hardware mode\n");
            printf("  hw_session_init <id> <type>      - Initialize session in hardware mode\n");
            printf("  hw_session_new <id> <type>       - Alias for hw_session_init\n");
            printf("  hw_session_deinit <id>           - Deinitialize session in hardware mode\n");
            printf("  hw_session_close <id>            - Alias for hw_session_deinit\n");
            printf("  hw_session_start <id>            - Start session in hardware mode\n");
            printf("  hw_start_ranging <id>            - Alias for hw_session_start\n");
            printf("  hw_session_stop <id>             - Stop session in hardware mode\n");
            printf("  hw_stop_ranging <id>             - Alias for hw_session_stop\n");
            printf("  hw_get_session_state <id>        - Get session state in hardware mode\n");
            printf("  hw_session_status <id>           - Alias for hw_get_session_state\n");
            printf("  hw_set_app_config <id> <param> <value>  - Set app config in hardware mode\n");
            printf("  hw_get_app_config <id> <param>   - Get app config in hardware mode\n\n");
            
            printf("Simulation & Testing Commands:\n");
            printf("  simulate_notification            - Simulate a device status notification\n");
            printf("  simulate_session_status          - Simulate a session status notification\n");
            printf("  simulate_data_credit             - Simulate a data credit notification\n");
            printf("  simulate_ranging                 - Simulate ranging measurements\n");
            printf("  simulate_multi_target_ranging    - Simulate multi-target ranging\n");
            printf("  demo_session_flow                - Demonstrate a complete session flow\n");
            printf("  analyze_packet <bytes...>        - Analyze hex packet bytes\n\n");
            
            printf("For complete documentation, see:\n");
            printf("  - README.md - Project overview and usage\n");
            printf("  - FINAL_SUMMARY.md - Complete feature summary and technical details\n");
            printf("  - uci_analysis/ - Detailed UCI protocol analysis based on Android UWB specification\n");
        } else {
            ui_print_command_not_found(command);
        }

        // Note: In a real implementation, notifications would arrive asynchronously from the UWB device
        // Here we're simulating synchronous processing, so we don't need periodic dummy notifications
    }

    return 0;
}
