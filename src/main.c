#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> // For sleep

#include "uci.h"
#include "uci_functions.h"
#include "uci_hw.h"
#include "uci_hw_interface.h"
#include "uci_hw_chardev.h"

#define MAX_LINE_LENGTH 256
#define MAX_PAYLOAD_LENGTH 255

// Global variables for hardware mode
static int g_hardware_mode = 0;  // Flag to track if hardware mode is enabled
static uci_hw_chardev_t g_uwb_chardev;  // Character device interface for UWB communication

// Global character device interface for UWB communication
static uci_hw_chardev_t g_uwb_chardev;

int main() {
    char line[MAX_LINE_LENGTH];

    printf("UCI Interactive Shell\n");
    printf("Enter 'quit' to exit.\n");
    printf("Commands: send, get_device_info, device_reset, get_caps_info, set_config, get_config,\n");
    printf("          get_device_state, set_device_active, set_device_ready, device_suspend,\n");
    printf("          session_init, session_deinit, session_start, session_stop, get_session_state,\n");
    printf("          set_app_config, get_app_config,\n");
    printf("          simulate_notification, simulate_session_status, simulate_data_credit,\n");
    printf("          simulate_ranging, simulate_multi_target_ranging, demo_session_flow,\n");
    printf("          hw_init <device_path> - Initialize hardware mode\n");
    printf("          hw_send <mt> <gid> <oid> [payload_bytes...] - Send command in hardware mode\n");
    printf("\n");
    printf("For complete documentation, see:\n");
    printf("  - README.md - Project overview and usage\n");
    printf("  - FINAL_SUMMARY.md - Complete feature summary and technical details\n");
    printf("  - uci_analysis/ - Detailed UCI protocol analysis based on Android UWB specification\n");

    while (1) {
        printf("> ");
        if (fgets(line, sizeof(line), stdin) == NULL) {
            break;
        }

        // Remove trailing newline
        line[strcspn(line, "\r\n")] = 0;

        if (strcmp(line, "quit") == 0) {
            break;
        }

        char* command = strtok(line, " ");

        if (strcmp(command, "hw_init") == 0) {
            char* device_path = strtok(NULL, " ");
            if (!device_path) {
                printf("Usage: hw_init <device_path>\n");
                continue;
            }
            
            // Initialize both the legacy hardware interface and the new character device interface
            if (uci_hw_interface_init(device_path) == 0) {
                g_hardware_mode = 1;
                printf("Hardware mode initialized successfully with device: %s\n", device_path);
                
                // Also initialize the character device interface
                if (uci_hw_chardev_init(&g_uwb_chardev, device_path) == 0) {
                    if (uci_hw_chardev_open(&g_uwb_chardev) == 0) {
                        printf("Character device interface initialized successfully\n");
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
                    printf("Error: Packet too long (max %zu bytes)\n", sizeof(packet));
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
                printf("Hardware mode not initialized. Use 'hw_init <device_path>' first.\n");
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
                printf("Hardware mode initialized successfully with device: %s\n", device_path);
                printf("Ready to communicate with real UWB hardware!\n");
            } else {
                printf("Failed to initialize hardware mode with device: %s\n", device_path);
            }
        } else if (strcmp(command, "get_device_info") == 0) {
            send_uci_command(COMMAND, 0, CORE, CORE_DEVICE_INFO, NULL, 0);
        } else if (strcmp(command, "device_reset") == 0) {
            unsigned char payload[] = {UWBS_RESET};
            send_uci_command(COMMAND, 0, CORE, CORE_DEVICE_RESET, payload, sizeof(payload));
            // Simulate receiving a notification
            unsigned char dummy_notification_packet[] = {0x60, 0x01, 0x01, DEVICE_STATE_READY};
            parse_uci_packet(dummy_notification_packet, sizeof(dummy_notification_packet));
        } else if (strcmp(command, "get_caps_info") == 0) {
            send_uci_command(COMMAND, 0, CORE, CORE_GET_CAPS_INFO, NULL, 0);
        } else if (strcmp(command, "set_config") == 0) {
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
        } else if (strcmp(command, "session_init") == 0) {
            unsigned char payload[] = {0x01, 0x02, 0x03, 0x04, FIRA_RANGING_SESSION};
            send_uci_command(COMMAND, 0, SESSION_CONFIG, SESSION_INIT, payload, sizeof(payload));
        } else if (strcmp(command, "session_deinit") == 0) {
            unsigned char payload[] = {0x01, 0x02, 0x03, 0x04};
            send_uci_command(COMMAND, 0, SESSION_CONFIG, SESSION_DEINIT, payload, sizeof(payload));
        } else if (strcmp(command, "session_start") == 0) {
            unsigned char payload[] = {0x01, 0x02, 0x03, 0x04};
            send_uci_command(COMMAND, 0, SESSION_CONTROL, SESSION_START, payload, sizeof(payload));
        } else if (strcmp(command, "session_stop") == 0) {
            unsigned char payload[] = {0x01, 0x02, 0x03, 0x04};
            send_uci_command(COMMAND, 0, SESSION_CONTROL, SESSION_STOP, payload, sizeof(payload));
        } else if (strcmp(command, "get_session_state") == 0) {
            unsigned char payload[] = {0x01, 0x02, 0x03, 0x04};  // Session ID
            send_uci_command(COMMAND, 0, SESSION_CONFIG, SESSION_GET_STATE, payload, sizeof(payload));
        } else if (strcmp(command, "set_app_config") == 0) {
            // Set device type (0x00) to responder (0x01) with 1 byte value
            unsigned char payload[] = {0x01, 0x02, 0x03, 0x04, 0x01, 0x00, 0x01, 0x01};  // session_id + num_tlvs + cfg_id + len + value
            send_uci_command(COMMAND, 0, SESSION_CONFIG, SESSION_SET_APP_CONFIG, payload, sizeof(payload));
        } else if (strcmp(command, "get_app_config") == 0) {
            // Get device type configuration
            unsigned char payload[] = {0x01, 0x02, 0x03, 0x04, 0x01, 0x00};  // session_id + num_tlvs + cfg_id
            send_uci_command(COMMAND, 0, SESSION_CONFIG, SESSION_GET_APP_CONFIG, payload, sizeof(payload));
        } else if (strcmp(command, "simulate_notification") == 0) {
            // Simulate a device status notification
            unsigned char notification_packet[sizeof(struct uci_packet_header) + 1];
            struct uci_packet_header* ntf_header = (struct uci_packet_header*)notification_packet;
            set_header_values(ntf_header, NOTIFICATION, COMPLETE, CORE, CORE_DEVICE_STATUS_NTF, 1);
            notification_packet[sizeof(struct uci_packet_header)] = DEVICE_STATE_ACTIVE;
            parse_uci_packet(notification_packet, sizeof(struct uci_packet_header) + 1);
        } else if (strcmp(command, "simulate_session_status") == 0) {
            // Simulate a session status notification
            unsigned char notification_packet[sizeof(struct uci_packet_header) + 6];
            struct uci_packet_header* ntf_header = (struct uci_packet_header*)notification_packet;
            set_header_values(ntf_header, NOTIFICATION, COMPLETE, SESSION_CONFIG, SESSION_STATUS_NTF, 6);
            notification_packet[sizeof(struct uci_packet_header)] = 0x01;
            notification_packet[sizeof(struct uci_packet_header) + 1] = 0x02;
            notification_packet[sizeof(struct uci_packet_header) + 2] = 0x03;
            notification_packet[sizeof(struct uci_packet_header) + 3] = 0x04;
            notification_packet[sizeof(struct uci_packet_header) + 4] = SESSION_STATE_ACTIVE;
            notification_packet[sizeof(struct uci_packet_header) + 5] = STATE_CHANGE_WITH_SESSION_MANAGEMENT_COMMANDS;
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
        } else {
            printf("Unknown command: %s\n", command);
        }

        // Note: In a real implementation, notifications would arrive asynchronously from the UWB device
        // Here we're simulating synchronous processing, so we don't need periodic dummy notifications
    }

    return 0;
}

