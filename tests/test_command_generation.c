#include "test_runner.h"
#include "test_helpers.h"
#include "../include/uci_packet_utils.h"
#include "../include/uci.h"
#include <stdlib.h>

int main() {
    TEST_SUITE(command_generation);

    TEST_CASE(create_session_init_packet);
    {
        const uint32_t session_id = 0x12345678;
        const uint8_t session_type = FIRA_RANGING_SESSION;

        size_t packet_len = 0;
        unsigned char* packet = create_session_init_packet(session_id, session_type, &packet_len);

        if (packet == NULL) {
            TEST_FAIL("Packet is null");
            goto test_case_end;
        }
        if (packet_len != sizeof(struct uci_packet_header) + 5) {
            TEST_FAIL("Unexpected packet length");
            goto test_case_end;
        }

        // Skip the header for decoding the command payload
        const unsigned char* payload = packet + sizeof(struct uci_packet_header);
        const int payload_len = packet_len - sizeof(struct uci_packet_header);

        decoded_session_init_cmd_t decoded_cmd;
        int result = test_decode_session_init_cmd(payload, payload_len, &decoded_cmd);

        if (result != 0) {
            TEST_FAIL("Failed to decode session init command");
            goto test_case_end;
        }
        if (decoded_cmd.session_id != session_id) {
            TEST_FAIL("Decoded session ID does not match");
            goto test_case_end;
        }
        if (decoded_cmd.session_type != session_type) {
            TEST_FAIL("Decoded session type does not match");
            goto test_case_end;
        }

        free(packet);
        TEST_PASS();
        test_case_end:;
    }

    TEST_CASE(create_session_deinit_packet);
    {
        const uint32_t session_id = 0x87654321;

        size_t packet_len = 0;
        unsigned char* packet = create_session_deinit_packet(session_id, &packet_len);

        if (packet == NULL) {
            TEST_FAIL("Packet is null");
            goto test_case_end_deinit;
        }
        if (packet_len != sizeof(struct uci_packet_header) + 4) {
            TEST_FAIL("Unexpected packet length");
            goto test_case_end_deinit;
        }

        const unsigned char* payload = packet + sizeof(struct uci_packet_header);
        const int payload_len = packet_len - sizeof(struct uci_packet_header);

        decoded_session_deinit_cmd_t decoded_cmd;
        int result = test_decode_session_deinit_cmd(payload, payload_len, &decoded_cmd);

        if (result != 0) {
            TEST_FAIL("Failed to decode session deinit command");
            goto test_case_end_deinit;
        }
        if (decoded_cmd.session_id != session_id) {
            TEST_FAIL("Decoded session ID does not match");
            goto test_case_end_deinit;
        }

        free(packet);
        TEST_PASS();
        test_case_end_deinit:;
    }

    TEST_CASE(create_session_start_packet);
    {
        const uint32_t session_id = 0x11223344;

        size_t packet_len = 0;
        unsigned char* packet = create_session_start_packet(session_id, &packet_len);

        if (packet == NULL) {
            TEST_FAIL("Packet is null");
            goto test_case_end_start;
        }
        if (packet_len != sizeof(struct uci_packet_header) + 4) {
            TEST_FAIL("Unexpected packet length");
            goto test_case_end_start;
        }

        const unsigned char* payload = packet + sizeof(struct uci_packet_header);
        const int payload_len = packet_len - sizeof(struct uci_packet_header);

        decoded_session_start_cmd_t decoded_cmd;
        int result = test_decode_session_start_cmd(payload, payload_len, &decoded_cmd);

        if (result != 0) {
            TEST_FAIL("Failed to decode session start command");
            goto test_case_end_start;
        }
        if (decoded_cmd.session_id != session_id) {
            TEST_FAIL("Decoded session ID does not match");
            goto test_case_end_start;
        }

        free(packet);
        TEST_PASS();
        test_case_end_start:;
    }

    TEST_CASE(create_session_stop_packet);
    {
        const uint32_t session_id = 0x55667788;

        size_t packet_len = 0;
        unsigned char* packet = create_session_stop_packet(session_id, &packet_len);

        if (packet == NULL) {
            TEST_FAIL("Packet is null");
            goto test_case_end_stop;
        }
        if (packet_len != sizeof(struct uci_packet_header) + 4) {
            TEST_FAIL("Unexpected packet length");
            goto test_case_end_stop;
        }

        const unsigned char* payload = packet + sizeof(struct uci_packet_header);
        const int payload_len = packet_len - sizeof(struct uci_packet_header);

        decoded_session_stop_cmd_t decoded_cmd;
        int result = test_decode_session_stop_cmd(payload, payload_len, &decoded_cmd);

        if (result != 0) {
            TEST_FAIL("Failed to decode session stop command");
            goto test_case_end_stop;
        }
        if (decoded_cmd.session_id != session_id) {
            TEST_FAIL("Decoded session ID does not match");
            goto test_case_end_stop;
        }

        free(packet);
        TEST_PASS();
        test_case_end_stop:;
    }

    TEST_CASE(create_get_session_state_packet);
    {
        const uint32_t session_id = 0x99AABBCC;

        size_t packet_len = 0;
        unsigned char* packet = create_get_session_state_packet(session_id, &packet_len);

        if (packet == NULL) {
            TEST_FAIL("Packet is null");
            goto test_case_end_get_state;
        }
        if (packet_len != sizeof(struct uci_packet_header) + 4) {
            TEST_FAIL("Unexpected packet length");
            goto test_case_end_get_state;
        }

        const unsigned char* payload = packet + sizeof(struct uci_packet_header);
        const int payload_len = packet_len - sizeof(struct uci_packet_header);

        decoded_get_session_state_cmd_t decoded_cmd;
        int result = test_decode_get_session_state_cmd(payload, payload_len, &decoded_cmd);

        if (result != 0) {
            TEST_FAIL("Failed to decode get session state command");
            goto test_case_end_get_state;
        }
        if (decoded_cmd.session_id != session_id) {
            TEST_FAIL("Decoded session ID does not match");
            goto test_case_end_get_state;
        }

        free(packet);
        TEST_PASS();
        test_case_end_get_state:;
    }

    TEST_SUITE_END();
}
