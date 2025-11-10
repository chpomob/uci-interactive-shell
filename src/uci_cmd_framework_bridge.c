#include <string.h>

#include "../include/uci_cmd_core_new.h"
#include "../include/uci_cmd_framework_bridge.h"
#include "../include/uci_cmd_handlers.h"
#include "../include/uci_ui.h"

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

#define DEFINE_CMD_WRAPPER(fn)                                                          \
    static int fn##_framework_adapter(const char* cmd_name,                             \
                                      int argc,                                         \
                                      char** argv,                                      \
                                      const uci_param_def_t* params,                    \
                                      int param_count) {                                \
        (void)cmd_name;                                                                  \
        (void)params;                                                                    \
        (void)param_count;                                                               \
        return fn(argc, argv);                                                           \
    }

DEFINE_CMD_WRAPPER(cmd_help)
DEFINE_CMD_WRAPPER(cmd_mode_sim)
DEFINE_CMD_WRAPPER(cmd_mode_hw)
DEFINE_CMD_WRAPPER(cmd_mode_info)
DEFINE_CMD_WRAPPER(cmd_hw_init)
DEFINE_CMD_WRAPPER(cmd_hw_send)
DEFINE_CMD_WRAPPER(cmd_get_caps_info)
DEFINE_CMD_WRAPPER(cmd_show_device_configs)
DEFINE_CMD_WRAPPER(cmd_show_app_configs)
DEFINE_CMD_WRAPPER(cmd_session_init)
DEFINE_CMD_WRAPPER(cmd_session_deinit)
DEFINE_CMD_WRAPPER(cmd_session_start)
DEFINE_CMD_WRAPPER(cmd_session_stop)
DEFINE_CMD_WRAPPER(cmd_session_send_data)
DEFINE_CMD_WRAPPER(cmd_session_logical_link_create)
DEFINE_CMD_WRAPPER(cmd_session_logical_link_close)
DEFINE_CMD_WRAPPER(cmd_session_logical_link_get_param)
DEFINE_CMD_WRAPPER(cmd_get_session_state)
DEFINE_CMD_WRAPPER(cmd_set_app_config)
DEFINE_CMD_WRAPPER(cmd_get_app_config)
DEFINE_CMD_WRAPPER(cmd_session_update_multicast_list)
DEFINE_CMD_WRAPPER(cmd_session_update_dt_tag_rounds)
DEFINE_CMD_WRAPPER(cmd_session_data_transfer_phase_config)
DEFINE_CMD_WRAPPER(cmd_session_set_hybrid_controller_config)
DEFINE_CMD_WRAPPER(cmd_session_set_hybrid_controlee_config)
DEFINE_CMD_WRAPPER(cmd_session_query_data_size_in_ranging)
DEFINE_CMD_WRAPPER(cmd_analyze_packet)
DEFINE_CMD_WRAPPER(cmd_simulate_notification)
DEFINE_CMD_WRAPPER(cmd_simulate_session_status)
DEFINE_CMD_WRAPPER(cmd_simulate_data_credit)
DEFINE_CMD_WRAPPER(cmd_simulate_ranging)
DEFINE_CMD_WRAPPER(cmd_simulate_multi_target_ranging)
DEFINE_CMD_WRAPPER(cmd_demo_session_flow)
DEFINE_CMD_WRAPPER(cmd_simulate_qm_sdk_vendor_command)

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

static const uci_param_def_t k_session_init_params[] = {
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
        .name = "session_type",
        .type = PARAM_TYPE_SESSION_TYPE,
        .flags = PARAM_FLAG_OPTIONAL,
        .max_len = 0,
        .min_value = 0,
        .max_value = 0,
        .description = "Session type (e.g. fira_ranging)",
    },
};

static const uci_param_def_t k_session_id_only_params[] = {
    {
        .name = "session_id",
        .type = PARAM_TYPE_SESSION_ID,
        .flags = PARAM_FLAG_REQUIRED,
        .max_len = 0,
        .min_value = 0,
        .max_value = 0,
        .description = "Numeric session identifier",
    },
};

static const uci_param_def_t k_session_send_data_params[] = {
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
        .name = "destination",
        .type = PARAM_TYPE_STRING,
        .flags = PARAM_FLAG_REQUIRED,
        .max_len = 16,
        .min_value = 0,
        .max_value = 0,
        .description = "Destination short address",
    },
    {
        .name = "sequence",
        .type = PARAM_TYPE_STRING,
        .flags = PARAM_FLAG_REQUIRED,
        .max_len = 8,
        .min_value = 0,
        .max_value = 0,
        .description = "Sequence number",
    },
    {
        .name = "payload",
        .type = PARAM_TYPE_HEX_STRING,
        .flags = PARAM_FLAG_OPTIONAL,
        .max_len = 512,
        .min_value = 0,
        .max_value = 0,
        .description = "Hex payload bytes",
    },
};

static const uci_param_def_t k_session_link_create_params[] = {
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
        .name = "link_id",
        .type = PARAM_TYPE_STRING,
        .flags = PARAM_FLAG_REQUIRED,
        .max_len = 8,
        .min_value = 0,
        .max_value = 0,
        .description = "Logical link identifier",
    },
    {
        .name = "mode",
        .type = PARAM_TYPE_STRING,
        .flags = PARAM_FLAG_OPTIONAL,
        .max_len = 8,
        .min_value = 0,
        .max_value = 0,
        .description = "Link mode (e.g. reliable)",
    },
    {
        .name = "credits",
        .type = PARAM_TYPE_STRING,
        .flags = PARAM_FLAG_OPTIONAL,
        .max_len = 8,
        .min_value = 0,
        .max_value = 0,
        .description = "Initial credit count",
    },
};

static const uci_param_def_t k_session_link_id_params[] = {
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
        .name = "link_id",
        .type = PARAM_TYPE_STRING,
        .flags = PARAM_FLAG_REQUIRED,
        .max_len = 8,
        .min_value = 0,
        .max_value = 0,
        .description = "Logical link identifier",
    },
};

static const uci_param_def_t k_set_app_config_params[] = {
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
        .name = "config_name",
        .type = PARAM_TYPE_STRING,
        .flags = PARAM_FLAG_REQUIRED,
        .max_len = 64,
        .min_value = 0,
        .max_value = 0,
        .description = "Configuration parameter name",
    },
    {
        .name = "value",
        .type = PARAM_TYPE_STRING,
        .flags = PARAM_FLAG_REQUIRED,
        .max_len = 128,
        .min_value = 0,
        .max_value = 0,
        .description = "Value to assign",
    },
};

static const uci_param_def_t k_get_app_config_params[] = {
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
        .name = "config_name",
        .type = PARAM_TYPE_STRING,
        .flags = PARAM_FLAG_OPTIONAL,
        .max_len = 64,
        .min_value = 0,
        .max_value = 0,
        .description = "Optional parameter filter",
    },
};

static const uci_param_def_t k_multicast_list_params[] = {
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
        .name = "action",
        .type = PARAM_TYPE_STRING,
        .flags = PARAM_FLAG_REQUIRED,
        .max_len = 16,
        .min_value = 0,
        .max_value = 0,
        .description = "Action (add|remove)",
    },
    {
        .name = "short_address",
        .type = PARAM_TYPE_STRING,
        .flags = PARAM_FLAG_REQUIRED,
        .max_len = 8,
        .min_value = 0,
        .max_value = 0,
        .description = "Target short address",
    },
    {
        .name = "subsession_id",
        .type = PARAM_TYPE_STRING,
        .flags = PARAM_FLAG_OPTIONAL,
        .max_len = 8,
        .min_value = 0,
        .max_value = 0,
        .description = "Optional subsession identifier",
    },
};

static const uci_param_def_t k_dt_tag_rounds_params[] = {
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
        .name = "round_values",
        .type = PARAM_TYPE_STRING,
        .flags = PARAM_FLAG_OPTIONAL,
        .max_len = 128,
        .min_value = 0,
        .max_value = 0,
        .description = "Comma separated round configuration",
    },
};

static const uci_param_def_t k_session_data_phase_params[] = {
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
        .name = "repetition",
        .type = PARAM_TYPE_STRING,
        .flags = PARAM_FLAG_REQUIRED,
        .max_len = 8,
        .min_value = 0,
        .max_value = 0,
        .description = "Repetition count",
    },
    {
        .name = "control",
        .type = PARAM_TYPE_STRING,
        .flags = PARAM_FLAG_REQUIRED,
        .max_len = 8,
        .min_value = 0,
        .max_value = 0,
        .description = "Control flags",
    },
    {
        .name = "size",
        .type = PARAM_TYPE_STRING,
        .flags = PARAM_FLAG_REQUIRED,
        .max_len = 8,
        .min_value = 0,
        .max_value = 0,
        .description = "Payload size",
    },
    {
        .name = "payload",
        .type = PARAM_TYPE_HEX_STRING,
        .flags = PARAM_FLAG_OPTIONAL,
        .max_len = 512,
        .min_value = 0,
        .max_value = 0,
        .description = "Hex payload bytes",
    },
};

static const uci_param_def_t k_set_hybrid_config_params[] = {
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
        .name = "config_hex",
        .type = PARAM_TYPE_HEX_STRING,
        .flags = PARAM_FLAG_REQUIRED,
        .max_len = 512,
        .min_value = 0,
        .max_value = 0,
        .description = "Hex encoded configuration blob",
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
        .type = PARAM_TYPE_STRING,
        .flags = PARAM_FLAG_REQUIRED,
        .max_len = 8,
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
        .params = NULL,
        .param_count = 0,
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

    // Session lifecycle
    {
        .name = "session_init",
        .aliases = { "session_new", NULL, NULL, NULL },
        .group = CLI_GROUP_SESSION,
        .flags = CLI_CMD_FLAG_NONE,
        .description = "Initialize a ranging session",
        .params = k_session_init_params,
        .param_count = ARRAY_SIZE(k_session_init_params),
        .handler = cmd_session_init_framework_adapter,
    },
    {
        .name = "session_deinit",
        .aliases = { "session_close", NULL, NULL, NULL },
        .group = CLI_GROUP_SESSION,
        .flags = CLI_CMD_FLAG_NONE,
        .description = "Deinitialize a session",
        .params = k_session_id_only_params,
        .param_count = ARRAY_SIZE(k_session_id_only_params),
        .handler = cmd_session_deinit_framework_adapter,
    },
    {
        .name = "session_start",
        .aliases = { "start_ranging", NULL, NULL, NULL },
        .group = CLI_GROUP_SESSION,
        .flags = CLI_CMD_FLAG_NONE,
        .description = "Start a ranging session",
        .params = k_session_id_only_params,
        .param_count = ARRAY_SIZE(k_session_id_only_params),
        .handler = cmd_session_start_framework_adapter,
    },
    {
        .name = "session_stop",
        .aliases = { "stop_ranging", NULL, NULL, NULL },
        .group = CLI_GROUP_SESSION,
        .flags = CLI_CMD_FLAG_NONE,
        .description = "Stop a ranging session",
        .params = k_session_id_only_params,
        .param_count = ARRAY_SIZE(k_session_id_only_params),
        .handler = cmd_session_stop_framework_adapter,
    },
    {
        .name = "session_send_data",
        .aliases = { "send_data", NULL, NULL, NULL },
        .group = CLI_GROUP_SESSION,
        .flags = CLI_CMD_FLAG_NONE,
        .description = "Send DATA_MESSAGE_SND payload",
        .params = k_session_send_data_params,
        .param_count = ARRAY_SIZE(k_session_send_data_params),
        .handler = cmd_session_send_data_framework_adapter,
    },
    {
        .name = "session_logical_link_create",
        .aliases = { NULL },
        .group = CLI_GROUP_SESSION,
        .flags = CLI_CMD_FLAG_NONE,
        .description = "Create a logical link",
        .params = k_session_link_create_params,
        .param_count = ARRAY_SIZE(k_session_link_create_params),
        .handler = cmd_session_logical_link_create_framework_adapter,
    },
    {
        .name = "session_logical_link_close",
        .aliases = { NULL },
        .group = CLI_GROUP_SESSION,
        .flags = CLI_CMD_FLAG_NONE,
        .description = "Close a logical link",
        .params = k_session_link_id_params,
        .param_count = ARRAY_SIZE(k_session_link_id_params),
        .handler = cmd_session_logical_link_close_framework_adapter,
    },
    {
        .name = "session_logical_link_get_param",
        .aliases = { NULL },
        .group = CLI_GROUP_SESSION,
        .flags = CLI_CMD_FLAG_NONE,
        .description = "Query logical link parameters",
        .params = k_session_link_id_params,
        .param_count = ARRAY_SIZE(k_session_link_id_params),
        .handler = cmd_session_logical_link_get_param_framework_adapter,
    },
    {
        .name = "get_session_state",
        .aliases = { "session_status", NULL, NULL, NULL },
        .group = CLI_GROUP_SESSION,
        .flags = CLI_CMD_FLAG_NONE,
        .description = "Report session state",
        .params = k_session_id_only_params,
        .param_count = ARRAY_SIZE(k_session_id_only_params),
        .handler = cmd_get_session_state_framework_adapter,
    },

    // Session configuration
    {
        .name = "set_app_config",
        .aliases = { NULL },
        .group = CLI_GROUP_SESSION_CONFIG,
        .flags = CLI_CMD_FLAG_NONE,
        .description = "Configure session application parameters",
        .params = k_set_app_config_params,
        .param_count = ARRAY_SIZE(k_set_app_config_params),
        .handler = cmd_set_app_config_framework_adapter,
    },
    {
        .name = "get_app_config",
        .aliases = { NULL },
        .group = CLI_GROUP_SESSION_CONFIG,
        .flags = CLI_CMD_FLAG_NONE,
        .description = "Read session application parameters",
        .params = k_get_app_config_params,
        .param_count = ARRAY_SIZE(k_get_app_config_params),
        .handler = cmd_get_app_config_framework_adapter,
    },
    {
        .name = "show_app_configs",
        .aliases = { NULL },
        .group = CLI_GROUP_SESSION_CONFIG,
        .flags = CLI_CMD_FLAG_NONE,
        .description = "List supported session application parameters",
        .params = NULL,
        .param_count = 0,
        .handler = cmd_show_app_configs_framework_adapter,
    },
    {
        .name = "session_update_multicast_list",
        .aliases = { "update_multicast_list", NULL, NULL, NULL },
        .group = CLI_GROUP_SESSION_CONFIG,
        .flags = CLI_CMD_FLAG_NONE,
        .description = "Maintain multicast list entries",
        .params = k_multicast_list_params,
        .param_count = ARRAY_SIZE(k_multicast_list_params),
        .handler = cmd_session_update_multicast_list_framework_adapter,
    },
    {
        .name = "session_update_dt_tag_rounds",
        .aliases = { NULL },
        .group = CLI_GROUP_SESSION_CONFIG,
        .flags = CLI_CMD_FLAG_NONE,
        .description = "Configure DT-Tag active rounds",
        .params = k_dt_tag_rounds_params,
        .param_count = ARRAY_SIZE(k_dt_tag_rounds_params),
        .handler = cmd_session_update_dt_tag_rounds_framework_adapter,
    },
    {
        .name = "session_data_transfer_phase_config",
        .aliases = { NULL },
        .group = CLI_GROUP_SESSION_CONFIG,
        .flags = CLI_CMD_FLAG_NONE,
        .description = "Configure data transfer phase",
        .params = k_session_data_phase_params,
        .param_count = ARRAY_SIZE(k_session_data_phase_params),
        .handler = cmd_session_data_transfer_phase_config_framework_adapter,
    },
    {
        .name = "session_set_hybrid_controller_config",
        .aliases = { NULL },
        .group = CLI_GROUP_SESSION_CONFIG,
        .flags = CLI_CMD_FLAG_NONE,
        .description = "Set hybrid controller configuration",
        .params = k_set_hybrid_config_params,
        .param_count = ARRAY_SIZE(k_set_hybrid_config_params),
        .handler = cmd_session_set_hybrid_controller_config_framework_adapter,
    },
    {
        .name = "session_set_hybrid_controlee_config",
        .aliases = { NULL },
        .group = CLI_GROUP_SESSION_CONFIG,
        .flags = CLI_CMD_FLAG_NONE,
        .description = "Set hybrid controlee configuration",
        .params = k_set_hybrid_config_params,
        .param_count = ARRAY_SIZE(k_set_hybrid_config_params),
        .handler = cmd_session_set_hybrid_controlee_config_framework_adapter,
    },
    {
        .name = "session_query_data_size_in_ranging",
        .aliases = { NULL },
        .group = CLI_GROUP_SESSION_CONFIG,
        .flags = CLI_CMD_FLAG_NONE,
        .description = "Query data size in ranging",
        .params = k_session_id_only_params,
        .param_count = ARRAY_SIZE(k_session_id_only_params),
        .handler = cmd_session_query_data_size_in_ranging_framework_adapter,
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

    // Simulation helpers
    {
        .name = "simulate_notification",
        .aliases = { NULL },
        .group = CLI_GROUP_SIMULATION,
        .flags = CLI_CMD_FLAG_NONE,
        .description = "Simulate device notification",
        .params = k_simulate_notification_params,
        .param_count = ARRAY_SIZE(k_simulate_notification_params),
        .handler = cmd_simulate_notification_framework_adapter,
    },
    {
        .name = "simulate_session_status",
        .aliases = { NULL },
        .group = CLI_GROUP_SIMULATION,
        .flags = CLI_CMD_FLAG_NONE,
        .description = "Simulate session status notification",
        .params = k_simulate_session_status_params,
        .param_count = ARRAY_SIZE(k_simulate_session_status_params),
        .handler = cmd_simulate_session_status_framework_adapter,
    },
    {
        .name = "simulate_data_credit",
        .aliases = { NULL },
        .group = CLI_GROUP_SIMULATION,
        .flags = CLI_CMD_FLAG_NONE,
        .description = "Simulate session data credit notification",
        .params = NULL,
        .param_count = 0,
        .handler = cmd_simulate_data_credit_framework_adapter,
    },
    {
        .name = "simulate_ranging",
        .aliases = { NULL },
        .group = CLI_GROUP_SIMULATION,
        .flags = CLI_CMD_FLAG_NONE,
        .description = "Simulate single-target ranging notification",
        .params = NULL,
        .param_count = 0,
        .handler = cmd_simulate_ranging_framework_adapter,
    },
    {
        .name = "simulate_multi_target_ranging",
        .aliases = { NULL },
        .group = CLI_GROUP_SIMULATION,
        .flags = CLI_CMD_FLAG_NONE,
        .description = "Simulate multi-target ranging notification",
        .params = NULL,
        .param_count = 0,
        .handler = cmd_simulate_multi_target_ranging_framework_adapter,
    },
    {
        .name = "demo_session_flow",
        .aliases = { NULL },
        .group = CLI_GROUP_SIMULATION,
        .flags = CLI_CMD_FLAG_NONE,
        .description = "Demonstrate session flow",
        .params = NULL,
        .param_count = 0,
        .handler = cmd_demo_session_flow_framework_adapter,
    },
    {
        .name = "simulate_qm_sdk_vendor_command",
        .aliases = { NULL },
        .group = CLI_GROUP_SIMULATION,
        .flags = CLI_CMD_FLAG_NONE,
        .description = "Simulate QM SDK vendor-specific command (GID 0x0B)",
        .params = k_vendor_command_params,
        .param_count = ARRAY_SIZE(k_vendor_command_params),
        .handler = cmd_simulate_qm_sdk_vendor_command_framework_adapter,
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
