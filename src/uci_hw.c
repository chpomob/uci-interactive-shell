#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _XOPEN_SOURCE 600
#include "uci_hw.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <sys/select.h>
#include <sys/time.h>

// Global variables for hardware communication
static int g_hw_fd = -1;
static int g_verbose = 0;
static int g_hw_mode_enabled = 0;

// Set verbose mode for debugging
void uci_hw_set_verbose(int verbose) {
    g_verbose = verbose;
}

// Enable hardware mode
void uci_hw_enable_hardware_mode() {
    g_hw_mode_enabled = 1;
}

// Disable hardware mode
void uci_hw_disable_hardware_mode() {
    g_hw_mode_enabled = 0;
}

// Check if hardware mode is enabled
int uci_hw_is_hardware_mode_enabled() {
    return g_hw_mode_enabled;
}

// Open UCI device character device file
int uci_hw_open(const char* device_path) {
    if (g_verbose) {
        printf("Opening UCI device: %s\n", device_path);
    }
    
    // Open the device file
    g_hw_fd = open(device_path, O_RDWR | O_NOCTTY);
    if (g_hw_fd < 0) {
        perror("Failed to open UCI device");
        return -1;
    }
    
    // Configure the device for raw binary communication
    struct termios tty;
    if (tcgetattr(g_hw_fd, &tty) != 0) {
        if (g_verbose) {
            perror("tcgetattr failed");
        }
        // Continue anyway as this might not be a terminal device
    } else {
        // Set raw mode (no processing of input/output)
        cfmakeraw(&tty);
        
        // Set speed to 115200 baud (typical for UWB devices)
        cfsetispeed(&tty, B115200);
        cfsetospeed(&tty, B115200);
        
        // Apply settings
        if (tcsetattr(g_hw_fd, TCSANOW, &tty) != 0) {
            if (g_verbose) {
                perror("tcsetattr failed");
            }
            // Continue anyway
        }
    }
    
    if (g_verbose) {
        printf("Successfully opened UCI device (fd=%d)\n", g_hw_fd);
    }
    
    return g_hw_fd;
}

// Close UCI device
int uci_hw_close(int fd) {
    if (g_verbose) {
        printf("Closing UCI device (fd=%d)\n", fd);
    }
    
    if (fd >= 0) {
        close(fd);
        if (fd == g_hw_fd) {
            g_hw_fd = -1;
        }
        return 0;
    }
    return -1;
}

// Send data to UCI device
int uci_hw_send(int fd, const unsigned char* data, size_t length) {
    if (fd < 0 || !data || length == 0) {
        if (g_verbose) {
            printf("Invalid parameters for uci_hw_send\n");
        }
        return -1;
    }
    
    if (g_verbose) {
        printf("Sending %zu bytes to UCI device:\n  ", length);
        for (size_t i = 0; i < length; i++) {
            printf("%02X ", data[i]);
        }
        printf("\n");
    }
    
    ssize_t bytes_written = write(fd, data, length);
    if (bytes_written < 0) {
        perror("Failed to write to UCI device");
        return -1;
    }
    
    if ((size_t)bytes_written != length) {
        if (g_verbose) {
            printf("Warning: Only wrote %zd of %zu bytes\n", bytes_written, length);
        }
        return (int)bytes_written;
    }
    
    // Flush output to ensure data is sent
    tcdrain(fd);
    
    return (int)bytes_written;
}

// Receive data from UCI device with timeout
int uci_hw_receive(int fd, unsigned char* buffer, size_t buffer_size, int timeout_ms) {
    if (fd < 0 || !buffer || buffer_size == 0) {
        if (g_verbose) {
            printf("Invalid parameters for uci_hw_receive\n");
        }
        return -1;
    }
    
    fd_set read_fds;
    struct timeval timeout;
    
    FD_ZERO(&read_fds);
    FD_SET(fd, &read_fds);
    
    // Convert timeout to seconds and microseconds
    timeout.tv_sec = timeout_ms / 1000;
    timeout.tv_usec = (timeout_ms % 1000) * 1000;
    
    // Wait for data to be available
    int select_result = select(fd + 1, &read_fds, NULL, NULL, 
                              timeout_ms >= 0 ? &timeout : NULL);
    
    if (select_result < 0) {
        perror("select failed");
        return -1;
    } else if (select_result == 0) {
        // Timeout - no data available
        if (g_verbose) {
            printf("Timeout waiting for UCI data\n");
        }
        return 0; // No data available
    }
    
    // Data is available, read it
    ssize_t bytes_read = read(fd, buffer, buffer_size);
    if (bytes_read < 0) {
        perror("Failed to read from UCI device");
        return -1;
    }
    
    if (g_verbose && bytes_read > 0) {
        printf("Received %zd bytes from UCI device:\n  ", bytes_read);
        for (ssize_t i = 0; i < bytes_read; i++) {
            printf("%02X ", buffer[i]);
        }
        printf("\n");
    }
    
    return (int)bytes_read;
}