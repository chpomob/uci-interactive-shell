#include "../include/uci_command_framework.h"

// Example: Define parameters for session_init command
static const uci_param_def_t session_init_params[] = {
    {
        .name = "session_id",
        .type = PARAM_TYPE_UINT32,
        .flags = PARAM_FLAG_REQUIRED,
        .min_value = 0,
        .max_value = 0xFFFFFFFF,
        .description = "Unique identifier for the session"
    },
    {
        .name = "session_type",
        .type = PARAM_TYPE_SESSION_TYPE,
        .flags = PARAM_FLAG_REQUIRED,
        .description = "Type of session (fira_ranging, ranging_and_data, etc.)"
    }
};

// Example: Define parameters for device state command
static const uci_param_def_t device_state_params[] = {
    {
        .name = "state",
        .type = PARAM_TYPE_DEVICE_STATE,
        .flags = PARAM_FLAG_REQUIRED,
        .description = "Device state (active, ready, sleep)"
    }
};

// Example: Define parameters for session operations
static const uci_param_def_t session_id_params[] = {
    {
        .name = "session_id",
        .type = PARAM_TYPE_UINT32,
        .flags = PARAM_FLAG_REQUIRED,
        .min_value = 0,
        .max_value = 0xFFFFFFFF,
        .description = "Session identifier"
    }
};

// Define all command definitions using the new framework

const uci_command_def_t g_uci_command_defs[] = {
    {
        .name = "get_device_info",
        .aliases = { "device_info", NULL },
        .group = CLI_GROUP_DEVICE,
        .flags = CLI_CMD_FLAG_NONE,
        .description = "Query device information",
        .params = NULL,
        .param_count = 0,
        .handler = NULL,  // Will be implemented with new framework
    },
    {
        .name = "device_reset",
        .aliases = { NULL },
        .group = CLI_GROUP_DEVICE,
        .flags = CLI_CMD_FLAG_NONE,
        .description = "Reset the connected device",
        .params = NULL,
        .param_count = 0,
        .handler = NULL,  // Will be implemented with new framework
    },
    {
        .name = "set_power",
        .aliases = { NULL },
        .group = CLI_GROUP_DEVICE,
        .flags = CLI_CMD_FLAG_NONE,
        .description = "Set device power state",
        .params = device_state_params,
        .param_count = 1,
        .handler = NULL,  // Will be implemented with new framework
    },
    {
        .name = "get_config",
        .aliases = { NULL },
        .group = CLI_GROUP_DEVICE,
        .flags = CLI_CMD_FLAG_NONE,
        .description = "Read a device configuration parameter",
        .params = NULL,  // Will define parameters later
        .param_count = 0,
        .handler = NULL,  // Will be implemented with new framework
    },
    {
        .name = "set_config",
        .aliases = { NULL },
        .group = CLI_GROUP_DEVICE,
        .flags = CLI_CMD_FLAG_NONE,
        .description = "Update a device configuration parameter",
        .params = NULL,  // Will define parameters later
        .param_count = 0,
        .handler = NULL,  // Will be implemented with new framework
    },
    
    /* Session commands */
    {
        .name = "session_init",
        .aliases = { "session_new", NULL },
        .group = CLI_GROUP_SESSION,
        .flags = CLI_CMD_FLAG_NONE,
        .description = "Initialize a ranging session",
        .params = session_init_params,
        .param_count = 2,
        .handler = NULL,  // Will be implemented with new framework
    },
    {
        .name = "session_deinit",
        .aliases = { "session_close", NULL },
        .group = CLI_GROUP_SESSION,
        .flags = CLI_CMD_FLAG_NONE,
        .description = "Deinitialize a session",
        .params = session_id_params,
        .param_count = 1,
        .handler = NULL,  // Will be implemented with new framework
    },
    {
        .name = "session_start",
        .aliases = { "start_ranging", NULL },
        .group = CLI_GROUP_SESSION,
        .flags = CLI_CMD_FLAG_NONE,
        .description = "Start a ranging session",
        .params = session_id_params,
        .param_count = 1,
        .handler = NULL,  // Will be implemented with new framework
    },
    {
        .name = "session_stop",
        .aliases = { "stop_ranging", NULL },
        .group = CLI_GROUP_SESSION,
        .flags = CLI_CMD_FLAG_NONE,
        .description = "Stop a ranging session",
        .params = session_id_params,
        .param_count = 1,
        .handler = NULL,  // Will be implemented with new framework
    },
    {
        .name = "get_session_state",
        .aliases = { "session_status", NULL },
        .group = CLI_GROUP_SESSION,
        .flags = CLI_CMD_FLAG_NONE,
        .description = "Report current session state",
        .params = session_id_params,
        .param_count = 1,
        .handler = NULL,  // Will be implemented with new framework
    },
};

const int g_uci_command_defs_count = sizeof(g_uci_command_defs) / sizeof(uci_command_def_t);