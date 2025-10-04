#include "../include/uci_hw_interface.h"
#include "../include/uci_hw_chardev.h"
#include "../include/uci.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Global variables for hardware interface
static uci_hw_chardev_t g_uwb_chardev;
static int g_hw_initialized = 0;
static int g_verbose_mode = 0;
static char g_device_path[256] = "/dev/ttyUSB0";  // Default device path

// Enable verbose mode for hardware interface
void uci_hw_interface_set_verbose(int verbose) {
    g_verbose_mode = verbose ? 1 : 0;
    uci_hw_chardev_set_verbose(&g_uwb_chardev, g_verbose_mode);
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
    
    // Initialize the character device interface
    if (uci_hw_chardev_init(&g_uwb_chardev, device_path) == 0) {
        if (uci_hw_chardev_open(&g_uwb_chardev) == 0) {
            if (g_verbose_mode) {
                printf("Character device interface initialized successfully\n");
            }
            g_hw_initialized = 1;
            return 0;
        } else {
            if (g_verbose_mode) {
                printf("Failed to open character device interface\n");
            }
            return -1;
        }
    } else {
        if (g_verbose_mode) {
            printf("Failed to initialize character device interface\n");
        }
        return -1;
    }
}

// Send UCI command to hardware device
int uci_hw_interface_send_command(unsigned char mt, unsigned char pbf, unsigned char gid, unsigned char oid, 
                                 unsigned char* payload, int payload_len) {
    if (!g_hw_initialized) {
        if (g_verbose_mode) {
            printf("Hardware interface not initialized\n");
        }
        return -1;
    }
    
    if (!uci_hw_chardev_is_connected(&g_uwb_chardev)) {
        if (g_verbose_mode) {
            printf("Hardware device not connected\n");
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
    
    if (g_verbose_mode) {
        printf("Sending UCI packet to hardware (%s):\n", g_device_path);
        printf("  Header: %02X %02X %02X %02X\n", 
               ((unsigned char*)header)[0], ((unsigned char*)header)[1], 
               ((unsigned char*)header)[2], ((unsigned char*)header)[3]);
        if (payload_len > 0) {
            printf("  Payload: ");
            for (int i = 0; i < payload_len; i++) {
                printf("%02X ", payload[i]);
            }
            printf("\n");
        }
    }
    
    // Send packet to hardware using character device interface
    int result = uci_hw_chardev_send(&g_uwb_chardev, packet, packet_size);
    
    free(packet);
    
    if (result < 0) {
        if (g_verbose_mode) {
            printf("Failed to send UCI command to hardware\n");
        }
        return -1;
    }
    
    return 0;
}

// Receive UCI response from hardware device with timeout
int uci_hw_interface_receive_response(unsigned char* buffer, size_t buffer_size, int timeout_ms) {
    if (!g_hw_initialized) {
        if (g_verbose_mode) {
            printf("Hardware interface not initialized\n");
        }
        return -1;
    }
    
    if (!uci_hw_chardev_is_connected(&g_uwb_chardev)) {
        if (g_verbose_mode) {
            printf("Hardware device not connected\n");
        }
        return -1;
    }
    
    if (!buffer || buffer_size == 0) {
        if (g_verbose_mode) {
            printf("Invalid buffer parameters\n");
        }
        return -1;
    }
    
    // Receive data from hardware using character device interface
    int bytes_received = uci_hw_chardev_receive(&g_uwb_chardev, buffer, buffer_size, timeout_ms);
    
    if (bytes_received < 0) {
        if (g_verbose_mode) {
            printf("Failed to receive UCI response from hardware\n");
        }
        return -1;
    }
    
    if (g_verbose_mode && bytes_received > 0) {
        printf("Received %d bytes from UCI hardware (%s):\n  ", bytes_received, g_device_path);
        for (int i = 0; i < bytes_received; i++) {
            printf("%02X ", buffer[i]);
        }
        printf("\n");
    }
    
    return bytes_received;
}

// Cleanup hardware interface
void uci_hw_interface_cleanup(void) {
    if (g_hw_initialized) {
        if (g_verbose_mode) {
            printf("Cleaning up UCI hardware interface\n");
        }
        uci_hw_chardev_close(&g_uwb_chardev);
        g_hw_initialized = 0;
    }
}

// Check if hardware is connected
int uci_hw_interface_is_connected(void) {
    return g_hw_initialized && uci_hw_chardev_is_connected(&g_uwb_chardev);
}

// Get device path
const char* uci_hw_interface_get_device_path(void) {
    return g_device_path;
}