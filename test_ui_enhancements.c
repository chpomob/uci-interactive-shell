#include <stdio.h>
#include <stdlib.h>
#include "../include/uci_ui.h"

int main() {
    printf("=== UCI UI Enhancements Test ===\n\n");
    
    // Test all UI functions with colors enabled
    ui_print_header("UCI Interactive Shell - Enhanced UI");
    ui_print_subheader("Testing Colorized Output");
    
    ui_print_success("Successfully initialized UCI interface");
    ui_print_info("Device info: UCI Version 1.0, MAC Version 2.0");
    ui_print_warning("Low battery warning: 15%% remaining");
    ui_print_error("Connection timeout - retrying...");
    ui_print_debug("Debug: Packet sent with sequence number 42");
    
    ui_print_command("SEND CORE_DEVICE_INFO");
    ui_print_response("RECEIVED CORE_DEVICE_INFO_RSP: Status OK");
    ui_print_notification("DEVICE_STATUS_NTF: Device state changed to ACTIVE");
    
    // Test packet analysis
    unsigned char test_packet[] = {0x20, 0x08, 0x00, 0x00, 0x00, 0x01, 0x02, 0x03};
    ui_print_packet_analysis(test_packet, sizeof(test_packet));
    
    printf("\n--- Hex Dump Test ---\n");
    unsigned char test_data[] = {
        0x20, 0x08, 0x00, 0x00, 0x00, 0x01, 0x02, 0x03,
        0x48, 0x65, 0x6C, 0x6C, 0x6F, 0x20, 0x57, 0x6F,
        0x72, 0x6C, 0x64, 0x00, 0xFF, 0x00, 0x00, 0x00
    };
    ui_print_hex_dump(test_data, sizeof(test_data), "DATA");
    
    // Test with colors disabled
    printf("\n=== Same Output with Colors Disabled ===\n");
    ui_enable_color(0);
    
    ui_print_header("UCI Interactive Shell - Plain Text");
    ui_print_subheader("Testing Plain Text Output");
    
    ui_print_success("Successfully initialized UCI interface");
    ui_print_info("Device info: UCI Version 1.0, MAC Version 2.0");
    ui_print_warning("Low battery warning: 15%% remaining");
    ui_print_error("Connection timeout - retrying...");
    ui_print_debug("Debug: Packet sent with sequence number 42");
    
    ui_print_command("SEND CORE_DEVICE_INFO");
    ui_print_response("RECEIVED CORE_DEVICE_INFO_RSP: Status OK");
    ui_print_notification("DEVICE_STATUS_NTF: Device state changed to ACTIVE");
    
    ui_print_packet_analysis(test_packet, sizeof(test_packet));
    
    printf("\n--- Hex Dump Test (Plain) ---\n");
    ui_print_hex_dump(test_data, sizeof(test_data), "DATA");
    
    return 0;
}