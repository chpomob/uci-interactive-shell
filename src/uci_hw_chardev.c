#define _GNU_SOURCE
#include "uci_hw_chardev.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>

// Initialize character device interface
int uci_hw_chardev_init(uci_hw_chardev_t* hw, const char* device_path) {
    if (!hw || !device_path) {
        fprintf(stderr, "Error: Invalid parameters for uci_hw_chardev_init\n");
        return -1;
    }
    
    // Initialize structure
    memset(hw, 0, sizeof(uci_hw_chardev_t));
    hw->fd = -1;
    hw->is_open = 0;
    hw->verbose = 0;
    
    // Copy device path
    strncpy(hw->device_path, device_path, sizeof(hw->device_path) - 1);
    hw->device_path[sizeof(hw->device_path) - 1] = '\0';
    
    if (hw->verbose) {
        printf("Initialized UCI chardev interface with device: %s\n", hw->device_path);
    }
    
    return 0;
}

// Open character device
int uci_hw_chardev_open(uci_hw_chardev_t* hw) {
    if (!hw) {
        fprintf(stderr, "Error: Invalid parameters for uci_hw_chardev_open\n");
        return -1;
    }
    
    if (hw->is_open) {
        if (hw->verbose) {
            printf("Device already open: %s\n", hw->device_path);
        }
        return 0; // Already open
    }
    
    if (hw->verbose) {
        printf("Opening UCI character device: %s\n", hw->device_path);
    }
    
    // Open the device file
    hw->fd = open(hw->device_path, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (hw->fd < 0) {
        perror("Failed to open UCI character device");
        return -1;
    }
    
    // Configure the device for raw binary communication
    struct termios tty;
    if (tcgetattr(hw->fd, &tty) != 0) {
        if (hw->verbose) {
            perror("tcgetattr failed");
        }
        // Continue anyway as this might not be a terminal device
    } else {
        // Save current settings
        struct termios orig_tty = tty;
        
        // Set raw mode (no processing of input/output)
#ifndef _GNU_SOURCE
        // Manual implementation of cfmakeraw for systems without it
        tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
        tty.c_oflag &= ~OPOST;
        tty.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
        tty.c_cflag &= ~(CSIZE | PARENB);
        tty.c_cflag |= CS8;
#else
        cfmakeraw(&tty);
#endif
        
        // Set speed to 115200 baud (typical for UWB devices)
        cfsetispeed(&tty, B115200);
        cfsetospeed(&tty, B115200);
        
        // Set 8N1 (8 data bits, no parity, 1 stop bit)
        tty.c_cflag &= ~PARENB;  // No parity
        tty.c_cflag &= ~CSTOPB;  // 1 stop bit
        tty.c_cflag &= ~CSIZE;   // Clear data bits
        tty.c_cflag |= CS8;      // 8 data bits
        
        // No flow control (check if CRTSCTS is defined)
#ifndef CRTSCTS
#define CRTSCTS 020000000000
#endif
        tty.c_cflag &= ~CRTSCTS;
        
        // Apply settings
        if (tcsetattr(hw->fd, TCSANOW, &tty) != 0) {
            if (hw->verbose) {
                perror("tcsetattr failed");
            }
            // Restore original settings
            tcsetattr(hw->fd, TCSANOW, &orig_tty);
        }
    }
    
    hw->is_open = 1;
    
    if (hw->verbose) {
        printf("Successfully opened UCI character device (fd=%d)\n", hw->fd);
    }
    
    return 0;
}

// Close character device
int uci_hw_chardev_close(uci_hw_chardev_t* hw) {
    if (!hw) {
        fprintf(stderr, "Error: Invalid parameters for uci_hw_chardev_close\n");
        return -1;
    }
    
    if (hw->verbose) {
        printf("Closing UCI character device (fd=%d)\n", hw->fd);
    }
    
    if (hw->fd >= 0) {
        close(hw->fd);
        hw->fd = -1;
    }
    
    hw->is_open = 0;
    
    return 0;
}

// Send UCI packet to character device
int uci_hw_chardev_send(uci_hw_chardev_t* hw, const unsigned char* data, size_t length) {
    if (!hw || !data || length == 0) {
        fprintf(stderr, "Error: Invalid parameters for uci_hw_chardev_send\n");
        return -1;
    }
    
    if (!hw->is_open || hw->fd < 0) {
        fprintf(stderr, "Error: Device not open for sending\n");
        return -1;
    }
    
    if (hw->verbose) {
        printf("Sending %zu bytes to UCI device (%s):\n  ", length, hw->device_path);
        for (size_t i = 0; i < length; i++) {
            printf("%02X ", data[i]);
        }
        printf("\n");
    }
    
    ssize_t bytes_written = write(hw->fd, data, length);
    if (bytes_written < 0) {
        perror("Failed to write to UCI device");
        return -1;
    }
    
    if ((size_t)bytes_written != length) {
        if (hw->verbose) {
            fprintf(stderr, "Warning: Only wrote %zd of %zu bytes\n", bytes_written, length);
        }
        return (int)bytes_written;
    }
    
    // Flush output to ensure data is sent
    tcdrain(hw->fd);
    
    return (int)bytes_written;
}

// Receive UCI packet from character device with timeout

// Receive UCI packet from character device with timeout

// Set verbose mode

// Check if device is connected

// Get device path
const char* uci_hw_chardev_get_device_path(uci_hw_chardev_t* hw) {
    if (!hw) {
        return NULL;
    }
    return hw->device_path;
}

// Send UCI command and receive response
int uci_hw_chardev_send_command_and_receive_response(uci_hw_chardev_t* hw, 
                                                    const unsigned char* command_data, 
                                                    size_t command_length,
                                                    unsigned char* response_buffer, 
                                                    size_t response_buffer_size, 
                                                    int timeout_ms) {
    if (!hw || !command_data || command_length == 0 || !response_buffer || response_buffer_size == 0) {
        fprintf(stderr, "Error: Invalid parameters for uci_hw_chardev_send_command_and_receive_response\n");
        return -1;
    }
    
    if (!hw->is_open || hw->fd < 0) {
        fprintf(stderr, "Error: Device not open for communication\n");
        return -1;
    }
    
    // Send the command
    int send_result = uci_hw_chardev_send(hw, command_data, command_length);
    if (send_result < 0) {
        if (hw->verbose) {
            fprintf(stderr, "Failed to send UCI command\n");
        }
        return -1;
    }
    
    // Receive the response with timeout
    int receive_result = uci_hw_chardev_receive(hw, response_buffer, response_buffer_size, timeout_ms);
    if (receive_result < 0) {
        if (hw->verbose) {
            fprintf(stderr, "Failed to receive UCI response\n");
        }
        return -1;
    }
    
    if (receive_result == 0) {
        if (hw->verbose) {
            fprintf(stderr, "Timeout waiting for UCI response\n");
        }
        return 0; // Timeout
    }
    
    return receive_result; // Return number of bytes received
}

// Receive UCI packet from character device with timeout
int uci_hw_chardev_receive(uci_hw_chardev_t* hw, unsigned char* buffer, size_t buffer_size, int timeout_ms) {
    if (!hw || !buffer || buffer_size == 0) {
        fprintf(stderr, "Error: Invalid parameters for uci_hw_chardev_receive\n");
        return -1;
    }
    
    if (!hw->is_open || hw->fd < 0) {
        fprintf(stderr, "Error: Device not open for receiving\n");
        return -1;
    }
    
    fd_set read_fds;
    struct timeval timeout;
    
    FD_ZERO(&read_fds);
    FD_SET(hw->fd, &read_fds);
    
    // Convert timeout to seconds and microseconds
    timeout.tv_sec = timeout_ms / 1000;
    timeout.tv_usec = (timeout_ms % 1000) * 1000;
    
    // Wait for data to be available
    int select_result = select(hw->fd + 1, &read_fds, NULL, NULL, 
                              timeout_ms >= 0 ? &timeout : NULL);
    
    if (select_result < 0) {
        if (hw->verbose) {
            perror("select failed");
        }
        return -1;
    } else if (select_result == 0) {
        // Timeout - no data available
        if (hw->verbose) {
            printf("Timeout waiting for UCI data\n");
        }
        return 0; // No data available
    }
    
    // Data is available, read it
    ssize_t bytes_read = read(hw->fd, buffer, buffer_size);
    if (bytes_read < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            if (hw->verbose) {
                perror("Failed to read from UCI device");
            }
            return -1;
        }
        return 0; // No data available (non-blocking)
    }
    
    if (hw->verbose && bytes_read > 0) {
        printf("Received %zd bytes from UCI device (%s):\n  ", bytes_read, hw->device_path);
        for (size_t i = 0; i < (size_t)bytes_read; i++) {
            printf("%02X ", buffer[i]);
        }
        printf("\n");
    }
    
    return (int)bytes_read;
}

// Set verbose mode
void uci_hw_chardev_set_verbose(uci_hw_chardev_t* hw, int verbose) {
    if (hw) {
        hw->verbose = verbose ? 1 : 0;
    }
}

// Check if device is connected
int uci_hw_chardev_is_connected(uci_hw_chardev_t* hw) {
    if (!hw) {
        return 0;
    }
    return hw->is_open && hw->fd >= 0;
}