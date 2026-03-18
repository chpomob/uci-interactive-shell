#include "test_runner.h"
#include "../include/uci_cmd_core.h"
#include "../include/uci_config_manager.h"
#include "../include/uci_cmd_session.h"
#include "../include/uci_cmd_session_config.h"
#include "../include/uci_cmd_session_config_ext.h"
#include "../include/uci_functions.h"
#include "../include/uci_packet_utils.h"
#include "../include/uci_pdl.h"
#include "../include/uci.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

typedef struct {
    int called;
    uci_uint8 mt;
    uci_uint8 pbf;
    uci_uint8 gid;
    uci_uint8 oid;
    int payload_len;
    size_t packet_len;
    unsigned char payload[512];
    unsigned char packet[512 + sizeof(struct uci_packet_header)];
} captured_command_t;

typedef struct {
    int called;
    size_t payload_len;
    unsigned char payload[1024];
} captured_data_message_t;

typedef struct {
    int count;
    uci_header_fields_t last_fields;
    size_t payload_len;
    unsigned char payload[512];
} captured_notification_t;

static captured_command_t g_captured_command;
static captured_data_message_t g_captured_data;
static captured_notification_t g_captured_notification;

static void reset_command_capture(void) {
    memset(&g_captured_command, 0, sizeof(g_captured_command));
}

static void reset_data_capture(void) {
    memset(&g_captured_data, 0, sizeof(g_captured_data));
}

static void reset_notification_capture(void) {
    memset(&g_captured_notification, 0, sizeof(g_captured_notification));
}

static size_t capture_output_from_command(int (*command_fn)(const char*, const char*, int),
                                          const char* id_filter,
                                          const char* name_filter,
                                          int detail_full,
                                          char* buffer,
                                          size_t buffer_len) {
    FILE* tmp = tmpfile();
    if (!tmp) {
        return 0;
    }
    int tmp_fd = fileno(tmp);
    int saved_stdout = dup(STDOUT_FILENO);
    if (saved_stdout < 0) {
        fclose(tmp);
        return 0;
    }

    fflush(stdout);
    if (dup2(tmp_fd, STDOUT_FILENO) < 0) {
        close(saved_stdout);
        fclose(tmp);
        return 0;
    }

    command_fn(id_filter, name_filter, detail_full);

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
    return read_bytes;
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

    size_t packet_len = 0;
    unsigned char* packet = create_uci_packet(mt, pbf, gid, oid, payload,
                                              (payload_len > 0) ? (size_t)payload_len : 0,
                                              &packet_len);
    if (packet) {
        g_captured_command.packet_len = packet_len;
        size_t copy_len = packet_len;
        if (copy_len > sizeof(g_captured_command.packet)) {
            copy_len = sizeof(g_captured_command.packet);
        }
        memcpy(g_captured_command.packet, packet, copy_len);
        parse_uci_packet(packet, packet_len);
        free(packet);
    } else {
        g_captured_command.packet_len = 0;
    }

    return 0; // consume the command to avoid hardware/sim side-effects
}

static int data_message_capture_hook(const unsigned char* payload, size_t payload_len) {
    g_captured_data.called++;
    g_captured_data.payload_len = payload_len;
    if (payload && payload_len > 0) {
        size_t copy_len = payload_len;
        if (copy_len > sizeof(g_captured_data.payload)) {
            copy_len = sizeof(g_captured_data.payload);
        }
        memcpy(g_captured_data.payload, payload, copy_len);
    }
    return 1;
}

static void notification_capture_callback(const struct uci_packet_header* header,
                                          const uci_header_fields_t* fields,
                                          const unsigned char* payload,
                                          size_t payload_len) {
    (void)header;
    g_captured_notification.count++;
    if (fields) {
        g_captured_notification.last_fields = *fields;
    }
    g_captured_notification.payload_len = payload_len;
    if (payload && payload_len > 0) {
        size_t copy_len = payload_len;
        if (copy_len > sizeof(g_captured_notification.payload)) {
            copy_len = sizeof(g_captured_notification.payload);
        }
        memcpy(g_captured_notification.payload, payload, copy_len);
    }
}

static int payload_matches(const unsigned char* expected, size_t len) {
    return memcmp(g_captured_command.payload, expected, len) == 0;
}

int main(void) {
    TEST_SUITE(command_handler_validation);

    uci_set_command_capture_hook(command_capture_hook);
    uci_set_data_message_hook(data_message_capture_hook);
    uci_set_notification_callback(notification_capture_callback);

    reset_command_capture();
    reset_data_capture();
    reset_notification_capture();

#define test_case_end test_case_end_core_get_device_info
    TEST_CASE(core_get_device_info);
    {
        reset_command_capture();
        handle_get_device_info_command();

        ASSERT_EQUAL(1, g_captured_command.called);
        ASSERT_EQUAL(COMMAND, g_captured_command.mt);
        ASSERT_EQUAL(COMPLETE, g_captured_command.pbf);
        ASSERT_EQUAL(CORE, g_captured_command.gid);
        ASSERT_EQUAL(CORE_DEVICE_INFO, g_captured_command.oid);
        ASSERT_EQUAL(0, g_captured_command.payload_len);

        TEST_PASS();
        test_case_end:;
    }
#undef test_case_end

#define test_case_end test_case_end_multicast_values
    TEST_CASE(session_multicast_values);
    {
        reset_command_capture();
        int rc = handle_update_multicast_list_command_values(7, "add", 0x4321, 0x01020304);

        ASSERT_EQUAL(0, rc);
        ASSERT_EQUAL(1, g_captured_command.called);
        ASSERT_EQUAL(SESSION_UPDATE_CONTROLLER_MULTICAST_LIST, g_captured_command.oid);
        ASSERT_EQUAL(12, g_captured_command.payload_len);

        const unsigned char expected[] = {
            0x07, 0x00, 0x00, 0x00, // session_id
            0x01,                   // num entries
            MULTICAST_ACTION_ADD_SHORT_KEY,
            0x21, 0x43,             // short address
            0x04, 0x03, 0x02, 0x01  // subsession_id
        };
        ASSERT_TRUE(payload_matches(expected, sizeof(expected)));

        rc = handle_update_multicast_list_command_values(7, "unknown", 0x1234, 0);
        ASSERT_EQUAL(-1, rc);

        TEST_PASS();
        test_case_end:;
    }
#undef test_case_end

#define test_case_end test_case_end_dt_tag_values
    TEST_CASE(session_dt_tag_rounds_values);
    {
        reset_command_capture();
        unsigned char rounds[] = {0x01, 0x02, 0x03};
        int rc = handle_session_update_dt_tag_rounds_command_values(10, rounds, sizeof(rounds));

        ASSERT_EQUAL(0, rc);
        ASSERT_EQUAL(1, g_captured_command.called);
        ASSERT_EQUAL(SESSION_UPDATE_ACTIVE_ROUNDS_DT_TAG, g_captured_command.oid);

        const unsigned char expected[] = {0x0A, 0x00, 0x00, 0x00, 0x03, 0x01, 0x02, 0x03};
        ASSERT_TRUE(payload_matches(expected, sizeof(expected)));

        rc = handle_session_update_dt_tag_rounds_command_values(10, NULL, 1);
        ASSERT_EQUAL(-1, rc);

        TEST_PASS();
        test_case_end:;
    }
#undef test_case_end

#define test_case_end test_case_end_dtp_values
    TEST_CASE(session_dtp_values_handler);
    {
        reset_command_capture();
        unsigned char payload_bytes[] = {0xAA, 0xBB, 0xCC};
        int rc = handle_session_data_transfer_phase_config_command_values(9,
                                                                          1,
                                                                          2,
                                                                          3,
                                                                          payload_bytes,
                                                                          sizeof(payload_bytes));

        ASSERT_EQUAL(0, rc);
        ASSERT_EQUAL(1, g_captured_command.called);
        ASSERT_EQUAL(SESSION_DATA_TRANSFER_PHASE_CONFIG, g_captured_command.oid);

        const unsigned char expected[] = {
            0x09, 0x00, 0x00, 0x00,
            0x01, 0x02, 0x03,
            0xAA, 0xBB, 0xCC
        };
        ASSERT_TRUE(payload_matches(expected, sizeof(expected)));

        rc = handle_session_data_transfer_phase_config_command_values(9, 1, 2, 0, payload_bytes, sizeof(payload_bytes));
        ASSERT_EQUAL(-1, rc);

        rc = handle_session_data_transfer_phase_config_command_values(9, 1, 2, 3, NULL, 0);
        ASSERT_EQUAL(-1, rc);

        TEST_PASS();
        test_case_end:;
    }
#undef test_case_end

#define test_case_end test_case_end_get_app_config_values
    TEST_CASE(session_get_app_config_values);
    {
        reset_command_capture();
        const char* single_name[] = {"device_type"};
        int rc = handle_get_app_config_command_values(8, 1, single_name);

        ASSERT_EQUAL(0, rc);
        ASSERT_EQUAL(1, g_captured_command.called);
        ASSERT_EQUAL(SESSION_GET_APP_CONFIG, g_captured_command.oid);
        ASSERT_EQUAL(6, g_captured_command.payload_len);

        const unsigned char expected_single[] = {0x08, 0x00, 0x00, 0x00, 0x01, 0x00};
        ASSERT_TRUE(payload_matches(expected_single, sizeof(expected_single)));

        reset_command_capture();
        const char* multi_names[] = {"device_type", "multi_node_mode", "device_role"};
        rc = handle_get_app_config_command_values(8, 3, multi_names);

        ASSERT_EQUAL(0, rc);
        ASSERT_EQUAL(1, g_captured_command.called);
        ASSERT_EQUAL(SESSION_GET_APP_CONFIG, g_captured_command.oid);
        ASSERT_EQUAL(8, g_captured_command.payload_len);

        const unsigned char expected_multi[] = {0x08, 0x00, 0x00, 0x00, 0x03, 0x00, 0x03, 0x11};
        ASSERT_TRUE(payload_matches(expected_multi, sizeof(expected_multi)));

        reset_command_capture();
        rc = handle_get_app_config_command_values(8, 0, NULL);

        ASSERT_EQUAL(0, rc);
        ASSERT_EQUAL(1, g_captured_command.called);
        ASSERT_EQUAL(SESSION_GET_APP_CONFIG, g_captured_command.oid);
        ASSERT_EQUAL(5, g_captured_command.payload_len);

        const unsigned char expected_all[] = {0x08, 0x00, 0x00, 0x00, 0x00};
        ASSERT_TRUE(payload_matches(expected_all, sizeof(expected_all)));

        rc = handle_get_app_config_command_values(8, 1, NULL);
        ASSERT_EQUAL(-1, rc);

        TEST_PASS();
        test_case_end:;
    }
#undef test_case_end

#define test_case_end test_case_end_core_device_reset
    TEST_CASE(core_device_reset);
    {
        reset_command_capture();
        handle_device_reset_command();

        ASSERT_EQUAL(1, g_captured_command.called);
        ASSERT_EQUAL(CORE_DEVICE_RESET, g_captured_command.oid);
        ASSERT_EQUAL(1, g_captured_command.payload_len);
        ASSERT_EQUAL(UWBS_RESET, g_captured_command.payload[0]);

        ASSERT_TRUE(g_captured_notification.count >= 1);

        TEST_PASS();
        test_case_end:;
    }
#undef test_case_end

#define test_case_end test_case_end_core_get_caps
    TEST_CASE(core_get_caps_info);
    {
        reset_command_capture();
        handle_get_caps_info_command();

        ASSERT_EQUAL(1, g_captured_command.called);
        ASSERT_EQUAL(CORE_GET_CAPS_INFO, g_captured_command.oid);
        ASSERT_EQUAL(0, g_captured_command.payload_len);

        TEST_PASS();
        test_case_end:;
    }
#undef test_case_end

#define test_case_end test_case_end_set_power_active
    TEST_CASE(core_set_power_active);
    {
        reset_command_capture();
        int rc = handle_set_power_command("active");

        ASSERT_EQUAL(0, rc);
        ASSERT_EQUAL(1, g_captured_command.called);
        ASSERT_EQUAL(CORE_SET_CONFIG, g_captured_command.oid);
        ASSERT_EQUAL(4, g_captured_command.payload_len);

        const unsigned char expected[] = {0x01, DEVICE_STATE, 0x01, DEVICE_STATE_ACTIVE};
        if (!payload_matches(expected, sizeof(expected))) {
            TEST_FAIL("Payload mismatch for active power state");
            goto test_case_end;
        }

        TEST_PASS();
        test_case_end:;
    }
#undef test_case_end

#define test_case_end test_case_end_set_power_sleep
    TEST_CASE(core_set_power_sleep);
    {
        reset_command_capture();
        int rc = handle_set_power_command("sleep");

        ASSERT_EQUAL(0, rc);
        ASSERT_EQUAL(1, g_captured_command.called);
        ASSERT_EQUAL(CORE_DEVICE_SUSPEND, g_captured_command.oid);
        ASSERT_EQUAL(1, g_captured_command.payload_len);
        ASSERT_EQUAL(0x00, g_captured_command.payload[0]);

        TEST_PASS();
        test_case_end:;
    }
#undef test_case_end

#define test_case_end test_case_end_set_power_invalid
    TEST_CASE(core_set_power_invalid);
    {
        reset_command_capture();
        int rc = handle_set_power_command("invalid");

        ASSERT_EQUAL(-1, rc);
        ASSERT_EQUAL(0, g_captured_command.called);

        TEST_PASS();
        test_case_end:;
    }
#undef test_case_end

#define test_case_end test_case_end_device_on_off
    TEST_CASE(core_device_on_off_helpers);
    {
        reset_command_capture();
        handle_device_on_command();
        ASSERT_EQUAL(1, g_captured_command.called);
        ASSERT_EQUAL(CORE_SET_CONFIG, g_captured_command.oid);
        reset_command_capture();

        handle_device_off_command();
        ASSERT_EQUAL(1, g_captured_command.called);
        ASSERT_EQUAL(CORE_SET_CONFIG, g_captured_command.oid);
        const unsigned char expected_ready[] = {0x01, DEVICE_STATE, 0x01, DEVICE_STATE_READY};
        if (!payload_matches(expected_ready, sizeof(expected_ready))) {
            TEST_FAIL("Payload mismatch for device_off");
            goto test_case_end;
        }

        TEST_PASS();
        test_case_end:;
    }
#undef test_case_end

#define test_case_end test_case_end_get_config
    TEST_CASE(core_get_config_device_state);
    {
        reset_command_capture();
        int rc = handle_get_config_command("device_state");

        ASSERT_EQUAL(0, rc);
        ASSERT_EQUAL(1, g_captured_command.called);
        ASSERT_EQUAL(CORE_GET_CONFIG, g_captured_command.oid);
        ASSERT_EQUAL(2, g_captured_command.payload_len);

        const unsigned char expected[] = {0x01, DEVICE_STATE};
        if (!payload_matches(expected, sizeof(expected))) {
            TEST_FAIL("Payload mismatch for get_config device_state");
            goto test_case_end;
        }

        TEST_PASS();
        test_case_end:;
    }
#undef test_case_end

#define test_case_end test_case_end_get_config_invalid
    TEST_CASE(core_get_config_invalid);
    {
        reset_command_capture();
        int rc = handle_get_config_command("unknown_cfg");

        ASSERT_EQUAL(-1, rc);
        ASSERT_EQUAL(0, g_captured_command.called);

        TEST_PASS();
        test_case_end:;
    }
#undef test_case_end

#define test_case_end test_case_end_get_device_state
    TEST_CASE(core_get_device_state_helper);
    {
        reset_command_capture();
        handle_get_device_state_command();

        ASSERT_EQUAL(1, g_captured_command.called);
        ASSERT_EQUAL(CORE_GET_CONFIG, g_captured_command.oid);
        const unsigned char expected[] = {0x01, DEVICE_STATE};
        if (!payload_matches(expected, sizeof(expected))) {
            TEST_FAIL("Payload mismatch for get_device_state helper");
            goto test_case_end;
        }

        TEST_PASS();
        test_case_end:;
    }
#undef test_case_end

#define test_case_end test_case_end_set_device_state_helpers
    TEST_CASE(core_set_device_state_helpers);
    {
        reset_command_capture();
        handle_set_device_active_command();
        ASSERT_EQUAL(1, g_captured_command.called);
        const unsigned char expected_active[] = {0x01, DEVICE_STATE, 0x01, DEVICE_STATE_ACTIVE};
        if (!payload_matches(expected_active, sizeof(expected_active))) {
            TEST_FAIL("Payload mismatch for set_device_active");
            goto test_case_end;
        }

        reset_command_capture();
        handle_set_device_ready_command();
        ASSERT_EQUAL(1, g_captured_command.called);
        const unsigned char expected_ready[] = {0x01, DEVICE_STATE, 0x01, DEVICE_STATE_READY};
        if (!payload_matches(expected_ready, sizeof(expected_ready))) {
            TEST_FAIL("Payload mismatch for set_device_ready");
            goto test_case_end;
        }

        TEST_PASS();
        test_case_end:;
    }
#undef test_case_end

#define test_case_end test_case_end_set_config_device_state
    TEST_CASE(core_set_config_device_state);
    {
        reset_command_capture();
        int rc = handle_set_config_command("device_state", "active");

        ASSERT_EQUAL(0, rc);
        ASSERT_EQUAL(1, g_captured_command.called);
        ASSERT_EQUAL(CORE_SET_CONFIG, g_captured_command.oid);

        const unsigned char expected[] = {0x01, DEVICE_STATE, 0x01, DEVICE_STATE_ACTIVE};
        if (!payload_matches(expected, sizeof(expected))) {
            TEST_FAIL("Payload mismatch for set_config device_state");
            goto test_case_end;
        }

        TEST_PASS();
        test_case_end:;
    }
#undef test_case_end

#define test_case_end test_case_end_set_config_low_power_mode
    TEST_CASE(core_set_config_low_power_mode);
    {
        reset_command_capture();
        int rc = handle_set_config_command("low_power_mode", "on");

        ASSERT_EQUAL(0, rc);
        ASSERT_EQUAL(1, g_captured_command.called);

        const unsigned char expected[] = {0x01, LOW_POWER_MODE, 0x01, 0x01};
        if (!payload_matches(expected, sizeof(expected))) {
            TEST_FAIL("Payload mismatch for set_config low_power_mode");
            goto test_case_end;
        }

        TEST_PASS();
        test_case_end:;
    }
#undef test_case_end

#define test_case_end test_case_end_set_config_device_channel
    TEST_CASE(core_set_config_device_channel);
    {
        reset_command_capture();
        int rc = handle_set_config_command("device_channel", "9");

        ASSERT_EQUAL(0, rc);
        ASSERT_EQUAL(1, g_captured_command.called);

        const unsigned char expected[] = {0x01, DEVICE_CHANNEL, 0x01, 0x09};
        if (!payload_matches(expected, sizeof(expected))) {
            TEST_FAIL("Payload mismatch for set_config device_channel");
            goto test_case_end;
        }

        TEST_PASS();
        test_case_end:;
    }
#undef test_case_end

#define test_case_end test_case_end_set_config_device_pan_id
    TEST_CASE(core_set_config_device_pan_id);
    {
        reset_command_capture();
        int rc = handle_set_config_command("device_pan_id", "0x1234");

        ASSERT_EQUAL(0, rc);
        ASSERT_EQUAL(1, g_captured_command.called);

        const unsigned char expected[] = {0x01, DEVICE_PAN_ID, 0x02, 0x34, 0x12};
        if (!payload_matches(expected, sizeof(expected))) {
            TEST_FAIL("Payload mismatch for set_config device_pan_id");
            goto test_case_end;
        }

        TEST_PASS();
        test_case_end:;
    }
#undef test_case_end

#define test_case_end test_case_end_set_config_device_extended_addr
    TEST_CASE(core_set_config_device_extended_addr);
    {
        reset_command_capture();
        int rc = handle_set_config_command("device_extended_addr", "0x1122334455667788");

        ASSERT_EQUAL(0, rc);
        ASSERT_EQUAL(1, g_captured_command.called);

        const unsigned char expected[] = {
            0x01, DEVICE_EXTENDED_ADDR, 0x08,
            0x88, 0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11
        };
        if (!payload_matches(expected, sizeof(expected))) {
            TEST_FAIL("Payload mismatch for set_config device_extended_addr");
            goto test_case_end;
        }

        TEST_PASS();
        test_case_end:;
    }
#undef test_case_end

#define test_case_end test_case_end_set_config_invalid
    TEST_CASE(core_set_config_invalid_value);
    {
        reset_command_capture();
        int rc = handle_set_config_command("device_state", "unsupported");

        ASSERT_EQUAL(-1, rc);
        ASSERT_EQUAL(0, g_captured_command.called);

        TEST_PASS();
        test_case_end:;
    }
#undef test_case_end

#define test_case_end test_case_end_set_config_invalid_channel_value
    TEST_CASE(core_set_config_invalid_channel_value);
    {
        reset_command_capture();
        int rc = handle_set_config_command("device_channel", "invalid");

        ASSERT_EQUAL(-1, rc);
        ASSERT_EQUAL(0, g_captured_command.called);

        TEST_PASS();
        test_case_end:;
    }
#undef test_case_end

#define test_case_end test_case_end_device_suspend
    TEST_CASE(core_device_suspend_command);
    {
        reset_command_capture();
        handle_device_suspend_command();

        ASSERT_EQUAL(1, g_captured_command.called);
        ASSERT_EQUAL(CORE_DEVICE_SUSPEND, g_captured_command.oid);
        ASSERT_EQUAL(1, g_captured_command.payload_len);
        ASSERT_EQUAL(0x00, g_captured_command.payload[0]);

        TEST_PASS();
        test_case_end:;
    }
#undef test_case_end

#define test_case_end test_case_end_query_timestamp
    TEST_CASE(core_query_timestamp);
    {
        reset_command_capture();
        handle_query_timestamp_command();

        ASSERT_EQUAL(1, g_captured_command.called);
        ASSERT_EQUAL(CORE_QUERY_UWBS_TIMESTAMP, g_captured_command.oid);
        ASSERT_EQUAL(0, g_captured_command.payload_len);

        TEST_PASS();
        test_case_end:;
    }
#undef test_case_end

#define test_case_end test_case_end_session_init
    TEST_CASE(session_init_valid);
    {
        reset_command_capture();
        int rc = handle_session_init_command_values(0x1234, FIRA_RANGING_SESSION);

        ASSERT_EQUAL(0, rc);
        ASSERT_EQUAL(1, g_captured_command.called);
        ASSERT_EQUAL(SESSION_INIT, g_captured_command.oid);
        ASSERT_EQUAL(5, g_captured_command.payload_len);

        const unsigned char expected[] = {0x34, 0x12, 0x00, 0x00, FIRA_RANGING_SESSION};
        if (!payload_matches(expected, sizeof(expected))) {
            TEST_FAIL("Payload mismatch for session_init");
            goto test_case_end;
        }

        TEST_PASS();
        test_case_end:;
    }
#undef test_case_end

#define test_case_end test_case_end_session_init_invalid
    TEST_CASE(session_init_invalid_type);
    {
        reset_command_capture();
        int rc = handle_session_init_command_values(1, (SessionType)0xFF);

        ASSERT_EQUAL(-1, rc);
        ASSERT_EQUAL(0, g_captured_command.called);

        TEST_PASS();
        test_case_end:;
    }
#undef test_case_end

#define test_case_end test_case_end_session_deinit
    TEST_CASE(session_deinit_valid);
    {
        reset_command_capture();
        int rc = handle_session_deinit_command_value(0x12345678);

        ASSERT_EQUAL(0, rc);
        ASSERT_EQUAL(1, g_captured_command.called);
        ASSERT_EQUAL(SESSION_DEINIT, g_captured_command.oid);

        const unsigned char expected[] = {0x78, 0x56, 0x34, 0x12};
        if (!payload_matches(expected, sizeof(expected))) {
            TEST_FAIL("Payload mismatch for session_deinit");
            goto test_case_end;
        }

        TEST_PASS();
        test_case_end:;
    }
#undef test_case_end

#define test_case_end test_case_end_session_start
    TEST_CASE(session_start_valid);
    {
        reset_command_capture();
        int rc = handle_session_start_command_value(7);

        ASSERT_EQUAL(0, rc);
        ASSERT_EQUAL(1, g_captured_command.called);
        ASSERT_EQUAL(SESSION_START, g_captured_command.oid);

        const unsigned char expected[] = {0x07, 0x00, 0x00, 0x00};
        if (!payload_matches(expected, sizeof(expected))) {
            TEST_FAIL("Payload mismatch for session_start");
            goto test_case_end;
        }

        TEST_PASS();
        test_case_end:;
    }
#undef test_case_end

#define test_case_end test_case_end_session_stop
    TEST_CASE(session_stop_valid);
    {
        reset_command_capture();
        int rc = handle_session_stop_command_value(9);

        ASSERT_EQUAL(0, rc);
        ASSERT_EQUAL(1, g_captured_command.called);
        ASSERT_EQUAL(SESSION_STOP, g_captured_command.oid);

        const unsigned char expected[] = {0x09, 0x00, 0x00, 0x00};
        if (!payload_matches(expected, sizeof(expected))) {
            TEST_FAIL("Payload mismatch for session_stop");
            goto test_case_end;
        }

        TEST_PASS();
        test_case_end:;
    }
#undef test_case_end

#define test_case_end test_case_end_session_get_state
    TEST_CASE(session_get_state_valid);
    {
        reset_command_capture();
        int rc = handle_get_session_state_command_value(10);

        ASSERT_EQUAL(0, rc);
        ASSERT_EQUAL(1, g_captured_command.called);
        ASSERT_EQUAL(SESSION_GET_STATE, g_captured_command.oid);

        const unsigned char expected[] = {0x0A, 0x00, 0x00, 0x00};
        if (!payload_matches(expected, sizeof(expected))) {
            TEST_FAIL("Payload mismatch for get_session_state");
            goto test_case_end;
        }

        TEST_PASS();
        test_case_end:;
    }
#undef test_case_end

#define test_case_end test_case_end_session_send_data
    TEST_CASE(session_send_data_valid);
    {
        reset_data_capture();
        reset_notification_capture();
        unsigned char payload_bytes[] = {0xAA, 0xBB, 0xCC};
        int rc = handle_session_send_data_command_values(3,
                                                         0x0011223344556677ULL,
                                                         2,
                                                         payload_bytes,
                                                         sizeof(payload_bytes));
        uci_process_pending_notifications();

        ASSERT_EQUAL(0, rc);
        ASSERT_EQUAL(1, g_captured_data.called);
        ASSERT_TRUE(g_captured_data.payload_len >= UCI_DATA_MESSAGE_SND_HEADER);

        const unsigned char* payload = g_captured_data.payload;
        ASSERT_EQUAL(0x03, payload[0]);
        ASSERT_EQUAL(0x00, payload[1]);
        ASSERT_EQUAL(0x00, payload[2]);
        ASSERT_EQUAL(0x00, payload[3]);

        uint64_t destination = read_u64_le(payload + 4);
        ASSERT_UINT64_EQUAL(0x0011223344556677ULL, destination);

        ASSERT_EQUAL(0x02, payload[12]);
        ASSERT_EQUAL(0x00, payload[13]);

        uint16_t data_len = read_u16_le(payload + 14);
        ASSERT_EQUAL(3, data_len);

        ASSERT_EQUAL(0xAA, payload[16]);
        ASSERT_EQUAL(0xBB, payload[17]);
        ASSERT_EQUAL(0xCC, payload[18]);

        ASSERT_TRUE(g_captured_notification.count >= 1);
        ASSERT_EQUAL(NOTIFICATION, g_captured_notification.last_fields.message_type);
        ASSERT_EQUAL(SESSION_CONTROL, g_captured_notification.last_fields.group_id);
        ASSERT_EQUAL(SESSION_DATA_TRANSFER_STATUS_NTF, g_captured_notification.last_fields.opcode_id);
        ASSERT_EQUAL(8, g_captured_notification.payload_len);

        uint32_t notified_session = read_u32_le(g_captured_notification.payload);
        ASSERT_EQUAL(0x00000003, (int)notified_session);
        ASSERT_EQUAL(0x0002, (int)read_u16_le(g_captured_notification.payload + 4));
        ASSERT_EQUAL(UCI_DATA_TRANSFER_STATUS_ERROR_REJECTED, g_captured_notification.payload[6]);

        TEST_PASS();
        test_case_end:;
    }
#undef test_case_end

#define test_case_end test_case_end_session_send_data_invalid
    TEST_CASE(session_send_data_invalid_payload);
    {
        reset_data_capture();
        int rc = handle_session_send_data_command_values(1,
                                                         0x01,
                                                         1,
                                                         NULL,
                                                         0);

        ASSERT_EQUAL(-1, rc);
        ASSERT_EQUAL(0, g_captured_data.called);

        TEST_PASS();
        test_case_end:;
    }
#undef test_case_end

#define test_case_end test_case_end_session_send_data_values
    TEST_CASE(session_send_data_values_path);
    {
        reset_data_capture();
        reset_notification_capture();
        unsigned char payload_bytes[] = {0xDE, 0xAD};
        int rc = handle_session_send_data_command_values(2,
                                                         0x0102030405060708ULL,
                                                         5,
                                                         payload_bytes,
                                                         sizeof(payload_bytes));
        uci_process_pending_notifications();

        ASSERT_EQUAL(0, rc);
        ASSERT_EQUAL(1, g_captured_data.called);
        ASSERT_TRUE(g_captured_data.payload_len >= UCI_DATA_MESSAGE_SND_HEADER);

        const unsigned char* payload = g_captured_data.payload;
        ASSERT_EQUAL(0x02, payload[0]);
        ASSERT_EQUAL(0x00, payload[1]);
        ASSERT_EQUAL(0x00, payload[2]);

        uint64_t destination = read_u64_le(payload + 4);
        ASSERT_UINT64_EQUAL(0x0102030405060708ULL, destination);

        ASSERT_EQUAL(5, payload[12]);

        uint16_t data_len = read_u16_le(payload + 14);
        ASSERT_EQUAL(2, data_len);
        ASSERT_EQUAL(0xDE, payload[16]);
        ASSERT_EQUAL(0xAD, payload[17]);

        rc = handle_session_send_data_command_values(2,
                                                     0,
                                                     0,
                                                     payload_bytes,
                                                     0);
        ASSERT_EQUAL(-1, rc);

        TEST_PASS();
        test_case_end:;
    }
#undef test_case_end

#define test_case_end test_case_end_logical_link_create_values
    TEST_CASE(session_logical_link_create_values);
    {
        reset_command_capture();
        int rc = handle_session_logical_link_create_command_values(8, 0x12, true, 0x77, true, 0x05);

        ASSERT_EQUAL(0, rc);
        ASSERT_EQUAL(1, g_captured_command.called);
        ASSERT_EQUAL(SESSION_LOGICAL_LINK_CREATE, g_captured_command.oid);
        ASSERT_EQUAL(7, g_captured_command.payload_len);

        const unsigned char expected[] = {0x08, 0x00, 0x00, 0x00, 0x12, 0x77, 0x05};
        ASSERT_TRUE(payload_matches(expected, sizeof(expected)));

        rc = handle_session_logical_link_create_command_values(8, 0x12, true, 0x77, false, 0);
        ASSERT_EQUAL(0, rc);

        TEST_PASS();
        test_case_end:;
    }
#undef test_case_end

#define test_case_end test_case_end_logical_link_close_values
    TEST_CASE(session_logical_link_close_values);
    {
        reset_command_capture();
        int rc = handle_session_logical_link_close_command_value(9, 0x34);

        ASSERT_EQUAL(0, rc);
        ASSERT_EQUAL(1, g_captured_command.called);
        ASSERT_EQUAL(SESSION_LOGICAL_LINK_CLOSE, g_captured_command.oid);
        const unsigned char expected_close[] = {0x09, 0x00, 0x00, 0x00, 0x34};
        ASSERT_TRUE(payload_matches(expected_close, sizeof(expected_close)));

        reset_command_capture();
        rc = handle_session_logical_link_get_param_command_value(9, 0x34);

        ASSERT_EQUAL(0, rc);
        ASSERT_EQUAL(1, g_captured_command.called);
        ASSERT_EQUAL(SESSION_LOGICAL_LINK_GET_PARAM, g_captured_command.oid);

        TEST_PASS();
        test_case_end:;
    }
#undef test_case_end

#define test_case_end test_case_end_hybrid_value
    TEST_CASE(session_hybrid_config_value_handler);
    {
        reset_command_capture();
        const char config_str[] = "AB";
        int rc = handle_session_set_hybrid_controller_config_command_value(4,
                                                                           config_str,
                                                                           strlen(config_str));

        ASSERT_EQUAL(0, rc);
        ASSERT_EQUAL(1, g_captured_command.called);
        ASSERT_EQUAL(SESSION_SET_HYBRID_CONTROLLER_CONFIG, g_captured_command.oid);
        ASSERT_EQUAL(6, g_captured_command.payload_len);
        ASSERT_EQUAL('A', g_captured_command.payload[4]);
        ASSERT_EQUAL('B', g_captured_command.payload[5]);

        rc = handle_session_set_hybrid_controller_config_command_value(4, config_str, sizeof(config_str) + 300);
        ASSERT_EQUAL(-1, rc);

        TEST_PASS();
        test_case_end:;
    }
#undef test_case_end

#define test_case_end test_case_end_show_device_configs_command
    TEST_CASE(core_show_device_configs_command);
    {
        char* argv[] = { "show_device_configs" };
        ASSERT_EQUAL(0, cmd_show_device_configs(1, argv));

        char* argv_full[] = { "show_device_configs", "--full" };
        ASSERT_EQUAL(0, cmd_show_device_configs(2, argv_full));

        char* argv_filter[] = { "show_device_configs", "--filter", "device" };
        ASSERT_EQUAL(0, cmd_show_device_configs(3, argv_filter));

        char* argv_id[] = { "show_device_configs", "--id", "0x00" };
        ASSERT_EQUAL(0, cmd_show_device_configs(3, argv_id));

        char* argv_unknown[] = { "show_device_configs", "--unknown" };
        ASSERT_EQUAL(-1, cmd_show_device_configs(2, argv_unknown));
        TEST_PASS();
    }
    test_case_end:;
#undef test_case_end

#define test_case_end test_case_end_show_app_configs_command
    TEST_CASE(core_show_app_configs_command);
    {
        char* argv[] = { "show_app_configs" };
        ASSERT_EQUAL(0, cmd_show_app_configs(1, argv));

        char* argv_full[] = { "show_app_configs", "--full" };
        ASSERT_EQUAL(0, cmd_show_app_configs(2, argv_full));

        char* argv_filter[] = { "show_app_configs", "--filter", "session" };
        ASSERT_EQUAL(0, cmd_show_app_configs(3, argv_filter));

        char* argv_id[] = { "show_app_configs", "--id", "0x0" };
        ASSERT_EQUAL(0, cmd_show_app_configs(3, argv_id));

        char* argv_bad_id[] = { "show_app_configs", "--id", "not_a_number" };
        ASSERT_EQUAL(-1, cmd_show_app_configs(3, argv_bad_id));

        TEST_PASS();
    }
    test_case_end:;
#undef test_case_end

#define test_case_end test_case_end_show_device_configs_filters
    TEST_CASE(core_show_device_configs_filters);
    {
        char buffer[2048];
        capture_output_from_command(show_device_configs_with_filters, NULL, NULL, 0,
                                    buffer, sizeof(buffer));
        ASSERT_TRUE(strstr(buffer, "device_state") != NULL);

        capture_output_from_command(show_device_configs_with_filters, "0x00", NULL, 1,
                                    buffer, sizeof(buffer));
        ASSERT_TRUE(strstr(buffer, "device_state") != NULL);
        ASSERT_TRUE(strstr(buffer, "Total matches: 1") != NULL);

        capture_output_from_command(show_device_configs_with_filters, "0xFF", NULL, 0,
                                    buffer, sizeof(buffer));
        ASSERT_TRUE(strstr(buffer, "No device configuration matches") != NULL);
        TEST_PASS();
    }
    test_case_end:;
#undef test_case_end

#define test_case_end test_case_end_show_app_configs_filters
    TEST_CASE(core_show_app_configs_filters);
    {
        char buffer[2048];
        capture_output_from_command(show_app_configs_with_filters, NULL, NULL, 0,
                                    buffer, sizeof(buffer));
        ASSERT_TRUE(strstr(buffer, "device_type") != NULL);

        capture_output_from_command(show_app_configs_with_filters, "0x00", NULL, 1,
                                    buffer, sizeof(buffer));
        ASSERT_TRUE(strstr(buffer, "device_type") != NULL);
        ASSERT_TRUE(strstr(buffer, "Total matches: 1") != NULL);

        capture_output_from_command(show_app_configs_with_filters, "0xFF", NULL, 0,
                                    buffer, sizeof(buffer));
        ASSERT_TRUE(strstr(buffer, "No application configuration matches") != NULL);
        TEST_PASS();
    }
    test_case_end:;
#undef test_case_end

    TEST_SUITE_END();
}
