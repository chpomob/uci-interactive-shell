#include "uci_cli.h"
#include "uci_cmd_framework_bridge.h"

// Command table definition
const cli_command_t g_cli_commands[] = {
    { "help", { NULL }, CLI_GROUP_GENERAL, CLI_CMD_FLAG_NONE, "Show this help message", uci_cmd_framework_handler },
    { "mode_sim", { "sim_mode", NULL }, CLI_GROUP_HARDWARE, CLI_CMD_FLAG_NONE, "Switch to simulation mode", uci_cmd_framework_handler },
    { "mode_hw", { "hw_mode", NULL }, CLI_GROUP_HARDWARE, CLI_CMD_FLAG_NONE, "Switch to hardware mode", uci_cmd_framework_handler },
    { "mode_info", { "current_mode", NULL }, CLI_GROUP_HARDWARE, CLI_CMD_FLAG_NONE, "Display current mode", uci_cmd_framework_handler },
    { "hw_init", { "hw_connect", NULL }, CLI_GROUP_HARDWARE, CLI_CMD_FLAG_NONE, "Initialize hardware mode and connect", uci_cmd_framework_handler },
    { "hw_send", { NULL }, CLI_GROUP_HARDWARE, CLI_CMD_FLAG_REQUIRES_HW_MODE, "Send a raw packet to hardware", uci_cmd_framework_handler },

    { "get_device_info", { "device_info", NULL }, CLI_GROUP_DEVICE, CLI_CMD_FLAG_NONE, "Query device information", uci_cmd_framework_handler },
    { "device_reset", { NULL }, CLI_GROUP_DEVICE, CLI_CMD_FLAG_NONE, "Reset the connected device", uci_cmd_framework_handler },
    { "set_power", { NULL }, CLI_GROUP_DEVICE, CLI_CMD_FLAG_NONE, "Set device power state", uci_cmd_framework_handler },
    { "device_on", { NULL }, CLI_GROUP_DEVICE, CLI_CMD_FLAG_NONE, "Power on the device", uci_cmd_framework_handler },
    { "device_off", { NULL }, CLI_GROUP_DEVICE, CLI_CMD_FLAG_NONE, "Power off the device", uci_cmd_framework_handler },
    { "get_caps_info", { NULL }, CLI_GROUP_DEVICE, CLI_CMD_FLAG_NONE, "Query capability information", uci_cmd_framework_handler },
    { "get_config", { NULL }, CLI_GROUP_DEVICE, CLI_CMD_FLAG_NONE, "Read a device configuration parameter", uci_cmd_framework_handler },
    { "get_device_state", { NULL }, CLI_GROUP_DEVICE, CLI_CMD_FLAG_NONE, "Report current device state", uci_cmd_framework_handler },
    { "set_device_active", { NULL }, CLI_GROUP_DEVICE, CLI_CMD_FLAG_NONE, "Force device Active state", uci_cmd_framework_handler },
    { "set_device_ready", { NULL }, CLI_GROUP_DEVICE, CLI_CMD_FLAG_NONE, "Force device Ready state", uci_cmd_framework_handler },
    { "set_config", { NULL }, CLI_GROUP_DEVICE, CLI_CMD_FLAG_NONE, "Update a device configuration parameter", uci_cmd_framework_handler },
    { "show_device_configs", { NULL }, CLI_GROUP_DEVICE, CLI_CMD_FLAG_NONE, "List supported device configuration parameters", uci_cmd_framework_handler },
    { "device_suspend", { NULL }, CLI_GROUP_DEVICE, CLI_CMD_FLAG_NONE, "Suspend device operation", uci_cmd_framework_handler },
    { "query_timestamp", { NULL }, CLI_GROUP_DEVICE, CLI_CMD_FLAG_NONE, "Query device timestamp", uci_cmd_framework_handler },
    { "validate_arguments", { NULL }, CLI_GROUP_DEVICE, CLI_CMD_FLAG_NONE, "Demonstrate framework-driven argument validation", uci_cmd_framework_handler },

    { "session_init", { "session_new", NULL }, CLI_GROUP_SESSION, CLI_CMD_FLAG_NONE, "Initialize a ranging session", uci_cmd_framework_handler },
    { "session_deinit", { "session_close", NULL }, CLI_GROUP_SESSION, CLI_CMD_FLAG_NONE, "Deinitialize a session", uci_cmd_framework_handler },
    { "session_start", { "start_ranging", NULL }, CLI_GROUP_SESSION, CLI_CMD_FLAG_NONE, "Start a ranging session", uci_cmd_framework_handler },
    { "session_stop", { "stop_ranging", NULL }, CLI_GROUP_SESSION, CLI_CMD_FLAG_NONE, "Stop a ranging session", uci_cmd_framework_handler },
    { "session_send_data", { "send_data", NULL }, CLI_GROUP_SESSION, CLI_CMD_FLAG_NONE, "Send DATA_MESSAGE_SND payload", uci_cmd_framework_handler },
    { "session_logical_link_create", { NULL }, CLI_GROUP_SESSION, CLI_CMD_FLAG_NONE, "Create a logical link", uci_cmd_framework_handler },
    { "session_logical_link_close", { NULL }, CLI_GROUP_SESSION, CLI_CMD_FLAG_NONE, "Close a logical link", uci_cmd_framework_handler },
    { "session_logical_link_get_param", { NULL }, CLI_GROUP_SESSION, CLI_CMD_FLAG_NONE, "Query logical link parameters", uci_cmd_framework_handler },
    { "get_session_state", { "session_status", NULL }, CLI_GROUP_SESSION, CLI_CMD_FLAG_NONE, "Report session state", uci_cmd_framework_handler },

    { "set_app_config", { NULL }, CLI_GROUP_SESSION_CONFIG, CLI_CMD_FLAG_NONE, "Configure session application parameters", uci_cmd_framework_handler },
    { "get_app_config", { NULL }, CLI_GROUP_SESSION_CONFIG, CLI_CMD_FLAG_NONE, "Read session application parameters", uci_cmd_framework_handler },
    { "show_app_configs", { NULL }, CLI_GROUP_SESSION_CONFIG, CLI_CMD_FLAG_NONE, "List supported session application parameters", uci_cmd_framework_handler },
    { "session_update_multicast_list", { "update_multicast_list", NULL }, CLI_GROUP_SESSION_CONFIG, CLI_CMD_FLAG_NONE, "Maintain multicast list entries", uci_cmd_framework_handler },
    { "session_update_dt_tag_rounds", { NULL }, CLI_GROUP_SESSION_CONFIG, CLI_CMD_FLAG_NONE, "Configure DT-Tag active rounds", uci_cmd_framework_handler },
    { "session_data_transfer_phase_config", { NULL }, CLI_GROUP_SESSION_CONFIG, CLI_CMD_FLAG_NONE, "Configure data transfer phase", uci_cmd_framework_handler },
    { "session_set_hybrid_controller_config", { NULL }, CLI_GROUP_SESSION_CONFIG, CLI_CMD_FLAG_NONE, "Set hybrid controller configuration", uci_cmd_framework_handler },
    { "session_set_hybrid_controlee_config", { NULL }, CLI_GROUP_SESSION_CONFIG, CLI_CMD_FLAG_NONE, "Set hybrid controlee configuration", uci_cmd_framework_handler },
    { "session_query_data_size_in_ranging", { NULL }, CLI_GROUP_SESSION_CONFIG, CLI_CMD_FLAG_NONE, "Query data size in ranging", uci_cmd_framework_handler },

    { "analyze_packet", { NULL }, CLI_GROUP_ANALYSIS, CLI_CMD_FLAG_NONE, "Analyze packet bytes with enhanced decoder", uci_cmd_framework_handler },

    { "simulate_notification", { NULL }, CLI_GROUP_SIMULATION, CLI_CMD_FLAG_NONE, "Simulate device notification", uci_cmd_framework_handler },
    { "simulate_session_status", { NULL }, CLI_GROUP_SIMULATION, CLI_CMD_FLAG_NONE, "Simulate session status notification", uci_cmd_framework_handler },
    { "simulate_data_credit", { NULL }, CLI_GROUP_SIMULATION, CLI_CMD_FLAG_NONE, "Simulate session data credit notification", uci_cmd_framework_handler },
    { "simulate_ranging", { NULL }, CLI_GROUP_SIMULATION, CLI_CMD_FLAG_NONE, "Simulate single-target ranging notification", uci_cmd_framework_handler },
    { "simulate_multi_target_ranging", { NULL }, CLI_GROUP_SIMULATION, CLI_CMD_FLAG_NONE, "Simulate multi-target ranging notification", uci_cmd_framework_handler },
    { "demo_session_flow", { NULL }, CLI_GROUP_SIMULATION, CLI_CMD_FLAG_NONE, "Demonstrate session flow", uci_cmd_framework_handler },
    { "simulate_qm_sdk_vendor_command", { NULL }, CLI_GROUP_SIMULATION, CLI_CMD_FLAG_NONE, "Simulate QM SDK vendor-specific command (GID 0x0B)", uci_cmd_framework_handler },
};

const int g_cli_commands_count = sizeof(g_cli_commands) / sizeof(cli_command_t);
