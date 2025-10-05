#include "../tests/test_runner.h"
#include "../include/uci.h"
#include "../include/uci_functions.h"
#include <string.h>
#include <stdint.h>

#ifndef ANDROID_GET_POWER_STATS
#define ANDROID_GET_POWER_STATS 0x00
#define ANDROID_SET_COUNTRY_CODE 0x01
#define ANDROID_FIRA_RANGE_DIAGNOSTICS 0x02
#define ANDROID_RADAR_SET_APP_CONFIG 0x11
#define ANDROID_RADAR_GET_APP_CONFIG 0x12
#define TEST_RF_SET_CONFIG 0x00
#define TEST_RF_PERIODIC_TX 0x02
#define TEST_RF_STOP 0x07
#endif

static inline void write_u16_le(unsigned char* buffer, uint16_t value) {
    buffer[0] = value & 0xFF;
    buffer[1] = (value >> 8) & 0xFF;
}

static inline void write_u32_le(unsigned char* buffer, uint32_t value) {
    buffer[0] = value & 0xFF;
    buffer[1] = (value >> 8) & 0xFF;
    buffer[2] = (value >> 16) & 0xFF;
    buffer[3] = (value >> 24) & 0xFF;
}

// Test suite for UCI functions
int main() {
    TEST_SUITE(uci_functions);
    
    // Test basic header creation
    TEST_CASE(header_creation);
    {
        struct uci_packet_header header;
        set_header_values(&header, COMMAND, COMPLETE, CORE, CORE_DEVICE_INFO, 0);
        
        if (get_mt(&header) != COMMAND) {
            TEST_FAIL("MT field mismatch");
            goto test_case_end;
        }
        if (get_pbf(&header) != COMPLETE) {
            TEST_FAIL("PBF field mismatch");
            goto test_case_end;
        }
        if (get_gid(&header) != CORE) {
            TEST_FAIL("GID field mismatch");
            goto test_case_end;
        }
        if (get_opcode(&header) != CORE_DEVICE_INFO) {
            TEST_FAIL("Opcode field mismatch");
            goto test_case_end;
        }
        if (header.payload_len != 0) {
            TEST_FAIL("Payload length mismatch");
            goto test_case_end;
        }
        
        TEST_PASS();
    }
    test_case_end:;
    
    // Test header values extraction
    TEST_CASE(header_extraction);
    {
        struct uci_packet_header header;
        set_header_values(&header, RESPONSE, NOT_COMPLETE, SESSION_CONFIG, SESSION_INIT, 10);
        
        if (get_mt(&header) != RESPONSE) {
            TEST_FAIL("MT field mismatch");
            goto test_case_end2;
        }
        if (get_pbf(&header) != NOT_COMPLETE) {
            TEST_FAIL("PBF field mismatch");
            goto test_case_end2;
        }
        if (get_gid(&header) != SESSION_CONFIG) {
            TEST_FAIL("GID field mismatch");
            goto test_case_end2;
        }
        if (get_opcode(&header) != SESSION_INIT) {
            TEST_FAIL("Opcode field mismatch");
            goto test_case_end2;
        }
        if (header.payload_len != 10) {
            TEST_FAIL("Payload length mismatch");
            goto test_case_end2;
        }
        
        TEST_PASS();
    }
    test_case_end2:;
    
    // Test notification header creation
    TEST_CASE(notification_header);
    {
        struct uci_packet_header header;
        set_header_values(&header, NOTIFICATION, COMPLETE, SESSION_CONTROL, SESSION_DATA_CREDIT_NTF, 5);
        
        if (get_mt(&header) != NOTIFICATION) {
            TEST_FAIL("MT field mismatch");
            goto test_case_end3;
        }
        if (get_pbf(&header) != COMPLETE) {
            TEST_FAIL("PBF field mismatch");
            goto test_case_end3;
        }
        if (get_gid(&header) != SESSION_CONTROL) {
            TEST_FAIL("GID field mismatch");
            goto test_case_end3;
        }
        if (get_opcode(&header) != SESSION_DATA_CREDIT_NTF) {
            TEST_FAIL("Opcode field mismatch");
            goto test_case_end3;
        }
        if (header.payload_len != 5) {
            TEST_FAIL("Payload length mismatch");
            goto test_case_end3;
        }
        
        TEST_PASS();
    }
    test_case_end3:;
    
    // Test session management functions
    TEST_CASE(session_management_init);
    {
        init_uci_sessions();
        
        // Verify all sessions are initialized properly
        for (int i = 0; i < MAX_SESSIONS; i++) {
            if (uci_sessions[i].is_allocated != 0) {
                TEST_FAIL("Session should not be allocated after init");
                goto test_case_end4;
            }
            if (uci_sessions[i].session_state != SESSION_STATE_DEINIT) {
                TEST_FAIL("Session state should be deinit after init");
                goto test_case_end4;
            }
            if (uci_sessions[i].num_configs != 0) {
                TEST_FAIL("Number of configs should be 0 after init");
                goto test_case_end4;
            }
        }
        
        TEST_PASS();
    }
    test_case_end4:;
    
    // Test finding free session slot
    TEST_CASE(session_management_find_free_slot);
    {
        // Initially all slots should be free
        int slot = find_free_session_slot();
        if (slot < 0 || slot >= MAX_SESSIONS) {
            TEST_FAIL("Should find free slot initially");
            goto test_case_end5;
        }
        
        // Mark this slot as allocated
        uci_sessions[slot].is_allocated = 1;
        
        // Find another free slot
        int slot2 = find_free_session_slot();
        if (slot2 < 0 || slot2 == slot || slot2 >= MAX_SESSIONS) {
            TEST_FAIL("Should find second free slot");
            goto test_case_end5;
        }
        
        // Mark all slots except last one as allocated
        for (int i = 0; i < MAX_SESSIONS - 1; i++) {
            uci_sessions[i].is_allocated = 1;
        }
        
        // Last slot should still be free
        int final_slot = find_free_session_slot();
        if (final_slot != MAX_SESSIONS - 1) {
            TEST_FAIL("Last slot should still be free");
            goto test_case_end5;
        }
        
        // Mark last slot as allocated
        uci_sessions[MAX_SESSIONS - 1].is_allocated = 1;
        
        // Now no slots should be free
        int slot3 = find_free_session_slot();
        if (slot3 != -1) {
            TEST_FAIL("Should not find free slot when all are allocated");
            goto test_case_end5;
        }
        
        // Restore initial state for other tests
        init_uci_sessions();
        
        TEST_PASS();
    }
    test_case_end5:;
    
    // Test finding session by ID
    TEST_CASE(session_management_find_by_id);
    {
        // Initially no sessions should be found
        int session_idx = find_session_by_id(12345);
        if (session_idx != -1) {
            TEST_FAIL("Should not find session that doesn't exist");
            goto test_case_end6;
        }
        
        // Create a session
        int slot = find_free_session_slot();
        if (slot < 0) {
            TEST_FAIL("Could not find slot for test");
            goto test_case_end6;
        }
        
        uci_sessions[slot].session_id = 54321;
        uci_sessions[slot].is_allocated = 1;
        uci_sessions[slot].session_state = SESSION_STATE_ACTIVE;
        
        // Should find the created session
        session_idx = find_session_by_id(54321);
        if (session_idx != slot) {
            TEST_FAIL("Should find session that was created");
            goto test_case_end6;
        }
        
        // Should not find a different session ID
        session_idx = find_session_by_id(12345);
        if (session_idx != -1) {
            TEST_FAIL("Should not find session with different ID");
            goto test_case_end6;
        }
        
        // Restore initial state for other tests
        init_uci_sessions();
        
        TEST_PASS();
    }
    test_case_end6:;
    
    // Test storing and getting session configuration
    TEST_CASE(session_config_storage);
    {
        int slot = find_free_session_slot();
        if (slot < 0) {
            TEST_FAIL("Could not find slot for test");
            goto test_case_end7;
        }
        
        // Test storing configuration
        unsigned char test_value = 0x42;
        store_session_config(slot, DEVICE_STATE, &test_value, 1);
        
        if (uci_sessions[slot].num_configs != 1) {
            TEST_FAIL("Number of configs should be 1 after storing");
            goto test_case_end7;
        }
        
        // Test getting configuration
        unsigned char retrieved_value[MAX_SESSION_CONFIG_VALUE_SIZE] = {0};
        unsigned char retrieved_len = (unsigned char)sizeof(retrieved_value);
        int result = get_session_config(slot, DEVICE_STATE, retrieved_value, &retrieved_len);

        if (result != 1) {
            TEST_FAIL("Should successfully retrieve config");
            goto test_case_end7;
        }
        if (retrieved_value[0] != test_value) {
            TEST_FAIL("Retrieved value should match stored value");
            goto test_case_end7;
        }
        if (retrieved_len != 1) {
            TEST_FAIL("Retrieved length should match stored length");
            goto test_case_end7;
        }

        // Try getting non-existent config
        retrieved_len = (unsigned char)sizeof(retrieved_value);
        result = get_session_config(slot, LOW_POWER_MODE, retrieved_value, &retrieved_len);
        if (result != 0) {
            TEST_FAIL("Should not retrieve non-existent config");
            goto test_case_end7;
        }
        
        // Restore initial state for other tests
        init_uci_sessions();
        
        TEST_PASS();
    }
    test_case_end7:;
    
    // Test notification handler for session info
    TEST_CASE(notification_handler_session_info);
    {
        // Create a minimal session info notification payload
        // According to UCI spec, this should have sequence number + session_token + etc.
        // For this test, just verify the function doesn't crash with minimal valid payload
        unsigned char minimal_payload[] = {
            0x01, 0x00, 0x00, 0x00,  // sequence_number (4 bytes)
            0x02, 0x00, 0x00, 0x00,  // session_token (4 bytes)
            0x01,                    // rcr_indicator (1 byte)
            0x0A, 0x00, 0x00, 0x00,  // current_ranging_interval (4 bytes) = 10 ms
            0x01,                    // ranging_measurement_type (1 byte) = TWO_WAY
            0x00,                    // reserved
            0x00,                    // mac_address_indicator
            0x01, 0x00, 0x00, 0x00,  // hus_primary_session_id (4 bytes)
            0x01,                    // number of measurements (1 byte)
            0x11, 0x00,              // mac address (2 bytes)
            0x00,                    // status
            0x00,                    // nlos
            0x2C, 0x01,              // distance (300 cm)
            0x00, 0x00,              // aoa azimuth
            0x00,                    // aoa azimuth fom
            0x00, 0x00,              // aoa elevation
            0x00,                    // aoa elevation fom
            0x00, 0x00, 0x00, 0x00,  // aoa dest azimuth
            0x00,                    // aoa dest azimuth fom
            0x00, 0x00,              // aoa dest elevation
            0x00,                    // aoa dest elevation fom
            0x01,                    // slot index
            0x50                     // rssi
        };
        
        // This should not crash
        handle_session_info_ntf(minimal_payload, sizeof(minimal_payload));
        
        TEST_PASS();
    }
    test_case_end8:;
    
#define test_case_end test_case_end_9
    // Test end-to-end session command flow through send_uci_command
    TEST_CASE(session_command_flow);
    {
        init_uci_sessions();

        unsigned char init_payload[5] = {0x01, 0x00, 0x00, 0x00, FIRA_RANGING_SESSION};
        send_uci_command(COMMAND, COMPLETE, SESSION_CONFIG, SESSION_INIT, init_payload, sizeof(init_payload));

        int slot = find_session_by_id(0x00000001);
        ASSERT_TRUE(slot >= 0);
        ASSERT_EQUAL(SESSION_STATE_INIT, uci_sessions[slot].session_state);

        unsigned int handle = uci_sessions[slot].session_handle;
        ASSERT_TRUE(handle != 0);

        unsigned char handle_payload[4];
        write_u32_le(handle_payload, handle);

        send_uci_command(COMMAND, COMPLETE, SESSION_CONTROL, SESSION_START, handle_payload, sizeof(handle_payload));
        ASSERT_EQUAL(SESSION_STATE_ACTIVE, uci_sessions[slot].session_state);

        send_uci_command(COMMAND, COMPLETE, SESSION_CONTROL, SESSION_STOP, handle_payload, sizeof(handle_payload));
        ASSERT_EQUAL(SESSION_STATE_IDLE, uci_sessions[slot].session_state);
        ASSERT_EQUAL(1, uci_sessions[slot].ranging_count);

        send_uci_command(COMMAND, COMPLETE, SESSION_CONTROL, SESSION_GET_RANGING_COUNT, handle_payload, sizeof(handle_payload));
        ASSERT_EQUAL(1, uci_sessions[slot].ranging_count);

        send_uci_command(COMMAND, COMPLETE, SESSION_CONFIG, SESSION_DEINIT, handle_payload, sizeof(handle_payload));
        ASSERT_EQUAL(0, uci_sessions[slot].is_allocated);

        uci_process_pending_notifications();

        TEST_PASS();
    }
    test_case_end:;
#undef test_case_end

#define test_case_end test_case_end_10
    // Test core configuration commands for coverage
    TEST_CASE(core_config_commands);
    {
        unsigned char bad_set_payload[] = {0x01, DEVICE_STATE, 0x02, 0xAA};
        send_uci_command(COMMAND, COMPLETE, CORE, CORE_SET_CONFIG, bad_set_payload, sizeof(bad_set_payload));

        unsigned char get_payload[] = {0x01, DEVICE_STATE};
        send_uci_command(COMMAND, COMPLETE, CORE, CORE_GET_CONFIG, get_payload, sizeof(get_payload));

        uci_process_pending_notifications();

        TEST_PASS();
    }
    test_case_end:;
#undef test_case_end

#define test_case_end test_case_end_11
    // Test vendor Android and RF test command stubs
    TEST_CASE(vendor_and_test_commands);
    {
        unsigned char country_payload[2] = {'U', 'S'};
        send_uci_command(COMMAND, COMPLETE, VENDOR_ANDROID, ANDROID_SET_COUNTRY_CODE, country_payload, sizeof(country_payload));
        send_uci_command(COMMAND, COMPLETE, VENDOR_ANDROID, ANDROID_GET_POWER_STATS, NULL, 0);

        unsigned char rf_payload[5] = {0x00, 0x00, 0x00, 0x00, 0x00};
        send_uci_command(COMMAND, COMPLETE, TEST, TEST_RF_SET_CONFIG, rf_payload, sizeof(rf_payload));
        send_uci_command(COMMAND, COMPLETE, TEST, TEST_RF_PERIODIC_TX, rf_payload, sizeof(rf_payload));
        send_uci_command(COMMAND, COMPLETE, TEST, TEST_RF_STOP, NULL, 0);

        TEST_PASS();
    }
    test_case_end:;
#undef test_case_end

    // Test additional core command coverage
    TEST_CASE(core_timestamp_and_get_config_variants);
    {
        // Query timestamp twice to exercise counter increment path
        send_uci_command(COMMAND, COMPLETE, CORE, CORE_QUERY_UWBS_TIMESTAMP, NULL, 0);
        send_uci_command(COMMAND, COMPLETE, CORE, CORE_QUERY_UWBS_TIMESTAMP, NULL, 0);

        // Trigger invalid GET_CONFIG path (null payload)
        send_uci_command(COMMAND, COMPLETE, CORE, CORE_GET_CONFIG, NULL, 0);

        // Request multiple config IDs including one unknown to cover default value path
        unsigned char multi_get_payload[] = {0x03, DEVICE_STATE, LOW_POWER_MODE, 0xFF};
        send_uci_command(COMMAND, COMPLETE, CORE, CORE_GET_CONFIG, multi_get_payload, sizeof(multi_get_payload));

        uci_process_pending_notifications();

        TEST_PASS();
    }
    // Exercise extended TLV handling via session commands
    TEST_CASE(session_app_config_extended_tlvs_command_path);
    {
        init_uci_sessions();

        uci_sessions[0].is_allocated = 1;
        uci_sessions[0].session_state = SESSION_STATE_ACTIVE;
        uci_sessions[0].session_handle = 0xAABBCCDD;
        uci_sessions[0].session_id = 0xAABBCCDD;
        uci_sessions[0].num_configs = 0;

        unsigned char set_payload[16] = {0};
        write_u32_le(set_payload, uci_sessions[0].session_handle);
        set_payload[4] = 2; // number of TLVs
        set_payload[5] = SESSION_TIME_BASE;
        set_payload[6] = 4;
        set_payload[7] = 0x11;
        set_payload[8] = 0x22;
        set_payload[9] = 0x33;
        set_payload[10] = 0x44;
        set_payload[11] = NB_OF_ELEVATION_MEASUREMENTS;
        set_payload[12] = 1;
        set_payload[13] = 0x05;

        send_uci_command(COMMAND, COMPLETE, SESSION_CONFIG, SESSION_SET_APP_CONFIG, set_payload, 14);

        unsigned char value_buffer[MAX_SESSION_CONFIG_VALUE_SIZE] = {0};
        unsigned char value_len = (unsigned char)sizeof(value_buffer);

        ASSERT_EQUAL(1, get_session_config(0, SESSION_TIME_BASE, value_buffer, &value_len));
        ASSERT_EQUAL(4, value_len);
        ASSERT_EQUAL(0x11, value_buffer[0]);
        ASSERT_EQUAL(0x22, value_buffer[1]);
        ASSERT_EQUAL(0x33, value_buffer[2]);
        ASSERT_EQUAL(0x44, value_buffer[3]);

        value_len = (unsigned char)sizeof(value_buffer);
        ASSERT_EQUAL(1, get_session_config(0, NB_OF_ELEVATION_MEASUREMENTS, value_buffer, &value_len));
        ASSERT_EQUAL(1, value_len);
        ASSERT_EQUAL(0x05, value_buffer[0]);

        unsigned char get_payload[7] = {0};
        write_u32_le(get_payload, uci_sessions[0].session_handle);
        get_payload[4] = 2; // number of requested IDs
        get_payload[5] = SESSION_TIME_BASE;
        get_payload[6] = NB_OF_ELEVATION_MEASUREMENTS;

        send_uci_command(COMMAND, COMPLETE, SESSION_CONFIG, SESSION_GET_APP_CONFIG, get_payload, sizeof(get_payload));

        uci_process_pending_notifications();

        TEST_PASS();
    }
    // Test session edge cases and invalid parameter handling
    TEST_CASE(session_command_edge_cases);
    {
        init_uci_sessions();

        unsigned char short_payload[] = {0x00};
        send_uci_command(COMMAND, COMPLETE, SESSION_CONFIG, SESSION_UPDATE_CONTROLLER_MULTICAST_LIST, short_payload, sizeof(short_payload));
        send_uci_command(COMMAND, COMPLETE, SESSION_CONFIG, SESSION_QUERY_DATA_SIZE_IN_RANGING, short_payload, sizeof(short_payload));
        send_uci_command(COMMAND, COMPLETE, SESSION_CONTROL, SESSION_GET_RANGING_COUNT, short_payload, sizeof(short_payload));
        send_uci_command(COMMAND, COMPLETE, SESSION_CONFIG, SESSION_GET_APP_CONFIG, short_payload, sizeof(short_payload));

        uci_process_pending_notifications();

        TEST_PASS();
    }
    // Verify capability payload exposes rich Android-aligned TLVs
    TEST_CASE(core_caps_info_payload_shape);
    {
        unsigned char caps_payload[255] = {0};
        size_t payload_len = uci_build_core_capabilities_payload(caps_payload, sizeof(caps_payload));

        if (payload_len <= 2) {
            TEST_FAIL("Capability payload length too small");
            goto test_case_end_caps_info_payload_shape;
        }

        if (caps_payload[0] != UCI_STATUS_OK) {
            TEST_FAIL("Capability status not OK");
            goto test_case_end_caps_info_payload_shape;
        }

        unsigned char num_tlvs = caps_payload[1];
        if (num_tlvs < 30) {
            TEST_FAIL("Expected at least 10 capability TLVs");
            goto test_case_end_caps_info_payload_shape;
        }

        size_t offset = 2;
        int found_phy = 0;
        int found_mac = 0;
        int found_sts = 0;
        int found_max_msg = 0;
        int found_max_data = 0;
        int found_v2_ext_mac = 0;
        int found_psdu_support = 0;
        int found_power_stats = 0;
        int found_ccc_channels = 0;
        int found_aoa_interleave = 0;

        for (unsigned char tlv_index = 0; tlv_index < num_tlvs; tlv_index++) {
            if (offset + 2 > payload_len) {
                TEST_FAIL("Truncated TLV header in capability payload");
                goto test_case_end_caps_info_payload_shape;
            }

            unsigned char tlv_type = caps_payload[offset];
            unsigned char tlv_len = caps_payload[offset + 1];
            offset += 2;

            if (offset + tlv_len > payload_len) {
                TEST_FAIL("Truncated TLV value in capability payload");
                goto test_case_end_caps_info_payload_shape;
            }

            const unsigned char* value = &caps_payload[offset];

            switch (tlv_type) {
                case SUPPORTED_V1_FIRA_PHY_VERSION_RANGE_V2_MAX_MESSAGE_SIZE:
                    if (tlv_len != 4 || value[0] != 0x01 || value[1] != 0x00 ||
                        value[2] != 0x02 || value[3] != 0x00) {
                        TEST_FAIL("Unexpected PHY version range values");
                        goto test_case_end_caps_info_payload_shape;
                    }
                    found_phy = 1;
                    break;
                case SUPPORTED_V1_FIRA_MAC_VERSION_RANGE_V2_MAX_DATA_PAYLOAD_SIZE:
                    if (tlv_len != 4 || value[0] != 0x01 || value[1] != 0x00 ||
                        value[2] != 0x02 || value[3] != 0x00) {
                        TEST_FAIL("Unexpected MAC version range values");
                        goto test_case_end_caps_info_payload_shape;
                    }
                    found_mac = 1;
                    break;
                case SUPPORTED_V1_STS_CONFIG_V2_DEVICE_TYPE:
                    if (tlv_len != 1 || value[0] != 0x07) {
                        TEST_FAIL("Unexpected STS config mask");
                        goto test_case_end_caps_info_payload_shape;
                    }
                    found_sts = 1;
                    break;
                case SUPPORTED_V1_MAX_MESSAGE_SIZE_V2_ASSIGNED:
                    if (tlv_len != 2 || value[0] != 0x00 || value[1] != 0x04) {
                        TEST_FAIL("Unexpected max message size");
                        goto test_case_end_caps_info_payload_shape;
                    }
                    found_max_msg = 1;
                    break;
                case SUPPORTED_V1_MAX_DATA_PACKET_PAYLOAD_SIZE_V2_SESSION_KEY_LENGTH:
                    if (tlv_len != 2 || value[0] != 0x10 || value[1] != 0x01) {
                        TEST_FAIL("Unexpected max data payload size");
                        goto test_case_end_caps_info_payload_shape;
                    }
                    found_max_data = 1;
                    break;
                case SUPPORTED_V2_EXTENDED_MAC_ADDRESS:
                    if (tlv_len != 1 || value[0] != 0x01) {
                        TEST_FAIL("Unexpected V2 extended MAC flag");
                        goto test_case_end_caps_info_payload_shape;
                    }
                    found_v2_ext_mac = 1;
                    break;
                case SUPPORTED_V2_PSDU_LENGTH_SUPPORT:
                    if (tlv_len != 2 || value[0] != 0xFF || value[1] != 0x0F) {
                        TEST_FAIL("Unexpected PSDU length support values");
                        goto test_case_end_caps_info_payload_shape;
                    }
                    found_psdu_support = 1;
                    break;
                case SUPPORTED_POWER_STATS:
                    if (tlv_len != 1 || value[0] != 0x01) {
                        TEST_FAIL("Unexpected power stats flag");
                        goto test_case_end_caps_info_payload_shape;
                    }
                    found_power_stats = 1;
                    break;
                case CCC_SUPPORTED_CHANNELS:
                    if (tlv_len != 3 || value[0] != 0x20 || value[1] != 0x08 || value[2] != 0x00) {
                        TEST_FAIL("Unexpected CCC channel mask");
                        goto test_case_end_caps_info_payload_shape;
                    }
                    found_ccc_channels = 1;
                    break;
                case SUPPORTED_AOA_RESULT_REQ_ANTENNA_INTERLEAVING:
                    if (tlv_len != 1 || value[0] != 0x01) {
                        TEST_FAIL("Unexpected AOA interleaving flag");
                        goto test_case_end_caps_info_payload_shape;
                    }
                    found_aoa_interleave = 1;
                    break;
                default:
                    break;
            }

            offset += tlv_len;
        }

        if (!found_phy || !found_mac || !found_sts || !found_max_msg || !found_max_data ||
            !found_v2_ext_mac || !found_psdu_support || !found_power_stats ||
            !found_ccc_channels || !found_aoa_interleave) {
            TEST_FAIL("Missing expected capability TLVs");
            goto test_case_end_caps_info_payload_shape;
        }

        TEST_PASS();
    }
    test_case_end_caps_info_payload_shape:;

    TEST_CASE(android_range_diagnostics_notification);
    {
        unsigned char payload[128] = {0};
        size_t payload_len = 0;

        write_u32_le(&payload[payload_len], 0xAABBCCDD);
        payload_len += 4;
        write_u32_le(&payload[payload_len], 0x01020304);
        payload_len += 4;
        payload[payload_len++] = 0x01; // one frame report

        payload[payload_len++] = 0x10; // UWB message ID
        payload[payload_len++] = 0x01; // action
        payload[payload_len++] = 0x02; // antenna set
        payload[payload_len++] = 0x04; // TLV count

        // TLV 0: RSSI samples
        payload[payload_len++] = FRAME_REPORT_TLV_RSSI;
        write_u16_le(&payload[payload_len], 2);
        payload_len += 2;
        payload[payload_len++] = 0xF6;
        payload[payload_len++] = 0xEC;

        // TLV 1: AOA measurement block (single measurement)
        payload[payload_len++] = FRAME_REPORT_TLV_AOA;
        write_u16_le(&payload[payload_len], 8);
        payload_len += 2;
        write_u16_le(&payload[payload_len], 100); // TDOA
        payload_len += 2;
        write_u16_le(&payload[payload_len], 200); // PDOA
        payload_len += 2;
        write_u16_le(&payload[payload_len], 300); // AOA
        payload_len += 2;
        payload[payload_len++] = 0x05; // FoM
        payload[payload_len++] = 0x02; // measurement type

        // TLV 2: CIR with one entry and 4-byte sample window
        payload[payload_len++] = FRAME_REPORT_TLV_CIR;
        size_t cir_len_index = payload_len;
        write_u16_le(&payload[payload_len], 0);
        payload_len += 2;
        size_t cir_start = payload_len;

        payload[payload_len++] = 0x01; // number of entries
        write_u16_le(&payload[payload_len], 10); // first_path_index
        payload_len += 2;
        write_u16_le(&payload[payload_len], 20); // first_path_snr
        payload_len += 2;
        write_u16_le(&payload[payload_len], 30); // first_path_ns
        payload_len += 2;
        write_u16_le(&payload[payload_len], 40); // peak_path_index
        payload_len += 2;
        write_u16_le(&payload[payload_len], 50); // peak_path_snr
        payload_len += 2;
        write_u16_le(&payload[payload_len], 60); // peak_path_ns
        payload_len += 2;
        payload[payload_len++] = 0x07; // first_path_sample_offset
        payload[payload_len++] = 0x02; // samples_number
        write_u16_le(&payload[payload_len], 4); // sample window length
        payload_len += 2;
        payload[payload_len++] = 0xAA;
        payload[payload_len++] = 0xBB;
        payload[payload_len++] = 0xCC;
        payload[payload_len++] = 0xDD;

        size_t cir_length = payload_len - cir_start;
        write_u16_le(&payload[cir_len_index], (uint16_t)cir_length);

        // TLV 3: Segment metrics
        payload[payload_len++] = FRAME_REPORT_TLV_SEGMENT_METRICS;
        write_u16_le(&payload[payload_len], 17);
        payload_len += 2;
        const unsigned char segment_metrics_sample[17] = {
            0x08, 0xB0, 0xFF, 0x04, 0x3C, 0xE1, 0x02, 0xA1,
            0xFF, 0x52, 0xB8, 0xE3, 0x02, 0xC4, 0xFF, 0xC0, 0xB8
        };
        memcpy(&payload[payload_len], segment_metrics_sample, sizeof(segment_metrics_sample));
        payload_len += sizeof(segment_metrics_sample);

        struct uci_packet_header header;
        set_header_values(&header, NOTIFICATION, COMPLETE, VENDOR_ANDROID,
                          ANDROID_FIRA_RANGE_DIAGNOSTICS, (unsigned char)payload_len);

        unsigned char packet[sizeof(struct uci_packet_header) + 128] = {0};
        memcpy(packet, &header, sizeof(struct uci_packet_header));
        memcpy(packet + sizeof(struct uci_packet_header), payload, payload_len);

        parse_uci_packet(packet, sizeof(struct uci_packet_header) + payload_len);

        TEST_PASS();
    }

    TEST_CASE(range_data_notification_decoding);
    {
        unsigned char payload[64] = {0};
        size_t offset = 0;

        write_u32_le(&payload[offset], 0x2A); // session token
        offset += 4;
        write_u32_le(&payload[offset], 0x01); // sequence number
        offset += 4;

        unsigned int control = (0u << 24) | (1u << 16) | (0u << 8) | UCI_STATUS_OK;
        write_u32_le(&payload[offset], control);
        offset += 4;

        payload[offset++] = 0x34; // MAC (little-endian: 0x1234)
        payload[offset++] = 0x12;
        payload[offset++] = UCI_STATUS_OK;
        payload[offset++] = 0x00; // NLOS
        write_u16_le(&payload[offset], 7); // distance cm
        offset += 2;
        write_u16_le(&payload[offset], 0); // AoA azimuth
        offset += 2;
        payload[offset++] = 0;   // AoA azimuth FoM
        write_u16_le(&payload[offset], 0); // AoA elevation
        offset += 2;
        payload[offset++] = 0;   // AoA elevation FoM
        write_u16_le(&payload[offset], 0); // Dest azimuth
        offset += 2;
        payload[offset++] = 0;   // Dest azimuth FoM
        write_u16_le(&payload[offset], 0); // Dest elevation
        offset += 2;
        payload[offset++] = 0;   // Dest elevation FoM
        payload[offset++] = 0;   // Slot index
        payload[offset++] = 0xF6; // RSSI (-10 dBm)

        // Vendor distance TLV (field 0x01/0x01, value 7)
        payload[offset++] = 0x01;
        payload[offset++] = 0x01;
        payload[offset++] = 0x00;
        payload[offset++] = 0x00;
        payload[offset++] = 0x00;
        payload[offset++] = 0x07;

        struct uci_packet_header header;
        set_header_values(&header, NOTIFICATION, COMPLETE, RANGING_DATA,
                          RANGE_DATA_NTF_OPCODE, (unsigned char)offset);

        unsigned char packet[sizeof(struct uci_packet_header) + 64] = {0};
        memcpy(packet, &header, sizeof(struct uci_packet_header));
        memcpy(packet + sizeof(struct uci_packet_header), payload, offset);

        parse_uci_packet(packet, sizeof(struct uci_packet_header) + offset);

        TEST_PASS();
    }

    // Test radar-specific Android vendor commands
    TEST_CASE(android_radar_commands);
    {
        unsigned char set_payload[] = {
            0x05, 0x00, 0x00, 0x00, // session handle
            0x01,                   // number of TLVs
            0x01, 0x01, 0xAA        // cfg_id, len, value
        };
        send_uci_command(COMMAND, COMPLETE, VENDOR_ANDROID, ANDROID_RADAR_SET_APP_CONFIG, set_payload, sizeof(set_payload));

        unsigned char get_payload[] = {
            0x05, 0x00, 0x00, 0x00, // session handle
            0x01,                   // number of TLVs
            0x01                    // cfg_id
        };
        send_uci_command(COMMAND, COMPLETE, VENDOR_ANDROID, ANDROID_RADAR_GET_APP_CONFIG, get_payload, sizeof(get_payload));

        uci_process_pending_notifications();

        TEST_PASS();
    }

    TEST_SUITE_END();
}
