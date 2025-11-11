#include "../include/uci_cmd_framework_session.h"
#include "../include/uci_cmd_framework_wrappers.h"
#include "../include/uci_cmd_handlers.h"

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
DEFINE_CMD_WRAPPER(cmd_show_app_configs)
DEFINE_CMD_WRAPPER(cmd_session_update_multicast_list)
DEFINE_CMD_WRAPPER(cmd_session_update_dt_tag_rounds)
DEFINE_CMD_WRAPPER(cmd_session_data_transfer_phase_config)
DEFINE_CMD_WRAPPER(cmd_session_set_hybrid_controller_config)
DEFINE_CMD_WRAPPER(cmd_session_set_hybrid_controlee_config)
DEFINE_CMD_WRAPPER(cmd_session_query_data_size_in_ranging)

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
        .type = PARAM_TYPE_UINT64,
        .flags = PARAM_FLAG_REQUIRED,
        .max_len = 0,
        .min_value = 0,
        .max_value = 0xFFFFFFFFFFFFFFFFULL,
        .description = "Destination short address (hex or decimal)",
    },
    {
        .name = "sequence",
        .type = PARAM_TYPE_UINT16,
        .flags = PARAM_FLAG_REQUIRED,
        .max_len = 0,
        .min_value = 0,
        .max_value = 0xFFFF,
        .description = "Sequence number (0-65535)",
    },
    {
        .name = "payload",
        .type = PARAM_TYPE_HEX_STRING,
        .flags = PARAM_FLAG_REQUIRED,
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
        .description = "Named app configuration parameter",
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
        .type = PARAM_TYPE_UINT16,
        .flags = PARAM_FLAG_REQUIRED,
        .max_len = 0,
        .min_value = 0,
        .max_value = 0xFFFF,
        .description = "Target short address",
    },
    {
        .name = "subsession_id",
        .type = PARAM_TYPE_UINT32,
        .flags = PARAM_FLAG_REQUIRED,
        .max_len = 0,
        .min_value = 0,
        .max_value = 0xFFFFFFFFu,
        .description = "Subsession identifier",
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
        .type = PARAM_TYPE_UINT8,
        .flags = PARAM_FLAG_REQUIRED,
        .max_len = 0,
        .min_value = 0,
        .max_value = 255,
        .description = "Repetition count (0-255)",
    },
    {
        .name = "control",
        .type = PARAM_TYPE_UINT8,
        .flags = PARAM_FLAG_REQUIRED,
        .max_len = 0,
        .min_value = 0,
        .max_value = 255,
        .description = "Control flags byte",
    },
    {
        .name = "size",
        .type = PARAM_TYPE_UINT8,
        .flags = PARAM_FLAG_REQUIRED,
        .max_len = 0,
        .min_value = 0,
        .max_value = 64,
        .description = "Declared payload size (0-64 bytes)",
    },
    {
        .name = "payload",
        .type = PARAM_TYPE_HEX_STRING,
        .flags = PARAM_FLAG_OPTIONAL,
        .max_len = 128,
        .min_value = 0,
        .max_value = 0,
        .description = "Optional payload bytes as hex; length must match size",
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

const uci_command_def_t g_uci_session_command_defs[] = {
    // Session commands
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

    // Session configuration commands
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
};

const int g_uci_session_command_defs_count = (int)ARRAY_SIZE(g_uci_session_command_defs);
