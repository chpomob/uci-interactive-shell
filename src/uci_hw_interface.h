#ifndef UCI_HW_INTERFACE_H
#define UCI_HW_INTERFACE_H

#include <stddef.h>

// Hardware interface functions
int uci_hw_interface_init(const char* device_path);
void uci_hw_interface_set_verbose(int verbose);  // Add verbose mode support
int uci_hw_interface_send_command(unsigned char mt, unsigned char pbf, unsigned char gid, unsigned char oid, 
                                 unsigned char* payload, int payload_len);
int uci_hw_interface_receive_response(unsigned char* buffer, size_t buffer_size, int timeout_ms);
void uci_hw_interface_cleanup(void);
int uci_hw_interface_is_connected(void);
const char* uci_hw_interface_get_device_path(void);  // Add function to get device path

#endif // UCI_HW_INTERFACE_H