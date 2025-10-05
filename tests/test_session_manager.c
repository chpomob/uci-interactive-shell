#include "../tests/test_runner.h"
#include "../include/uci.h"
#include "../include/uci_functions.h"

// Test suite for UCI session management functions
int main() {
    TEST_SUITE(session_manager);
    
    // Test session initialization
    TEST_CASE(init_uci_sessions);
    {
        // Initialize sessions
        init_uci_sessions();
        
        // Verify that all sessions are initialized properly
        for (int i = 0; i < MAX_SESSIONS; i++) {
            ASSERT_EQUAL(0, uci_sessions[i].is_allocated);
            ASSERT_EQUAL(SESSION_STATE_DEINIT, uci_sessions[i].session_state);
            ASSERT_EQUAL(0, uci_sessions[i].num_configs);
        }
        TEST_PASS();
    }
    test_case_end:;
    
    // Test finding free session slot - should return 0 when no sessions allocated
    TEST_CASE(find_free_session_slot_initial);
    {
        // After initialization, first slot should be free
        int free_slot = find_free_session_slot();
        ASSERT_EQUAL(0, free_slot);
        TEST_PASS();
    }
    test_case_end_1:;
    
    // Test finding free session slot after allocating some slots
    TEST_CASE(find_free_session_slot_after_allocation);
    {
        // Reset sessions first
        init_uci_sessions();
        
        // Allocate first session
        uci_sessions[0].is_allocated = 1;
        uci_sessions[0].session_state = SESSION_STATE_ACTIVE;
        
        // Second session should be free
        int free_slot = find_free_session_slot();
        ASSERT_EQUAL(1, free_slot);
        
        // Allocate second session
        uci_sessions[1].is_allocated = 1;
        uci_sessions[1].session_state = SESSION_STATE_ACTIVE;
        
        // Third session should be free
        free_slot = find_free_session_slot();
        ASSERT_EQUAL(2, free_slot);
        TEST_PASS();
    }
    test_case_end_2:;
    
    // Test finding free session slot when all are allocated
    TEST_CASE(find_free_session_slot_full);
    {
        // Reset and fill all slots
        init_uci_sessions();
        for (int i = 0; i < MAX_SESSIONS; i++) {
            uci_sessions[i].is_allocated = 1;
        }
        
        int free_slot = find_free_session_slot();
        ASSERT_EQUAL(-1, free_slot);
        TEST_PASS();
    }
    test_case_end_3:;
    
    // Test finding session by ID - non-existent session
    TEST_CASE(find_session_by_id_not_found);
    {
        init_uci_sessions();
        
        int result = find_session_by_id(12345);
        ASSERT_EQUAL(-1, result);
        TEST_PASS();
    }
    test_case_end_4:;
    
    // Test finding session by ID - existing session
    TEST_CASE(find_session_by_id_found);
    {
        init_uci_sessions();
        
        // Allocate a session with a specific ID
        uci_sessions[3].is_allocated = 1;
        uci_sessions[3].session_id = 9999;
        uci_sessions[3].session_state = SESSION_STATE_ACTIVE;
        
        int result = find_session_by_id(9999);
        ASSERT_EQUAL(3, result);
        TEST_PASS();
    }
    test_case_end_5:;
    
    // Test finding session by ID - with multiple sessions but only one matching
    TEST_CASE(find_session_by_id_multiple_sessions);
    {
        init_uci_sessions();
        
        // Allocate multiple sessions with different IDs
        uci_sessions[2].is_allocated = 1;
        uci_sessions[2].session_id = 5555;
        uci_sessions[2].session_state = SESSION_STATE_ACTIVE;
        
        uci_sessions[6].is_allocated = 1;
        uci_sessions[6].session_id = 7777;
        uci_sessions[6].session_state = SESSION_STATE_ACTIVE;
        
        // Search for one of them
        int result = find_session_by_id(7777);
        ASSERT_EQUAL(6, result);
        
        // Search for the other
        result = find_session_by_id(5555);
        ASSERT_EQUAL(2, result);
        
        // Search for non-existent
        result = find_session_by_id(8888);
        ASSERT_EQUAL(-1, result);
        TEST_PASS();
    }
    test_case_end_6:;
    
    // Test storing and getting session configuration
    TEST_CASE(store_and_get_session_config);
    {
        init_uci_sessions();
        
        // Allocate a session
        uci_sessions[0].is_allocated = 1;
        uci_sessions[0].session_id = 1000;
        uci_sessions[0].session_state = SESSION_STATE_ACTIVE;
        
        unsigned char test_value = 0xAB;
        unsigned char test_len = 1;
        
        // Store config
        store_session_config(0, DEVICE_STATE, &test_value, test_len);
        
        // Verify num_configs incremented
        ASSERT_EQUAL(1, uci_sessions[0].num_configs);
        
        // Retrieve config
        unsigned char retrieved_value[MAX_SESSION_CONFIG_VALUE_SIZE] = {0};
        unsigned char retrieved_len = (unsigned char)sizeof(retrieved_value);
        int result = get_session_config(0, DEVICE_STATE, retrieved_value, &retrieved_len);

        ASSERT_EQUAL(1, result);  // Successful retrieval
        ASSERT_EQUAL(test_value, retrieved_value[0]);
        ASSERT_EQUAL(test_len, retrieved_len);
        TEST_PASS();
    }
    test_case_end_7:;
    
    // Test getting non-existent session configuration
    TEST_CASE(get_session_config_not_found);
    {
        init_uci_sessions();
        
        // Allocate a session
        uci_sessions[0].is_allocated = 1;
        uci_sessions[0].session_id = 1000;
        uci_sessions[0].session_state = SESSION_STATE_ACTIVE;
        
        // Try to get config that hasn't been set
        unsigned char retrieved_value[MAX_SESSION_CONFIG_VALUE_SIZE] = {0};
        unsigned char retrieved_len = (unsigned char)sizeof(retrieved_value);
        int result = get_session_config(0, RANGING_ROUND_USAGE, retrieved_value, &retrieved_len);
        
        ASSERT_EQUAL(0, result);  // Not found
        ASSERT_EQUAL(0, retrieved_len);  // Length should be 0
        TEST_PASS();
    }
    test_case_end_8:;
    
    // Test storing multiple configurations in a session
    TEST_CASE(store_multiple_session_configs);
    {
        init_uci_sessions();
        
        // Allocate a session
        uci_sessions[0].is_allocated = 1;
        uci_sessions[0].session_id = 1000;
        uci_sessions[0].session_state = SESSION_STATE_ACTIVE;
        uci_sessions[0].num_configs = 0;  // Reset counter
        
        // Store multiple configs
        unsigned char val1 = 0x01;
        unsigned char val2 = 0x02;
        unsigned char val3 = 0x03;
        
        store_session_config(0, DEVICE_STATE, &val1, 1);
        store_session_config(0, CHANNEL_NUMBER, &val2, 1);
        store_session_config(0, DEVICE_ROLE, &val3, 1);
        
        // Verify num_configs incremented (though logic is simplified in source)
        ASSERT_EQUAL(3, uci_sessions[0].num_configs);
        
        // Retrieve each config
        unsigned char retrieved_val[MAX_SESSION_CONFIG_VALUE_SIZE] = {0};
        unsigned char retrieved_len = (unsigned char)sizeof(retrieved_val);

        ASSERT_EQUAL(1, get_session_config(0, DEVICE_STATE, retrieved_val, &retrieved_len));
        ASSERT_EQUAL(1, retrieved_len);
        ASSERT_EQUAL(0x01, retrieved_val[0]);

        retrieved_len = (unsigned char)sizeof(retrieved_val);
        ASSERT_EQUAL(1, get_session_config(0, CHANNEL_NUMBER, retrieved_val, &retrieved_len));
        ASSERT_EQUAL(1, retrieved_len);
        ASSERT_EQUAL(0x02, retrieved_val[0]);

        retrieved_len = (unsigned char)sizeof(retrieved_val);
        ASSERT_EQUAL(1, get_session_config(0, DEVICE_ROLE, retrieved_val, &retrieved_len));
        ASSERT_EQUAL(1, retrieved_len);
        ASSERT_EQUAL(0x03, retrieved_val[0]);
        TEST_PASS();
    }
    test_case_end_9:;

    // Test finding a session by handle
    TEST_CASE(find_session_by_handle);
    {
        init_uci_sessions();

        uci_sessions[1].is_allocated = 1;
        uci_sessions[1].session_handle = 0x12345678;

        int result = find_session_by_handle(0x12345678);
        ASSERT_EQUAL(1, result);

        result = find_session_by_handle(0x87654321);
        ASSERT_EQUAL(-1, result);
        TEST_PASS();
    }
    test_case_end_10:;

    // Test counting allocated sessions
    TEST_CASE(get_allocated_session_count_basic);
    {
        init_uci_sessions();
        ASSERT_EQUAL(0, get_allocated_session_count());

        uci_sessions[0].is_allocated = 1;
        uci_sessions[3].is_allocated = 1;
        ASSERT_EQUAL(2, get_allocated_session_count());
        TEST_PASS();
    }
    test_case_end_11:;

    // Test incrementing ranging count saturation
    TEST_CASE(increment_session_ranging_count_basic);
    {
        init_uci_sessions();
        uci_sessions[0].is_allocated = 1;
        uci_sessions[0].ranging_count = 0;

        increment_session_ranging_count(0);
        ASSERT_EQUAL(1, uci_sessions[0].ranging_count);

        uci_sessions[0].ranging_count = 0xFFFF;
        increment_session_ranging_count(0);
        ASSERT_EQUAL(0xFFFF, uci_sessions[0].ranging_count);
        TEST_PASS();
    }
    test_case_end_12:;
    
    TEST_SUITE_END();
}
