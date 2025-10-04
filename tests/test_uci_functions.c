#include "../tests/test_runner.h"
#include "../include/uci.h"
#include "../include/uci_functions.h"

// Test suite for UCI functions
int main() {
    TEST_SUITE(uci_functions);
    
    // Test basic header creation
    TEST_CASE(header_creation);
    {
        struct uci_packet_header header;
        set_header_values(&header, COMMAND, COMPLETE, CORE, CORE_DEVICE_INFO, 0);
        
        if (get_mt(&header) != COMMAND) {
            TEST_FAIL("MT field mismatch");
            goto test_case_end;
        }
        if (get_pbf(&header) != COMPLETE) {
            TEST_FAIL("PBF field mismatch");
            goto test_case_end;
        }
        if (get_gid(&header) != CORE) {
            TEST_FAIL("GID field mismatch");
            goto test_case_end;
        }
        if (get_opcode(&header) != CORE_DEVICE_INFO) {
            TEST_FAIL("Opcode field mismatch");
            goto test_case_end;
        }
        if (header.payload_len != 0) {
            TEST_FAIL("Payload length mismatch");
            goto test_case_end;
        }
        
        TEST_PASS();
    }
    test_case_end:;
    
    // Test header values extraction
    TEST_CASE(header_extraction);
    {
        struct uci_packet_header header;
        set_header_values(&header, RESPONSE, NOT_COMPLETE, SESSION_CONFIG, SESSION_INIT, 10);
        
        if (get_mt(&header) != RESPONSE) {
            TEST_FAIL("MT field mismatch");
            goto test_case_end2;
        }
        if (get_pbf(&header) != NOT_COMPLETE) {
            TEST_FAIL("PBF field mismatch");
            goto test_case_end2;
        }
        if (get_gid(&header) != SESSION_CONFIG) {
            TEST_FAIL("GID field mismatch");
            goto test_case_end2;
        }
        if (get_opcode(&header) != SESSION_INIT) {
            TEST_FAIL("Opcode field mismatch");
            goto test_case_end2;
        }
        if (header.payload_len != 10) {
            TEST_FAIL("Payload length mismatch");
            goto test_case_end2;
        }
        
        TEST_PASS();
    }
    test_case_end2:;
    
    // Test notification header creation
    TEST_CASE(notification_header);
    {
        struct uci_packet_header header;
        set_header_values(&header, NOTIFICATION, COMPLETE, SESSION_CONTROL, SESSION_DATA_CREDIT_NTF, 5);
        
        if (get_mt(&header) != NOTIFICATION) {
            TEST_FAIL("MT field mismatch");
            goto test_case_end3;
        }
        if (get_pbf(&header) != COMPLETE) {
            TEST_FAIL("PBF field mismatch");
            goto test_case_end3;
        }
        if (get_gid(&header) != SESSION_CONTROL) {
            TEST_FAIL("GID field mismatch");
            goto test_case_end3;
        }
        if (get_opcode(&header) != SESSION_DATA_CREDIT_NTF) {
            TEST_FAIL("Opcode field mismatch");
            goto test_case_end3;
        }
        if (header.payload_len != 5) {
            TEST_FAIL("Payload length mismatch");
            goto test_case_end3;
        }
        
        TEST_PASS();
    }
    test_case_end3:;
    
    TEST_SUITE_END();
}