#include <stdio.h>
#include <stdlib.h>
#include "../include/uci_ui.h"

int main() {
    printf("=== UCI UI Enhancements Test ===\n\n");
    
    // Test all UI functions with colors enabled
    printf("With colors enabled:\n");
    ui_print_success("Successfully initialized UCI interface");
    ui_print_info("Device info: UCI Version 1.0, MAC Version 2.0");
    ui_print_warning("Low battery warning: 15%% remaining");
    ui_print_error("Connection timeout - retrying...");
    
    // Show raw ANSI codes for verification
    printf("\nRaw ANSI codes demonstration:\n");
    printf("Success: %s%s✓ Success message%s\n", ANSI_COLOR_GREEN, ANSI_BOLD, ANSI_RESET);
    printf("Error: %s%s✗ Error message%s\n", ANSI_COLOR_RED, ANSI_BOLD, ANSI_RESET);
    printf("Warning: %s%s⚠ Warning message%s\n", ANSI_COLOR_YELLOW, ANSI_BOLD, ANSI_RESET);
    printf("Info: %sℹ Info message%s\n", ANSI_COLOR_BRIGHT_CYAN, ANSI_RESET);
    
    // Test with colors disabled
    printf("\nWith colors disabled:\n");
    ui_enable_color(0);
    ui_print_success("Successfully initialized UCI interface");
    ui_print_info("Device info: UCI Version 1.0, MAC Version 2.0");
    ui_print_warning("Low battery warning: 15%% remaining");
    ui_print_error("Connection timeout - retrying...");
    
    return 0;
}