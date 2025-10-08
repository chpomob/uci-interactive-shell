#ifndef UCI_CMD_SIMULATION_H
#define UCI_CMD_SIMULATION_H

/**
 * Handle simulate_notification command
 * Simulates a UCI device status notification
 */
void handle_simulate_notification_command(void);

/**
 * Handle simulate_session_status command
 * Simulates a session status notification with custom state
 * @param session_id_str Session ID in hex format
 * @param state_str Session state (init, active, idle, deinit)
 */
void handle_simulate_session_status_command(char* session_id_str, char* state_str);

/**
 * Handle simulate_data_credit command
 * Simulates a data credit notification
 * @param session_id_str Session ID in hex format
 */
void handle_simulate_data_credit_command(char* session_id_str);

/**
 * Handle demo_session_flow command
 * Demonstrates a complete session flow with notifications
 */
void handle_demo_session_flow_command(void);

/**
 * Handle simulate_ranging command
 * Simulates a single-target ranging data notification
 */
void handle_simulate_ranging_command(void);

/**
 * Handle simulate_multi_target_ranging command
 * Simulates a multi-target ranging data notification
 */
void handle_simulate_multi_target_ranging_command(void);

#endif // UCI_CMD_SIMULATION_H
