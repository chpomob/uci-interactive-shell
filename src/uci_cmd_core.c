#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include "../include/uci.h"
#include "../include/uci_functions.h"
#include "../include/uci_pdl.h"
#include "../include/uci_packet_utils.h"
#include "../include/uci_config_manager.h"
#include "../include/uci_command_framework.h"

#define CORE_CMD_MAX_PAYLOAD 512

static int send_core_command(unsigned char mt,
                             unsigned char pbf,
                             unsigned char gid,
                             unsigned char oid,
                             const unsigned char* payload,
                             size_t payload_len) {
    if (payload_len > CORE_CMD_MAX_PAYLOAD) {
        printf("Error: payload too large (%zu bytes)\n", payload_len);
        return -1;
    }

    unsigned char buffer[CORE_CMD_MAX_PAYLOAD];
    uci_command_context_t ctx = {
        .mt = mt,
        .pbf = pbf,
        .gid = gid,
        .oid = oid,
        .payload = buffer,
        .payload_len = 0,
        .max_payload_len = sizeof(buffer),
    };

    if (payload_len > 0) {
        if (!payload) {
            printf("Error: NULL payload with non-zero length\n");
            return -1;
        }
        memcpy(buffer, payload, payload_len);
    }
    ctx.payload_len = payload_len;

    return uci_cmd_execute_unified(&ctx);
}

void handle_get_device_info_command(void) {
    send_core_command(COMMAND, COMPLETE, CORE, CORE_DEVICE_INFO, NULL, 0);
}

void handle_device_reset_command(void) {
    unsigned char payload[] = {UWBS_RESET};
    send_core_command(COMMAND, COMPLETE, CORE, CORE_DEVICE_RESET, payload, sizeof(payload));

    // In simulation mode, also send status notification
    if (!uci_is_hardware_mode_enabled()) {
        unsigned char dummy_notification_packet[] = {0x60, 0x01, 0x01, DEVICE_STATE_READY};
        parse_uci_packet(dummy_notification_packet, sizeof(dummy_notification_packet));
    }
}

void handle_get_caps_info_command(void) {
    send_core_command(COMMAND, COMPLETE, CORE, CORE_GET_CAPS_INFO, NULL, 0);
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
        send_core_command(COMMAND, COMPLETE, CORE, CORE_DEVICE_SUSPEND, suspend_payload, sizeof(suspend_payload));
        return 0;
    } else {
        printf("Invalid power state: %s. Use 'active', 'ready', 'sleep', or 'suspend'.\n", power_state);
        return -1;
    }

    unsigned char payload[] = {0x01, cfg_id, 0x01, value};
    return send_core_command(COMMAND, COMPLETE, CORE, CORE_SET_CONFIG, payload, sizeof(payload));
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
        printf("  Example: get_config device_state\n");
        printf("  Hint: run 'show_device_configs' to list available parameters.\n");
        return -1;
    }

    DeviceConfigId cfg_id;
    const device_config_param_info_t* info = NULL;
    if (uci_config_lookup_device_param(config_name, &cfg_id, &info) != 0) {
        printf("Unknown config_name: %s. Use 'show_device_configs' to list supported parameters.\n",
               config_name);
        return -1;
    }

    unsigned char payload[] = {0x01, (unsigned char)cfg_id};
    send_core_command(COMMAND, COMPLETE, CORE, CORE_GET_CONFIG, payload, sizeof(payload));
    return 0;
}

void handle_get_device_state_command(void) {
    unsigned char payload[] = {0x01, DEVICE_STATE}; // num_tlvs(1), cfg_id
    send_core_command(COMMAND, COMPLETE, CORE, CORE_GET_CONFIG, payload, sizeof(payload));
}

void handle_set_device_active_command(void) {
    unsigned char payload[] = {0x01, DEVICE_STATE, 0x01, DEVICE_STATE_ACTIVE};
    send_core_command(COMMAND, COMPLETE, CORE, CORE_SET_CONFIG, payload, sizeof(payload));
}

void handle_set_device_ready_command(void) {
    unsigned char payload[] = {0x01, DEVICE_STATE, 0x01, DEVICE_STATE_READY};
    send_core_command(COMMAND, COMPLETE, CORE, CORE_SET_CONFIG, payload, sizeof(payload));
}

static int send_device_config(DeviceConfigId cfg_id, const unsigned char* value, size_t value_len) {
    if (!value || value_len == 0 || value_len > 255) {
        printf("Invalid device config payload length: %zu\n", value_len);
        return -1;
    }

    unsigned char payload[1 + 2 + 256];
    payload[0] = 0x01; // number of TLVs
    payload[1] = (unsigned char)cfg_id;
    payload[2] = (unsigned char)value_len;
    memcpy(&payload[3], value, value_len);

    return send_core_command(COMMAND, COMPLETE, CORE, CORE_SET_CONFIG, payload, value_len + 3);
}

int handle_set_config_command(char* config_name, char* value_str) {
    if (!config_name || !value_str) {
        printf("Usage: set_config <config_name> <value>\n");
        printf("  Example: set_config device_channel 9\n");
        printf("  Hint: run 'show_device_configs' for supported parameters.\n");
        return -1;
    }

    DeviceConfigId cfg_id;
    const device_config_param_info_t* info = NULL;
    if (uci_config_lookup_device_param(config_name, &cfg_id, &info) != 0) {
        printf("Unknown config_name: %s. Use 'show_device_configs' to list supported parameters.\n",
               config_name);
        return -1;
    }

    if (cfg_id == DEVICE_STATE &&
        (strcasecmp(value_str, "sleep") == 0 || strcasecmp(value_str, "suspend") == 0)) {
        unsigned char suspend_payload[] = {0x00};
        send_core_command(COMMAND, COMPLETE, CORE, CORE_DEVICE_SUSPEND,
                          suspend_payload, sizeof(suspend_payload));
        return 0;
    }

    unsigned char value_buffer[256];
    size_t value_len = sizeof(value_buffer);
    if (uci_config_parse_device_value(cfg_id, value_str, value_buffer, &value_len) != 0) {
        const char* name = info ? info->name : NULL;
        printf("Invalid value '%s' for %s (ID 0x%02X).\n", value_str,
               name ? name : "device config", cfg_id);
        if (info) {
            uci_config_show_device_param_help(cfg_id);
        }
        return -1;
    }

    return send_device_config(cfg_id, value_buffer, value_len);
}

void handle_device_suspend_command(void) {
    unsigned char payload[] = {0x00};  // Wakeup source
    send_core_command(COMMAND, COMPLETE, CORE, CORE_DEVICE_SUSPEND, payload, sizeof(payload));
}

void handle_query_timestamp_command(void) {
    send_core_command(COMMAND, COMPLETE, CORE, CORE_QUERY_UWBS_TIMESTAMP, NULL, 0);
}
