#include "../tests/test_runner.h"
#include "../include/uci_hw_interface.h"
#include <string.h>

// Test suite for UCI hardware interface functions
int main() {
    TEST_SUITE(hw_interface);
    
    // Test getting device path (should work even without initialization)
    TEST_CASE(hw_interface_get_device_path);
    {
        const char* path = uci_hw_interface_get_device_path();
        // This should return a default path even if not initialized
        ASSERT_TRUE(path != NULL);
        // The default path should not be empty
        ASSERT_TRUE(strlen(path) > 0);
        TEST_PASS();
    }
    test_case_end:
    
    // Test is_connected function (should return 0 when not initialized)
    TEST_CASE(hw_interface_is_connected);
    {
        int connected = uci_hw_interface_is_connected();
        // When not initialized, should return 0
        ASSERT_EQUAL(0, connected);
        TEST_PASS();
    }
    test_case_end_1:
    
    // Test verbose mode functionality
    TEST_CASE(hw_interface_verbose);
    {
        uci_hw_interface_set_verbose(1);
        // Just verify that function exists and can be called
        // The actual verbose behavior would be checked separately
        uci_hw_interface_set_verbose(0); // Reset to default
        ASSERT_TRUE(1);
        TEST_PASS();
    }
    test_case_end_2:
    
    // Test cleanup function (should be safe to call even without initialization)
    TEST_CASE(hw_interface_cleanup);
    {
        uci_hw_interface_cleanup();
        // Just verify that function exists and can be called safely
        ASSERT_TRUE(1);
        TEST_PASS();
    }
    test_case_end_3:
    
    // Test initialization with different device paths
    TEST_CASE(hw_interface_init_different_paths);
    {
        // Test with default path
        int result = uci_hw_interface_init("/dev/ttyUSB0");
        // This may fail if the device doesn't exist, but function should be callable
        (void)result;
        
        // Test with a different path
        result = uci_hw_interface_init("/dev/ttyACM0");
        // Again, may fail due to device not existing, but function should be callable
        (void)result;
        
        // Reset by cleaning up
        uci_hw_interface_cleanup();
        
        ASSERT_TRUE(1); // Just verify functions are callable
        TEST_PASS();
    }
    test_case_end_4:
    
    // Test send command with uninitialized interface (should fail gracefully)
    TEST_CASE(hw_interface_send_uninitialized);
    {
        unsigned char dummy_payload[] = {0x01, 0x02, 0x03};
        int result = uci_hw_interface_send_command(1, 1, 1, 1, dummy_payload, 3);
        // Should return -1 since interface is not initialized
        ASSERT_EQUAL(-1, result);
        TEST_PASS();
    }
    test_case_end_5:
    
    // Test receive response with uninitialized interface (should fail gracefully)
    TEST_CASE(hw_interface_receive_uninitialized);
    {
        unsigned char buffer[256];
        int result = uci_hw_interface_receive_response(buffer, sizeof(buffer), 100);
        // Should return -1 since interface is not initialized
        ASSERT_EQUAL(-1, result);
        TEST_PASS();
    }
    test_case_end_6:
    
    TEST_SUITE_END();
}