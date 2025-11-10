#include "uci_cli.h"
#include "uci_cmd_handlers.h"
#include "uci_cmd_framework_bridge.h"

// Command table definition
const cli_command_t g_cli_commands[] = {
    { "help", { NULL }, CLI_GROUP_GENERAL, CLI_CMD_FLAG_NONE, "Show this help message", cmd_help },
    { "mode_sim", { "sim_mode", NULL }, CLI_GROUP_HARDWARE, CLI_CMD_FLAG_NONE, "Switch to simulation mode", cmd_mode_sim },
    { "mode_hw", { "hw_mode", NULL }, CLI_GROUP_HARDWARE, CLI_CMD_FLAG_NONE, "Switch to hardware mode", cmd_mode_hw },
    { "mode_info", { "current_mode", NULL }, CLI_GROUP_HARDWARE, CLI_CMD_FLAG_NONE, "Display current mode", cmd_mode_info },
    { "hw_init", { "hw_connect", NULL }, CLI_GROUP_HARDWARE, CLI_CMD_FLAG_NONE, "Initialize hardware mode and connect", cmd_hw_init },
    { "hw_send", { NULL }, CLI_GROUP_HARDWARE, CLI_CMD_FLAG_REQUIRES_HW_MODE, "Send a raw packet to hardware", cmd_hw_send },

    { "get_device_info", { "device_info", NULL }, CLI_GROUP_DEVICE, CLI_CMD_FLAG_NONE, "Query device information", uci_cmd_framework_handler },
    { "device_reset", { NULL }, CLI_GROUP_DEVICE, CLI_CMD_FLAG_NONE, "Reset the connected device", uci_cmd_framework_handler },
    { "set_power", { NULL }, CLI_GROUP_DEVICE, CLI_CMD_FLAG_NONE, "Set device power state", uci_cmd_framework_handler },
    { "device_on", { NULL }, CLI_GROUP_DEVICE, CLI_CMD_FLAG_NONE, "Power on the device", uci_cmd_framework_handler },
    { "device_off", { NULL }, CLI_GROUP_DEVICE, CLI_CMD_FLAG_NONE, "Power off the device", uci_cmd_framework_handler },
    { "get_caps_info", { NULL }, CLI_GROUP_DEVICE, CLI_CMD_FLAG_NONE, "Query capability information", cmd_get_caps_info },
    { "get_config", { NULL }, CLI_GROUP_DEVICE, CLI_CMD_FLAG_NONE, "Read a device configuration parameter", uci_cmd_framework_handler },
    { "get_device_state", { NULL }, CLI_GROUP_DEVICE, CLI_CMD_FLAG_NONE, "Report current device state", uci_cmd_framework_handler },
    { "set_device_active", { NULL }, CLI_GROUP_DEVICE, CLI_CMD_FLAG_NONE, "Force device Active state", uci_cmd_framework_handler },
    { "set_device_ready", { NULL }, CLI_GROUP_DEVICE, CLI_CMD_FLAG_NONE, "Force device Ready state", uci_cmd_framework_handler },
    { "set_config", { NULL }, CLI_GROUP_DEVICE, CLI_CMD_FLAG_NONE, "Update a device configuration parameter", uci_cmd_framework_handler },
    { "show_device_configs", { NULL }, CLI_GROUP_DEVICE, CLI_CMD_FLAG_NONE, "List supported device configuration parameters", cmd_show_device_configs },
    { "device_suspend", { NULL }, CLI_GROUP_DEVICE, CLI_CMD_FLAG_NONE, "Suspend device operation", uci_cmd_framework_handler },
    { "query_timestamp", { NULL }, CLI_GROUP_DEVICE, CLI_CMD_FLAG_NONE, "Query device timestamp", uci_cmd_framework_handler },
    { "validate_arguments", { NULL }, CLI_GROUP_DEVICE, CLI_CMD_FLAG_NONE, "Demonstrate framework-driven argument validation", uci_cmd_framework_handler },

    { "session_init", { "session_new", NULL }, CLI_GROUP_SESSION, CLI_CMD_FLAG_NONE, "Initialize a ranging session", cmd_session_init },
    { "session_deinit", { "session_close", NULL }, CLI_GROUP_SESSION, CLI_CMD_FLAG_NONE, "Deinitialize a session", cmd_session_deinit },
    { "session_start", { "start_ranging", NULL }, CLI_GROUP_SESSION, CLI_CMD_FLAG_NONE, "Start a ranging session", cmd_session_start },
    { "session_stop", { "stop_ranging", NULL }, CLI_GROUP_SESSION, CLI_CMD_FLAG_NONE, "Stop a ranging session", cmd_session_stop },
    { "session_send_data", { "send_data", NULL }, CLI_GROUP_SESSION, CLI_CMD_FLAG_NONE, "Send DATA_MESSAGE_SND payload", cmd_session_send_data },
    { "session_logical_link_create", { NULL }, CLI_GROUP_SESSION, CLI_CMD_FLAG_NONE, "Create a logical link", cmd_session_logical_link_create },
    { "session_logical_link_close", { NULL }, CLI_GROUP_SESSION, CLI_CMD_FLAG_NONE, "Close a logical link", cmd_session_logical_link_close },
    { "session_logical_link_get_param", { NULL }, CLI_GROUP_SESSION, CLI_CMD_FLAG_NONE, "Query logical link parameters", cmd_session_logical_link_get_param },
    { "get_session_state", { "session_status", NULL }, CLI_GROUP_SESSION, CLI_CMD_FLAG_NONE, "Report session state", cmd_get_session_state },

    { "set_app_config", { NULL }, CLI_GROUP_SESSION_CONFIG, CLI_CMD_FLAG_NONE, "Configure session application parameters", cmd_set_app_config },
    { "get_app_config", { NULL }, CLI_GROUP_SESSION_CONFIG, CLI_CMD_FLAG_NONE, "Read session application parameters", cmd_get_app_config },
    { "show_app_configs", { NULL }, CLI_GROUP_SESSION_CONFIG, CLI_CMD_FLAG_NONE, "List supported session application parameters", cmd_show_app_configs },
    { "session_update_multicast_list", { "update_multicast_list", NULL }, CLI_GROUP_SESSION_CONFIG, CLI_CMD_FLAG_NONE, "Maintain multicast list entries", cmd_session_update_multicast_list },
    { "session_update_dt_tag_rounds", { NULL }, CLI_GROUP_SESSION_CONFIG, CLI_CMD_FLAG_NONE, "Configure DT-Tag active rounds", cmd_session_update_dt_tag_rounds },
    { "session_data_transfer_phase_config", { NULL }, CLI_GROUP_SESSION_CONFIG, CLI_CMD_FLAG_NONE, "Configure data transfer phase", cmd_session_data_transfer_phase_config },
    { "session_set_hybrid_controller_config", { NULL }, CLI_GROUP_SESSION_CONFIG, CLI_CMD_FLAG_NONE, "Set hybrid controller configuration", cmd_session_set_hybrid_controller_config },
    { "session_set_hybrid_controlee_config", { NULL }, CLI_GROUP_SESSION_CONFIG, CLI_CMD_FLAG_NONE, "Set hybrid controlee configuration", cmd_session_set_hybrid_controlee_config },
    { "session_query_data_size_in_ranging", { NULL }, CLI_GROUP_SESSION_CONFIG, CLI_CMD_FLAG_NONE, "Query data size in ranging", cmd_session_query_data_size_in_ranging },

    { "analyze_packet", { NULL }, CLI_GROUP_ANALYSIS, CLI_CMD_FLAG_NONE, "Analyze packet bytes with enhanced decoder", cmd_analyze_packet },

    { "simulate_notification", { NULL }, CLI_GROUP_SIMULATION, CLI_CMD_FLAG_NONE, "Simulate device notification", cmd_simulate_notification },
    { "simulate_session_status", { NULL }, CLI_GROUP_SIMULATION, CLI_CMD_FLAG_NONE, "Simulate session status notification", cmd_simulate_session_status },
    { "simulate_data_credit", { NULL }, CLI_GROUP_SIMULATION, CLI_CMD_FLAG_NONE, "Simulate session data credit notification", cmd_simulate_data_credit },
    { "simulate_ranging", { NULL }, CLI_GROUP_SIMULATION, CLI_CMD_FLAG_NONE, "Simulate single-target ranging notification", cmd_simulate_ranging },
    { "simulate_multi_target_ranging", { NULL }, CLI_GROUP_SIMULATION, CLI_CMD_FLAG_NONE, "Simulate multi-target ranging notification", cmd_simulate_multi_target_ranging },
    { "demo_session_flow", { NULL }, CLI_GROUP_SIMULATION, CLI_CMD_FLAG_NONE, "Demonstrate session flow", cmd_demo_session_flow },
    { "simulate_qm_sdk_vendor_command", { NULL }, CLI_GROUP_SIMULATION, CLI_CMD_FLAG_NONE, "Simulate QM SDK vendor-specific command (GID 0x0B)", cmd_simulate_qm_sdk_vendor_command },
};

const int g_cli_commands_count = sizeof(g_cli_commands) / sizeof(cli_command_t);
