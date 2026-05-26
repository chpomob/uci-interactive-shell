#include "../include/uci_cmd_framework_simulation.h"
#include "../include/uci_cmd_simulation_typed.h"

static const uci_param_def_t k_simulate_notification_params[] = {
    {
        .name = "type",
        .type = PARAM_TYPE_STRING,
        .flags = PARAM_FLAG_REQUIRED,
        .max_len = 32,
        .min_value = 0,
        .max_value = 0,
        .description = "Notification type (e.g. device_status)",
    },
    {
        .name = "value",
        .type = PARAM_TYPE_STRING,
        .flags = PARAM_FLAG_REQUIRED,
        .max_len = 32,
        .min_value = 0,
        .max_value = 0,
        .description = "Value for the notification",
    },
};

static const uci_param_def_t k_simulate_session_status_params[] = {
    {
        .name = "session_id",
        .type = PARAM_TYPE_SESSION_ID,
        .flags = PARAM_FLAG_REQUIRED,
        .max_len = 0,
        .min_value = 0,
        .max_value = 0,
        .description = "Numeric session identifier",
    },
    {
        .name = "state",
        .type = PARAM_TYPE_STRING,
        .flags = PARAM_FLAG_REQUIRED,
        .max_len = 16,
        .min_value = 0,
        .max_value = 0,
        .description = "State (init|deinit|active|idle)",
    },
    {
        .name = "reason",
        .type = PARAM_TYPE_STRING,
        .flags = PARAM_FLAG_REQUIRED,
        .max_len = 16,
        .min_value = 0,
        .max_value = 0,
        .description = "Reason (mgmt_cmd, ...)",
    },
};

static const uci_param_def_t k_vendor_command_params[] = {
    {
        .name = "opcode",
        .type = PARAM_TYPE_HEX_BYTE,
        .flags = PARAM_FLAG_REQUIRED,
        .max_len = 0,
        .min_value = 0,
        .max_value = 0,
        .description = "Vendor opcode (hex or decimal)",
    },
    {
        .name = "params",
        .type = PARAM_TYPE_HEX_STRING,
        .flags = PARAM_FLAG_OPTIONAL,
        .max_len = 512,
        .min_value = 0,
        .max_value = 0,
        .description = "Optional payload bytes",
    },
};

const uci_command_def_t g_uci_simulation_command_defs[] = {
    {
        .name = "simulate_notification",
        .aliases = { NULL },
        .group = CLI_GROUP_SIMULATION,
        .flags = CLI_CMD_FLAG_NONE,
        .description = "Simulate device notification",
        .params = k_simulate_notification_params,
        .param_count = ARRAY_SIZE(k_simulate_notification_params),
        .handler = handle_simulate_notification_command_typed,
    },
    {
        .name = "simulate_session_status",
        .aliases = { NULL },
        .group = CLI_GROUP_SIMULATION,
        .flags = CLI_CMD_FLAG_NONE,
        .description = "Simulate session status notification",
        .params = k_simulate_session_status_params,
        .param_count = ARRAY_SIZE(k_simulate_session_status_params),
        .handler = handle_simulate_session_status_command_typed,
    },
    {
        .name = "simulate_data_credit",
        .aliases = { NULL },
        .group = CLI_GROUP_SIMULATION,
        .flags = CLI_CMD_FLAG_NONE,
        .description = "Simulate session data credit notification",
        .params = NULL,
        .param_count = 0,
        .handler = handle_simulate_data_credit_command_typed,
    },
    {
        .name = "simulate_ranging",
        .aliases = { NULL },
        .group = CLI_GROUP_SIMULATION,
        .flags = CLI_CMD_FLAG_NONE,
        .description = "Simulate single-target ranging notification",
        .params = NULL,
        .param_count = 0,
        .handler = handle_simulate_ranging_command_typed,
    },
    {
        .name = "simulate_multi_target_ranging",
        .aliases = { NULL },
        .group = CLI_GROUP_SIMULATION,
        .flags = CLI_CMD_FLAG_NONE,
        .description = "Simulate multi-target ranging notification",
        .params = NULL,
        .param_count = 0,
        .handler = handle_simulate_multi_target_ranging_command_typed,
    },
    {
        .name = "demo_session_flow",
        .aliases = { NULL },
        .group = CLI_GROUP_SIMULATION,
        .flags = CLI_CMD_FLAG_NONE,
        .description = "Demonstrate session flow",
        .params = NULL,
        .param_count = 0,
        .handler = handle_demo_session_flow_command_typed,
    },
    {
        .name = "simulate_qm_sdk_vendor_command",
        .aliases = { NULL },
        .group = CLI_GROUP_SIMULATION,
        .flags = CLI_CMD_FLAG_NONE,
        .description = "Simulate QM SDK vendor-specific command (GID 0x0B)",
        .params = k_vendor_command_params,
        .param_count = ARRAY_SIZE(k_vendor_command_params),
        .handler = handle_simulate_qm_sdk_vendor_command_typed,
    },
    {
        .name = "get_calibration",
        .aliases = { "get_calib", NULL },
        .group = CLI_GROUP_SIMULATION,
        .flags = CLI_CMD_FLAG_NONE,
        .description = "Query Qorvo calibration parameters (GID 0x0E OID 0x2B)",
        .params = NULL,
        .param_count = 0,
        .handler = handle_get_calibration_command_typed,
    },
};

const int g_uci_simulation_command_defs_count =
    (int)(sizeof(g_uci_simulation_command_defs) / sizeof(g_uci_simulation_command_defs[0]));
