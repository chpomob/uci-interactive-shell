#include "uci_hw_chardev.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main() {
    printf("=== UCI Character Device Interface Test ===\n");
    
    // Initialize character device interface
    uci_hw_chardev_t hw;
    if (uci_hw_chardev_init(&hw, "/dev/null") == 0) {
        printf("✓ Character device interface initialized successfully\n");
        
        // Enable verbose mode
        uci_hw_chardev_set_verbose(&hw, 1);
        
        // Try to open the device
        if (uci_hw_chardev_open(&hw) == 0) {
            printf("✓ Character device opened successfully (fd=%d)\n", hw.fd);
            
            // Check if connected
            if (uci_hw_chardev_is_connected(&hw)) {
                printf("✓ Character device is connected\n");
                
                // Try to send some data
                unsigned char test_data[] = {0x01, 0x02, 0x03, 0x04};
                int send_result = uci_hw_chardev_send(&hw, test_data, sizeof(test_data));
                if (send_result >= 0) {
                    printf("✓ Sent %d bytes to character device\n", send_result);
                } else {
                    printf("⚠ Warning: Failed to send data to character device\n");
                }
                
                // Try to receive some data (with timeout)
                unsigned char recv_buffer[256];
                int recv_result = uci_hw_chardev_receive(&hw, recv_buffer, sizeof(recv_buffer), 100);
                if (recv_result > 0) {
                    printf("✓ Received %d bytes from character device\n", recv_result);
                } else if (recv_result == 0) {
                    printf("ℹ No data received (timeout)\n");
                } else {
                    printf("⚠ Warning: Failed to receive data from character device\n");
                }
            } else {
                printf("✗ Character device is not connected\n");
            }
            
            // Close the device
            if (uci_hw_chardev_close(&hw) == 0) {
                printf("✓ Character device closed successfully\n");
            } else {
                printf("✗ Failed to close character device\n");
            }
        } else {
            printf("✗ Failed to open character device\n");
        }
    } else {
        printf("✗ Failed to initialize character device interface\n");
    }
    
    printf("=== Test Complete ===\n");
    return 0;
}