#ifndef UCI_CMD_HARDWARE_H
#define UCI_CMD_HARDWARE_H

#include "uci_hw_chardev.h"

// Initialize the hardware command module with pointers to global state
void uci_cmd_hardware_init(int* hw_mode, uci_hw_chardev_t* chardev);

// Hardware initialization and management commands
int handle_hw_init_command(char* device_path);
int handle_hw_send_command(char* mt_str,
                           char* pbf_str,
                           char* gid_str,
                           char* oid_str,
                           char** payload_tokens,
                           int payload_count);

// Mode switching commands
void handle_mode_sim_command(void);
int handle_mode_hw_command(char* device_path);
void handle_mode_info_command(void);

#endif // UCI_CMD_HARDWARE_H
