#include "../tests/test_runner.h"
#include "../include/uci.h"
#include "../include/uci_config_manager.h"

// Test suite for UCI configuration manager functions
int main() {
    TEST_SUITE(config_manager);
    
    // Test configuration manager initialization
    TEST_CASE(config_manager_init);
    {
        int result = uci_config_init();
        ASSERT_EQUAL(0, result);
        TEST_PASS();
    }
    test_case_end:;
    
    // Test application configuration parameter name retrieval
    TEST_CASE(app_config_param_name);
    {
        const char* name = uci_config_get_app_param_name(DEVICE_TYPE);
        ASSERT_TRUE(name != NULL);
        ASSERT_STRING_EQUAL("device_type", name);
        TEST_PASS();
    }
    test_case_end_1:;
    
    // Test device configuration parameter name retrieval
    TEST_CASE(device_config_param_name);
    {
        const char* name = uci_config_get_device_param_name(DEVICE_STATE);
        ASSERT_TRUE(name != NULL);
        ASSERT_STRING_EQUAL("device_state", name);
        TEST_PASS();
    }
    test_case_end_2:;
    
    // Test application configuration parameter description retrieval
    TEST_CASE(app_config_param_desc);
    {
        const char* desc = uci_config_get_app_param_desc(DEVICE_TYPE);
        ASSERT_TRUE(desc != NULL);
        // Just check that we get a non-empty description
        ASSERT_TRUE(strlen(desc) > 0);
        TEST_PASS();
    }
    test_case_end_3:;
    
    // Test device configuration parameter description retrieval
    TEST_CASE(device_config_param_desc);
    {
        const char* desc = uci_config_get_device_param_desc(DEVICE_STATE);
        ASSERT_TRUE(desc != NULL);
        // Just check that we get a non-empty description
        ASSERT_TRUE(strlen(desc) > 0);
        TEST_PASS();
    }
    test_case_end_4:;
    
    // Test application configuration parameter default value retrieval
    TEST_CASE(app_config_param_default);
    {
        uint64_t default_val = uci_config_get_app_param_default(DEVICE_TYPE);
        // DEVICE_TYPE default should be 0x01 (responder)
        ASSERT_UINT64_EQUAL(0x01, default_val);
        TEST_PASS();
    }
    test_case_end_5:;
    
    // Test device configuration parameter default value retrieval
    TEST_CASE(device_config_param_default);
    {
        uint64_t default_val = uci_config_get_device_param_default(DEVICE_STATE);
        // DEVICE_STATE default should be 0x01 (ready)
        ASSERT_UINT64_EQUAL(0x01, default_val);
        TEST_PASS();
    }
    test_case_end_6:;
    
    TEST_SUITE_END();
}