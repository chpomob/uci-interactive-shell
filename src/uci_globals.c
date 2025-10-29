#include "../include/uci_globals.h"

// Forward declarations for command handlers
extern int cmd_hw_init(int argc, char** argv);
extern int cmd_hw_send(int argc, char** argv);
extern int cmd_mode_sim(int argc, char** argv);
extern int cmd_mode_hw(int argc, char** argv);
extern int cmd_mode_info(int argc, char** argv);
extern int cmd_get_device_info(int argc, char** argv);
extern int cmd_device_reset(int argc, char** argv);
extern int cmd_set_power(int argc, char** argv);
extern int cmd_device_on(int argc, char** argv);
extern int cmd_device_off(int argc, char** argv);
extern int cmd_get_caps_info(int argc, char** argv);
extern int cmd_get_config(int argc, char** argv);
extern int cmd_get_device_state(int argc, char** argv);
extern int cmd_set_device_active(int argc, char** argv);
extern int cmd_set_device_ready(int argc, char** argv);
extern int cmd_set_config(int argc, char** argv);
extern int cmd_device_suspend(int argc, char** argv);
extern int cmd_query_timestamp(int argc, char** argv);
extern int cmd_session_init(int argc, char** argv);
extern int cmd_session_deinit(int argc, char** argv);
extern int cmd_session_start(int argc, char** argv);
extern int cmd_session_stop(int argc, char** argv);
extern int cmd_session_send_data(int argc, char** argv);
extern int cmd_session_logical_link_create(int argc, char** argv);
extern int cmd_session_logical_link_close(int argc, char** argv);
extern int cmd_session_logical_link_get_param(int argc, char** argv);
extern int cmd_get_session_state(int argc, char** argv);
extern int cmd_set_app_config(int argc, char** argv);
extern int cmd_get_app_config(int argc, char** argv);
extern int cmd_session_update_multicast_list(int argc, char** argv);
extern int cmd_session_update_dt_tag_rounds(int argc, char** argv);
extern int cmd_session_data_transfer_phase_config(int argc, char** argv);
extern int cmd_session_set_hybrid_controller_config(int argc, char** argv);
extern int cmd_session_set_hybrid_controlee_config(int argc, char** argv);
extern int cmd_session_query_data_size_in_ranging(int argc, char** argv);
extern int cmd_simulate_notification(int argc, char** argv);
extern int cmd_simulate_session_status(int argc, char** argv);
extern int cmd_simulate_data_credit(int argc, char** argv);
extern int cmd_simulate_ranging(int argc, char** argv);
extern int cmd_simulate_multi_target_ranging(int argc, char** argv);
extern int cmd_demo_session_flow(int argc, char** argv);
extern int cmd_analyze_packet(int argc, char** argv);
extern int cmd_help(int argc, char** argv);

// Command table definition
const cli_command_t g_cli_commands[] = {
    { "help", { NULL }, CLI_GROUP_GENERAL, CLI_CMD_FLAG_NONE, "Show this help message", cmd_help },
    { "mode_sim", { "sim_mode", NULL }, CLI_GROUP_HARDWARE, CLI_CMD_FLAG_NONE, "Switch to simulation mode", cmd_mode_sim },
    { "mode_hw", { "hw_mode", NULL }, CLI_GROUP_HARDWARE, CLI_CMD_FLAG_NONE, "Switch to hardware mode", cmd_mode_hw },
    { "mode_info", { "current_mode", NULL }, CLI_GROUP_HARDWARE, CLI_CMD_FLAG_NONE, "Display current mode", cmd_mode_info },
    { "hw_init", { "hw_connect", NULL }, CLI_GROUP_HARDWARE, CLI_CMD_FLAG_NONE, "Initialize hardware mode and connect", cmd_hw_init },
    { "hw_send", { NULL }, CLI_GROUP_HARDWARE, CLI_CMD_FLAG_REQUIRES_HW_MODE, "Send a raw packet to hardware", cmd_hw_send },

    { "get_device_info", { "device_info", NULL }, CLI_GROUP_DEVICE, CLI_CMD_FLAG_NONE, "Query device information", cmd_get_device_info },
    { "device_reset", { NULL }, CLI_GROUP_DEVICE, CLI_CMD_FLAG_NONE, "Reset the connected device", cmd_device_reset },
    { "set_power", { NULL }, CLI_GROUP_DEVICE, CLI_CMD_FLAG_NONE, "Set device power state", cmd_set_power },
    { "device_on", { NULL }, CLI_GROUP_DEVICE, CLI_CMD_FLAG_NONE, "Power on the device", cmd_device_on },
    { "device_off", { NULL }, CLI_GROUP_DEVICE, CLI_CMD_FLAG_NONE, "Power off the device", cmd_device_off },
    { "get_caps_info", { NULL }, CLI_GROUP_DEVICE, CLI_CMD_FLAG_NONE, "Query capability information", cmd_get_caps_info },
    { "get_config", { NULL }, CLI_GROUP_DEVICE, CLI_CMD_FLAG_NONE, "Read a device configuration parameter", cmd_get_config },
    { "get_device_state", { NULL }, CLI_GROUP_DEVICE, CLI_CMD_FLAG_NONE, "Report current device state", cmd_get_device_state },
    { "set_device_active", { NULL }, CLI_GROUP_DEVICE, CLI_CMD_FLAG_NONE, "Force device Active state", cmd_set_device_active },
    { "set_device_ready", { NULL }, CLI_GROUP_DEVICE, CLI_CMD_FLAG_NONE, "Force device Ready state", cmd_set_device_ready },
    { "set_config", { NULL }, CLI_GROUP_DEVICE, CLI_CMD_FLAG_NONE, "Update a device configuration parameter", cmd_set_config },
    { "device_suspend", { NULL }, CLI_GROUP_DEVICE, CLI_CMD_FLAG_NONE, "Suspend device operation", cmd_device_suspend },
    { "query_timestamp", { NULL }, CLI_GROUP_DEVICE, CLI_CMD_FLAG_NONE, "Query device timestamp", cmd_query_timestamp },

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
};

const int g_cli_commands_count = sizeof(g_cli_commands) / sizeof(cli_command_t);

// Global variables definitions
int g_hardware_mode = 0;
uci_hw_chardev_t g_uwb_chardev;
