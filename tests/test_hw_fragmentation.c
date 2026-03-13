#include "../tests/test_runner.h"
#include "../include/uci.h"
#include "../include/uci_hw_interface.h"
#include "../include/uci_hw_interface_internal.h"
#include "../include/uci_packet_utils.h"

#include <stdlib.h>
#include <string.h>

int main(void) {
    TEST_SUITE(hw_fragmentation);

#define test_case_end test_case_end_data_send_fragmentation
    TEST_CASE(data_send_fragmentation_matches_cherry_style_chunks);
    {
        unsigned char payload[300];
        unsigned char first_packet[sizeof(struct uci_packet_header) + 255];
        unsigned char second_packet[sizeof(struct uci_packet_header) + 64];
        size_t packet_len = 0;
        size_t first_len = 0;
        size_t second_len = 0;
        size_t next_offset = 0;
        unsigned char* packet = NULL;

        for (size_t i = 0; i < sizeof(payload); ++i) {
            payload[i] = (unsigned char)i;
        }

        packet = create_uci_packet(DATA,
                                   COMPLETE,
                                   DATA_PACKET_FORMAT_SEND,
                                   0x00,
                                   payload,
                                   sizeof(payload),
                                   &packet_len);
        if (!packet) {
            TEST_FAIL("Failed to create large DATA packet");
            goto test_case_end;
        }

        if (uci_hw_interface_build_fragment_for_test(packet,
                                                     packet_len,
                                                     0,
                                                     first_packet,
                                                     sizeof(first_packet),
                                                     &first_len,
                                                     &next_offset) != 0) {
            free(packet);
            TEST_FAIL("Failed to build first outbound DATA fragment");
            goto test_case_end;
        }

        if (uci_hw_interface_build_fragment_for_test(packet,
                                                     packet_len,
                                                     next_offset,
                                                     second_packet,
                                                     sizeof(second_packet),
                                                     &second_len,
                                                     &next_offset) != 0) {
            free(packet);
            TEST_FAIL("Failed to build second outbound DATA fragment");
            goto test_case_end;
        }

        ASSERT_EQUAL(sizeof(struct uci_packet_header) + 255, first_len);
        ASSERT_EQUAL(sizeof(struct uci_packet_header) + 45, second_len);
        ASSERT_EQUAL(DATA, get_mt((const struct uci_packet_header*)first_packet));
        ASSERT_EQUAL(NOT_COMPLETE, get_pbf((const struct uci_packet_header*)first_packet));
        ASSERT_EQUAL(DATA, get_mt((const struct uci_packet_header*)second_packet));
        ASSERT_EQUAL(COMPLETE, get_pbf((const struct uci_packet_header*)second_packet));
        ASSERT_EQUAL(255, (int)uci_get_payload_length_from_header_bytes(
                              get_mt((const struct uci_packet_header*)first_packet),
                              first_packet[2], first_packet[3]));
        ASSERT_EQUAL(45, (int)uci_get_payload_length_from_header_bytes(
                             get_mt((const struct uci_packet_header*)second_packet),
                             second_packet[2], second_packet[3]));
        ASSERT_TRUE(memcmp(first_packet + sizeof(struct uci_packet_header), payload, 255) == 0);
        ASSERT_TRUE(memcmp(second_packet + sizeof(struct uci_packet_header), payload + 255, 45) == 0);

        free(packet);
        TEST_PASS();
        test_case_end:;
    }
#undef test_case_end

#define test_case_end test_case_end_data_receive_passthrough
    TEST_CASE(data_receive_passthrough_does_not_reassemble_fragments);
    {
        unsigned char payload[300];
        unsigned char* first_fragment = NULL;
        unsigned char* second_fragment = NULL;
        unsigned char buffer[sizeof(struct uci_packet_header) + 255];
        size_t first_len = 0;
        size_t second_len = 0;
        int received_len = 0;

        for (size_t i = 0; i < sizeof(payload); ++i) {
            payload[i] = (unsigned char)(0xA0 + (i & 0x0F));
        }
        uci_hw_interface_reset_transport_state_for_test();

        first_fragment = create_uci_packet(DATA,
                                           NOT_COMPLETE,
                                           DATA_PACKET_FORMAT_SEND,
                                           0x00,
                                           payload,
                                           255,
                                           &first_len);
        second_fragment = create_uci_packet(DATA,
                                            COMPLETE,
                                            DATA_PACKET_FORMAT_SEND,
                                            0x00,
                                            payload + 255,
                                            45,
                                            &second_len);
        if (!first_fragment || !second_fragment) {
            free(first_fragment);
            free(second_fragment);
            TEST_FAIL("Failed to create incoming DATA fragments");
            goto test_case_end;
        }

        received_len = uci_hw_interface_process_fragment_for_test(first_fragment,
                                                                  first_len,
                                                                  buffer,
                                                                  sizeof(buffer));
        if (received_len <= 0) {
            free(first_fragment);
            free(second_fragment);
            TEST_FAIL("Hardware interface did not return the first DATA fragment");
            goto test_case_end;
        }

        ASSERT_EQUAL((int)first_len, received_len);
        ASSERT_TRUE(memcmp(buffer, first_fragment, first_len) == 0);

        memset(buffer, 0, sizeof(buffer));
        received_len = uci_hw_interface_process_fragment_for_test(second_fragment,
                                                                  second_len,
                                                                  buffer,
                                                                  sizeof(buffer));
        if (received_len <= 0) {
            free(first_fragment);
            free(second_fragment);
            TEST_FAIL("Hardware interface did not return the final DATA fragment");
            goto test_case_end;
        }

        ASSERT_EQUAL((int)second_len, received_len);
        ASSERT_TRUE(memcmp(buffer, second_fragment, second_len) == 0);

        free(first_fragment);
        free(second_fragment);
        TEST_PASS();
        test_case_end:;
    }
#undef test_case_end

    TEST_SUITE_END();
}
