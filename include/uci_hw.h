#ifndef UCI_HW_H
#define UCI_HW_H

#include <stddef.h>

// Hardware communication interface
int uci_hw_open(const char* device_path);
int uci_hw_close(int fd);
int uci_hw_send(int fd, const unsigned char* data, size_t length);
int uci_hw_receive(int fd, unsigned char* buffer, size_t buffer_size, int timeout_ms);
void uci_hw_set_verbose(int verbose);

// Hardware mode functions
void uci_hw_enable_hardware_mode();
void uci_hw_disable_hardware_mode();
int uci_hw_is_hardware_mode_enabled();

#endif // UCI_HW_H