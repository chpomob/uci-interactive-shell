#include <string.h>

#include "../include/uci_cmd_core_new.h"
#include "../include/uci_cmd_framework_bridge.h"
#include "../include/uci_cmd_framework_simulation.h"
#include "../include/uci_cmd_framework_session.h"
#include "../include/uci_cmd_framework_wrappers.h"
#include "../include/uci_cmd_handlers.h"
#include "../include/uci_ui.h"

DEFINE_CMD_WRAPPER(cmd_help)
DEFINE_CMD_WRAPPER(cmd_mode_sim)
DEFINE_CMD_WRAPPER(cmd_mode_hw)
DEFINE_CMD_WRAPPER(cmd_mode_info)
DEFINE_CMD_WRAPPER(cmd_hw_init)
DEFINE_CMD_WRAPPER(cmd_hw_send)
DEFINE_CMD_WRAPPER(cmd_get_caps_info)
DEFINE_CMD_WRAPPER(cmd_show_device_configs)
DEFINE_CMD_WRAPPER(cmd_analyze_packet)

static const uci_param_def_t k_set_power_params[] = {
    {
        .name = "state",
        .type = PARAM_TYPE_DEVICE_STATE,
        .flags = PARAM_FLAG_REQUIRED,
        .max_len = 0,
        .min_value = 0,
        .max_value = 0,
        .description = "Target device state (active|ready|sleep|suspend)",
    },
};

static const uci_param_def_t k_hw_send_params[] = {
    {
        .name = "mt",
        .type = PARAM_TYPE_HEX_BYTE,
        .flags = PARAM_FLAG_REQUIRED,
        .max_len = 0,
        .min_value = 0,
        .max_value = 0,
        .description = "Message Type byte (hex, e.g. 01)",
    },
    {
        .name = "pbf",
        .type = PARAM_TYPE_HEX_BYTE,
        .flags = PARAM_FLAG_REQUIRED,
        .max_len = 0,
        .min_value = 0,
        .max_value = 0,
        .description = "Packet Boundary Flag (hex)",
    },
    {
        .name = "gid",
        .type = PARAM_TYPE_HEX_BYTE,
        .flags = PARAM_FLAG_REQUIRED,
        .max_len = 0,
        .min_value = 0,
        .max_value = 0,
        .description = "Group Identifier (hex)",
    },
    {
        .name = "oid",
        .type = PARAM_TYPE_HEX_BYTE,
        .flags = PARAM_FLAG_REQUIRED,
        .max_len = 0,
        .min_value = 0,
        .max_value = 0,
        .description = "Opcode Identifier (hex)",
    },
};

static const uci_param_def_t k_get_config_params[] = {
    {
        .name = "config_name",
        .type = PARAM_TYPE_STRING,
        .flags = PARAM_FLAG_REQUIRED,
        .max_len = 64,
        .min_value = 0,
        .max_value = 0,
        .description = "Named device configuration parameter",
    },
};

static const uci_param_def_t k_set_config_params[] = {
    {
        .name = "config_name",
        .type = PARAM_TYPE_STRING,
        .flags = PARAM_FLAG_REQUIRED,
        .max_len = 64,
        .min_value = 0,
        .max_value = 0,
        .description = "Named device configuration parameter",
    },
    {
        .name = "value",
        .type = PARAM_TYPE_STRING,
        .flags = PARAM_FLAG_REQUIRED,
        .max_len = 128,
        .min_value = 0,
        .max_value = 0,
        .description = "String representation of the desired value",
    },
};

static const uci_param_def_t k_validate_arguments_params[] = {
    {
        .name = "integer_value",
        .type = PARAM_TYPE_STRING,
        .flags = PARAM_FLAG_REQUIRED,
        .max_len = 32,
        .min_value = 0,
        .max_value = 0,
        .description = "Signed integer that will be validated by the handler",
    },
    {
        .name = "hex_payload",
        .type = PARAM_TYPE_HEX_STRING,
        .flags = PARAM_FLAG_REQUIRED,
        .max_len = 64,
        .min_value = 0,
        .max_value = 0,
        .description = "Hex string (even number of characters)",
    },
    {
        .name = "session_id",
        .type = PARAM_TYPE_SESSION_ID,
        .flags = PARAM_FLAG_REQUIRED,
        .max_len = 0,
        .min_value = 0,
        .max_value = 0,
        .description = "Session identifier",
    },
};

static const uci_param_def_t k_device_path_param[] = {
    {
        .name = "device_path",
        .type = PARAM_TYPE_STRING,
        .flags = PARAM_FLAG_OPTIONAL,
        .max_len = 255,
        .min_value = 0,
        .max_value = 0,
        .description = "Serial or char device path (e.g. /dev/uwb0)",
    },
};

static const uci_param_def_t k_analyze_packet_params[] = {
    {
        .name = "packet_bytes",
        .type = PARAM_TYPE_HEX_STRING,
        .flags = PARAM_FLAG_REQUIRED,
        .max_len = 1024,
        .min_value = 0,
        .max_value = 0,
        .description = "Hex encoded packet bytes",
    },
};

const uci_command_def_t g_uci_command_defs[] = {
    // General
    {
        .name = "help",
        .aliases = { NULL },
        .group = CLI_GROUP_GENERAL,
        .flags = CLI_CMD_FLAG_NONE,
        .description = "Show this help message",
        .params = NULL,
        .param_count = 0,
        .handler = cmd_help_framework_adapter,
    },

    // Hardware mode management
    {
        .name = "mode_sim",
        .aliases = { "sim_mode", NULL, NULL, NULL },
        .group = CLI_GROUP_HARDWARE,
        .flags = CLI_CMD_FLAG_NONE,
        .description = "Switch to simulation mode",
        .params = NULL,
        .param_count = 0,
        .handler = cmd_mode_sim_framework_adapter,
    },
    {
        .name = "mode_hw",
        .aliases = { "hw_mode", NULL, NULL, NULL },
        .group = CLI_GROUP_HARDWARE,
        .flags = CLI_CMD_FLAG_NONE,
        .description = "Switch to hardware mode",
        .params = k_device_path_param,
        .param_count = ARRAY_SIZE(k_device_path_param),
        .handler = cmd_mode_hw_framework_adapter,
    },
    {
        .name = "mode_info",
        .aliases = { "current_mode", NULL, NULL, NULL },
        .group = CLI_GROUP_HARDWARE,
        .flags = CLI_CMD_FLAG_NONE,
        .description = "Display current mode",
        .params = NULL,
        .param_count = 0,
        .handler = cmd_mode_info_framework_adapter,
    },
    {
        .name = "hw_init",
        .aliases = { "hw_connect", NULL, NULL, NULL },
        .group = CLI_GROUP_HARDWARE,
        .flags = CLI_CMD_FLAG_NONE,
        .description = "Initialize hardware mode and connect",
        .params = k_device_path_param,
        .param_count = ARRAY_SIZE(k_device_path_param),
        .handler = cmd_hw_init_framework_adapter,
    },
    {
        .name = "hw_send",
        .aliases = { NULL },
        .group = CLI_GROUP_HARDWARE,
        .flags = CLI_CMD_FLAG_REQUIRES_HW_MODE,
        .description = "Send a raw packet to hardware",
        .params = k_hw_send_params,
        .param_count = ARRAY_SIZE(k_hw_send_params),
        .handler = cmd_hw_send_framework_adapter,
    },

    // Device commands powered by dedicated framework handlers
    {
        .name = "get_device_info",
        .aliases = { "device_info", NULL, NULL, NULL },
        .group = CLI_GROUP_DEVICE,
        .flags = CLI_CMD_FLAG_NONE,
        .description = "Query device information",
        .params = NULL,
        .param_count = 0,
        .handler = handle_get_device_info_command_new,
    },
    {
        .name = "device_reset",
        .aliases = { NULL },
        .group = CLI_GROUP_DEVICE,
        .flags = CLI_CMD_FLAG_NONE,
        .description = "Reset the connected device",
        .params = NULL,
        .param_count = 0,
        .handler = handle_device_reset_command_new,
    },
    {
        .name = "set_power",
        .aliases = { NULL },
        .group = CLI_GROUP_DEVICE,
        .flags = CLI_CMD_FLAG_NONE,
        .description = "Set device power state",
        .params = k_set_power_params,
        .param_count = ARRAY_SIZE(k_set_power_params),
        .handler = handle_set_power_command_new,
    },
    {
        .name = "device_on",
        .aliases = { NULL },
        .group = CLI_GROUP_DEVICE,
        .flags = CLI_CMD_FLAG_NONE,
        .description = "Power on the device",
        .params = NULL,
        .param_count = 0,
        .handler = handle_device_on_command_new,
    },
    {
        .name = "device_off",
        .aliases = { NULL },
        .group = CLI_GROUP_DEVICE,
        .flags = CLI_CMD_FLAG_NONE,
        .description = "Power off the device",
        .params = NULL,
        .param_count = 0,
        .handler = handle_device_off_command_new,
    },
    {
        .name = "get_caps_info",
        .aliases = { NULL },
        .group = CLI_GROUP_DEVICE,
        .flags = CLI_CMD_FLAG_NONE,
        .description = "Query capability information",
        .params = NULL,
        .param_count = 0,
        .handler = cmd_get_caps_info_framework_adapter,
    },
    {
        .name = "get_config",
        .aliases = { NULL },
        .group = CLI_GROUP_DEVICE,
        .flags = CLI_CMD_FLAG_NONE,
        .description = "Read a device configuration parameter",
        .params = k_get_config_params,
        .param_count = ARRAY_SIZE(k_get_config_params),
        .handler = handle_get_config_command_new,
    },
    {
        .name = "get_device_state",
        .aliases = { NULL },
        .group = CLI_GROUP_DEVICE,
        .flags = CLI_CMD_FLAG_NONE,
        .description = "Report current device state",
        .params = NULL,
        .param_count = 0,
        .handler = handle_get_device_state_command_new,
    },
    {
        .name = "set_device_active",
        .aliases = { NULL },
        .group = CLI_GROUP_DEVICE,
        .flags = CLI_CMD_FLAG_NONE,
        .description = "Force device Active state",
        .params = NULL,
        .param_count = 0,
        .handler = handle_set_device_active_command_new,
    },
    {
        .name = "set_device_ready",
        .aliases = { NULL },
        .group = CLI_GROUP_DEVICE,
        .flags = CLI_CMD_FLAG_NONE,
        .description = "Force device Ready state",
        .params = NULL,
        .param_count = 0,
        .handler = handle_set_device_ready_command_new,
    },
    {
        .name = "set_config",
        .aliases = { NULL },
        .group = CLI_GROUP_DEVICE,
        .flags = CLI_CMD_FLAG_NONE,
        .description = "Update a device configuration parameter",
        .params = k_set_config_params,
        .param_count = ARRAY_SIZE(k_set_config_params),
        .handler = handle_set_config_command_new,
    },
    {
        .name = "show_device_configs",
        .aliases = { NULL },
        .group = CLI_GROUP_DEVICE,
        .flags = CLI_CMD_FLAG_NONE,
        .description = "List supported device configuration parameters",
        .params = NULL,
        .param_count = 0,
        .handler = cmd_show_device_configs_framework_adapter,
    },
    {
        .name = "device_suspend",
        .aliases = { NULL },
        .group = CLI_GROUP_DEVICE,
        .flags = CLI_CMD_FLAG_NONE,
        .description = "Suspend device operation",
        .params = NULL,
        .param_count = 0,
        .handler = handle_device_suspend_command_new,
    },
    {
        .name = "query_timestamp",
        .aliases = { NULL },
        .group = CLI_GROUP_DEVICE,
        .flags = CLI_CMD_FLAG_NONE,
        .description = "Query device timestamp",
        .params = NULL,
        .param_count = 0,
        .handler = handle_query_timestamp_command_new,
    },
    {
        .name = "validate_arguments",
        .aliases = { NULL },
        .group = CLI_GROUP_DEVICE,
        .flags = CLI_CMD_FLAG_NONE,
        .description = "Demo new command framework argument validation",
        .params = k_validate_arguments_params,
        .param_count = ARRAY_SIZE(k_validate_arguments_params),
        .handler = handle_validate_arguments_command_new,
    },
    // Analysis
    {
        .name = "analyze_packet",
        .aliases = { NULL },
        .group = CLI_GROUP_ANALYSIS,
        .flags = CLI_CMD_FLAG_NONE,
        .description = "Analyze packet bytes with enhanced decoder",
        .params = k_analyze_packet_params,
        .param_count = ARRAY_SIZE(k_analyze_packet_params),
        .handler = cmd_analyze_packet_framework_adapter,
    },

};

const int g_uci_command_defs_count = (int)ARRAY_SIZE(g_uci_command_defs);

static const uci_command_def_t* uci_cmd_framework_find_in_defs(const uci_command_def_t* defs,
                                                               int count,
                                                               const char* name) {
    if (!name || !defs || count <= 0) {
        return NULL;
    }

    for (int idx = 0; idx < count; idx++) {
        const uci_command_def_t* def = &defs[idx];
        if (strcmp(name, def->name) == 0) {
            return def;
        }

        for (size_t alias_idx = 0; alias_idx < ARRAY_SIZE(def->aliases); alias_idx++) {
            const char* alias = def->aliases[alias_idx];
            if (!alias) {
                break;
            }
            if (strcmp(name, alias) == 0) {
                return def;
            }
        }
    }

    return NULL;
}

const uci_command_def_t* uci_cmd_framework_find(const char* name) {
    const uci_command_def_t* def = uci_cmd_framework_find_in_defs(g_uci_command_defs,
                                                                  g_uci_command_defs_count,
                                                                  name);
    if (def) {
        return def;
    }

    def = uci_cmd_framework_find_in_defs(g_uci_session_command_defs,
                                         g_uci_session_command_defs_count,
                                         name);
    if (def) {
        return def;
    }

    return uci_cmd_framework_find_in_defs(g_uci_simulation_command_defs,
                                          g_uci_simulation_command_defs_count,
                                          name);
}

int uci_cmd_framework_handler(int argc, char** argv) {
    if (argc <= 0 || !argv) {
        ui_print_error("Missing command arguments");
        return -1;
    }

    const uci_command_def_t* cmd_def = uci_cmd_framework_find(argv[0]);
    if (!cmd_def) {
        ui_print_command_not_found(argv[0]);
        return -1;
    }

    return uci_cmd_dispatch(cmd_def, argc, argv);
}
