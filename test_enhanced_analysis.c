/**
 * Test program to verify enhanced packet analysis functionality
 * This demonstrates the improvements made to the UCI packet analyzer
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "include/uci.h"
#include "include/uci_packet_analyzer.h"

void test_enhanced_error_analysis() {
    printf("=== Testing Enhanced Error Analysis ===\n");
    
    // Test various error codes
    unsigned char test_codes[] = {
        UCI_STATUS_OK,
        UCI_STATUS_REJECTED,
        UCI_STATUS_INVALID_PARAM,
        UCI_STATUS_UNKNOWN_GID,
        UCI_STATUS_UNKNOWN_OID,
        UCI_STATUS_SESSION_DUPLICATE,
        UCI_STATUS_SESSION_NOT_EXIST,
        UCI_STATUS_FAILED,
        UCI_STATUS_INVALID_RANGE,
        UCI_STATUS_INVALID_MSG_SIZE,
        0xFF  // Unknown code
    };
    
    int num_tests = sizeof(test_codes) / sizeof(test_codes[0]);
    
    for (int i = 0; i < num_tests; i++) {
        printf("\nTesting error code 0x%02X:\n", test_codes[i]);
        enhanced_error_analysis(test_codes[i]);
    }
    
    printf("\n=== Enhanced Error Analysis Test Complete ===\n\n");
}

void test_packet_analysis() {
    printf("=== Testing Packet Analysis ===\n");
    
    // Create a simple CORE_DEVICE_INFO response packet
    unsigned char packet[] = {
        0x40, 0x02, 0x00, 0x11,  // Header: RESPONSE, CORE, CORE_DEVICE_INFO
        0x00,                    // Status: OK
        0x01, 0x00,              // UCI Version: 1.0
        0x02, 0x00,              // MAC Version: 2.0
        0x03, 0x00,              // PHY Version: 3.0
        0x04, 0x00,              // Test Version: 4.0
        0x01,                    // Vendor Info Count: 1
        0xAA, 0xBB, 0xCC        // Vendor Specific Info
    };
    
    printf("Analyzing CORE_DEVICE_INFO response packet:\n");
    for (size_t i = 0; i < sizeof(packet); i++) {
        printf("%02X ", packet[i]);
    }
    printf("\n\n");
    
    uci_analyze_packet_core(packet, sizeof(packet));
    
    printf("\n=== Packet Analysis Test Complete ===\n\n");
}

void test_session_init_response() {
    printf("=== Testing SESSION_INIT Response Analysis ===\n");
    
    // Create a SESSION_INIT response packet
    unsigned char packet[] = {
        0x41, 0x00, 0x00, 0x05,  // Header: RESPONSE, SESSION_CONFIG, SESSION_INIT
        0x00,                    // Status: OK
        0x01, 0x00, 0x00, 0x00   // Session Handle: 1
    };
    
    printf("Analyzing SESSION_INIT response packet:\n");
    for (size_t i = 0; i < sizeof(packet); i++) {
        printf("%02X ", packet[i]);
    }
    printf("\n\n");
    
    uci_analyze_packet_core(packet, sizeof(packet));
    
    printf("\n=== SESSION_INIT Response Analysis Test Complete ===\n\n");
}

void test_core_set_config_response() {
    printf("=== Testing CORE_SET_CONFIG Response Analysis ===\n");
    
    // Create a CORE_SET_CONFIG response packet with error
    unsigned char packet[] = {
        0x40, 0x04, 0x00, 0x04,  // Header: RESPONSE, CORE, CORE_SET_CONFIG
        0x04,                    // Status: INVALID_PARAM
        0x01,                    // Number of config status: 1
        0x01,                    // Config ID: LOW_POWER_MODE
        0x04                     // Config Status: INVALID_PARAM
    };
    
    printf("Analyzing CORE_SET_CONFIG response with error:\n");
    for (size_t i = 0; i < sizeof(packet); i++) {
        printf("%02X ", packet[i]);
    }
    printf("\n\n");
    
    uci_analyze_packet_core(packet, sizeof(packet));
    
    printf("\n=== CORE_SET_CONFIG Response Analysis Test Complete ===\n\n");
}

int main() {
    printf("Enhanced UCI Packet Analysis Test Program\n");
    printf("===========================================\n\n");
    
    // Run all tests
    test_enhanced_error_analysis();
    test_packet_analysis();
    test_session_init_response();
    test_core_set_config_response();
    
    printf("All enhanced analysis tests completed successfully!\n");
    return 0;
}