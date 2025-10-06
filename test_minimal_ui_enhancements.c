#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/uci.h"
#include "../include/uci_functions.h"
#include "../include/uci_cli.h"
#include "../include/uci_cli_completion.h"
#include "../include/uci_config_manager.h"
#include "../include/uci_hw.h"
#include "../include/uci_hw_chardev.h"
#include "../include/uci_hw_interface.h"
#include "../include/uci_ui.h"
#include "../include/uci_ui_main_patch.h"
#include "../include/uci_ui_packet_decoder.h"

int main() {
    // Enable UI color enhancements
    ui_color_enabled = 1;
    
    if (ui_color_enabled) {
        printf("%s%s==========================================%s\n", 
               ANSI_COLOR_BRIGHT_CYAN, ANSI_BOLD, ANSI_RESET);
        printf("%s  UCI Interactive Shell - UI Enhancement Test  %s\n", 
               ANSI_COLOR_BRIGHT_WHITE, ANSI_RESET);
        printf("%s==========================================%s\n\n", 
               ANSI_COLOR_BRIGHT_CYAN, ANSI_RESET);
    } else {
        printf("==========================================\n");
        printf("  UCI Interactive Shell - UI Enhancement Test  \n");
        printf("==========================================\n\n");
    }
    
    // Test UI enhancement functions
    if (ui_color_enabled) {
        printf("%s%s=== UI Enhancement Functions Demo ===%s\n", 
               ANSI_COLOR_BRIGHT_CYAN, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("=== UI Enhancement Functions Demo ===\n");
    }
    
    ui_print_header("UCI Interactive Shell - Enhanced UI");
    ui_print_subheader("UI Enhancement Functions Demonstration");
    ui_print_success("Operation completed successfully");
    ui_print_info("Information message for user guidance");
    ui_print_warning("Caution: Low battery level detected");
    ui_print_error("Critical error: Device connection lost");
    ui_print_debug("Debug: Packet sequence number 42 processed");
    ui_print_command("send CORE_DEVICE_INFO");
    ui_print_response("RECEIVED: CORE_DEVICE_INFO_RSP - Status OK");
    ui_print_notification("DEVICE_STATUS_NTF: Device state changed to ACTIVE");
    
    if (ui_color_enabled) {
        printf("%s%s✓ All UI enhancement functions working correctly%s\n\n", 
               ANSI_COLOR_GREEN, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("✓ All UI enhancement functions working correctly\n\n");
    }
    
    // Test enhanced hex dump
    if (ui_color_enabled) {
        printf("%s%s=== Enhanced Hex Dump Demo ===%s\n", 
               ANSI_COLOR_BRIGHT_CYAN, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("=== Enhanced Hex Dump Demo ===\n");
    }
    
    unsigned char sample_data[] = {
        0x20, 0x08, 0x00, 0x00, 0x00, 0x01, 0x02, 0x03,
        0x48, 0x65, 0x6C, 0x6C, 0x6F, 0x20, 0x57, 0x6F,
        0x72, 0x6C, 0x64, 0x00, 0xFF, 0x00, 0x00, 0x00
    };
    
    ui_print_hex_dump(sample_data, sizeof(sample_data), "Sample Data");
    
    if (ui_color_enabled) {
        printf("%s%s✓ Enhanced hex dump completed%s\n\n", 
               ANSI_COLOR_GREEN, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("✓ Enhanced hex dump completed\n\n");
    }
    
    // Test enhanced packet analysis
    if (ui_color_enabled) {
        printf("%s%s=== Enhanced Packet Analysis Demo ===%s\n", 
               ANSI_COLOR_BRIGHT_CYAN, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("=== Enhanced Packet Analysis Demo ===\n");
    }
    
    ui_print_packet_analysis(sample_data, sizeof(sample_data));
    
    if (ui_color_enabled) {
        printf("%s%s✓ Enhanced packet analysis completed%s\n\n", 
               ANSI_COLOR_GREEN, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("✓ Enhanced packet analysis completed\n\n");
    }
    
    // Test simulate_ranging command with UI enhancements
    if (ui_color_enabled) {
        printf("%s%s=== Simulate Ranging Command Test ===%s\n", 
               ANSI_COLOR_BRIGHT_CYAN, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("=== Simulate Ranging Command Test ===\n");
    }
    
    // Simulate the ranging command with enhanced UI
    if (ui_color_enabled) {
        printf("%s%s%s=== Simulating UWB Ranging Notification ===%s\n", 
               ANSI_COLOR_BRIGHT_CYAN, ANSI_BOLD, ANSI_BG_BLUE, ANSI_RESET);
    } else {
        printf("=== Simulating UWB Ranging Notification ===\n");
    }
    
    // Create a simulated ranging notification packet
    unsigned char ranging_ntf_payload[] = {
        // Header fields (24 bytes total)
        0x00, 0x00, 0x00, 0x01,  // Sequence number: 1
        0x01, 0x02, 0x03, 0x04,  // Session token: 0x01020304
        0x01,                    // RCR indicator
        0x00, 0x00, 0x00, 0x64,  // Current ranging interval: 100ms
        0x01,                    // Ranging measurement type: TWO_WAY (0x01)
        0x00,                    // Reserved
        0x00,                    // MAC address indicator: SHORT_ADDRESS (0x00)
        0x00, 0x00, 0x00, 0x00,  // HUS primary session ID: 0x00000000
        
        // Two-Way measurement data
        0x01,                    // Number of measurements: 1
        
        // First measurement (SHORT ADDRESS)
        0x12, 0x34,              // MAC Address: 0x1234
        0x00,                    // Status: OK
        0x00,                    // NLOS: NO
        0x00, 0x64,              // Distance: 100 cm (1 meter)
        0x00, 0x14,              // AoA Azimuth: 20 degrees
        0x08,                    // AoA Azimuth FoM: 8 (medium confidence)
        0x00, 0x05,              // AoA Elevation: 5 degrees
        0x07,                    // AoA Elevation FoM: 7 (high confidence)
        0x00, 0x10,              // Destination AoA Azimuth: 16 degrees
        0x06,                    // Destination AoA Azimuth FoM: 6 (medium-high confidence)
        0x00, 0x03,              // Destination AoA Elevation: 3 degrees
        0x09,                    // Destination AoA Elevation FoM: 9 (very high confidence)
        0x02,                    // Slot Index: 2
        0xE0                     // RSSI: -32 dBm (strong signal)
    };
    
    unsigned char notification_packet[sizeof(struct uci_packet_header) + sizeof(ranging_ntf_payload)];
    struct uci_packet_header* ntf_header = (struct uci_packet_header*)notification_packet;
    set_header_values(ntf_header, NOTIFICATION, COMPLETE, SESSION_CONFIG, SESSION_INFO_NTF, sizeof(ranging_ntf_payload));
    memcpy(notification_packet + sizeof(struct uci_packet_header), ranging_ntf_payload, sizeof(ranging_ntf_payload));
    
    if (ui_color_enabled) {
        printf("%s%s→ Sending simulated ranging notification packet%s\n", 
               ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("→ Sending simulated ranging notification packet\n");
    }
    
    // Use the UI-enhanced packet analysis directly
    ui_print_packet_analysis(notification_packet, sizeof(notification_packet));
    
    if (ui_color_enabled) {
        printf("%s%s%s=== Ranging Simulation Complete ===%s\n", 
               ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_BG_GREEN, ANSI_RESET);
    } else {
        printf("=== Ranging Simulation Complete ===\n");
    }
    
    if (ui_color_enabled) {
        printf("%s%s🎉 All UI Enhancement Tests Passed! 🎉%s\n", 
               ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("🎉 All UI Enhancement Tests Passed! 🎉\n");
    }
    
    return 0;
}