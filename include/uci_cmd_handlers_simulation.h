#ifndef UCI_CMD_HANDLERS_SIMULATION_H
#define UCI_CMD_HANDLERS_SIMULATION_H

int cmd_simulate_notification(int argc, char** argv);
int cmd_simulate_session_status(int argc, char** argv);
int cmd_simulate_data_credit(int argc, char** argv);
int cmd_demo_session_flow(int argc, char** argv);
int cmd_simulate_ranging(int argc, char** argv);
int cmd_simulate_multi_target_ranging(int argc, char** argv);
int cmd_simulate_qm_sdk_vendor_command(int argc, char** argv);

#endif /* UCI_CMD_HANDLERS_SIMULATION_H */
