/*
 * UCI Interactive Shell - Enhanced UI Demo
 *
 * This is a demonstration of how the UCI Interactive Shell would look
 * with the UI enhancements integrated.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Include UI enhancement headers
#include "../include/uci_ui.h"
#include "../include/uci_ui_main_patch.h"

// Mock UCI functions to demonstrate integration
void mock_send_uci_command(const char* command_name) {
    ui_print_sending_uci_packet("simulator");
    
    // Simulate command processing
    for (int i = 0; i < 3; i++) {
        if (ui_color_enabled) {
            printf("%s.", ANSI_COLOR_BRIGHT_BLACK);
        } else {
            printf(".");
        }
        fflush(stdout);
        usleep(300000); // 0.3 seconds
    }
    
    if (ui_color_enabled) {
        printf("%s done\n", ANSI_RESET);
    } else {
        printf(" done\n");
    }
    
    ui_print_received_uci_packet();
    
    // Simulate response
    if (ui_color_enabled) {
        printf("%s%sResponse:%s CORE_DEVICE_INFO_RSP - Status OK, UCI Version 1.0\n", 
               ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("Response: CORE_DEVICE_INFO_RSP - Status OK, UCI Version 1.0\n");
    }
}

void mock_hardware_init(const char* device_path) {
    if (device_path && strlen(device_path) > 0) {
        if (ui_color_enabled) {
            printf("%sInitializing hardware device: %s%s\n", 
                   ANSI_COLOR_BRIGHT_BLUE, device_path, ANSI_RESET);
        } else {
            printf("Initializing hardware device: %s\n", device_path);
        }
        
        // Simulate initialization delay
        for (int i = 0; i < 5; i++) {
            if (ui_color_enabled) {
                printf("%s.", ANSI_COLOR_BRIGHT_BLACK);
            } else {
                printf(".");
            }
            fflush(stdout);
            usleep(200000); // 0.2 seconds
        }
        
        if (ui_color_enabled) {
            printf("%s\n", ANSI_RESET);
        } else {
            printf("\n");
        }
        
        ui_print_hardware_mode_initialized(device_path);
    } else {
        ui_print_hardware_mode_not_initialized();
    }
}

void mock_session_flow_demo() {
    ui_print_simulation_started("UCI Session Flow");
    
    // Step 1: Session Init
    if (ui_color_enabled) {
        printf("%s%s1.%s Session Initialization\n", 
               ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("1. Session Initialization\n");
    }
    
    mock_send_uci_command("SESSION_INIT");
    
    // Step 2: Session Start
    if (ui_color_enabled) {
        printf("%s%s2.%s Session Start\n", 
               ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("2. Session Start\n");
    }
    
    mock_send_uci_command("SESSION_START");
    
    // Step 3: Ranging Simulation
    if (ui_color_enabled) {
        printf("%s%s3.%s Ranging Simulation\n", 
               ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("3. Ranging Simulation\n");
    }
    
    // Simulate ranging data
    if (ui_color_enabled) {
        printf("%s%sRanging Data:%s Target 0x1234 - Distance: %s%d cm%s, AoA: %s%d°%s\n",
               ANSI_COLOR_BRIGHT_CYAN, ANSI_BOLD, ANSI_RESET,
               ANSI_COLOR_BRIGHT_GREEN, 150, ANSI_RESET,
               ANSI_COLOR_BRIGHT_GREEN, 30, ANSI_RESET);
    } else {
        printf("Ranging Data: Target 0x1234 - Distance: 150 cm, AoA: 30°\n");
    }
    
    if (ui_color_enabled) {
        printf("%s%sRanging Data:%s Target 0x5678 - Distance: %s%d cm%s, AoA: %s%d°%s\n",
               ANSI_COLOR_BRIGHT_CYAN, ANSI_BOLD, ANSI_RESET,
               ANSI_COLOR_BRIGHT_GREEN, 320, ANSI_RESET,
               ANSI_COLOR_BRIGHT_GREEN, 120, ANSI_RESET);
    } else {
        printf("Ranging Data: Target 0x5678 - Distance: 320 cm, AoA: 120°\n");
    }
    
    // Step 4: Session Stop
    if (ui_color_enabled) {
        printf("%s%s4.%s Session Stop\n", 
               ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("4. Session Stop\n");
    }
    
    mock_send_uci_command("SESSION_STOP");
    
    ui_print_simulation_completed("UCI Session Flow");
}

int main() {
    printf("=== UCI Interactive Shell - Enhanced UI Demo ===\n\n");
    
    // Show welcome message with colors
    ui_print_welcome_message();
    
    // Demonstrate various UI elements
    printf("Demonstrating UI enhancements:\n\n");
    
    // Success message
    ui_print_success("UCI Shell initialized successfully");
    
    // Info message
    ui_print_info("Using simulation mode - no hardware device connected");
    
    // Warning message
    ui_print_warning("Low memory available - consider closing other applications");
    
    // Error message
    ui_print_error("Failed to load configuration file");
    
    // Debug message
    ui_print_debug("Processing command: get_device_info");
    
    printf("\n");
    
    // Demonstrate hardware initialization
    mock_hardware_init("/dev/ttyUSB0");
    
    printf("\n");
    
    // Demonstrate command execution
    if (ui_color_enabled) {
        printf("%s%sExecuting:%s get_device_info\n", 
               ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("Executing: get_device_info\n");
    }
    
    mock_send_uci_command("CORE_DEVICE_INFO");
    
    printf("\n");
    
    // Demonstrate session flow
    mock_session_flow_demo();
    
    printf("\n");
    
    // Show completion
    if (ui_color_enabled) {
        printf("%s%s%s%sDemo completed successfully!%s\n", 
               ANSI_BG_GREEN, ANSI_COLOR_BLACK, ANSI_BOLD, 
               ANSI_BLINK, ANSI_RESET);
    } else {
        printf("Demo completed successfully!\n");
    }
    
    return 0;
}