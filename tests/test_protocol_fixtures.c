#include "test_runner.h"

#include "../include/uci.h"
#include "../include/uci_packet_utils.h"
#include "../include/uci_response_core.h"
#include <stdlib.h>
#include <string.h>

extern unsigned long long g_fake_timestamp;

static int assert_bytes_equal(const unsigned char* expected,
                              const unsigned char* actual,
                              size_t len) {
    return memcmp(expected, actual, len) == 0;
}

int main(void) {
    TEST_SUITE(protocol_fixtures);

#define test_case_end test_case_end_core_device_info_command
    TEST_CASE(core_device_info_command_fixture);
    {
        size_t packet_len = 0;
        unsigned char* packet = create_get_device_info_packet(&packet_len);
        const unsigned char expected[] = {0x20, 0x02, 0x00, 0x00};

        ASSERT_TRUE(packet != NULL);
        ASSERT_EQUAL((int)sizeof(expected), (int)packet_len);
        ASSERT_TRUE(assert_bytes_equal(expected, packet, sizeof(expected)));

        free(packet);
        TEST_PASS();
    }
    test_case_end:;
#undef test_case_end

#define test_case_end test_case_end_session_init_command
    TEST_CASE(session_init_command_fixture);
    {
        size_t packet_len = 0;
        unsigned char* packet = create_session_init_packet(0x12345678,
                                                           FIRA_RANGING_SESSION,
                                                           &packet_len);
        const unsigned char expected[] = {
            0x21, 0x00, 0x00, 0x05,
            0x78, 0x56, 0x34, 0x12,
            FIRA_RANGING_SESSION
        };

        ASSERT_TRUE(packet != NULL);
        ASSERT_EQUAL((int)sizeof(expected), (int)packet_len);
        ASSERT_TRUE(assert_bytes_equal(expected, packet, sizeof(expected)));

        free(packet);
        TEST_PASS();
    }
    test_case_end:;
#undef test_case_end

#define test_case_end test_case_end_core_device_info_response
    TEST_CASE(core_device_info_response_fixture);
    {
        unsigned char payload[16] = {0};
        const unsigned char expected[] = {
            UCI_STATUS_OK,
            0x00, 0x01,
            0x00, 0x02,
            0x00, 0x02,
            0x00, 0x01,
            0x00
        };
        int payload_len = build_core_device_info_response(payload, sizeof(payload));

        ASSERT_EQUAL((int)sizeof(expected), payload_len);
        ASSERT_TRUE(assert_bytes_equal(expected, payload, sizeof(expected)));

        TEST_PASS();
    }
    test_case_end:;
#undef test_case_end

#define test_case_end test_case_end_core_query_timestamp_response
    TEST_CASE(core_query_timestamp_response_fixture);
    {
        unsigned char payload[16] = {0};
        const unsigned char expected[] = {
            UCI_STATUS_OK,
            0x88, 0x77, 0x66, 0x55,
            0x44, 0x33, 0x22, 0x11
        };
        g_fake_timestamp = 0x1122334455667788ULL;

        int payload_len = build_core_query_timestamp_response(payload, sizeof(payload));
        ASSERT_EQUAL((int)sizeof(expected), payload_len);
        ASSERT_TRUE(assert_bytes_equal(expected, payload, sizeof(expected)));

        TEST_PASS();
    }
    test_case_end:;
#undef test_case_end

#define test_case_end test_case_end_session_status_notification
    TEST_CASE(session_status_notification_fixture);
    {
        const unsigned char payload[] = {
            0x01, 0x00, 0x00, 0x00,
            SESSION_STATE_INIT,
            STATE_CHANGE_WITH_SESSION_MANAGEMENT_COMMANDS
        };
        const unsigned char expected[] = {
            0x61, 0x02, 0x00, 0x06,
            0x01, 0x00, 0x00, 0x00,
            SESSION_STATE_INIT,
            STATE_CHANGE_WITH_SESSION_MANAGEMENT_COMMANDS
        };
        size_t packet_len = 0;
        unsigned char* packet = create_uci_packet(NOTIFICATION,
                                                  COMPLETE,
                                                  SESSION_CONFIG,
                                                  SESSION_STATUS_NTF,
                                                  payload,
                                                  sizeof(payload),
                                                  &packet_len);

        ASSERT_TRUE(packet != NULL);
        ASSERT_EQUAL((int)sizeof(expected), (int)packet_len);
        ASSERT_TRUE(assert_bytes_equal(expected, packet, sizeof(expected)));

        free(packet);
        TEST_PASS();
    }
    test_case_end:;
#undef test_case_end

#define test_case_end test_case_end_session_info_notification
    TEST_CASE(session_info_notification_fixture);
    {
        const unsigned char payload[] = {
            0x00, 0x00, 0x00, 0x00,
            0x02, 0x03, 0x04, 0x05,
            0x06,
            0x07, 0x08, 0x00, 0x00,
            0x0A,
            0x01,
            0x01,
            0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00,
            0x00
        };
        const unsigned char expected[] = {
            0x62, 0x00, 0x00, 0x19,
            0x00, 0x00, 0x00, 0x00,
            0x02, 0x03, 0x04, 0x05,
            0x06,
            0x07, 0x08, 0x00, 0x00,
            0x0A,
            0x01,
            0x01,
            0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00,
            0x00
        };
        size_t packet_len = 0;
        unsigned char* packet = create_uci_packet(NOTIFICATION,
                                                  COMPLETE,
                                                  SESSION_CONTROL,
                                                  SESSION_INFO_NTF,
                                                  payload,
                                                  sizeof(payload),
                                                  &packet_len);

        ASSERT_TRUE(packet != NULL);
        ASSERT_EQUAL((int)sizeof(expected), (int)packet_len);
        ASSERT_TRUE(assert_bytes_equal(expected, packet, sizeof(expected)));

        free(packet);
        TEST_PASS();
    }
    test_case_end:;
#undef test_case_end

    TEST_SUITE_END();
}
