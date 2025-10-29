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

    TEST_CASE(create_device_reset_packet);
    {
        const uint8_t reset_config = UWBS_RESET;

        size_t packet_len = 0;
        unsigned char* packet = create_device_reset_packet(reset_config, &packet_len);

        if (packet == NULL) {
            TEST_FAIL("Packet is null");
            goto test_case_end_device_reset;
        }
        if (packet_len != sizeof(struct uci_packet_header) + 1) {
            TEST_FAIL("Unexpected packet length");
            goto test_case_end_device_reset;
        }

        const unsigned char* payload = packet + sizeof(struct uci_packet_header);
        const int payload_len = packet_len - sizeof(struct uci_packet_header);

        decoded_device_reset_cmd_t decoded_cmd;
        int result = test_decode_device_reset_cmd(payload, payload_len, &decoded_cmd);

        if (result != 0) {
            TEST_FAIL("Failed to decode device reset command");
            goto test_case_end_device_reset;
        }
        if (decoded_cmd.reset_config != reset_config) {
            TEST_FAIL("Decoded reset config does not match");
            goto test_case_end_device_reset;
        }

        free(packet);
        TEST_PASS();
        test_case_end_device_reset:;
    }

    TEST_CASE(create_get_caps_info_packet);
    {
        size_t packet_len = 0;
        unsigned char* packet = create_get_caps_info_packet(&packet_len);

        if (packet == NULL) {
            TEST_FAIL("Packet is null");
            goto test_case_end_get_caps_info;
        }
        if (packet_len != sizeof(struct uci_packet_header)) {
            TEST_FAIL("Unexpected packet length");
            goto test_case_end_get_caps_info;
        }

        struct uci_packet_header* header = (struct uci_packet_header*)packet;
        uci_header_fields_t header_fields;
        uci_extract_header_fields_safe(header, &header_fields);

        if (header_fields.message_type != COMMAND) {
            TEST_FAIL("Decoded message type does not match");
            goto test_case_end_get_caps_info;
        }
        if (header_fields.group_id != CORE) {
            TEST_FAIL("Decoded group ID does not match");
            goto test_case_end_get_caps_info;
        }
        if (header_fields.opcode_id != CORE_GET_CAPS_INFO) {
            TEST_FAIL("Decoded opcode ID does not match");
            goto test_case_end_get_caps_info;
        }

        free(packet);
        TEST_PASS();
        test_case_end_get_caps_info:;
    }

    TEST_CASE(create_get_config_packet);
    {
        const uint8_t num_configs = 2;
        const unsigned char config_ids[] = {DEVICE_STATE, LOW_POWER_MODE};

        size_t packet_len = 0;
        unsigned char* packet = create_get_config_packet(num_configs, config_ids, sizeof(config_ids), &packet_len);

        if (packet == NULL) {
            TEST_FAIL("Packet is null");
            goto test_case_end_get_config;
        }
        if (packet_len != sizeof(struct uci_packet_header) + 1 + sizeof(config_ids)) {
            TEST_FAIL("Unexpected packet length");
            goto test_case_end_get_config;
        }

        const unsigned char* payload = packet + sizeof(struct uci_packet_header);
        const int payload_len = packet_len - sizeof(struct uci_packet_header);

        decoded_get_config_cmd_t decoded_cmd;
        int result = test_decode_get_config_cmd(payload, payload_len, &decoded_cmd);

        if (result != 0) {
            TEST_FAIL("Failed to decode get config command");
            goto test_case_end_get_config;
        }
        if (decoded_cmd.num_configs != num_configs) {
            TEST_FAIL("Decoded num_configs does not match");
            goto test_case_end_get_config;
        }
        if (decoded_cmd.config_ids_len != sizeof(config_ids)) {
            TEST_FAIL("Decoded config_ids length does not match");
            goto test_case_end_get_config;
        }
        if (memcmp(decoded_cmd.config_ids, config_ids, sizeof(config_ids)) != 0) {
            TEST_FAIL("Decoded config IDs do not match");
            goto test_case_end_get_config;
        }

        free(packet);
        TEST_PASS();
        test_case_end_get_config:;
    }

    TEST_CASE(create_set_config_packet);
    {
        const uint8_t num_configs = 1;
        const unsigned char configs[] = {DEVICE_STATE, 0x01, DEVICE_STATE_ACTIVE};

        size_t packet_len = 0;
        unsigned char* packet = create_set_config_packet(num_configs, configs, sizeof(configs), &packet_len);

        if (packet == NULL) {
            TEST_FAIL("Packet is null");
            goto test_case_end_set_config;
        }
        if (packet_len != sizeof(struct uci_packet_header) + 1 + sizeof(configs)) {
            TEST_FAIL("Unexpected packet length");
            goto test_case_end_set_config;
        }

        const unsigned char* payload = packet + sizeof(struct uci_packet_header);
        const int payload_len = packet_len - sizeof(struct uci_packet_header);

        decoded_set_config_cmd_t decoded_cmd;
        int result = test_decode_set_config_cmd(payload, payload_len, &decoded_cmd);

        if (result != 0) {
            TEST_FAIL("Failed to decode set config command");
            goto test_case_end_set_config;
        }
        if (decoded_cmd.num_configs != num_configs) {
            TEST_FAIL("Decoded num_configs does not match");
            goto test_case_end_set_config;
        }
        if (memcmp(decoded_cmd.configs, configs, sizeof(configs)) != 0) {
            TEST_FAIL("Decoded configs do not match");
            goto test_case_end_set_config;
        }

        free(packet);
        TEST_PASS();
        test_case_end_set_config:;
    }

    TEST_SUITE_END();
}
