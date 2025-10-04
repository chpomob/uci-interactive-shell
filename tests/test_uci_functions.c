#include "../tests/test_runner.h"
#include "../include/uci.h"
#include "../include/uci_functions.h"
#include <string.h>

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
    
    // Test session management functions
    TEST_CASE(session_management_init);
    {
        init_uci_sessions();
        
        // Verify all sessions are initialized properly
        for (int i = 0; i < MAX_SESSIONS; i++) {
            if (uci_sessions[i].is_allocated != 0) {
                TEST_FAIL("Session should not be allocated after init");
                goto test_case_end4;
            }
            if (uci_sessions[i].session_state != SESSION_STATE_DEINIT) {
                TEST_FAIL("Session state should be deinit after init");
                goto test_case_end4;
            }
            if (uci_sessions[i].num_configs != 0) {
                TEST_FAIL("Number of configs should be 0 after init");
                goto test_case_end4;
            }
        }
        
        TEST_PASS();
    }
    test_case_end4:;
    
    // Test finding free session slot
    TEST_CASE(session_management_find_free_slot);
    {
        // Initially all slots should be free
        int slot = find_free_session_slot();
        if (slot < 0 || slot >= MAX_SESSIONS) {
            TEST_FAIL("Should find free slot initially");
            goto test_case_end5;
        }
        
        // Mark this slot as allocated
        uci_sessions[slot].is_allocated = 1;
        
        // Find another free slot
        int slot2 = find_free_session_slot();
        if (slot2 < 0 || slot2 == slot || slot2 >= MAX_SESSIONS) {
            TEST_FAIL("Should find second free slot");
            goto test_case_end5;
        }
        
        // Mark all slots except last one as allocated
        for (int i = 0; i < MAX_SESSIONS - 1; i++) {
            uci_sessions[i].is_allocated = 1;
        }
        
        // Last slot should still be free
        int final_slot = find_free_session_slot();
        if (final_slot != MAX_SESSIONS - 1) {
            TEST_FAIL("Last slot should still be free");
            goto test_case_end5;
        }
        
        // Mark last slot as allocated
        uci_sessions[MAX_SESSIONS - 1].is_allocated = 1;
        
        // Now no slots should be free
        int slot3 = find_free_session_slot();
        if (slot3 != -1) {
            TEST_FAIL("Should not find free slot when all are allocated");
            goto test_case_end5;
        }
        
        // Restore initial state for other tests
        init_uci_sessions();
        
        TEST_PASS();
    }
    test_case_end5:;
    
    // Test finding session by ID
    TEST_CASE(session_management_find_by_id);
    {
        // Initially no sessions should be found
        int session_idx = find_session_by_id(12345);
        if (session_idx != -1) {
            TEST_FAIL("Should not find session that doesn't exist");
            goto test_case_end6;
        }
        
        // Create a session
        int slot = find_free_session_slot();
        if (slot < 0) {
            TEST_FAIL("Could not find slot for test");
            goto test_case_end6;
        }
        
        uci_sessions[slot].session_id = 54321;
        uci_sessions[slot].is_allocated = 1;
        uci_sessions[slot].session_state = SESSION_STATE_ACTIVE;
        
        // Should find the created session
        session_idx = find_session_by_id(54321);
        if (session_idx != slot) {
            TEST_FAIL("Should find session that was created");
            goto test_case_end6;
        }
        
        // Should not find a different session ID
        session_idx = find_session_by_id(12345);
        if (session_idx != -1) {
            TEST_FAIL("Should not find session with different ID");
            goto test_case_end6;
        }
        
        // Restore initial state for other tests
        init_uci_sessions();
        
        TEST_PASS();
    }
    test_case_end6:;
    
    // Test storing and getting session configuration
    TEST_CASE(session_config_storage);
    {
        int slot = find_free_session_slot();
        if (slot < 0) {
            TEST_FAIL("Could not find slot for test");
            goto test_case_end7;
        }
        
        // Test storing configuration
        unsigned char test_value = 0x42;
        store_session_config(slot, DEVICE_STATE, &test_value, 1);
        
        if (uci_sessions[slot].num_configs != 1) {
            TEST_FAIL("Number of configs should be 1 after storing");
            goto test_case_end7;
        }
        
        // Test getting configuration
        unsigned char retrieved_value = 0;
        unsigned char retrieved_len = 0;
        int result = get_session_config(slot, DEVICE_STATE, &retrieved_value, &retrieved_len);
        
        if (result != 1) {
            TEST_FAIL("Should successfully retrieve config");
            goto test_case_end7;
        }
        if (retrieved_value != test_value) {
            TEST_FAIL("Retrieved value should match stored value");
            goto test_case_end7;
        }
        if (retrieved_len != 1) {
            TEST_FAIL("Retrieved length should match stored length");
            goto test_case_end7;
        }
        
        // Try getting non-existent config
        result = get_session_config(slot, LOW_POWER_MODE, &retrieved_value, &retrieved_len);
        if (result != 0) {
            TEST_FAIL("Should not retrieve non-existent config");
            goto test_case_end7;
        }
        
        // Restore initial state for other tests
        init_uci_sessions();
        
        TEST_PASS();
    }
    test_case_end7:;
    
    // Test notification handler for session info
    TEST_CASE(notification_handler_session_info);
    {
        // Create a minimal session info notification payload
        // According to UCI spec, this should have sequence number + session_token + etc.
        // For this test, just verify the function doesn't crash with minimal valid payload
        unsigned char minimal_payload[] = {
            0x00, 0x00, 0x00, 0x01,  // sequence_number (4 bytes)
            0x00, 0x00, 0x00, 0x02,  // session_token (4 bytes)
            0x01,                    // rcr_indicator (1 byte)
            0x00, 0x00, 0x00, 0x0A,  // current_ranging_interval (4 bytes) = 10 ms
            0x01,                    // ranging_measurement_type (1 byte) = TWO_WAY
            0x00,                    // reserved
            0x00, 0x00, 0x00, 0x01,  // hus_primary_session_id (4 bytes)
            0x01,                    // number of measurements (1 byte)
            0x00, 0x11,              // mac address (2 bytes)
            0x00,                    // status
            0x00,                    // nlos
            0x01, 0x2C,              // distance (300 cm)
            0x00, 0x00,              // aoa azimuth
            0x00,                    // aoa azimuth fom
            0x00, 0x00,              // aoa elevation
            0x00,                    // aoa elevation fom
            0x00, 0x00, 0x00, 0x00,  // aoa dest azimuth
            0x00,                    // aoa dest azimuth fom
            0x00, 0x00,              // aoa dest elevation
            0x00,                    // aoa dest elevation fom
            0x01,                    // slot index
            0x50                     // rssi
        };
        
        // This should not crash
        handle_session_info_ntf(minimal_payload, sizeof(minimal_payload));
        
        TEST_PASS();
    }
    test_case_end8:;
    
    TEST_SUITE_END();
}