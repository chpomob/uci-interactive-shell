/*
 * UCI Interactive Shell - UI Enhancement Integration Test
 *
 * This test verifies that the UI enhancements can be properly integrated
 * into the main application.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/uci_ui.h"
#include "../include/uci_ui_main_patch.h"

// Mock function to simulate hardware initialization
void mock_hardware_init(const char* device_path) {
    // Instead of:
    // printf("Hardware mode initialized successfully with device: %s\n", device_path);
    
    // Use the enhanced UI version:
    ui_print_hardware_mode_initialized(device_path);
}

// Mock function to simulate command execution
void mock_execute_command(const char* command) {
    if (strcmp(command, "get_device_info") == 0) {
        ui_print_command("Executing: get_device_info");
        ui_print_sending_uci_packet("simulator");
        
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
            printf("%s done\n", ANSI_RESET);
        } else {
            printf(" done\n");
        }
        
        ui_print_received_uci_packet();
        ui_print_success("CORE_DEVICE_INFO_RSP - Status OK, UCI Version 1.0");
    } else if (strcmp(command, "invalid_command") == 0) {
        ui_print_command_not_found(command);
    } else {
        char command_display[256];
        snprintf(command_display, sizeof(command_display), "Executing: %s", command);
        ui_print_command(command_display);
        ui_print_info("Command executed successfully");
    }
}

// Mock function to simulate session flow
void mock_session_flow() {
    ui_print_simulation_started("UCI Session Flow");
    
    ui_print_info("1. Session Initialization");
    mock_execute_command("session_init");
    
    ui_print_info("2. Session Start");
    mock_execute_command("session_start");
    
    ui_print_info("3. Ranging Simulation");
    if (ui_color_enabled) {
        printf("%s%sRanging Data:%s Target %s0x1234%s - Distance: %s%d cm%s, AoA: %s%d°%s\n",
               ANSI_COLOR_BRIGHT_CYAN, ANSI_BOLD, ANSI_RESET,
               ANSI_COLOR_BRIGHT_YELLOW, ANSI_RESET,
               ANSI_COLOR_BRIGHT_GREEN, 150, ANSI_RESET,
               ANSI_COLOR_BRIGHT_BLUE, 30, ANSI_RESET);
    } else {
        printf("Ranging Data: Target 0x1234 - Distance: 150 cm, AoA: 30°\n");
    }
    
    ui_print_info("4. Session Stop");
    mock_execute_command("session_stop");
    
    ui_print_simulation_completed("UCI Session Flow");
}

int main() {
    printf("=== UCI UI Enhancement Integration Test ===\n\n");
    
    // Test with colors enabled
    printf("1. Testing with colors enabled:\n");
    ui_enable_color(1);
    
    // Enhanced welcome message
    ui_print_welcome_message();
    
    // Test hardware initialization
    mock_hardware_init("/dev/ttyUSB0");
    
    // Test command execution
    mock_execute_command("get_device_info");
    mock_execute_command("invalid_command");
    
    // Test session flow
    mock_session_flow();
    
    printf("\n---\n\n");
    
    // Test with colors disabled
    printf("2. Testing with colors disabled:\n");
    ui_enable_color(0);
    
    ui_print_welcome_message();
    mock_hardware_init("/dev/ttyUSB0");
    mock_execute_command("get_device_info");
    mock_execute_command("invalid_command");
    mock_session_flow();
    
    printf("\n%s%sIntegration test completed successfully!%s\n", 
           ANSI_COLOR_GREEN, ANSI_BOLD, ANSI_RESET);
    
    return 0;
}