#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../include/uci.h"
#include "../include/uci_functions.h"
#include "../include/uci_hw_interface.h"
#include "../include/uci_pdl.h"
#include "../include/uci_packet_utils.h"

// Helper function to send UCI command in unified mode (hardware or simulation)
static int send_unified_command(unsigned char mt, unsigned char pbf, unsigned char gid, unsigned char oid,
                                unsigned char* payload, int payload_len) {
    if (uci_is_hardware_mode_enabled()) {
        if (!uci_hw_interface_is_connected()) {
            printf("Hardware not connected. Use 'hw_connect <device_path>' first.\n");
            return -1;
        }

        if (uci_hw_interface_send_command(mt, pbf, gid, oid, payload, payload_len) == 0) {
            printf("Command sent to hardware successfully\n");
            unsigned char response_buffer[1024];
            int response_len = uci_hw_interface_receive_response(response_buffer, sizeof(response_buffer), 1000);
            if (response_len > 0) {
                printf("Received %d bytes from hardware: ", response_len);
                for (int i = 0; i < response_len; i++) {
                    printf("%02X ", response_buffer[i]);
                }
                printf("\n");
                parse_uci_packet(response_buffer, response_len);
            }
            return 0;
        } else {
            printf("Failed to send command to hardware\n");
            return -1;
        }
    } else {
        send_uci_command(mt, pbf, gid, oid, payload, payload_len);
        return 0;
    }
}

void handle_get_device_info_command(void) {
    send_unified_command(COMMAND, 0, CORE, CORE_DEVICE_INFO, NULL, 0);
}

void handle_device_reset_command(void) {
    size_t packet_len;
    unsigned char* packet = create_device_reset_packet(UWBS_RESET, &packet_len);
    if (packet) {
        struct uci_packet_header* header = (struct uci_packet_header*)packet;
        uci_header_fields_t header_fields;
        uci_extract_header_fields_safe(header, &header_fields);
        send_unified_command(header_fields.message_type, header_fields.packet_boundary, header_fields.group_id, header_fields.opcode_id, packet + sizeof(struct uci_packet_header), header_fields.payload_length);
        free(packet);
    }

    // In simulation mode, also send status notification
    if (!uci_is_hardware_mode_enabled()) {
        unsigned char dummy_notification_packet[] = {0x60, 0x01, 0x01, DEVICE_STATE_READY};
        parse_uci_packet(dummy_notification_packet, sizeof(dummy_notification_packet));
    }
}

void handle_get_caps_info_command(void) {
    size_t packet_len;
    unsigned char* packet = create_get_caps_info_packet(&packet_len);
    if (packet) {
        struct uci_packet_header* header = (struct uci_packet_header*)packet;
        uci_header_fields_t header_fields;
        uci_extract_header_fields_safe(header, &header_fields);
        send_unified_command(header_fields.message_type, header_fields.packet_boundary, header_fields.group_id, header_fields.opcode_id, packet + sizeof(struct uci_packet_header), header_fields.payload_length);
        free(packet);
    }
}

int handle_set_power_command(char* power_state) {
    if (!power_state) {
        printf("Usage: set_power <state> (active, ready, sleep)\n");
        return -1;
    }

    DeviceConfigId cfg_id = DEVICE_STATE;
    unsigned char value;

    if (strcmp(power_state, "active") == 0) {
        value = DEVICE_STATE_ACTIVE;
    } else if (strcmp(power_state, "ready") == 0) {
        value = DEVICE_STATE_READY;
    } else if (strcmp(power_state, "sleep") == 0 || strcmp(power_state, "suspend") == 0) {
        // For sleep, send the device suspend command
        unsigned char suspend_payload[] = {0x00}; // Wakeup source
        send_unified_command(COMMAND, 0, CORE, CORE_DEVICE_SUSPEND, suspend_payload, sizeof(suspend_payload));
        return 0;
    } else {
        printf("Invalid power state: %s. Use 'active', 'ready', 'sleep', or 'suspend'.\n", power_state);
        return -1;
    }

    unsigned char payload[] = {0x01, cfg_id, 0x01, value};
    send_unified_command(COMMAND, 0, CORE, CORE_SET_CONFIG, payload, sizeof(payload));
    return 0;
}

void handle_device_on_command(void) {
    handle_set_power_command("active");
}

void handle_device_off_command(void) {
    handle_set_power_command("ready");
}

int handle_get_config_command(char* config_name) {
    if (!config_name) {
        printf("Usage: get_config <config_name>\n");
        printf("  Examples:\n");
        printf("    get_config device_state\n");
        printf("    get_config low_power_mode\n");
        return -1;
    }

    DeviceConfigId cfg_id;
    if (strcmp(config_name, "device_state") == 0) {
        cfg_id = DEVICE_STATE;
    } else if (strcmp(config_name, "low_power_mode") == 0) {
        cfg_id = LOW_POWER_MODE;
    } else {
        printf("Unknown config_name: %s. Supported configs: device_state, low_power_mode\n", config_name);
        return -1;
    }

    unsigned char payload[] = {0x01, cfg_id}; // num_tlvs(1), cfg_id
    send_unified_command(COMMAND, 0, CORE, CORE_GET_CONFIG, payload, sizeof(payload));
    return 0;
}

void handle_get_device_state_command(void) {
    unsigned char payload[] = {0x01, DEVICE_STATE}; // num_tlvs(1), cfg_id
    send_unified_command(COMMAND, 0, CORE, CORE_GET_CONFIG, payload, sizeof(payload));
}

void handle_set_device_active_command(void) {
    unsigned char payload[] = {0x01, DEVICE_STATE, 0x01, DEVICE_STATE_ACTIVE};
    send_unified_command(COMMAND, 0, CORE, CORE_SET_CONFIG, payload, sizeof(payload));
}

void handle_set_device_ready_command(void) {
    unsigned char payload[] = {0x01, DEVICE_STATE, 0x01, DEVICE_STATE_READY};
    send_unified_command(COMMAND, 0, CORE, CORE_SET_CONFIG, payload, sizeof(payload));
}

int handle_set_config_command(char* config_name, char* value_str) {
    if (!config_name || !value_str) {
        printf("Usage: set_config <config_name> <value>\n");
        printf("  Examples:\n");
        printf("    set_config device_state active\n");
        printf("    set_config low_power_mode off\n");
        return -1;
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
            // For sleep, send the device suspend command
            unsigned char suspend_payload[] = {0x00}; // Wakeup source
            send_unified_command(COMMAND, 0, CORE, CORE_DEVICE_SUSPEND, suspend_payload, sizeof(suspend_payload));
            return 0;
        } else {
            printf("Invalid device_state value: %s. Use 'active', 'ready', 'sleep', or 'suspend'.\n", value_str);
            return -1;
        }
    } else if (strcmp(config_name, "low_power_mode") == 0) {
        cfg_id = LOW_POWER_MODE;
        if (strcmp(value_str, "on") == 0) {
            value = 0x01;
        } else if (strcmp(value_str, "off") == 0) {
            value = 0x00;
        } else {
            printf("Invalid low_power_mode value: %s. Use 'on' or 'off'.\n", value_str);
            return -1;
        }
    } else {
        printf("Unknown config_name: %s. Supported configs: device_state, low_power_mode\n", config_name);
        return -1;
    }

    unsigned char configs[] = {cfg_id, 0x01, value};
    size_t packet_len;
    unsigned char* packet = create_set_config_packet(1, configs, sizeof(configs), &packet_len);
    if (packet) {
        struct uci_packet_header* header = (struct uci_packet_header*)packet;
        uci_header_fields_t header_fields;
        uci_extract_header_fields_safe(header, &header_fields);
        send_unified_command(header_fields.message_type, header_fields.packet_boundary, header_fields.group_id, header_fields.opcode_id, packet + sizeof(struct uci_packet_header), header_fields.payload_length);
        free(packet);
    }
    return 0;
}

void handle_device_suspend_command(void) {
    unsigned char payload[] = {0x00};  // Wakeup source
    send_unified_command(COMMAND, 0, CORE, CORE_DEVICE_SUSPEND, payload, sizeof(payload));
}

void handle_query_timestamp_command(void) {
    send_unified_command(COMMAND, 0, CORE, CORE_QUERY_UWBS_TIMESTAMP, NULL, 0);
}
