#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/uci_ui.h"
#include "../include/uci_ui_main_patch.h"

// Mock function to simulate sending UCI packets
void mock_send_uci_packet(const char* destination) {
    ui_print_sending_uci_packet(destination);
    
    // Simulate some processing time
    for (int i = 0; i < 3; i++) {
        if (ui_color_enabled) {
            printf("%s.", ANSI_COLOR_BRIGHT_BLACK);
        } else {
            printf(".");
        }
        fflush(stdout);
        // In a real implementation, we would actually send the packet here
    }
    
    if (ui_color_enabled) {
        printf("%s done\n", ANSI_RESET);
    } else {
        printf(" done\n");
    }
    
    ui_print_received_uci_packet();
}

// Mock function to simulate hardware initialization
int mock_hardware_init(const char* device_path) {
    if (device_path && strlen(device_path) > 0) {
        printf("Initializing hardware device: %s\n", device_path);
        
        // Simulate initialization delay
        for (int i = 0; i < 5; i++) {
            if (ui_color_enabled) {
                printf("%s.", ANSI_COLOR_BRIGHT_BLACK);
            } else {
                printf(".");
            }
            fflush(stdout);
            // In a real implementation, we would initialize the hardware here
        }
        
        if (ui_color_enabled) {
            printf("%s\n", ANSI_RESET);
        } else {
            printf("\n");
        }
        
        ui_print_hardware_mode_initialized(device_path);
        return 0; // Success
    } else {
        ui_print_hardware_mode_not_initialized();
        return -1; // Failure
    }
}

// Mock function to simulate running a command
void mock_run_command(const char* command) {
    if (strcmp(command, "hw_init") == 0) {
        mock_hardware_init("/dev/ttyUSB0");
    } else if (strcmp(command, "send_packet") == 0) {
        mock_send_uci_packet("UWB Device");
    } else if (strcmp(command, "simulate_ranging") == 0) {
        ui_print_simulation_started("Two-Way Ranging");
        // Simulate processing
        for (int i = 0; i < 3; i++) {
            if (ui_color_enabled) {
                printf("%s.", ANSI_COLOR_BRIGHT_BLACK);
            } else {
                printf(".");
            }
            fflush(stdout);
        }
        if (ui_color_enabled) {
            printf("%s\n", ANSI_RESET);
        } else {
            printf("\n");
        }
        ui_print_simulation_completed("Two-Way Ranging");
    } else {
        ui_print_command_not_found(command);
    }
}

int main() {
    printf("=== UCI UI Integration Demo ===\n\n");
    
    // Test with colors enabled
    printf("1. Testing with colors enabled:\n");
    ui_print_welcome_message();
    
    mock_run_command("hw_init");
    mock_run_command("send_packet");
    mock_run_command("simulate_ranging");
    mock_run_command("unknown_command");
    
    printf("\n---\n\n");
    
    // Test with colors disabled
    printf("2. Testing with colors disabled:\n");
    ui_enable_color(0);
    ui_print_welcome_message();
    
    mock_run_command("hw_init");
    mock_run_command("send_packet");
    mock_run_command("simulate_ranging");
    mock_run_command("unknown_command");
    
    printf("\n%s%sDemo completed successfully!%s\n", 
           ANSI_COLOR_GREEN, ANSI_BOLD, ANSI_RESET);
           
    return 0;
}