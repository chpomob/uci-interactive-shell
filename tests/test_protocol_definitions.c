#include "test_runner.h"

#include "../include/uci.h"
#include "../include/uci_cli.h"
#include "../include/uci_command_framework.h"
#include "../include/uci_config_manager.h"
#include "../include/uci_functions.h"
#include "../include/uci_packet_utils.h"
#include "../include/uci_response_core.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct {
    int called;
    uci_uint8 mt;
    uci_uint8 pbf;
    uci_uint8 gid;
    uci_uint8 oid;
    int payload_len;
    unsigned char payload[512];
} captured_command_t;

static captured_command_t g_captured_command;

static void reset_command_capture(void) {
    memset(&g_captured_command, 0, sizeof(g_captured_command));
}

static int command_capture_hook(uci_uint8 mt,
                                uci_uint8 pbf,
                                uci_uint8 gid,
                                uci_uint8 oid,
                                const uci_uint8* payload,
                                int payload_len) {
    g_captured_command.called++;
    g_captured_command.mt = mt;
    g_captured_command.pbf = pbf;
    g_captured_command.gid = gid;
    g_captured_command.oid = oid;
    g_captured_command.payload_len = payload_len;

    if (payload && payload_len > 0) {
        size_t copy_len = (size_t)payload_len;
        if (copy_len > sizeof(g_captured_command.payload)) {
            copy_len = sizeof(g_captured_command.payload);
        }
        memcpy(g_captured_command.payload, payload, copy_len);
    }

    return 0;
}

static int capture_dispatch_output(const uci_command_def_t* def,
                                   int argc,
                                   char** argv,
                                   char* buffer,
                                   size_t buffer_len) {
    FILE* tmp = tmpfile();
    if (!tmp) {
        return -1;
    }

    int tmp_fd = fileno(tmp);
    int saved_stdout = dup(STDOUT_FILENO);
    if (saved_stdout < 0) {
        fclose(tmp);
        return -1;
    }

    fflush(stdout);
    if (dup2(tmp_fd, STDOUT_FILENO) < 0) {
        close(saved_stdout);
        fclose(tmp);
        return -1;
    }

    int rc = uci_cmd_dispatch(def, argc, argv);

    fflush(stdout);
    fflush(tmp);
    long size = ftell(tmp);
    if (size < 0) {
        size = 0;
    }
    rewind(tmp);

    size_t to_read = (size_t)((size < (long)(buffer_len - 1)) ? size : (long)(buffer_len - 1));
    size_t read_bytes = fread(buffer, 1, to_read, tmp);
    buffer[read_bytes] = '\0';

    dup2(saved_stdout, STDOUT_FILENO);
    close(saved_stdout);
    fclose(tmp);
    return rc;
}

int main(void) {
    TEST_SUITE(protocol_definitions);

    uci_config_init();
    uci_set_command_capture_hook(command_capture_hook);

#define test_case_end test_case_end_authoritative_constants
    TEST_CASE(authoritative_constants);
    {
        ASSERT_EQUAL(COMMAND, 0x01);
        ASSERT_EQUAL(RESPONSE, 0x02);
        ASSERT_EQUAL(NOTIFICATION, 0x03);
        ASSERT_EQUAL(CORE, 0x00);
        ASSERT_EQUAL(SESSION_CONFIG, 0x01);
        ASSERT_EQUAL(SESSION_CONTROL, 0x02);
        ASSERT_EQUAL(QORVO_EXT2, 0x0B);
        ASSERT_EQUAL(ANDROID, 0x0C);
        ASSERT_EQUAL(TEST, 0x0D);
        ASSERT_EQUAL(CORE_DEVICE_INFO, 0x02);
        ASSERT_EQUAL(CORE_GET_CAPS_INFO, 0x03);
        ASSERT_EQUAL(CORE_SET_CONFIG, 0x04);
        ASSERT_EQUAL(CORE_GET_CONFIG, 0x05);
        ASSERT_EQUAL(SESSION_INIT, 0x00);
        ASSERT_EQUAL(SESSION_UPDATE_CONTROLLER_MULTICAST_LIST, 0x07);
        ASSERT_EQUAL(SESSION_DATA_TRANSFER_PHASE_CONFIG, 0x0E);
        ASSERT_EQUAL(SESSION_START, 0x00);
        ASSERT_EQUAL(SESSION_STOP, 0x01);
        ASSERT_EQUAL(SESSION_INFO_NTF, 0x00);
        ASSERT_EQUAL(SESSION_INFO_NTF_OPCODE, SESSION_INFO_NTF);
        ASSERT_EQUAL(QORVO_FIRA_RANGE_DIAGNOSTICS, 0x03);
        ASSERT_EQUAL(QORVO_CORE_DEVICE_BOOT, 0x31);
        ASSERT_EQUAL(ANDROID_FIRA_RANGE_DIAGNOSTICS, 0x02);

        TEST_PASS();
    }
    test_case_end:;
#undef test_case_end

#define test_case_end test_case_end_command_metadata
    TEST_CASE(command_metadata_matches_protocol_defs);
    {
        const uci_command_def_t* get_device_info = cli_find_command("get_device_info");
        const uci_command_def_t* get_device_info_alias = cli_find_command("device_info");
        const uci_command_def_t* set_power = cli_find_command("set_power");
        const uci_command_def_t* show_device_configs = cli_find_command("show_device_configs");
        const uci_command_def_t* session_start = cli_find_command("session_start");

        ASSERT_TRUE(get_device_info != NULL);
        ASSERT_TRUE(get_device_info_alias == get_device_info);
        ASSERT_TRUE(set_power != NULL);
        ASSERT_TRUE(show_device_configs != NULL);
        ASSERT_TRUE(session_start != NULL);
        ASSERT_EQUAL(CLI_GROUP_DEVICE, get_device_info->group);
        ASSERT_EQUAL(CLI_GROUP_DEVICE, set_power->group);
        ASSERT_EQUAL(CLI_GROUP_DEVICE, show_device_configs->group);
        ASSERT_EQUAL(CLI_GROUP_SESSION, session_start->group);

        TEST_PASS();
    }
    test_case_end:;
#undef test_case_end

#define test_case_end test_case_end_session_type_lookup
    TEST_CASE(session_type_lookup_uses_shared_definitions);
    {
        ASSERT_STRING_EQUAL("FIRA_RANGING_SESSION", uci_session_type_to_string(FIRA_RANGING_SESSION));
        ASSERT_STRING_EQUAL("FIRA_RANGING_WITH_DATA_PHASE", uci_session_type_to_string(FIRA_RANGING_WITH_DATA_PHASE));
        ASSERT_STRING_EQUAL("CCC_RANGING_SESSION", uci_session_type_to_string(CCC_RANGING_SESSION));
        ASSERT_STRING_EQUAL("DEVICE_TEST_MODE", uci_session_type_to_string(DEVICE_TEST_MODE));
        ASSERT_STRING_EQUAL("UNKNOWN", uci_session_type_to_string(0x7F));

        TEST_PASS();
    }
    test_case_end:;
#undef test_case_end

#define test_case_end test_case_end_core_device_info_defaults
    TEST_CASE(core_device_info_defaults_are_explicit);
    {
        unsigned char payload[16] = {0};
        int len = build_core_device_info_response(payload, sizeof(payload));

        ASSERT_EQUAL(10, len);
        ASSERT_EQUAL(UCI_STATUS_OK, payload[0]);
        ASSERT_EQUAL(0x0100, (int)read_u16_le(&payload[1]));
        ASSERT_EQUAL(0x0200, (int)read_u16_le(&payload[3]));
        ASSERT_EQUAL(0x0200, (int)read_u16_le(&payload[5]));
        ASSERT_EQUAL(0x0100, (int)read_u16_le(&payload[7]));
        ASSERT_EQUAL(0x00, payload[9]);

        TEST_PASS();
    }
    test_case_end:;
#undef test_case_end

#define test_case_end test_case_end_typed_set_power_suspend
    TEST_CASE(typed_set_power_dispatch_uses_validated_suspend_path);
    {
        const uci_command_def_t* def = cli_find_command("set_power");
        char* argv[] = { "set_power", "sleep" };

        ASSERT_TRUE(def != NULL);
        reset_command_capture();
        ASSERT_EQUAL(0, uci_cmd_dispatch(def, 2, argv));
        ASSERT_EQUAL(1, g_captured_command.called);
        ASSERT_EQUAL(COMMAND, g_captured_command.mt);
        ASSERT_EQUAL(CORE, g_captured_command.gid);
        ASSERT_EQUAL(CORE_DEVICE_SUSPEND, g_captured_command.oid);
        ASSERT_EQUAL(1, g_captured_command.payload_len);
        ASSERT_EQUAL(0x00, g_captured_command.payload[0]);

        TEST_PASS();
    }
    test_case_end:;
#undef test_case_end

#define test_case_end test_case_end_typed_config_dispatch
    TEST_CASE(typed_config_dispatch_uses_validated_params);
    {
        const uci_command_def_t* get_config_def = cli_find_command("get_config");
        const uci_command_def_t* set_config_def = cli_find_command("set_config");
        char* get_argv[] = { "get_config", "device_state" };
        char* set_argv[] = { "set_config", "device_state", "active" };

        ASSERT_TRUE(get_config_def != NULL);
        ASSERT_TRUE(set_config_def != NULL);

        reset_command_capture();
        ASSERT_EQUAL(0, uci_cmd_dispatch(get_config_def, 2, get_argv));
        ASSERT_EQUAL(1, g_captured_command.called);
        ASSERT_EQUAL(CORE_GET_CONFIG, g_captured_command.oid);
        ASSERT_EQUAL(2, g_captured_command.payload_len);
        ASSERT_EQUAL(0x01, g_captured_command.payload[0]);
        ASSERT_EQUAL(DEVICE_STATE, g_captured_command.payload[1]);

        reset_command_capture();
        ASSERT_EQUAL(0, uci_cmd_dispatch(set_config_def, 3, set_argv));
        ASSERT_EQUAL(1, g_captured_command.called);
        ASSERT_EQUAL(CORE_SET_CONFIG, g_captured_command.oid);
        ASSERT_EQUAL(4, g_captured_command.payload_len);
        ASSERT_EQUAL(0x01, g_captured_command.payload[0]);
        ASSERT_EQUAL(DEVICE_STATE, g_captured_command.payload[1]);
        ASSERT_EQUAL(0x01, g_captured_command.payload[2]);
        ASSERT_EQUAL(DEVICE_STATE_ACTIVE, g_captured_command.payload[3]);

        TEST_PASS();
    }
    test_case_end:;
#undef test_case_end

#define test_case_end test_case_end_validate_arguments
    TEST_CASE(validate_arguments_command_uses_parsed_session_id);
    {
        const uci_command_def_t* def = cli_find_command("validate_arguments");
        char* argv[] = { "validate_arguments", "42", "AABB", "7" };
        char output[256];

        ASSERT_TRUE(def != NULL);
        ASSERT_EQUAL(0, capture_dispatch_output(def, 4, argv, output, sizeof(output)));
        ASSERT_TRUE(strstr(output, "Arguments validated successfully") != NULL);
        ASSERT_TRUE(strstr(output, "Session ID: 7") != NULL);

        TEST_PASS();
    }
    test_case_end:;
#undef test_case_end

    TEST_SUITE_END();
}
