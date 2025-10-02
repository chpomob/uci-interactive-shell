#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> // For sleep

#include "uci.h"
#include "uci_functions.h"

#define MAX_LINE_LENGTH 256
#define MAX_PAYLOAD_LENGTH 255

int main() {
    char line[MAX_LINE_LENGTH];

    printf("UCI Interactive Shell\n");
    printf("Enter 'quit' to exit.\n");

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

        if (strcmp(command, "send") == 0) {
            unsigned char mt = (unsigned char)strtol(strtok(NULL, " "), NULL, 16);
            unsigned char gid = (unsigned char)strtol(strtok(NULL, " "), NULL, 16);
            unsigned char oid = (unsigned char)strtol(strtok(NULL, " "), NULL, 16);
            unsigned char payload[MAX_PAYLOAD_LENGTH];
            int payload_len = 0;

            char* token;
            while ((token = strtok(NULL, " ")) != NULL) {
                payload[payload_len++] = (unsigned char)strtol(token, NULL, 16);
            }

            send_uci_command(mt, 0, gid, oid, payload, payload_len);
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
            unsigned char payload[] = {0x01, DEVICE_STATE, 0x01, DEVICE_STATE_ACTIVE}; // num_tlvs(1), cfg_id, length, value
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
        } else {
            printf("Unknown command: %s\n", command);
        }

        // Note: In a real implementation, notifications would arrive asynchronously from the UWB device
        // Here we're simulating synchronous processing, so we don't need periodic dummy notifications
    }

    return 0;
}

