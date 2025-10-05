#include <stdio.h>
#include <string.h>
#include "../include/uci_ui.h"
#include "../include/uci_ui_main_patch.h"

void ui_print_welcome_message() {
    if (ui_color_enabled) {
        printf("%s%s%s", ANSI_BG_BLUE, ANSI_COLOR_BRIGHT_WHITE, ANSI_BOLD);
        printf("==========================================\n");
        printf("  UCI Interactive Shell - Enhanced UI     \n");
        printf("==========================================%s\n", ANSI_RESET);
    } else {
        printf("==========================================\n");
        printf("  UCI Interactive Shell                  \n");
        printf("==========================================\n");
    }
    
    if (ui_color_enabled) {
        printf("%s%s", ANSI_COLOR_BRIGHT_CYAN, ANSI_BOLD);
        printf("Welcome to the UCI (Ultra-Wideband Control Interface) Shell\n");
        printf("Type 'help' for available commands or 'quit' to exit%s\n\n", ANSI_RESET);
    } else {
        printf("Welcome to the UCI (Ultra-Wideband Control Interface) Shell\n");
        printf("Type 'help' for available commands or 'quit' to exit\n\n");
    }
}

void ui_print_hardware_mode_initialized(const char* device_path) {
    if (ui_color_enabled) {
        printf("%s%s✓ Hardware mode initialized successfully with device: %s%s\n", 
               ANSI_COLOR_GREEN, ANSI_BOLD, ANSI_RESET, device_path);
    } else {
        printf("✓ Hardware mode initialized successfully with device: %s\n", device_path);
    }
}

void ui_print_hardware_mode_not_initialized() {
    if (ui_color_enabled) {
        printf("%s%s⚠ Hardware mode not initialized. Use 'hw_init <device_path>' first.%s\n", 
               ANSI_COLOR_YELLOW, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("⚠ Hardware mode not initialized. Use 'hw_init <device_path>' first.\n");
    }
}

void ui_print_command_not_found(const char* command) {
    if (ui_color_enabled) {
        printf("%s%s✗ Unknown command: %s%s\n", 
               ANSI_COLOR_RED, ANSI_BOLD, ANSI_RESET, command);
    } else {
        printf("✗ Unknown command: %s\n", command);
    }
}

void ui_print_sending_uci_packet(const char* destination) {
    if (ui_color_enabled) {
        printf("%s%s→ Sending UCI packet to %s%s\n", 
               ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET, destination);
    } else {
        printf("→ Sending UCI packet to %s\n", destination);
    }
}

void ui_print_received_uci_packet() {
    if (ui_color_enabled) {
        printf("%s%s← Received UCI packet%s\n", 
               ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("← Received UCI packet\n");
    }
}

void ui_print_simulation_started(const char* simulation_name) {
    if (ui_color_enabled) {
        printf("%s%s↺ Starting simulation: %s%s\n", 
               ANSI_COLOR_BRIGHT_BLUE, ANSI_BOLD, ANSI_RESET, simulation_name);
    } else {
        printf("↺ Starting simulation: %s\n", simulation_name);
    }
}

void ui_print_simulation_completed(const char* simulation_name) {
    if (ui_color_enabled) {
        printf("%s%s✓ Simulation completed: %s%s\n", 
               ANSI_COLOR_GREEN, ANSI_BOLD, ANSI_RESET, simulation_name);
    } else {
        printf("✓ Simulation completed: %s\n", simulation_name);
    }
}