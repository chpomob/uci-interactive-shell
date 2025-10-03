#include "uci_hw_interface.h"
#include "uci_hw.h"
#include "uci.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Global variables for hardware interface
static int g_hw_fd = -1;
static int g_hw_initialized = 0;
static int g_hw_connected = 0;
static int g_verbose_mode = 0;
static char g_device_path[256] = "/dev/ttyUSB0";  // Default device path

// Enable verbose mode for hardware interface
void uci_hw_interface_set_verbose(int verbose) {
    g_verbose_mode = verbose ? 1 : 0;
    uci_hw_set_verbose(g_verbose_mode); // Also set verbose mode for underlying hardware
}

// Initialize hardware interface
int uci_hw_interface_init(const char* device_path) {
    if (g_hw_initialized) {
        return 0; // Already initialized
    }
    
    if (g_verbose_mode) {
        printf("Initializing UCI hardware interface with device: %s\n", device_path);
    }
    
    // Store device path
    strncpy(g_device_path, device_path, sizeof(g_device_path) - 1);
    g_device_path[sizeof(g_device_path) - 1] = '\0'; // Ensure null termination
    
    // Enable hardware mode
    uci_hw_enable_hardware_mode();
    uci_hw_set_verbose(g_verbose_mode); // Enable verbose output for debugging
    
    // Open the device
    g_hw_fd = uci_hw_open(device_path);
    if (g_hw_fd < 0) {
        if (g_verbose_mode) {
            printf("Failed to open UCI device: %s\n", device_path);
        }
        return -1;
    }
    
    g_hw_initialized = 1;
    g_hw_connected = 1;
    
    if (g_verbose_mode) {
        printf("UCI hardware interface initialized successfully\n");
    }
    return 0;
}

// Send command to hardware device
int uci_hw_interface_send_command(unsigned char mt, unsigned char pbf, unsigned char gid, unsigned char oid, 
                                 unsigned char* payload, int payload_len) {
    if (!g_hw_initialized || !g_hw_connected) {
        if (g_verbose_mode) {
            printf("Hardware interface not initialized or not connected\n");
        }
        return -1;
    }
    
    // Create UCI packet
    size_t packet_size = sizeof(struct uci_packet_header) + (payload_len > 0 ? payload_len : 0);
    unsigned char* packet = malloc(packet_size);
    if (!packet) {
        if (g_verbose_mode) {
            printf("Failed to allocate memory for UCI packet\n");
        }
        return -1;
    }
    
    // Set up packet header
    struct uci_packet_header* header = (struct uci_packet_header*)packet;
    set_header_values(header, mt, pbf, gid, oid, payload_len);
    
    // Copy payload if present
    if (payload && payload_len > 0) {
        memcpy(packet + sizeof(struct uci_packet_header), payload, payload_len);
    }
    
    // Send packet to hardware
    int result = uci_hw_send(g_hw_fd, packet, packet_size);
    
    free(packet);
    
    if (result < 0) {
        if (g_verbose_mode) {
            printf("Failed to send UCI command to hardware\n");
        }
        return -1;
    }
    
    return 0;
}

// Receive response from hardware device
int uci_hw_interface_receive_response(unsigned char* buffer, size_t buffer_size, int timeout_ms) {
    if (!g_hw_initialized || !g_hw_connected) {
        if (g_verbose_mode) {
            printf("Hardware interface not initialized or not connected\n");
        }
        return -1;
    }
    
    if (!buffer || buffer_size == 0) {
        if (g_verbose_mode) {
            printf("Invalid buffer parameters\n");
        }
        return -1;
    }
    
    // Receive data from hardware
    int bytes_received = uci_hw_receive(g_hw_fd, buffer, buffer_size, timeout_ms);
    
    if (bytes_received < 0) {
        if (g_verbose_mode) {
            printf("Failed to receive UCI response from hardware\n");
        }
        return -1;
    }
    
    return bytes_received;
}

// Get device path
const char* uci_hw_interface_get_device_path(void) {
    return g_device_path;
}

// Cleanup hardware interface
void uci_hw_interface_cleanup(void) {
    if (g_hw_initialized) {
        if (g_verbose_mode) {
            printf("Cleaning up UCI hardware interface\n");
        }
        if (g_hw_fd >= 0) {
            uci_hw_close(g_hw_fd);
            g_hw_fd = -1;
        }
        g_hw_initialized = 0;
        g_hw_connected = 0;
    }
}

// Check if hardware is connected
int uci_hw_interface_is_connected(void) {
    return g_hw_connected;
}