/*
 * UCI UI Enhancement - User Experience Comparison
 *
 * This program demonstrates the difference between the original
 * plain text output and the enhanced colorized output.
 */

#include <stdio.h>
#include <stdlib.h>
#include "../include/uci_ui.h"
#include "../include/uci_ui_main_patch.h"

void print_original_output() {
    printf("=== ORIGINAL OUTPUT (Plain Text) ===\n");
    
    // Simulate original output
    printf("UCI Interactive Shell\n");
    printf("Enter 'quit' to exit.\n");
    printf("Commands: send, get_device_info, device_info, device_reset, ...\n");
    printf("Hardware mode initialized successfully with device: /dev/ttyUSB0\n");
    printf("Sending UCI packet to hardware (/dev/ttyUSB0):\n");
    printf("  Header: 20 08 00 00\n");
    printf("  Payload: \n");
    printf("Received UCI packet:\n");
    printf("  MT: 0x2\n");
    printf("  PBF: 0x0\n");
    printf("  GID: 0x0\n");
    printf("  Opcode: 0x02\n");
    printf("  Payload Length: 9\n");
    printf("  Payload: 00 01 00 02 00 03 00 04 00\n");
    printf("Unknown command: invalid_cmd\n");
    printf("Hardware mode not initialized. Use 'hw_init <device_path>' first.\n");
    printf("=== UCI Session Flow Demonstration ===\n");
    printf("Session initialized successfully\n");
    printf("Session started\n");
    printf("Ranging data received: Target 0x1234, Distance: 150 cm\n");
    printf("Session stopped\n");
    printf("=== Session Flow Demonstration Complete ===\n");
    
    printf("\n");
}

void print_enhanced_output() {
    printf("=== ENHANCED OUTPUT (Colorized) ===\n");
    
    // Enable colors for demonstration
    ui_enable_color(1);
    
    // Enhanced welcome message
    ui_print_welcome_message();
    
    // Enhanced success messages
    ui_print_success("Hardware mode initialized successfully with device: /dev/ttyUSB0");
    
    // Enhanced packet sending/receiving
    ui_print_sending_uci_packet("/dev/ttyUSB0");
    ui_print_hex_dump((unsigned char*)"\x20\x08\x00\x00", 4, "Header");
    ui_print_hex_dump((unsigned char*)"", 0, "Payload");
    
    ui_print_received_uci_packet();
    ui_print_hex_dump((unsigned char*)"\x02\x00\x00\x02\x00\x01\x00\x02\x00\x03\x00\x04\x00", 13, "Packet");
    
    // Enhanced error messages
    ui_print_command_not_found("invalid_cmd");
    ui_print_hardware_mode_not_initialized();
    
    // Enhanced simulation
    ui_print_simulation_started("UCI Session Flow");
    ui_print_success("Session initialized successfully");
    ui_print_success("Session started");
    
    // Enhanced ranging data with color coding
    if (ui_color_enabled) {
        printf("%s%sRanging Data:%s Target %s0x1234%s - Distance: %s%d cm%s\n",
               ANSI_COLOR_BRIGHT_CYAN, ANSI_BOLD, ANSI_RESET,
               ANSI_COLOR_BRIGHT_YELLOW, ANSI_RESET,
               ANSI_COLOR_BRIGHT_GREEN, 150, ANSI_RESET);
    } else {
        printf("Ranging Data: Target 0x1234 - Distance: 150 cm\n");
    }
    
    ui_print_success("Session stopped");
    ui_print_simulation_completed("UCI Session Flow");
    
    printf("\n");
}

void print_plain_text_comparison() {
    printf("=== PLAIN TEXT COMPARISON (Colors Disabled) ===\n");
    
    // Disable colors to show plain text version
    ui_enable_color(0);
    
    ui_print_welcome_message();
    ui_print_success("Hardware mode initialized successfully with device: /dev/ttyUSB0");
    ui_print_sending_uci_packet("/dev/ttyUSB0");
    ui_print_hex_dump((unsigned char*)"\x20\x08\x00\x00", 4, "Header");
    ui_print_received_uci_packet();
    ui_print_command_not_found("invalid_cmd");
    ui_print_hardware_mode_not_initialized();
    ui_print_simulation_started("UCI Session Flow");
    ui_print_success("Session initialized successfully");
    if (ui_color_enabled) {
        printf("%s%sRanging Data:%s Target %s0x1234%s - Distance: %s%d cm%s\n",
               ANSI_COLOR_BRIGHT_CYAN, ANSI_BOLD, ANSI_RESET,
               ANSI_COLOR_BRIGHT_YELLOW, ANSI_RESET,
               ANSI_COLOR_BRIGHT_GREEN, 150, ANSI_RESET);
    } else {
        printf("Ranging Data: Target 0x1234 - Distance: 150 cm\n");
    }
    ui_print_success("Session stopped");
    ui_print_simulation_completed("UCI Session Flow");
    
    printf("\n");
}

int main() {
    printf("UCI UI Enhancement - User Experience Comparison\n");
    printf("================================================\n\n");
    
    print_original_output();
    print_enhanced_output();
    print_plain_text_comparison();
    
    // Show benefits of UI enhancements
    printf("=== BENEFITS OF UI ENHANCEMENTS ===\n");
    if (ui_color_enabled) {
        printf("%s%s1. Improved Visual Hierarchy%s\n", ANSI_COLOR_BRIGHT_BLUE, ANSI_BOLD, ANSI_RESET);
        printf("   - Different message types use distinct colors\n");
        printf("   - Headers and titles stand out with bold formatting\n");
        printf("   - Important information is highlighted\n\n");
        
        printf("%s%s2. Better Error Recognition%s\n", ANSI_COLOR_BRIGHT_RED, ANSI_BOLD, ANSI_RESET);
        printf("   - Errors displayed in red for immediate attention\n");
        printf("   - Warnings use yellow to indicate caution\n");
        printf("   - Success messages in green for positive feedback\n\n");
        
        printf("%s%s3. Enhanced Readability%s\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_RESET);
        printf("   - Color coding reduces cognitive load\n");
        printf("   - Visual separation between different types of information\n");
        printf("   - Clear distinction between commands, responses, and notifications\n\n");
        
        printf("%s%s4. Professional Appearance%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
        printf("   - Modern, polished interface\n");
        printf("   - Consistent with other professional CLI tools\n");
        printf("   - Branding through color scheme\n\n");
        
        printf("%s%s5. Backward Compatibility%s\n", ANSI_COLOR_BRIGHT_CYAN, ANSI_BOLD, ANSI_RESET);
        printf("   - Can be disabled for plain text output\n");
        printf("   - Works in all terminal environments\n");
        printf("   - No dependencies on external libraries\n");
    } else {
        printf("1. Improved Visual Hierarchy\n");
        printf("   - Different message types use distinct formatting\n");
        printf("   - Headers and titles stand out with bold formatting\n");
        printf("   - Important information is highlighted\n\n");
        
        printf("2. Better Error Recognition\n");
        printf("   - Errors clearly marked with symbols\n");
        printf("   - Warnings indicated with warning symbols\n");
        printf("   - Success messages clearly identified\n\n");
        
        printf("3. Enhanced Readability\n");
        printf("   - Visual separation between different types of information\n");
        printf("   - Clear distinction between commands, responses, and notifications\n\n");
        
        printf("4. Professional Appearance\n");
        printf("   - Modern, polished interface\n");
        printf("   - Consistent with other professional CLI tools\n\n");
        
        printf("5. Backward Compatibility\n");
        printf("   - Can be disabled for plain text output\n");
        printf("   - Works in all terminal environments\n");
    }
    
    printf("\n%s%sUI Enhancement Implementation Successful!%s\n", 
           ANSI_COLOR_GREEN, ANSI_BOLD, ANSI_RESET);
    
    return 0;
}