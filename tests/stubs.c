// Stub implementations for command handler functions to allow tests to link
// These are only used for linking purposes - tests don't actually call them

#include "../include/uci.h"
#include "../include/uci_cli.h"
#include "../include/uci_cmd_handlers.h"
#include "../include/uci_cmd_handlers_simulation.h"

// Stub implementations that just return -1 (error)
static int stub_cmd_handler(int argc, char** argv) {
    (void)argc;
    (void)argv;
    return -1;
}

// Define all the missing command handlers as stubs
int cmd_session_init(int argc, char** argv) { return stub_cmd_handler(argc, argv); }
int cmd_session_deinit(int argc, char** argv) { return stub_cmd_handler(argc, argv); }
int cmd_session_start(int argc, char** argv) { return stub_cmd_handler(argc, argv); }
int cmd_session_stop(int argc, char** argv) { return stub_cmd_handler(argc, argv); }
int cmd_session_send_data(int argc, char** argv) { return stub_cmd_handler(argc, argv); }
int cmd_session_logical_link_create(int argc, char** argv) { return stub_cmd_handler(argc, argv); }
int cmd_session_logical_link_close(int argc, char** argv) { return stub_cmd_handler(argc, argv); }
int cmd_session_logical_link_get_param(int argc, char** argv) { return stub_cmd_handler(argc, argv); }
int cmd_get_session_state(int argc, char** argv) { return stub_cmd_handler(argc, argv); }
int cmd_set_app_config(int argc, char** argv) { return stub_cmd_handler(argc, argv); }
int cmd_get_app_config(int argc, char** argv) { return stub_cmd_handler(argc, argv); }
int cmd_session_update_multicast_list(int argc, char** argv) { return stub_cmd_handler(argc, argv); }
int cmd_session_update_dt_tag_rounds(int argc, char** argv) { return stub_cmd_handler(argc, argv); }
int cmd_session_data_transfer_phase_config(int argc, char** argv) { return stub_cmd_handler(argc, argv); }
int cmd_session_set_hybrid_controller_config(int argc, char** argv) { return stub_cmd_handler(argc, argv); }
int cmd_session_set_hybrid_controlee_config(int argc, char** argv) { return stub_cmd_handler(argc, argv); }
int cmd_session_query_data_size_in_ranging(int argc, char** argv) { return stub_cmd_handler(argc, argv); }
int cmd_simulate_notification(int argc, char** argv) { return stub_cmd_handler(argc, argv); }
int cmd_simulate_session_status(int argc, char** argv) { return stub_cmd_handler(argc, argv); }
int cmd_simulate_data_credit(int argc, char** argv) { return stub_cmd_handler(argc, argv); }
int cmd_simulate_ranging(int argc, char** argv) { return stub_cmd_handler(argc, argv); }
int cmd_simulate_multi_target_ranging(int argc, char** argv) { return stub_cmd_handler(argc, argv); }
int cmd_demo_session_flow(int argc, char** argv) { return stub_cmd_handler(argc, argv); }
int cmd_analyze_packet(int argc, char** argv) { return stub_cmd_handler(argc, argv); }
int cmd_help(int argc, char** argv) { return stub_cmd_handler(argc, argv); }
int cmd_simulate_qm_sdk_vendor_command(int argc, char** argv) { return stub_cmd_handler(argc, argv); }
