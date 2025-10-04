#ifndef UCI_HW_CHARDEV_H
#define UCI_HW_CHARDEV_H

#include <stddef.h>
#include <stdint.h>

// Character device UCI hardware interface
typedef struct {
    int fd;                    // File descriptor for the character device
    char device_path[256];     // Path to the character device
    int is_open;              // Flag indicating if device is open
    int verbose;              // Verbose output flag
} uci_hw_chardev_t;

// Initialize character device interface
int uci_hw_chardev_init(uci_hw_chardev_t* hw, const char* device_path);

// Open character device
int uci_hw_chardev_open(uci_hw_chardev_t* hw);

// Close character device
int uci_hw_chardev_close(uci_hw_chardev_t* hw);

// Send UCI packet to character device
int uci_hw_chardev_send(uci_hw_chardev_t* hw, const unsigned char* data, size_t length);

// Receive UCI packet from character device with timeout
int uci_hw_chardev_receive(uci_hw_chardev_t* hw, unsigned char* buffer, size_t buffer_size, int timeout_ms);

// Set verbose mode
void uci_hw_chardev_set_verbose(uci_hw_chardev_t* hw, int verbose);

// Check if device is connected
int uci_hw_chardev_is_connected(uci_hw_chardev_t* hw);

// Get device path
const char* uci_hw_chardev_get_device_path(uci_hw_chardev_t* hw);

#endif // UCI_HW_CHARDEV_H