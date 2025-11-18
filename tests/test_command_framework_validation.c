#include "test_runner.h"
#include "../include/uci_command_framework.h"
#include "../include/uci_pdl.h"
#include <stddef.h>

static int g_handler_calls = 0;

static int dummy_handler(const char* cmd_name,
                         int argc,
                         char** argv,
                         const uci_param_def_t* params,
                         int param_count) {
    (void)cmd_name;
    (void)argc;
    (void)argv;
    (void)params;
    (void)param_count;
    g_handler_calls++;
    return 0;
}

static unsigned int g_captured_session_id = 0;
static unsigned char g_captured_session_type = 0;
static size_t g_captured_hex_len = 0;
static unsigned char g_captured_hex_bytes[8];
static int g_capture_handler_calls = 0;

static int capture_handler(const char* cmd_name,
                           int argc,
                           char** argv,
                           const uci_param_def_t* params,
                           int param_count) {
    (void)cmd_name;
    (void)argc;
    (void)argv;
    (void)params;
    (void)param_count;

    const uci_cmd_parsed_param_t* session_param = uci_cmd_get_parsed_param(0);
    const uci_cmd_parsed_param_t* type_param = uci_cmd_get_parsed_param(1);
    const uci_cmd_parsed_param_t* hex_param = uci_cmd_get_parsed_param(2);

    if (session_param && session_param->present) {
        g_captured_session_id = session_param->value.session_id;
    }
    if (type_param && type_param->present) {
        g_captured_session_type = type_param->value.session_type;
    }
    if (hex_param && hex_param->present) {
        g_captured_hex_len = hex_param->parsed_length;
        size_t copy_len = (g_captured_hex_len < sizeof(g_captured_hex_bytes))
                              ? g_captured_hex_len
                              : sizeof(g_captured_hex_bytes);
        memcpy(g_captured_hex_bytes, hex_param->value.hex_bytes, copy_len);
    }
    g_capture_handler_calls++;
    return 0;
}

int main(void) {
    TEST_SUITE(command_framework_validation);

#define test_case_end test_case_end_hex_byte
    TEST_CASE(hex_byte_validation);
    {
        const uci_param_def_t params[] = {
            { "mt", PARAM_TYPE_HEX_BYTE, PARAM_FLAG_REQUIRED, 0, 0, 0, "hex byte" },
        };
        const uci_command_def_t def = {
            .name = "hw_send",
            .aliases = { NULL },
            .group = CLI_GROUP_GENERAL,
            .flags = CLI_CMD_FLAG_NONE,
            .description = "test hex byte",
            .params = params,
            .param_count = 1,
            .handler = dummy_handler,
        };

        char* argv_valid[] = { "hw_send", "0A" };
        g_handler_calls = 0;
        ASSERT_EQUAL(0, uci_cmd_dispatch(&def, 2, argv_valid));
        ASSERT_EQUAL(1, g_handler_calls);

        char* argv_invalid_char[] = { "hw_send", "GG" };
        g_handler_calls = 0;
        ASSERT_EQUAL(-1, uci_cmd_dispatch(&def, 2, argv_invalid_char));
        ASSERT_EQUAL(0, g_handler_calls);

        char* argv_invalid_range[] = { "hw_send", "1FF" };
        g_handler_calls = 0;
        ASSERT_EQUAL(-1, uci_cmd_dispatch(&def, 2, argv_invalid_range));
        ASSERT_EQUAL(0, g_handler_calls);

        TEST_PASS();
    }
    test_case_end:;
#undef test_case_end

#define test_case_end test_case_end_uint8
    TEST_CASE(uint8_range_validation);
    {
        const uci_param_def_t params[] = {
            { "session_id", PARAM_TYPE_SESSION_ID, PARAM_FLAG_REQUIRED, 0, 0, 0, "session" },
            { "size", PARAM_TYPE_UINT8, PARAM_FLAG_REQUIRED, 0, 0, 255, "size" },
        };
        const uci_command_def_t def = {
            .name = "session_data_transfer_phase_config",
            .aliases = { NULL },
            .group = CLI_GROUP_SESSION,
            .flags = CLI_CMD_FLAG_NONE,
            .description = "validate size range",
            .params = params,
            .param_count = 2,
            .handler = dummy_handler,
        };

        char* argv_valid[] = { "session_data_transfer_phase_config", "1", "64" };
        g_handler_calls = 0;
        ASSERT_EQUAL(0, uci_cmd_dispatch(&def, 3, argv_valid));
        ASSERT_EQUAL(1, g_handler_calls);

        char* argv_size_too_big[] = { "session_data_transfer_phase_config", "1", "300" };
        g_handler_calls = 0;
        ASSERT_EQUAL(-1, uci_cmd_dispatch(&def, 3, argv_size_too_big));
        ASSERT_EQUAL(0, g_handler_calls);

        char* argv_session_too_big[] = { "session_data_transfer_phase_config", "4294967296", "10" };
        g_handler_calls = 0;
        ASSERT_EQUAL(-1, uci_cmd_dispatch(&def, 3, argv_session_too_big));
        ASSERT_EQUAL(0, g_handler_calls);

        TEST_PASS();
    }
    test_case_end:;
#undef test_case_end

#define test_case_end test_case_end_send_data
    TEST_CASE(session_send_data_validation);
    {
        const uci_param_def_t params[] = {
            { "session_id", PARAM_TYPE_SESSION_ID, PARAM_FLAG_REQUIRED, 0, 0, 0, "session" },
            { "destination", PARAM_TYPE_UINT64, PARAM_FLAG_REQUIRED, 0, 0, 0xFFFFFFFFFFFFFFFFULL, "destination" },
            { "sequence", PARAM_TYPE_UINT16, PARAM_FLAG_REQUIRED, 0, 0, 0xFFFF, "sequence" },
            { "payload", PARAM_TYPE_HEX_STRING, PARAM_FLAG_REQUIRED, 512, 0, 0, "payload" },
        };
        const uci_command_def_t def = {
            .name = "session_send_data",
            .aliases = { NULL },
            .group = CLI_GROUP_SESSION,
            .flags = CLI_CMD_FLAG_NONE,
            .description = "validate session_send_data arguments",
            .params = params,
            .param_count = 4,
            .handler = dummy_handler,
        };

        char* argv_valid[] = { "session_send_data", "1", "0x11223344", "15", "AABB" };
        g_handler_calls = 0;
        ASSERT_EQUAL(0, uci_cmd_dispatch(&def, 5, argv_valid));
        ASSERT_EQUAL(1, g_handler_calls);

        char* argv_bad_dest[] = { "session_send_data", "1", "not_hex", "15", "AABB" };
        g_handler_calls = 0;
        ASSERT_EQUAL(-1, uci_cmd_dispatch(&def, 5, argv_bad_dest));
        ASSERT_EQUAL(0, g_handler_calls);

        char* argv_bad_payload[] = { "session_send_data", "1", "0x12", "15", "ABC" };
        g_handler_calls = 0;
        ASSERT_EQUAL(-1, uci_cmd_dispatch(&def, 5, argv_bad_payload));
        ASSERT_EQUAL(0, g_handler_calls);

        TEST_PASS();
    }
    test_case_end:;
#undef test_case_end

#define test_case_end test_case_end_multicast
    TEST_CASE(multicast_validation);
    {
        const uci_param_def_t params[] = {
            { "session_id", PARAM_TYPE_SESSION_ID, PARAM_FLAG_REQUIRED, 0, 0, 0, "session" },
            { "action", PARAM_TYPE_STRING, PARAM_FLAG_REQUIRED, 16, 0, 0, "action" },
            { "short_address", PARAM_TYPE_UINT16, PARAM_FLAG_REQUIRED, 0, 0, 0xFFFF, "short_address" },
            { "subsession_id", PARAM_TYPE_UINT32, PARAM_FLAG_REQUIRED, 0, 0, 0xFFFFFFFFu, "subsession_id" },
        };
        const uci_command_def_t def = {
            .name = "session_update_multicast_list",
            .aliases = { NULL },
            .group = CLI_GROUP_SESSION_CONFIG,
            .flags = CLI_CMD_FLAG_NONE,
            .description = "validate multicast args",
            .params = params,
            .param_count = 4,
            .handler = dummy_handler,
        };

        char* argv_valid[] = { "session_update_multicast_list", "1", "add", "0x12", "0x20" };
        g_handler_calls = 0;
        ASSERT_EQUAL(0, uci_cmd_dispatch(&def, 5, argv_valid));
        ASSERT_EQUAL(1, g_handler_calls);

        char* argv_bad_short[] = { "session_update_multicast_list", "1", "add", "XYZ", "0x20" };
        g_handler_calls = 0;
        ASSERT_EQUAL(-1, uci_cmd_dispatch(&def, 5, argv_bad_short));
        ASSERT_EQUAL(0, g_handler_calls);

        TEST_PASS();
    }
    test_case_end:;
#undef test_case_end

#define test_case_end test_case_end_alias_lookup
    TEST_CASE(alias_lookup);
    {
        const uci_command_def_t* primary = uci_cmd_framework_find("get_device_info");
        const uci_command_def_t* alias = uci_cmd_framework_find("device_info");
        ASSERT_TRUE(primary != NULL);
        ASSERT_TRUE(alias != NULL);
        ASSERT_TRUE(primary == alias);
        TEST_PASS();
    }
    test_case_end:;
#undef test_case_end

#define test_case_end test_case_end_parsed_storage
    TEST_CASE(parsed_parameter_storage);
    {
        const uci_param_def_t params[] = {
            { "session_id", PARAM_TYPE_SESSION_ID, PARAM_FLAG_REQUIRED, 0, 0, 0, "session" },
            { "session_type", PARAM_TYPE_SESSION_TYPE, PARAM_FLAG_REQUIRED, 0, 0, 0, "type" },
            { "payload", PARAM_TYPE_HEX_STRING, PARAM_FLAG_REQUIRED, 8, 0, 0, "hex payload" },
        };
        const uci_command_def_t def = {
            .name = "typed_demo",
            .aliases = { NULL },
            .group = CLI_GROUP_SESSION,
            .flags = CLI_CMD_FLAG_NONE,
            .description = "verify parsed storage",
            .params = params,
            .param_count = 3,
            .handler = capture_handler,
        };

        char* argv_valid[] = { "typed_demo", "7", "fira_ranging", "AABBCCDD" };
        g_capture_handler_calls = 0;
        g_captured_hex_len = 0;
        ASSERT_EQUAL(0, uci_cmd_dispatch(&def, 4, argv_valid));
        ASSERT_EQUAL(1, g_capture_handler_calls);
        ASSERT_EQUAL(7, (int)g_captured_session_id);
        ASSERT_EQUAL((int)FIRA_RANGING_SESSION, (int)g_captured_session_type);
        ASSERT_EQUAL(4, (int)g_captured_hex_len);
        ASSERT_EQUAL(0xAA, g_captured_hex_bytes[0]);
        ASSERT_EQUAL(0xBB, g_captured_hex_bytes[1]);
        ASSERT_EQUAL(0xCC, g_captured_hex_bytes[2]);
        ASSERT_EQUAL(0xDD, g_captured_hex_bytes[3]);

        TEST_PASS();
    }
    test_case_end:;
#undef test_case_end

    TEST_SUITE_END();
}
