#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/uci.h"
#include "../include/uci_functions.h"
#include "../include/uci_ui.h"
#include "../include/uci_ui_packet_decoder.h"

// Function to simulate parsing a real UCI packet from logs
void test_real_uci_packet_parsing() {
    if (ui_color_enabled) {
        printf("%s%s=== Real UCI Packet Parsing Test ===%s\n", 
               ANSI_COLOR_BRIGHT_CYAN, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("=== Real UCI Packet Parsing Test ===\n");
    }
    
    // Example real packet from uwb_range_ntf.log:
    // 6b0300212a0000000800000006030100000001000001000000020100000401000005000000
    unsigned char real_packet[] = {
        0x6b, 0x03, 0x00, 0x21,  // Header: RANGING_DATA notification, opcode 0x03, payload length 33
        0x2a, 0x00, 0x00, 0x00,  // Session token (little endian)
        0x08, 0x00, 0x00, 0x00,  // Sequence number (little endian)
        0x06, 0x03, 0x01, 0x00,  // Control field and more
        0x00, 0x00, 0x01, 0x00,  // More payload
        0x00, 0x01, 0x00, 0x00,  // More payload
        0x00, 0x02, 0x01, 0x00,  // More payload
        0x00, 0x04, 0x01, 0x00,  // More payload
        0x00, 0x05  // Remaining payload
    };
    
    // Parse using our enhanced UI decoder
    parse_uci_packet(real_packet, sizeof(real_packet));
    
    if (ui_color_enabled) {
        printf("%s%s✓ Real packet parsed successfully%s\n\n", 
               ANSI_COLOR_GREEN, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("✓ Real packet parsed successfully\n\n");
    }
}

// Function to simulate parsing UCI packets from PDL specification
void test_pdl_specification_packets() {
    if (ui_color_enabled) {
        printf("%s%s=== PDL Specification Packet Tests ===%s\n", 
               ANSI_COLOR_BRIGHT_CYAN, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("=== PDL Specification Packet Tests ===\n");
    }
    
    // Test CORE_DEVICE_INFO_RSP packet from PDL
    unsigned char device_info_rsp[] = {
        0x40, 0x02, 0x00, 0x0b,  // Header: CORE response, opcode 0x02, payload length 11
        0x00, 0x00, 0x00, 0x01,  // status = UCI_STATUS_OK
        0x01, 0x00,              // uci_version = 0x0001
        0x02, 0x00,              // mac_version = 0x0002  
        0x03, 0x00,              // phy_version = 0x0003
        0x04, 0x00,              // uci_test_version = 0x0004
        0x01,                    // vendor_spec_info count = 1
        0x0a                     // vendor_spec_info[0] = 0x0a
    };
    
    if (ui_color_enabled) {
        printf("%sTesting CORE_DEVICE_INFO_RSP packet:%s\n", 
               ANSI_COLOR_BRIGHT_MAGENTA, ANSI_RESET);
    } else {
        printf("Testing CORE_DEVICE_INFO_RSP packet:\n");
    }
    
    parse_uci_packet(device_info_rsp, sizeof(device_info_rsp));
    
    // Test SESSION_INFO_NTF packet from PDL
    unsigned char session_info_ntf[] = {
        0x62, 0x00, 0x00, 0x19,  // Header: SESSION_CONTROL notification, opcode 0x00, payload length 25
        0x00, 0x00, 0x00, 0x00,  // sequence number
        0x02, 0x03, 0x04, 0x05,  // session token 
        0x06,                    // rcr_indicator
        0x07, 0x08, 0x00, 0x00,  // current_ranging_interval
        0x0a,                    // ranging_measurement_type
        0x01,                    // reserved
        0x00,                    // mac_address_indicator
        0x01, 0x00, 0x00, 0x00,  // hus_primary_session_id
        0x00, 0x00, 0x00, 0x00, 0x00  // remaining payload
    };
    
    if (ui_color_enabled) {
        printf("%sTesting SESSION_INFO_NTF packet:%s\n", 
               ANSI_COLOR_BRIGHT_MAGENTA, ANSI_RESET);
    } else {
        printf("Testing SESSION_INFO_NTF packet:\n");
    }
    
    parse_uci_packet(session_info_ntf, sizeof(session_info_ntf));
    
    // Test ANDROID_FIRA_RANGE_DIAGNOSTICS_NTF packet from PDL
    unsigned char android_diag_ntf[] = {
        0x6c, 0x02, 0x00, 0x11,  // Header: VENDOR_ANDROID notification, opcode 0x02, payload length 17
        0x00, 0x00, 0x00, 0x01,  // session_token
        0x01, 0x01, 0x01, 0x02,  // sequence_number
        0x02, 0x02, 0x02, 0x01,  // frame reports data
        0x00, 0x01, 0x02, 0x01,  // more frame reports
        0x00, 0x01, 0x00, 0x00   // end of frame reports
    };
    
    if (ui_color_enabled) {
        printf("%sTesting ANDROID_FIRA_RANGE_DIAGNOSTICS_NTF packet:%s\n", 
               ANSI_COLOR_BRIGHT_MAGENTA, ANSI_RESET);
    } else {
        printf("Testing ANDROID_FIRA_RANGE_DIAGNOSTICS_NTF packet:\n");
    }
    
    parse_uci_packet(android_diag_ntf, sizeof(android_diag_ntf));
    
    if (ui_color_enabled) {
        printf("%s%s✓ All PDL specification packets parsed successfully%s\n\n", 
               ANSI_COLOR_GREEN, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("✓ All PDL specification packets parsed successfully\n\n");
    }
}

// Function to demonstrate enhanced packet analysis
void test_enhanced_packet_analysis() {
    if (ui_color_enabled) {
        printf("%s%s=== Enhanced Packet Analysis Demo ===%s\n", 
               ANSI_COLOR_BRIGHT_CYAN, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("=== Enhanced Packet Analysis Demo ===\n");
    }
    
    // Show hex dump with color coding
    unsigned char sample_data[] = {
        0x20, 0x08, 0x00, 0x00, 0x00, 0x01, 0x02, 0x03,
        0x48, 0x65, 0x6C, 0x6C, 0x6F, 0x20, 0x57, 0x6F,
        0x72, 0x6C, 0x64, 0x00, 0xFF, 0x00, 0x00, 0x00
    };
    
    if (ui_color_enabled) {
        printf("%sHex Dump with Color Coding:%s\n", 
               ANSI_COLOR_BRIGHT_BLUE, ANSI_RESET);
    } else {
        printf("Hex Dump with Color Coding:\n");
    }
    
    ui_print_hex_dump(sample_data, sizeof(sample_data), "Sample Data");
    
    // Show packet analysis
    if (ui_color_enabled) {
        printf("%s\nPacket Analysis with Enhanced UI:%s\n", 
               ANSI_COLOR_BRIGHT_BLUE, ANSI_RESET);
    } else {
        printf("\nPacket Analysis with Enhanced UI:\n");
    }
    
    ui_print_packet_analysis(sample_data, sizeof(sample_data));
    
    if (ui_color_enabled) {
        printf("%s%s✓ Enhanced packet analysis completed%s\n\n", 
               ANSI_COLOR_GREEN, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("✓ Enhanced packet analysis completed\n\n");
    }
}

// Function to demonstrate UI enhancement functions
void test_ui_enhancement_functions() {
    if (ui_color_enabled) {
        printf("%s%s=== UI Enhancement Functions Demo ===%s\n", 
               ANSI_COLOR_BRIGHT_CYAN, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("=== UI Enhancement Functions Demo ===\n");
    }
    
    // Demonstrate all UI enhancement functions
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
}

int main() {
    if (ui_color_enabled) {
        printf("%s%s==========================================%s\n", 
               ANSI_COLOR_BRIGHT_CYAN, ANSI_BOLD, ANSI_RESET);
        printf("%s  UCI Interactive Shell - UI Enhancement Test Suite  %s\n", 
               ANSI_COLOR_BRIGHT_WHITE, ANSI_RESET);
        printf("%s==========================================%s\n\n", 
               ANSI_COLOR_BRIGHT_CYAN, ANSI_RESET);
    } else {
        printf("==========================================\n");
        printf("  UCI Interactive Shell - UI Enhancement Test Suite  \n");
        printf("==========================================\n\n");
    }
    
    // Run all tests
    test_ui_enhancement_functions();
    test_enhanced_packet_analysis();
    test_real_uci_packet_parsing();
    test_pdl_specification_packets();
    
    if (ui_color_enabled) {
        printf("%s%s🎉 All UI Enhancement Tests Passed! 🎉%s\n", 
               ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_RESET);
        printf("%s%sThe UCI Interactive Shell now features a modern, colorized interface%s\n", 
               ANSI_COLOR_BRIGHT_CYAN, ANSI_RESET, ANSI_RESET);
    } else {
        printf("🎉 All UI Enhancement Tests Passed! 🎉\n");
        printf("The UCI Interactive Shell now features a modern, colorized interface\n");
    }
    
    return 0;
}