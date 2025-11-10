#include <string.h>

#include "../include/uci_cmd_core_new.h"
#include "../include/uci_cmd_framework_bridge.h"
#include "../include/uci_ui.h"

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

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

const uci_command_def_t g_uci_command_defs[] = {
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
};

const int g_uci_command_defs_count = (int)ARRAY_SIZE(g_uci_command_defs);

const uci_command_def_t* uci_cmd_framework_find(const char* name) {
    if (!name) {
        return NULL;
    }

    for (int idx = 0; idx < g_uci_command_defs_count; idx++) {
        const uci_command_def_t* def = &g_uci_command_defs[idx];
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
