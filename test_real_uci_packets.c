#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/uci.h"
#include "../include/uci_functions.h"

// Test function to verify parsing of real packet from logs
int test_real_range_notification() {
    printf("Testing real range notification packet from logs...\n");
    
    // This is from our real log: 6b0300212a0000000800000006030100000001000001000000020100000401000005000000
    unsigned char packet[] = {
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
    
    struct uci_packet_header* header = (struct uci_packet_header*)packet;
    
    // Verify header fields
    unsigned char gid = get_gid(header);
    unsigned char mt = get_mt(header);
    unsigned char opcode = get_opcode(header);
    unsigned char payload_len = header->payload_len;
    
    printf("  GID: 0x%02x (%s)\n", gid, gid == RANGING_DATA ? "RANGING_DATA" : "OTHER");
    printf("  MT: %d (%s)\n", mt, mt == NOTIFICATION ? "NOTIFICATION" : "OTHER");
    printf("  Opcode: 0x%02x\n", opcode);
    printf("  Payload Length: %d\n", payload_len);
    
    // Expected values based on real packet
    if (gid != RANGING_DATA) {
        printf("  ERROR: Expected GID RANGING_DATA (0x0B), got 0x%02x\n", gid);
        return 0;
    }
    
    if (mt != NOTIFICATION) {
        printf("  ERROR: Expected MT NOTIFICATION, got %d\n", mt);
        return 0;
    }
    
    if (payload_len != 33) {
        printf("  ERROR: Expected payload length 33, got %d\n", payload_len);
        return 0;
    }
    
    printf("  SUCCESS: Real packet parsed correctly\n");
    return 1;
}

// Test function for PDL specification packet: DeviceResetCmd
int test_device_reset_cmd() {
    printf("\nTesting DeviceResetCmd from PDL specification...\n");
    
    // From PDL: "2000000100000000"
    unsigned char packet[] = {
        0x20, 0x00, 0x00, 0x01,  // Header: CORE command, opcode 0x00, payload length 1
        0x00  // reset_config payload
    };
    
    struct uci_packet_header* header = (struct uci_packet_header*)packet;
    
    unsigned char gid = get_gid(header);
    unsigned char mt = get_mt(header);
    unsigned char opcode = get_opcode(header);
    unsigned char payload_len = header->payload_len;
    
    printf("  GID: 0x%02x (%s)\n", gid, gid == CORE ? "CORE" : "OTHER");
    printf("  MT: %d (%s)\n", mt, mt == COMMAND ? "COMMAND" : "OTHER");
    printf("  Opcode: 0x%02x\n", opcode);
    printf("  Payload Length: %d\n", payload_len);
    
    if (gid != CORE) {
        printf("  ERROR: Expected GID CORE, got 0x%02x\n", gid);
        return 0;
    }
    
    if (mt != COMMAND) {
        printf("  ERROR: Expected MT COMMAND, got %d\n", mt);
        return 0;
    }
    
    if (opcode != 0x00) {  // CORE_DEVICE_RESET
        printf("  ERROR: Expected opcode 0x00, got 0x%02x\n", opcode);
        return 0;
    }
    
    if (payload_len != 1) {
        printf("  ERROR: Expected payload length 1, got %d\n", payload_len);
        return 0;
    }
    
    printf("  SUCCESS: DeviceResetCmd parsed correctly\n");
    return 1;
}

// Test function for PDL specification packet: GetDeviceInfoRsp
int test_get_device_info_rsp() {
    printf("\nTesting GetDeviceInfoRsp from PDL specification...\n");
    
    // From PDL: "4002000b000000010100020003000400010a"
    unsigned char packet[] = {
        0x40, 0x02, 0x00, 0x0b,  // Header: CORE response, opcode 0x02, payload length 11
        0x00, 0x00, 0x00, 0x01,  // status = UCI_STATUS_OK
        0x01, 0x00,              // uci_version = 0x0001
        0x02, 0x00,              // mac_version = 0x0002  
        0x03, 0x00,              // phy_version = 0x0003
        0x04, 0x00,              // uci_test_version = 0x0004
        0x01,                    // vendor_spec_info count = 1
        0x0a                     // vendor_spec_info[0] = 0x0a
    };
    
    struct uci_packet_header* header = (struct uci_packet_header*)packet;
    
    unsigned char gid = get_gid(header);
    unsigned char mt = get_mt(header);
    unsigned char opcode = get_opcode(header);
    unsigned char payload_len = header->payload_len;
    
    printf("  GID: 0x%02x (%s)\n", gid, gid == CORE ? "CORE" : "OTHER");
    printf("  MT: %d (%s)\n", mt, mt == RESPONSE ? "RESPONSE" : "OTHER");
    printf("  Opcode: 0x%02x\n", opcode);
    printf("  Payload Length: %d\n", payload_len);
    
    if (gid != CORE) {
        printf("  ERROR: Expected GID CORE, got 0x%02x\n", gid);
        return 0;
    }
    
    if (mt != RESPONSE) {
        printf("  ERROR: Expected MT RESPONSE, got %d\n", mt);
        return 0;
    }
    
    if (opcode != 0x02) {  // CORE_DEVICE_INFO
        printf("  ERROR: Expected opcode 0x02, got 0x%02x\n", opcode);
        return 0;
    }
    
    if (payload_len != 11) {
        printf("  ERROR: Expected payload length 11, got %d\n", payload_len);
        return 0;
    }
    
    // Check status field (first byte of payload)
    if (packet[4] != 0) {  // UCI_STATUS_OK
        printf("  ERROR: Expected status UCI_STATUS_OK (0), got %d\n", packet[4]);
        return 0;
    }
    
    printf("  SUCCESS: GetDeviceInfoRsp parsed correctly\n");
    return 1;
}

// Test function for Session Info Notification
int test_session_info_ntf() {
    printf("\nTesting SessionInfoNtf from PDL specification...\n");
    
    // From PDL: "620000190000000002030405060708000a010101010000000000000000000000"
    unsigned char packet[] = {
        0x62, 0x00, 0x00, 0x19,  // Header: SESSION_CONTROL notification, opcode 0x00, payload length 25
        0x00, 0x00, 0x00, 0x00,  // sequence number
        0x02, 0x03, 0x04, 0x05,  // session token 
        0x06,                    // rcr_indicator
        0x07, 0x08, 0x00, 0x00,  // current_ranging_interval
        0x0a,                    // ranging_measurement_type
        0x01,                    // reserved field
        0x01,                    // mac_address_indicator
        0x00, 0x00, 0x00, 0x00,  // hus_primary_session_id
        0x00, 0x00, 0x00, 0x00, 0x00  // remaining payload
    };
    
    struct uci_packet_header* header = (struct uci_packet_header*)packet;
    
    unsigned char gid = get_gid(header);
    unsigned char mt = get_mt(header);
    unsigned char opcode = get_opcode(header);
    unsigned char payload_len = header->payload_len;
    
    printf("  GID: 0x%02x (%s)\n", gid, gid == SESSION_CONTROL ? "SESSION_CONTROL" : "OTHER");
    printf("  MT: %d (%s)\n", mt, mt == NOTIFICATION ? "NOTIFICATION" : "OTHER");
    printf("  Opcode: 0x%02x\n", opcode);
    printf("  Payload Length: %d\n", payload_len);
    
    if (gid != SESSION_CONTROL) {
        printf("  ERROR: Expected GID SESSION_CONTROL, got 0x%02x\n", gid);
        return 0;
    }
    
    if (mt != NOTIFICATION) {
        printf("  ERROR: Expected MT NOTIFICATION, got %d\n", mt);
        return 0;
    }
    
    if (opcode != 0x00) {  // SESSION_INFO_NTF
        printf("  ERROR: Expected opcode 0x00, got 0x%02x\n", opcode);
        return 0;
    }
    
    if (payload_len != 25) {
        printf("  ERROR: Expected payload length 25, got %d\n", payload_len);
        return 0;
    }
    
    printf("  SUCCESS: SessionInfoNtf parsed correctly\n");
    return 1;
}

// Test function for Android Range Diagnostics Notification
int test_android_range_diagnostics_ntf() {
    printf("\nTesting AndroidRangeDiagnosticsNtf from PDL specification...\n");
    
    // From PDL: "6c0200110000000101010102020202010001020100010000" 
    unsigned char packet[] = {
        0x6c, 0x02, 0x00, 0x11,  // Header: VENDOR_ANDROID notification, opcode 0x02, payload length 17
        0x00, 0x00, 0x00, 0x01,  // session_token
        0x01, 0x01, 0x01, 0x02,  // sequence_number
        0x02, 0x02, 0x02, 0x01,  // frame reports data
        0x00, 0x01, 0x02, 0x01,  // more frame reports
        0x00, 0x01, 0x00, 0x00   // end of frame reports
    };
    
    struct uci_packet_header* header = (struct uci_packet_header*)packet;
    
    unsigned char gid = get_gid(header);
    unsigned char mt = get_mt(header);
    unsigned char opcode = get_opcode(header);
    unsigned char payload_len = header->payload_len;
    
    printf("  GID: 0x%02x (%s)\n", gid, gid == VENDOR_ANDROID ? "VENDOR_ANDROID" : "OTHER");
    printf("  MT: %d (%s)\n", mt, mt == NOTIFICATION ? "NOTIFICATION" : "OTHER");
    printf("  Opcode: 0x%02x\n", opcode);
    printf("  Payload Length: %d\n", payload_len);
    
    if (gid != VENDOR_ANDROID) {
        printf("  ERROR: Expected GID VENDOR_ANDROID, got 0x%02x\n", gid);
        return 0;
    }
    
    if (mt != NOTIFICATION) {
        printf("  ERROR: Expected MT NOTIFICATION, got %d\n", mt);
        return 0;
    }
    
    if (opcode != 0x02) {  // ANDROID_FIRA_RANGE_DIAGNOSTICS
        printf("  ERROR: Expected opcode 0x02, got 0x%02x\n", opcode);
        return 0;
    }
    
    if (payload_len != 17) {
        printf("  ERROR: Expected payload length 17, got %d\n", payload_len);
        return 0;
    }
    
    printf("  SUCCESS: AndroidRangeDiagnosticsNtf parsed correctly\n");
    return 1;
}

int main() {
    printf("UCI Packet Tests from Real Logs and PDL Specifications\n");
    printf("=====================================================\n");
    
    int total_tests = 5;
    int passed_tests = 0;
    
    if (test_real_range_notification()) passed_tests++;
    if (test_device_reset_cmd()) passed_tests++;
    if (test_get_device_info_rsp()) passed_tests++;
    if (test_session_info_ntf()) passed_tests++;
    if (test_android_range_diagnostics_ntf()) passed_tests++;
    
    printf("\nTest Results: %d/%d tests passed\n", passed_tests, total_tests);
    
    if (passed_tests == total_tests) {
        printf("All tests passed! ✓\n");
        return 0;
    } else {
        printf("Some tests failed! ✗\n");
        return 1;
    }
}