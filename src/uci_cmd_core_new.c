#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../include/uci.h"
#include "../include/uci_functions.h"
#include "../include/uci_hw_interface.h"
#include "../include/uci_pdl.h"
#include "../include/uci_command_framework.h"

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

// Handler for get_device_info command using framework
int handle_get_device_info_command_new(const char* cmd_name, int argc, char** argv, 
                                       const uci_param_def_t* params, int param_count) {
    // Unused parameters - prevent compiler warnings
    (void)cmd_name;
    (void)argc;
    (void)argv;
    (void)params;
    (void)param_count;
    
    send_unified_command(COMMAND, 0, CORE, CORE_DEVICE_INFO, NULL, 0);
    return 0;
}

// Handler for device_reset command using framework
int handle_device_reset_command_new(const char* cmd_name, int argc, char** argv, 
                                    const uci_param_def_t* params, int param_count) {
    // Unused parameters - prevent compiler warnings
    (void)cmd_name;
    (void)argc;
    (void)argv;
    (void)params;
    (void)param_count;
    
    unsigned char payload[] = {UWBS_RESET};
    send_unified_command(COMMAND, 0, CORE, CORE_DEVICE_RESET, payload, sizeof(payload));

    // In simulation mode, also send status notification
    if (!uci_is_hardware_mode_enabled()) {
        unsigned char dummy_notification_packet[] = {0x60, 0x01, 0x01, DEVICE_STATE_READY};
        parse_uci_packet(dummy_notification_packet, sizeof(dummy_notification_packet));
    }
    return 0;
}

// Handler for set_power command using framework
int handle_set_power_command_new(const char* cmd_name, int argc, char** argv, 
                                 const uci_param_def_t* params, int param_count) {
    // Unused parameters - prevent compiler warnings
    (void)cmd_name;
    (void)argc;
    (void)argv;
    (void)params;
    (void)param_count;
    
    if (argc < 2) {
        printf("Usage: %s <state> (active, ready, sleep)\n", cmd_name);
        return -1;
    }

    const char* power_state = argv[1];
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

// Handler for device_on command using framework
int handle_device_on_command_new(const char* cmd_name, int argc, char** argv, 
                                 const uci_param_def_t* params, int param_count) {
    // Unused parameters - prevent compiler warnings
    (void)cmd_name;
    (void)argc;
    (void)argv;
    (void)params;
    (void)param_count;
    
    return handle_set_power_command_new(cmd_name, 2, (char*[]){"set_power", "active"}, params, param_count);
}

// Handler for device_off command using framework
int handle_device_off_command_new(const char* cmd_name, int argc, char** argv, 
                                  const uci_param_def_t* params, int param_count) {
    // Unused parameters - prevent compiler warnings
    (void)cmd_name;
    (void)argc;
    (void)argv;
    (void)params;
    (void)param_count;
    
    return handle_set_power_command_new(cmd_name, 2, (char*[]){"set_power", "ready"}, params, param_count);
}

// Handler for get_config command using framework
int handle_get_config_command_new(const char* cmd_name, int argc, char** argv, 
                                  const uci_param_def_t* params, int param_count) {
    // Unused parameters - prevent compiler warnings
    (void)cmd_name;
    (void)argc;
    (void)argv;
    (void)params;
    (void)param_count;
    
    if (argc < 2) {
        printf("Usage: %s <config_name>\n", cmd_name);
        printf("  Examples:\n");
        printf("    %s device_state\n", cmd_name);
        printf("    %s low_power_mode\n", cmd_name);
        return -1;
    }

    const char* config_name = argv[1];
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

// Handler for get_device_state command using framework
int handle_get_device_state_command_new(const char* cmd_name, int argc, char** argv, 
                                        const uci_param_def_t* params, int param_count) {
    // Unused parameters - prevent compiler warnings
    (void)cmd_name;
    (void)argc;
    (void)argv;
    (void)params;
    (void)param_count;
    
    unsigned char payload[] = {0x01, DEVICE_STATE}; // num_tlvs(1), cfg_id
    send_unified_command(COMMAND, 0, CORE, CORE_GET_CONFIG, payload, sizeof(payload));
    return 0;
}

// Handler for set_device_active command using framework
int handle_set_device_active_command_new(const char* cmd_name, int argc, char** argv, 
                                         const uci_param_def_t* params, int param_count) {
    // Unused parameters - prevent compiler warnings
    (void)cmd_name;
    (void)argc;
    (void)argv;
    (void)params;
    (void)param_count;
    
    unsigned char payload[] = {0x01, DEVICE_STATE, 0x01, DEVICE_STATE_ACTIVE};
    send_unified_command(COMMAND, 0, CORE, CORE_SET_CONFIG, payload, sizeof(payload));
    return 0;
}

// Handler for set_device_ready command using framework
int handle_set_device_ready_command_new(const char* cmd_name, int argc, char** argv, 
                                        const uci_param_def_t* params, int param_count) {
    // Unused parameters - prevent compiler warnings
    (void)cmd_name;
    (void)argc;
    (void)argv;
    (void)params;
    (void)param_count;
    
    unsigned char payload[] = {0x01, DEVICE_STATE, 0x01, DEVICE_STATE_READY};
    send_unified_command(COMMAND, 0, CORE, CORE_SET_CONFIG, payload, sizeof(payload));
    return 0;
}

// Handler for set_config command using framework
int handle_set_config_command_new(const char* cmd_name, int argc, char** argv, 
                                  const uci_param_def_t* params, int param_count) {
    // Unused parameters - prevent compiler warnings
    (void)cmd_name;
    (void)argc;
    (void)argv;
    (void)params;
    (void)param_count;
    
    if (argc < 3) {
        printf("Usage: %s <config_name> <value>\n", cmd_name);
        printf("  Examples:\n");
        printf("    %s device_state active\n", cmd_name);
        printf("    %s low_power_mode off\n", cmd_name);
        return -1;
    }

    const char* config_name = argv[1];
    const char* value_str = argv[2];
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

    unsigned char payload[] = {0x01, cfg_id, 0x01, value};
    send_unified_command(COMMAND, 0, CORE, CORE_SET_CONFIG, payload, sizeof(payload));
    return 0;
}

// Handler for device_suspend command using framework
int handle_device_suspend_command_new(const char* cmd_name, int argc, char** argv, 
                                      const uci_param_def_t* params, int param_count) {
    // Unused parameters - prevent compiler warnings
    (void)cmd_name;
    (void)argc;
    (void)argv;
    (void)params;
    (void)param_count;
    
    unsigned char payload[] = {0x00};  // Wakeup source
    send_unified_command(COMMAND, 0, CORE, CORE_DEVICE_SUSPEND, payload, sizeof(payload));
    return 0;
}

// Handler for query_timestamp command using framework
int handle_query_timestamp_command_new(const char* cmd_name, int argc, char** argv, 
                                       const uci_param_def_t* params, int param_count) {
    // Unused parameters - prevent compiler warnings
    (void)cmd_name;
    (void)argc;
    (void)argv;
    (void)params;
    (void)param_count;
    
    send_unified_command(COMMAND, 0, CORE, CORE_QUERY_UWBS_TIMESTAMP, NULL, 0);
    return 0;
}