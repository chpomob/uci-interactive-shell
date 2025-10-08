#include "../tests/test_runner.h"
#include "../include/uci.h"
#include "../include/uci_functions.h"
#include "../include/uci_config_manager.h"
#include "../include/uci_response_core.h"
#include "../include/uci_ui_packet_decoder.h"
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

    uci_config_init();
    
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

    // Test header field extraction via struct helper
    TEST_CASE(header_struct_extraction);
    {
        struct uci_packet_header header;
        set_header_values(&header, RESPONSE, NOT_COMPLETE, SESSION_CONTROL, SESSION_GET_RANGING_COUNT, 12);

        uci_header_fields_t fields;
        memset(&fields, 0, sizeof(fields));
        uci_extract_header_fields(&header, &fields);

        if (fields.message_type != RESPONSE) {
            TEST_FAIL("message_type mismatch");
            goto test_case_header_struct_end;
        }
        if (fields.packet_boundary != NOT_COMPLETE) {
            TEST_FAIL("packet_boundary mismatch");
            goto test_case_header_struct_end;
        }
        if (fields.group_id != SESSION_CONTROL) {
            TEST_FAIL("group_id mismatch");
            goto test_case_header_struct_end;
        }
        if (fields.opcode_id != SESSION_GET_RANGING_COUNT) {
            TEST_FAIL("opcode_id mismatch");
            goto test_case_header_struct_end;
        }
        if (fields.payload_length != 12) {
            TEST_FAIL("payload_length mismatch");
            goto test_case_header_struct_end;
        }

        TEST_PASS();
    }
    test_case_header_struct_end:;

    // Test header packing masks extraneous bits
    TEST_CASE(header_field_masking);
    {
        struct uci_packet_header header;
        set_header_values(&header,
                          (unsigned char)(COMMAND | 0xF8),
                          (unsigned char)(NOT_COMPLETE | 0xF6),
                          (unsigned char)(SESSION_CONFIG | 0x30),
                          (unsigned char)(SESSION_INIT | 0xC0),
                          0x7B);

        if (get_mt(&header) != COMMAND) {
            TEST_FAIL("MT masking failed");
            goto test_case_header_mask_end;
        }
        if (get_pbf(&header) != NOT_COMPLETE) {
            TEST_FAIL("PBF masking failed");
            goto test_case_header_mask_end;
        }
        if (get_gid(&header) != SESSION_CONFIG) {
            TEST_FAIL("GID masking failed");
            goto test_case_header_mask_end;
        }
        if (get_opcode(&header) != SESSION_INIT) {
            TEST_FAIL("Opcode masking failed");
            goto test_case_header_mask_end;
        }
        if (header.payload_len != 0x7B) {
            TEST_FAIL("Payload length mismatch after masking");
            goto test_case_header_mask_end;
        }

        TEST_PASS();
    }
    test_case_header_mask_end:;

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
        // Create a minimal session info notification payload according to Android UCI spec
        // SessionInfoNtf format: sequence_number + session_token + rcr_indicator +
        // current_ranging_interval + ranging_measurement_type + reserved(1) +
        // mac_address_indicator + hus_primary_session_id + reserved(4) + measurements
        unsigned char minimal_payload[] = {
            0x01, 0x00, 0x00, 0x00,  // sequence_number (4 bytes)
            0x02, 0x00, 0x00, 0x00,  // session_token (4 bytes)
            0x01,                    // rcr_indicator (1 byte)
            0x0A, 0x00, 0x00, 0x00,  // current_ranging_interval (4 bytes) = 10 ms
            0x01,                    // ranging_measurement_type (1 byte) = TWO_WAY
            0x00,                    // reserved (1 byte)
            0x00,                    // mac_address_indicator (1 byte) = SHORT_ADDRESS
            0x01, 0x00, 0x00, 0x00,  // hus_primary_session_id (4 bytes)
            0x00, 0x00, 0x00, 0x00,  // reserved (4 bytes)
            0x01,                    // number of measurements (1 byte)
            0x11, 0x00,              // mac address (2 bytes)
            0x00,                    // status
            0x00,                    // nlos
            0x2C, 0x01,              // distance (300 cm)
            0x00, 0x00,              // aoa azimuth
            0x00,                    // aoa azimuth fom
            0x00, 0x00,              // aoa elevation
            0x00,                    // aoa elevation fom
            0x00, 0x00,              // aoa dest azimuth
            0x00,                    // aoa dest azimuth fom
            0x00, 0x00,              // aoa dest elevation
            0x00,                    // aoa dest elevation fom
            0x00,                    // slot index
            0x00,                    // rssi
            0x01, 0x50               // vendor_data (2 bytes)
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

#define test_case_end test_case_end_multicast_updates
    TEST_CASE(session_multicast_updates);
    {
        init_uci_sessions();

        unsigned char init_payload[5] = {0x02, 0x00, 0x00, 0x00, FIRA_RANGING_SESSION};
        send_uci_command(COMMAND, COMPLETE, SESSION_CONFIG, SESSION_INIT, init_payload, sizeof(init_payload));

        int slot = find_session_by_id(0x00000002);
        ASSERT_TRUE(slot >= 0);

        unsigned int handle = uci_sessions[slot].session_handle;
        unsigned char add_payload[6 + 6] = {0};
        write_u32_le(add_payload, handle);
        add_payload[4] = MULTICAST_ACTION_ADD;
        add_payload[5] = 1; // one controlee
        add_payload[6] = 0x34;
        add_payload[7] = 0x12; // short address 0x1234
        write_u32_le(&add_payload[8], 0xAABBCCDD);

        send_uci_command(COMMAND, COMPLETE, SESSION_CONFIG, SESSION_UPDATE_CONTROLLER_MULTICAST_LIST,
                         add_payload, sizeof(add_payload));

        ASSERT_EQUAL(1, uci_sessions[slot].multicast_count);
        ASSERT_EQUAL(0x1234, uci_sessions[slot].multicast_entries[0].short_address);
        ASSERT_UINT64_EQUAL(0xAABBCCDDu, uci_sessions[slot].multicast_entries[0].subsession_id);

        unsigned char remove_payload[6 + 6] = {0};
        write_u32_le(remove_payload, handle);
        remove_payload[4] = MULTICAST_ACTION_REMOVE;
        remove_payload[5] = 1;
        remove_payload[6] = 0x34;
        remove_payload[7] = 0x12;
        write_u32_le(&remove_payload[8], 0xAABBCCDD);

        send_uci_command(COMMAND, COMPLETE, SESSION_CONFIG, SESSION_UPDATE_CONTROLLER_MULTICAST_LIST,
                         remove_payload, sizeof(remove_payload));

        ASSERT_EQUAL(0, uci_sessions[slot].multicast_count);

        TEST_PASS();
    }
    test_case_end:;
#undef test_case_end

#define test_case_end test_case_end_dt_tag_rounds
    TEST_CASE(session_dt_tag_round_updates);
    {
        init_uci_sessions();

        unsigned char init_payload[5] = {0x03, 0x00, 0x00, 0x00, FIRA_RANGING_SESSION};
        send_uci_command(COMMAND, COMPLETE, SESSION_CONFIG, SESSION_INIT, init_payload, sizeof(init_payload));

        int slot = find_session_by_id(0x00000003);
        ASSERT_TRUE(slot >= 0);

        unsigned int handle = uci_sessions[slot].session_handle;
        unsigned char dt_payload[5 + 3] = {0};
        write_u32_le(dt_payload, handle);
        dt_payload[4] = 3;
        dt_payload[5] = 1;
        dt_payload[6] = 5;
        dt_payload[7] = 9;

        send_uci_command(COMMAND, COMPLETE, SESSION_CONFIG, SESSION_UPDATE_ACTIVE_ROUNDS_DT_TAG,
                         dt_payload, sizeof(dt_payload));

        ASSERT_EQUAL(3, uci_sessions[slot].dt_tag_round_count);
        ASSERT_EQUAL(1, uci_sessions[slot].dt_tag_round_indexes[0]);
        ASSERT_EQUAL(5, uci_sessions[slot].dt_tag_round_indexes[1]);
        ASSERT_EQUAL(9, uci_sessions[slot].dt_tag_round_indexes[2]);

        TEST_PASS();
    }
    test_case_end:;
#undef test_case_end

#define test_case_end test_case_end_dtp_config
    TEST_CASE(session_data_transfer_phase_config);
    {
        init_uci_sessions();

        unsigned char init_payload[5] = {0x04, 0x00, 0x00, 0x00, FIRA_RANGING_SESSION};
        send_uci_command(COMMAND, COMPLETE, SESSION_CONFIG, SESSION_INIT, init_payload, sizeof(init_payload));

        int slot = find_session_by_id(0x00000004);
        ASSERT_TRUE(slot >= 0);

        unsigned int handle = uci_sessions[slot].session_handle;
        unsigned char dtp_payload[7 + 3] = {0};
        write_u32_le(dtp_payload, handle);
        dtp_payload[4] = 7;  // repetition
        dtp_payload[5] = 0xA5; // control flags
        dtp_payload[6] = 0x03; // size
        dtp_payload[7] = 0x11;
        dtp_payload[8] = 0x22;
        dtp_payload[9] = 0x33;

        send_uci_command(COMMAND, COMPLETE, SESSION_CONFIG, SESSION_DATA_TRANSFER_PHASE_CONFIG,
                         dtp_payload, sizeof(dtp_payload));

        ASSERT_EQUAL(7, uci_sessions[slot].dtp_repetition);
        ASSERT_EQUAL(0xA5, uci_sessions[slot].dtp_control);
        ASSERT_EQUAL(0x03, uci_sessions[slot].dtp_size);
        ASSERT_EQUAL(3, uci_sessions[slot].dtp_payload_len);
        ASSERT_EQUAL(0x11, uci_sessions[slot].dtp_payload[0]);
        ASSERT_EQUAL(0x22, uci_sessions[slot].dtp_payload[1]);
        ASSERT_EQUAL(0x33, uci_sessions[slot].dtp_payload[2]);

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

        unsigned char multi_byte_payload[] = {0x01, LOW_POWER_MODE, 0x02, 0x34, 0x12};
        send_uci_command(COMMAND, COMPLETE, CORE, CORE_SET_CONFIG, multi_byte_payload, sizeof(multi_byte_payload));

        unsigned char stored_config[4] = {0};
        size_t stored_len = sizeof(stored_config);
        ASSERT_EQUAL(0, uci_config_get_device_param(LOW_POWER_MODE, stored_config, &stored_len));
        ASSERT_EQUAL(2, (int)stored_len);
        ASSERT_EQUAL(0x34, stored_config[0]);
        ASSERT_EQUAL(0x12, stored_config[1]);

        unsigned char get_payload[] = {0x01, DEVICE_STATE};
        send_uci_command(COMMAND, COMPLETE, CORE, CORE_GET_CONFIG, get_payload, sizeof(get_payload));

        unsigned char multi_get_payload[] = {0x02, DEVICE_STATE, LOW_POWER_MODE};
        send_uci_command(COMMAND, COMPLETE, CORE, CORE_GET_CONFIG, multi_get_payload, sizeof(multi_get_payload));

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

#define test_case_end test_case_end_real_world_packets
    // Test real UCI packets from logs and PDL specifications
    TEST_CASE(real_world_uci_packets);
    {
        // Test real range notification packet from logs
        // Raw: 6b0300212a0000000800000006030100000001000001000000020100000401000005000000
        unsigned char real_packet[] = {
            0x6b, 0x03, 0x00, 0x21,  // Header: RANGING_DATA notification, opcode 0x03, payload length 33
            0x2a, 0x00, 0x00, 0x00,  // Session token (little endian)
            0x08, 0x00, 0x00, 0x00,  // Sequence number (little endian)
            0x06, 0x03, 0x01, 0x00,  // Control field and more
            0x00, 0x00, 0x01, 0x00,  // More payload
            0x00, 0x01, 0x00, 0x00,  // More payload
            0x00, 0x02, 0x01, 0x00,  // More payload
            0x00, 0x04, 0x01, 0x00,  // More payload
            0x00, 0x05  // Remaining payload
        };

        struct uci_packet_header* header = (struct uci_packet_header*)real_packet;
        
        // Verify header fields from real packet
        ASSERT_EQUAL(RANGING_DATA, get_gid(header));
        ASSERT_EQUAL(NOTIFICATION, get_mt(header));
        ASSERT_EQUAL(33, header->payload_len);

        // Test PDL specification packet: DeviceResetCmd
        // Raw: 2000000100000000
        unsigned char pdl_device_reset[] = {
            0x20, 0x00, 0x00, 0x01,  // Header: CORE command, opcode 0x00, payload length 1
            0x00  // reset_config payload
        };

        header = (struct uci_packet_header*)pdl_device_reset;
        ASSERT_EQUAL(CORE, get_gid(header));
        ASSERT_EQUAL(COMMAND, get_mt(header));
        ASSERT_EQUAL(0x00, get_opcode(header));  // CORE_DEVICE_RESET
        ASSERT_EQUAL(1, header->payload_len);

        // Test PDL specification packet: GetDeviceInfoRsp
        // Raw: 4002000b000000010100020003000400010a
        unsigned char pdl_device_info_rsp[] = {
            0x40, 0x02, 0x00, 0x0b,  // Header: CORE response, opcode 0x02, payload length 11
            0x00, 0x00, 0x00, 0x01,  // status = UCI_STATUS_OK
            0x01, 0x00,              // uci_version = 0x0001
            0x02, 0x00,              // mac_version = 0x0002  
            0x03, 0x00,              // phy_version = 0x0003
            0x04, 0x00,              // uci_test_version = 0x0004
            0x01,                    // vendor_spec_info count = 1
            0x0a                     // vendor_spec_info[0] = 0x0a
        };

        header = (struct uci_packet_header*)pdl_device_info_rsp;
        ASSERT_EQUAL(CORE, get_gid(header));
        ASSERT_EQUAL(RESPONSE, get_mt(header));
        ASSERT_EQUAL(0x02, get_opcode(header));  // CORE_DEVICE_INFO
        ASSERT_EQUAL(11, header->payload_len);
        ASSERT_EQUAL(0, pdl_device_info_rsp[4]); // status = UCI_STATUS_OK

        // Test PDL specification packet: SessionInfoNtf
        // Raw: 620000190000000002030405060708000a010101010000000000000000000000
        unsigned char pdl_session_info[] = {
            0x62, 0x00, 0x00, 0x19,  // Header: SESSION_CONTROL notification, opcode 0x00, payload length 25
            0x00, 0x00, 0x00, 0x00,  // sequence number
            0x02, 0x03, 0x04, 0x05,  // session token 
            0x06,                    // rcr_indicator
            0x07, 0x08, 0x00, 0x00,  // current_ranging_interval
            0x0a,                    // ranging_measurement_type
            0x01,                    // reserved field
            0x01,                    // mac_address_indicator
            0x00, 0x00, 0x00, 0x00,  // hus_primary_session_id
            0x00, 0x00, 0x00, 0x00, 0x00  // remaining payload
        };

        header = (struct uci_packet_header*)pdl_session_info;
        ASSERT_EQUAL(SESSION_CONTROL, get_gid(header));
        ASSERT_EQUAL(NOTIFICATION, get_mt(header));
        ASSERT_EQUAL(0x00, get_opcode(header));  // SESSION_INFO_NTF
        ASSERT_EQUAL(25, header->payload_len);

        // Test PDL specification packet: AndroidRangeDiagnosticsNtf
        // Raw: 6c0200110000000101010102020202010001020100010000
        unsigned char pdl_android_diag[] = {
            0x6c, 0x02, 0x00, 0x11,  // Header: VENDOR_ANDROID notification, opcode 0x02, payload length 17
            0x00, 0x00, 0x00, 0x01,  // session_token
            0x01, 0x01, 0x01, 0x02,  // sequence_number
            0x02, 0x02, 0x02, 0x01,  // frame reports data
            0x00, 0x01, 0x02, 0x01,  // more frame reports
            0x00, 0x01, 0x00, 0x00   // end of frame reports
        };

        header = (struct uci_packet_header*)pdl_android_diag;
        ASSERT_EQUAL(VENDOR_ANDROID, get_gid(header));
        ASSERT_EQUAL(NOTIFICATION, get_mt(header));
        ASSERT_EQUAL(0x02, get_opcode(header));  // ANDROID_FIRA_RANGE_DIAGNOSTICS
        ASSERT_EQUAL(17, header->payload_len);

        TEST_PASS();
    }

    test_case_end:;

    // Test CORE_DEVICE_INFO_RSP vendor_spec_info field decoding
    TEST_CASE(core_device_info_vendor_spec_info);
    {
        // Build CORE_DEVICE_INFO_RSP response using the builder function
        unsigned char response_payload[255] = {0};
        int payload_len = build_core_device_info_response(response_payload, sizeof(response_payload));

        // Verify payload length is exactly 10 bytes (not 9)
        ASSERT_EQUAL(10, payload_len);

        // Verify structure per FiRa spec:
        // Byte 0: status
        // Bytes 1-2: uci_version (16-bit LE)
        // Bytes 3-4: mac_version (16-bit LE)
        // Bytes 5-6: phy_version (16-bit LE)
        // Bytes 7-8: uci_test_version (16-bit LE)
        // Byte 9: vendor_spec_info count
        ASSERT_EQUAL(UCI_STATUS_OK, response_payload[0]);

        // Verify version fields are 16-bit
        uint16_t uci_version = response_payload[1] | (response_payload[2] << 8);
        ASSERT_EQUAL(0x0100, uci_version);

        uint16_t mac_version = response_payload[3] | (response_payload[4] << 8);
        ASSERT_EQUAL(0x0200, mac_version);

        uint16_t phy_version = response_payload[5] | (response_payload[6] << 8);
        ASSERT_EQUAL(0x0200, phy_version);

        uint16_t uci_test_version = response_payload[7] | (response_payload[8] << 8);
        ASSERT_EQUAL(0x0100, uci_test_version);

        // Verify vendor_spec_info count field exists at byte 9
        ASSERT_EQUAL(0, response_payload[9]);

        // Test with vendor_spec_info data present
        unsigned char test_payload[15] = {
            UCI_STATUS_OK,
            0x00, 0x01,  // uci_version = 0x0100
            0x00, 0x02,  // mac_version = 0x0200
            0x00, 0x03,  // phy_version = 0x0300
            0x00, 0x04,  // uci_test_version = 0x0400
            0x03,        // vendor_spec_info count = 3
            0xAA, 0xBB, 0xCC  // vendor_spec_info data
        };

        // This should not crash and should parse all fields correctly
        ui_decode_core_device_info_rsp(test_payload, sizeof(test_payload));

        TEST_PASS();
    }

    // Test SESSION_GET_RANGING_COUNT_RSP 32-bit count field
    TEST_CASE(session_get_ranging_count_32bit);
    {
        init_uci_sessions();

        // Create a session with a large ranging count
        unsigned char init_payload[5] = {0x05, 0x00, 0x00, 0x00, FIRA_RANGING_SESSION};
        send_uci_command(COMMAND, COMPLETE, SESSION_CONFIG, SESSION_INIT, init_payload, sizeof(init_payload));

        int slot = find_session_by_id(0x00000005);
        ASSERT_TRUE(slot >= 0);

        // Set ranging count to a value that requires 32 bits
        uci_sessions[slot].ranging_count = 0x12345678;

        unsigned int handle = uci_sessions[slot].session_handle;
        unsigned char handle_payload[4];
        write_u32_le(handle_payload, handle);

        // Send SESSION_GET_RANGING_COUNT command
        send_uci_command(COMMAND, COMPLETE, SESSION_CONTROL, SESSION_GET_RANGING_COUNT, handle_payload, sizeof(handle_payload));

        // Verify the response was generated correctly
        // The response should be 5 bytes: status (1) + count (4)
        // Count should be 0x12345678 in little-endian format

        // Build expected response manually to verify
        unsigned char expected_response[5];
        expected_response[0] = UCI_STATUS_OK;
        write_u32_le(&expected_response[1], 0x12345678);

        // Now test the decoder with this payload
        ui_decode_session_get_ranging_count_rsp(expected_response, sizeof(expected_response));

        // Test with maximum 32-bit value
        unsigned char max_value_payload[5];
        max_value_payload[0] = UCI_STATUS_OK;
        write_u32_le(&max_value_payload[1], 0xFFFFFFFF);
        ui_decode_session_get_ranging_count_rsp(max_value_payload, sizeof(max_value_payload));

        // Test error handling - payload too short
        unsigned char short_payload[3];
        short_payload[0] = UCI_STATUS_OK;
        short_payload[1] = 0x01;
        short_payload[2] = 0x00;
        ui_decode_session_get_ranging_count_rsp(short_payload, sizeof(short_payload));

        TEST_PASS();
    }

    // Test SESSION_GET_STATE_RSP decoder
    TEST_CASE(session_get_state_decoder);
    {
        // Test with valid state values
        unsigned char init_state_payload[2] = {UCI_STATUS_OK, SESSION_STATE_INIT};
        ui_decode_session_get_state_rsp(init_state_payload, sizeof(init_state_payload));

        unsigned char active_state_payload[2] = {UCI_STATUS_OK, SESSION_STATE_ACTIVE};
        ui_decode_session_get_state_rsp(active_state_payload, sizeof(active_state_payload));

        unsigned char idle_state_payload[2] = {UCI_STATUS_OK, SESSION_STATE_IDLE};
        ui_decode_session_get_state_rsp(idle_state_payload, sizeof(idle_state_payload));

        unsigned char deinit_state_payload[2] = {UCI_STATUS_OK, SESSION_STATE_DEINIT};
        ui_decode_session_get_state_rsp(deinit_state_payload, sizeof(deinit_state_payload));

        // Test with error status
        unsigned char error_payload[2] = {UCI_STATUS_SESSION_NOT_EXIST, 0x00};
        ui_decode_session_get_state_rsp(error_payload, sizeof(error_payload));

        // Test with unknown state value
        unsigned char unknown_state_payload[2] = {UCI_STATUS_OK, 0xFF};
        ui_decode_session_get_state_rsp(unknown_state_payload, sizeof(unknown_state_payload));

        // Test error handling - payload too short
        unsigned char short_payload[1] = {UCI_STATUS_OK};
        ui_decode_session_get_state_rsp(short_payload, sizeof(short_payload));

        TEST_PASS();
    }

    // Test CORE_GET_CAPS_INFO_RSP TLV parsing with decoder
    TEST_CASE(core_get_caps_info_tlv_decoder);
    {
        // Build a real capabilities payload
        unsigned char caps_payload[255] = {0};
        size_t payload_len = uci_build_core_capabilities_payload(caps_payload, sizeof(caps_payload));

        ASSERT_TRUE(payload_len > 2);
        ASSERT_EQUAL(UCI_STATUS_OK, caps_payload[0]);

        // Test the decoder with the real payload
        ui_decode_core_get_caps_info_rsp(caps_payload, (int)payload_len);

        // Test with minimal valid payload (status + 0 TLVs)
        unsigned char minimal_payload[2] = {UCI_STATUS_OK, 0x00};
        ui_decode_core_get_caps_info_rsp(minimal_payload, sizeof(minimal_payload));

        // Test with single TLV
        unsigned char single_tlv_payload[] = {
            UCI_STATUS_OK,
            0x01,  // 1 TLV
            SUPPORTED_V1_FIRA_PHY_VERSION_RANGE_V2_MAX_MESSAGE_SIZE,
            0x04,  // length 4
            0x01, 0x00, 0x02, 0x00  // min version 1, max version 2
        };
        ui_decode_core_get_caps_info_rsp(single_tlv_payload, sizeof(single_tlv_payload));

        // Test error handling - payload too short
        unsigned char short_payload[1] = {UCI_STATUS_OK};
        ui_decode_core_get_caps_info_rsp(short_payload, sizeof(short_payload));

        // Test error handling - truncated TLV
        unsigned char truncated_tlv[] = {
            UCI_STATUS_OK,
            0x01,  // 1 TLV
            0x00,  // TLV type
            0x10   // length 16 but no data follows
        };
        ui_decode_core_get_caps_info_rsp(truncated_tlv, sizeof(truncated_tlv));

        TEST_PASS();
    }

    TEST_SUITE_END();
}
