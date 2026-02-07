#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include "../include/uci.h"
#include "../include/uci_functions.h"
#include "../include/uci_hw_interface.h"
#include "../include/uci_hw_chardev.h"
#include "../include/uci_ui.h"
#include "../include/uci_standardized_error_handling.h"

#define MAX_PAYLOAD_LENGTH 255

// Static reference to hardware mode variables passed from main
static int* g_hw_mode_ptr = NULL;
static uci_hw_chardev_t* g_chardev_ptr = NULL;

static int parse_u8_token(const char* token, int base, unsigned char* out_value) {
    char* endptr = NULL;
    long parsed = 0;

    if (!token || !out_value || *token == '\0') {
        return -1;
    }

    errno = 0;
    parsed = strtol(token, &endptr, base);
    if (errno != 0 || endptr == token || *endptr != '\0' || parsed < 0 || parsed > 255) {
        return -1;
    }

    *out_value = (unsigned char)parsed;
    return 0;
}

void uci_cmd_hardware_init(int* hw_mode, uci_hw_chardev_t* chardev) {
    g_hw_mode_ptr = hw_mode;
    g_chardev_ptr = chardev;
}

int handle_hw_init_command(char* device_path) {
    if (!device_path) {
        printf("Usage: hw_init <device_path>\n");
        UCI_LOG_ERROR("device_path is NULL", UCI_ERROR_INVALID_PARAM);
        return -1;
    }

    if (!g_hw_mode_ptr || !g_chardev_ptr) {
        printf("Error: Hardware command module not initialized\n");
        UCI_LOG_ERROR("Hardware command module not initialized", UCI_ERROR_INVALID_PARAM);
        return -1;
    }

    // Initialize both the legacy hardware interface and the new character device interface
    if (uci_hw_interface_init(device_path) == 0) {
        *g_hw_mode_ptr = 1;
        ui_print_hardware_mode_initialized(device_path);

        // Also initialize the character device interface
        if (uci_hw_chardev_init(g_chardev_ptr, device_path) == 0) {
            if (uci_hw_chardev_open(g_chardev_ptr) == 0) {
                ui_print_success("Character device interface initialized successfully");
            } else {
                ui_print_warning("Failed to open character device interface");
                UCI_LOG_ERROR("Failed to open character device interface", UCI_ERROR_INVALID_PARAM);
            }
        } else {
            ui_print_warning("Failed to initialize character device interface");
            UCI_LOG_ERROR("Failed to initialize character device interface", UCI_ERROR_INVALID_PARAM);
        }
        return 0;
    } else {
        ui_print_error("Failed to initialize hardware mode");
        UCI_LOG_ERROR("Failed to initialize hardware mode", UCI_ERROR_INVALID_PARAM);
        return -1;
    }
}

int handle_hw_send_command(char* mt_str,
                           char* pbf_str,
                           char* gid_str,
                           char* oid_str,
                           char** payload_tokens,
                           int payload_count) {
    if (!g_hw_mode_ptr || !*g_hw_mode_ptr) {
        ui_print_error("Hardware mode not enabled");
        UCI_LOG_ERROR("Hardware mode not enabled", UCI_ERROR_INVALID_PARAM);
        return -1;
    }

    if (!uci_hw_interface_is_connected()) {
        ui_print_error("Hardware not connected. Use 'hw_connect <device_path>' first.");
        UCI_LOG_ERROR("Hardware not connected", UCI_ERROR_INVALID_PARAM);
        return -1;
    }

    if (!mt_str || !pbf_str || !gid_str || !oid_str) {
        printf("Usage: hw_send <mt> <pbf> <gid> <oid> [payload_bytes...]\n");
        printf("  Example: hw_send 01 00 00 02 (send CORE_DEVICE_INFO command)\n");
        printf("  MT: 01=COMMAND, 02=RESPONSE, 03=NOTIFICATION\n");
        printf("  PBF: 00=COMPLETE, 01=START, 02=CONT, 03=END\n");
        printf("  GID: 00=CORE, 01=SESSION_CONFIG, 02=SESSION_CONTROL\n");
        printf("  OID: Command opcode (depends on GID)\n");
        UCI_LOG_ERROR("Missing required parameters", UCI_ERROR_INVALID_PARAM);
        return -1;
    }

    unsigned char mt = 0;
    unsigned char pbf = 0;
    unsigned char gid = 0;
    unsigned char oid = 0;
    if (parse_u8_token(mt_str, 16, &mt) != 0 ||
        parse_u8_token(pbf_str, 16, &pbf) != 0 ||
        parse_u8_token(gid_str, 16, &gid) != 0 ||
        parse_u8_token(oid_str, 16, &oid) != 0) {
        printf("Invalid command byte values. Expected hex bytes (00-FF).\n");
        UCI_LOG_ERROR("Invalid command byte values", UCI_ERROR_INVALID_PARAM);
        return -1;
    }

    unsigned char payload[MAX_PAYLOAD_LENGTH];
    int payload_len = 0;

    if (payload_tokens != NULL) {
        for (int i = 0; i < payload_count && i < MAX_PAYLOAD_LENGTH; i++) {
            unsigned char parsed_payload = 0;
            if (parse_u8_token(payload_tokens[i], 16, &parsed_payload) != 0) {
                printf("Invalid hex value: %s\n", payload_tokens[i]);
                UCI_LOG_ERROR("Invalid hex value in payload", UCI_ERROR_INVALID_PARAM);
                return -1;
            }
            payload[payload_len++] = parsed_payload;
        }
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
            ui_print_warning("No response received from hardware (timeout)");
        } else {
            ui_print_error("Error receiving response from hardware");
            UCI_LOG_ERROR("Error receiving response from hardware", UCI_ERROR_INVALID_PARAM);
        }
        return 0;
    } else {
        ui_print_error("Failed to send command to hardware");
        UCI_LOG_ERROR("Failed to send command to hardware", UCI_ERROR_INVALID_PARAM);
        return -1;
    }
}

void handle_mode_sim_command(void) {
    uci_disable_hardware_mode();
}

int handle_mode_hw_command(char* device_path) {
    if (!device_path) {
        printf("Usage: mode_hw <device_path>\n");
        printf("  Example: mode_hw /dev/ttyUSB0\n");
        UCI_LOG_ERROR("device_path is NULL", UCI_ERROR_INVALID_PARAM);
        return -1;
    }
    
    // Basic validation of device path to prevent potential security issues
    if (strlen(device_path) >= 256) {
        printf("Device path too long\n");
        UCI_LOG_ERROR("Device path too long", UCI_ERROR_INVALID_PARAM);
        return -1;
    }
    
    // Basic check to prevent directory traversal attacks
    if (strstr(device_path, "../") || strstr(device_path, "..\\")) {
        printf("Invalid device path: directory traversal detected\n");
        UCI_LOG_ERROR("Directory traversal detected in device path", UCI_ERROR_INVALID_PARAM);
        return -1;
    }
    
    uci_enable_hardware_mode(device_path);
    return 0;
}

void handle_mode_info_command(void) {
    if (uci_is_hardware_mode_enabled()) {
        printf("Current mode: HARDWARE\n");
        printf("Hardware device: %s\n", uci_get_hardware_device_path());
    } else {
        printf("Current mode: SIMULATION\n");
    }
}
