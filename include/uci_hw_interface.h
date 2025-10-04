#ifndef UCI_HW_INTERFACE_H
#define UCI_HW_INTERFACE_H

#include <stddef.h>
#include "uci_hw_chardev.h"

// Hardware interface functions
int uci_hw_interface_init(const char* device_path);
void uci_hw_interface_set_verbose(int verbose);  // Add verbose mode support
int uci_hw_interface_send_command(unsigned char mt, unsigned char pbf, unsigned char gid, unsigned char oid, 
                                 unsigned char* payload, int payload_len);
int uci_hw_interface_receive_response(unsigned char* buffer, size_t buffer_size, int timeout_ms);
void uci_hw_interface_cleanup(void);
int uci_hw_interface_is_connected(void);
const char* uci_hw_interface_get_device_path(void);  // Add function to get device path

// Character device communication functions
int uci_hw_chardev_init(uci_hw_chardev_t* hw, const char* device_path);
int uci_hw_chardev_open(uci_hw_chardev_t* hw);
int uci_hw_chardev_close(uci_hw_chardev_t* hw);
int uci_hw_chardev_send(uci_hw_chardev_t* hw, const unsigned char* data, size_t length);
int uci_hw_chardev_receive(uci_hw_chardev_t* hw, unsigned char* buffer, size_t buffer_size, int timeout_ms);
void uci_hw_chardev_set_verbose(uci_hw_chardev_t* hw, int verbose);
int uci_hw_chardev_is_connected(uci_hw_chardev_t* hw);
const char* uci_hw_chardev_get_device_path(uci_hw_chardev_t* hw);

#endif // UCI_HW_INTERFACE_H