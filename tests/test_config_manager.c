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
    
#define test_case_end test_case_end_1
    // Test application configuration parameter name retrieval
    TEST_CASE(app_config_param_name);
    {
        const char* name = uci_config_get_app_param_name(DEVICE_TYPE);
        ASSERT_TRUE(name != NULL);
        ASSERT_STRING_EQUAL("device_type", name);
        TEST_PASS();
    }
    test_case_end:;
#undef test_case_end

#define test_case_end test_case_end_device_config_set_and_get_pan
    TEST_CASE(device_config_set_and_get_pan_id);
    {
        unsigned char value[2] = {0x34, 0x12};
        ASSERT_EQUAL(0, uci_config_set_device_param(DEVICE_PAN_ID, value, sizeof(value)));

        unsigned char buffer[4] = {0};
        size_t buffer_len = sizeof(buffer);
        ASSERT_EQUAL(0, uci_config_get_device_param(DEVICE_PAN_ID, buffer, &buffer_len));
        ASSERT_EQUAL(2, (int)buffer_len);
        ASSERT_EQUAL(0x34, buffer[0]);
        ASSERT_EQUAL(0x12, buffer[1]);
        TEST_PASS();
    }
    test_case_end:;
#undef test_case_end

#define test_case_end test_case_end_device_config_invalid_length
    TEST_CASE(device_config_set_invalid_length);
    {
        unsigned char value[1] = {0x01};
        ASSERT_EQUAL(-1, uci_config_set_device_param(DEVICE_PAN_ID, value, sizeof(value)));
        TEST_PASS();
    }
    test_case_end:;
#undef test_case_end

#define test_case_end test_case_end_device_config_parse_values
    TEST_CASE(device_config_parse_value_helpers);
    {
        unsigned char buffer[8] = {0};
        size_t len = sizeof(buffer);
        ASSERT_EQUAL(0, uci_config_parse_device_value(DEVICE_CHANNEL, "7", buffer, &len));
        ASSERT_EQUAL(1, (int)len);
        ASSERT_EQUAL(7, buffer[0]);

        len = sizeof(buffer);
        ASSERT_EQUAL(0, uci_config_parse_device_value(DEVICE_PAN_ID, "0xBEEF", buffer, &len));
        ASSERT_EQUAL(2, (int)len);
        ASSERT_EQUAL(0xEF, buffer[0]);
        ASSERT_EQUAL(0xBE, buffer[1]);

        len = sizeof(buffer);
        ASSERT_EQUAL(0, uci_config_parse_device_value(DEVICE_EXTENDED_ADDR, "0x0102030405060708", buffer, &len));
        ASSERT_EQUAL(8, (int)len);
        ASSERT_EQUAL(0x08, buffer[0]);
        ASSERT_EQUAL(0x01, buffer[7]);

        len = sizeof(buffer);
        ASSERT_EQUAL(0, uci_config_parse_device_value(DEVICE_PROMISCUOUS, "on", buffer, &len));
        ASSERT_EQUAL(1, (int)len);
        ASSERT_EQUAL(1, buffer[0]);

        len = sizeof(buffer);
        ASSERT_EQUAL(-1, uci_config_parse_device_value(DEVICE_CHANNEL, "invalid", buffer, &len));
        TEST_PASS();
    }
    test_case_end:;
#undef test_case_end

#define test_case_end test_case_end_device_config_length_lookup
    TEST_CASE(device_config_get_length);
    {
        ASSERT_EQUAL(1u, uci_config_get_device_param_length(DEVICE_CHANNEL));
        ASSERT_EQUAL(2u, uci_config_get_device_param_length(DEVICE_PAN_ID));
        ASSERT_EQUAL(8u, uci_config_get_device_param_length(DEVICE_EXTENDED_ADDR));
        ASSERT_EQUAL(0u, uci_config_get_device_param_length((DeviceConfigId)0xEE));
        TEST_PASS();
    }
    test_case_end:;
#undef test_case_end

#define test_case_end test_case_end_2
    // Test device configuration parameter name retrieval
    TEST_CASE(device_config_param_name);
    {
        const char* name = uci_config_get_device_param_name(DEVICE_STATE);
        ASSERT_TRUE(name != NULL);
        ASSERT_STRING_EQUAL("device_state", name);
        TEST_PASS();
    }
    test_case_end:;
#undef test_case_end

#define test_case_end test_case_end_3
    // Test application configuration parameter description retrieval
    TEST_CASE(app_config_param_desc);
    {
        const char* desc = uci_config_get_app_param_desc(DEVICE_TYPE);
        ASSERT_TRUE(desc != NULL);
        // Just check that we get a non-empty description
        ASSERT_TRUE(strlen(desc) > 0);
        TEST_PASS();
    }
    test_case_end:;
#undef test_case_end

#define test_case_end test_case_end_4
    // Test device configuration parameter description retrieval
    TEST_CASE(device_config_param_desc);
    {
        const char* desc = uci_config_get_device_param_desc(DEVICE_STATE);
        ASSERT_TRUE(desc != NULL);
        // Just check that we get a non-empty description
        ASSERT_TRUE(strlen(desc) > 0);
        TEST_PASS();
    }
    test_case_end:;
#undef test_case_end

#define test_case_end test_case_end_5
    // Test application configuration parameter default value retrieval
    TEST_CASE(app_config_param_default);
    {
        uint64_t default_val = uci_config_get_app_param_default(DEVICE_TYPE);
        // DEVICE_TYPE default should be 0x01 (responder)
        ASSERT_UINT64_EQUAL(0x01, default_val);
        TEST_PASS();
    }
    test_case_end:;
#undef test_case_end

#define test_case_end test_case_end_6
    // Test device configuration parameter default value retrieval
    TEST_CASE(device_config_param_default);
    {
        uint64_t default_val = uci_config_get_device_param_default(DEVICE_STATE);
        // DEVICE_STATE default should be 0x01 (ready)
        ASSERT_UINT64_EQUAL(0x01, default_val);
        TEST_PASS();
    }
    test_case_end:;
#undef test_case_end

#define test_case_end test_case_end_7
    // Test handling of invalid application configuration parameter
    TEST_CASE(invalid_app_config_param);
    {
        const char* name = uci_config_get_app_param_name(0x31);
        ASSERT_TRUE(name == NULL);
        TEST_PASS();
    }
    test_case_end:;
#undef test_case_end

#define test_case_end test_case_end_8
    // Test setting and retrieving an application configuration value
    TEST_CASE(app_config_set_and_get_value);
    {
        unsigned char value[2] = {0x12, 0x34};
        ASSERT_EQUAL(0, uci_config_set_app_param(STATIC_STS_IV, value, sizeof(value)));

        unsigned char buffer[4] = {0};
        size_t buffer_len = sizeof(buffer);
        ASSERT_EQUAL(0, uci_config_get_app_param(STATIC_STS_IV, buffer, &buffer_len));
        ASSERT_EQUAL(2, (int)buffer_len);
        ASSERT_EQUAL(0x12, buffer[0]);
        ASSERT_EQUAL(0x34, buffer[1]);
        TEST_PASS();
    }
    test_case_end:;
#undef test_case_end

#define test_case_end test_case_end_9
    // Test retrieving an application configuration with insufficient buffer
    TEST_CASE(app_config_get_insufficient_buffer);
    {
        unsigned char value[2] = {0xAA, 0xBB};
        ASSERT_EQUAL(0, uci_config_set_app_param(DEVICE_MAC_ADDRESS, value, sizeof(value)));

        unsigned char buffer[1] = {0};
        size_t buffer_len = sizeof(buffer);
        ASSERT_EQUAL(-1, uci_config_get_app_param(DEVICE_MAC_ADDRESS, buffer, &buffer_len));
        TEST_PASS();
    }
    test_case_end:;
#undef test_case_end

#define test_case_end test_case_end_10
    // Test parsing application configuration parameter names
    TEST_CASE(app_config_parse_names);
    {
        AppConfigTlvType cfg_id;
        ASSERT_EQUAL(0, uci_config_parse_app_param_name("DEVICE_TYPE", &cfg_id));
        ASSERT_EQUAL(DEVICE_TYPE, cfg_id);
        ASSERT_EQUAL(0, uci_config_parse_app_param_name("device_type", &cfg_id));
        ASSERT_EQUAL(DEVICE_TYPE, cfg_id);
        ASSERT_EQUAL(0, uci_config_parse_app_param_name("0x1B", &cfg_id));
        ASSERT_EQUAL(SLOTS_PER_RR, cfg_id);
        ASSERT_EQUAL(-1, uci_config_parse_app_param_name("unknown_param", &cfg_id));
        TEST_PASS();
    }
    test_case_end:;
#undef test_case_end

#define test_case_end test_case_end_11
    // Test setting and retrieving a device configuration value
    TEST_CASE(device_config_set_and_get_value);
    {
        unsigned char value[1] = {0x01};
        ASSERT_EQUAL(0, uci_config_set_device_param(LOW_POWER_MODE, value, sizeof(value)));

        unsigned char buffer[2] = {0};
        size_t buffer_len = sizeof(buffer);
        ASSERT_EQUAL(0, uci_config_get_device_param(LOW_POWER_MODE, buffer, &buffer_len));
        ASSERT_EQUAL(1, (int)buffer_len);
        ASSERT_EQUAL(0x01, buffer[0]);
        TEST_PASS();
    }
    test_case_end:;
#undef test_case_end

#define test_case_end test_case_end_12
    // Test parsing device configuration parameter names
    TEST_CASE(device_config_parse_names);
    {
        DeviceConfigId cfg_id;
        ASSERT_EQUAL(0, uci_config_parse_device_param_name("DEVICE_STATE", &cfg_id));
        ASSERT_EQUAL(DEVICE_STATE, cfg_id);
        ASSERT_EQUAL(0, uci_config_parse_device_param_name("device_state", &cfg_id));
        ASSERT_EQUAL(DEVICE_STATE, cfg_id);
        ASSERT_EQUAL(0, uci_config_parse_device_param_name("device_channel", &cfg_id));
        ASSERT_EQUAL(DEVICE_CHANNEL, cfg_id);
        ASSERT_EQUAL(0, uci_config_parse_device_param_name("0x01", &cfg_id));
        ASSERT_EQUAL(LOW_POWER_MODE, cfg_id);
        ASSERT_EQUAL(-1, uci_config_parse_device_param_name("invalid_device_param", &cfg_id));
        TEST_PASS();
    }
    test_case_end:;
#undef test_case_end

#define test_case_end test_case_end_device_config_lookup_helper
    TEST_CASE(device_config_lookup_helper);
    {
        DeviceConfigId cfg_id = (DeviceConfigId)0;
        const device_config_param_info_t* info = NULL;

        ASSERT_EQUAL(0, uci_config_lookup_device_param("device_state", &cfg_id, &info));
        ASSERT_EQUAL(DEVICE_STATE, cfg_id);
        ASSERT_TRUE(info != NULL);
        ASSERT_STRING_EQUAL("device_state", info->name);

        cfg_id = (DeviceConfigId)0;
        info = NULL;
        ASSERT_EQUAL(0, uci_config_lookup_device_param("0xA0", &cfg_id, &info));
        ASSERT_EQUAL(DEVICE_CHANNEL, cfg_id);
        ASSERT_TRUE(info != NULL);
        ASSERT_STRING_EQUAL("device_channel", info->name);

        cfg_id = (DeviceConfigId)0;
        info = (const device_config_param_info_t*)1;
        ASSERT_EQUAL(0, uci_config_lookup_device_param("0x10", &cfg_id, &info));
        ASSERT_EQUAL((DeviceConfigId)0x10, cfg_id);
        ASSERT_TRUE(info == NULL);

        TEST_PASS();
    }
    test_case_end:;
#undef test_case_end

#define test_case_end test_case_end_13
    // Test device configuration parameter name retrieval for invalid ID
    TEST_CASE(device_config_param_name_invalid);
    {
        const char* name = uci_config_get_device_param_name(0x10);
        ASSERT_TRUE(name == NULL);
        TEST_PASS();
    }
    test_case_end:;
#undef test_case_end

    TEST_SUITE_END();
}
