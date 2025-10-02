#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
        } else {
            printf("Unknown command: %s\n", command);
        }
    }

    return 0;
}

