#include "uci_packet_utils.h"
#include <stdlib.h>
#include <string.h>

static const uci_lookup_entry_t k_device_state_table[] = {
    { DEVICE_STATE_READY, "READY", "Device initialized and ready for commands" },
    { DEVICE_STATE_ACTIVE, "ACTIVE", "Device actively processing UCI requests" },
    { DEVICE_STATE_ERROR, "ERROR", "Device reported an unrecoverable error" },
};

static const uci_lookup_entry_t k_session_state_table[] = {
    { SESSION_STATE_INIT, "INIT", "Session has been initialized" },
    { SESSION_STATE_DEINIT, "DEINIT", "Session has been de-initialized" },
    { SESSION_STATE_ACTIVE, "ACTIVE", "Session is actively ranging" },
    { SESSION_STATE_IDLE, "IDLE", "Session is idle and ready" },
};

static const uci_lookup_entry_t k_session_reason_table[] = {
    { STATE_CHANGE_WITH_SESSION_MANAGEMENT_COMMANDS, "STATE_CHANGE_WITH_SESSION_MANAGEMENT_COMMANDS", "State change triggered by management command" },
    { MAX_RANGING_ROUND_RETRY_COUNT_REACHED, "MAX_RANGING_ROUND_RETRY_COUNT_REACHED", "Retries exhausted while attempting ranging" },
    { MAX_NUMBER_OF_MEASUREMENTS_REACHED, "MAX_NUMBER_OF_MEASUREMENTS_REACHED", "Configured measurement target reached" },
    { SESSION_RESUMED_DUE_TO_INBAND_SIGNAL, "SESSION_RESUMED_DUE_TO_INBAND_SIGNAL", "Session resumed after in-band signal" },
    { SESSION_SUSPENDED_DUE_TO_INBAND_SIGNAL, "SESSION_SUSPENDED_DUE_TO_INBAND_SIGNAL", "Session suspended due to in-band signal" },
    { SESSION_STOPPED_DUE_TO_INBAND_SIGNAL, "SESSION_STOPPED_DUE_TO_INBAND_SIGNAL", "Session stopped due to in-band signal" },
    { 0x1E, "ERROR_MIN_RFRAMES_PER_RR_NOT_SUPPORTED", "Minimum RFRAME per round not supported" },
    { 0x1F, "ERROR_TX_DELAY_NOT_SUPPORTED", "Requested TX delay not supported" },
    { 0x20, "ERROR_SLOT_LENGTH_NOT_SUPPORTED", "Slot length not supported" },
    { 0x21, "ERROR_INSUFFICIENT_SLOTS_PER_RR", "Insufficient slots per ranging round" },
    { 0x22, "ERROR_MAC_ADDRESS_MODE_NOT_SUPPORTED", "MAC address mode not supported" },
    { 0x23, "ERROR_INVALID_RANGING_INTERVAL", "Invalid ranging interval" },
    { 0x24, "ERROR_INVALID_STS_CONFIG", "Invalid STS configuration" },
    { 0x25, "ERROR_INVALID_RFRAME_CONFIG", "Invalid RFRAME configuration" },
    { 0x26, "ERROR_HUS_NOT_ENOUGH_SLOTS", "Hybrid use case lacks enough slots" },
    { 0x27, "ERROR_HUS_CFP_PHASE_TOO_SHORT", "HUS CFP phase too short" },
    { 0x28, "ERROR_HUS_CAP_PHASE_TOO_SHORT", "HUS CAP phase too short" },
    { 0x29, "ERROR_HUS_OTHERS", "Hybrid use case reported a generic error" },
    { 0x2A, "ERROR_SESSION_KEY_NOT_FOUND", "Session key missing" },
    { 0x2B, "ERROR_SUB_SESSION_KEY_NOT_FOUND", "Sub-session key missing" },
    { 0x2C, "ERROR_INVALID_PREAMBLE_CODE_INDEX", "Preamble code index invalid" },
    { 0x2D, "ERROR_INVALID_SFD_ID", "SFD identifier invalid" },
    { 0x2E, "ERROR_INVALID_PSDU_DATA_RATE", "PSDU data rate invalid" },
    { 0x2F, "ERROR_INVALID_PHR_DATA_RATE", "PHR data rate invalid" },
    { 0x30, "ERROR_INVALID_PREAMBLE_DURATION", "Preamble duration invalid" },
    { 0x31, "ERROR_INVALID_STS_LENGTH", "STS length invalid" },
    { 0x32, "ERROR_INVALID_NUM_OF_STS_SEGMENTS", "Number of STS segments invalid" },
    { 0x33, "ERROR_INVALID_NUM_OF_CONTROLEES", "Number of controlees invalid" },
    { 0x34, "ERROR_MAX_RANGING_REPLY_TIME_EXCEEDED", "Maximum ranging reply time exceeded" },
    { 0x35, "ERROR_INVALID_DST_ADDRESS_LIST", "Destination address list invalid" },
    { 0x36, "ERROR_INVALID_OR_NOT_FOUND_SUB_SESSION_ID", "Sub-session identifier invalid or missing" },
    { 0x37, "ERROR_INVALID_RESULT_REPORT_CONFIG", "Result report configuration invalid" },
    { 0x38, "ERROR_INVALID_RANGING_ROUND_CONTROL_CONFIG", "Ranging round control configuration invalid" },
    { 0x39, "ERROR_INVALID_RANGING_ROUND_USAGE", "Ranging round usage invalid" },
    { 0x3A, "ERROR_INVALID_MULTI_NODE_MODE", "Multi-node mode invalid" },
    { 0x3B, "ERROR_RDS_FETCH_FAILURE", "RDS fetch failure" },
    { 0x3C, "ERROR_REF_SESSION_DOES_NOT_EXIST", "Referenced session does not exist" },
    { 0x3D, "ERROR_REF_SESSION_RANGING_DURATION_MISMATCH", "Referenced session ranging duration mismatch" },
    { 0x3E, "ERROR_REF_SESSION_INVALID_OFFSET_TIME", "Referenced session offset time invalid" },
    { 0x3F, "ERROR_REF_SESSION_LOST", "Referenced session lost" },
    { 0x40, "ERROR_DT_ANCHOR_RANGING_ROUNDS_NOT_CONFIGURED", "DT anchor ranging rounds not configured" },
    { 0x41, "ERROR_DT_TAG_RANGING_ROUNDS_NOT_CONFIGURED", "DT tag ranging rounds not configured" },
    { 0x42, "ERROR_UWB_INITIATION_TIME_EXPIRED", "UWB initiation time expired" },
    { 0x80, "ERROR_INVALID_CHANNEL_WITH_AOA", "Invalid channel for AoA" },
    { 0x81, "ERROR_STOPPED_DUE_TO_OTHER_SESSION_CONFLICT", "Stopped due to other session conflict" },
    { 0x82, "ERROR_REGULATION_UWB_OFF", "Regulation turned UWB off" },
    { 0xA1, "ERROR_SESSION_STOPPED_DUE_TO_URSK_EXPIRED", "Session stopped because URSK expired" },
    { 0xA2, "ERROR_SESSION_STOPPED_DUE_TO_MAX_STS", "Session stopped due to maximum STS usage" },
    { 0xF2, "ERROR_MAX_STS_REACHED", "Maximum STS reached" },
    { 0xF3, "ERROR_RADAR_MEASUREMENT_TIME_REACHED", "Radar measurement duration reached" },
    { 0xF4, "ERROR_INVALID_DEVICE_ROLE", "Device role invalid" },
    { 0xF5, "ERROR_NO_MEM", "Out of memory" },
    { 0xF7, "ERROR_DRIVER_DOWN", "UWB driver is down" },
    { 0xF8, "ERROR_INVALID_PROXIMITY_RANGE", "Proximity range invalid" },
    { 0xF9, "ERROR_INVALID_FRAME_INTERVAL", "Frame interval invalid" },
    { 0xFA, "ERROR_INVALID_CAP_SIZE_RANGE", "CAP size range invalid" },
    { 0xFB, "ERROR_INVALID_SCHEDULE_MODE", "Schedule mode invalid" },
    { 0xFC, "ERROR_INVALID_PRF_MODE", "PRF mode invalid" },
    { 0xFE, "ERROR_START_CONFIG", "Failed during session start configuration" },
    { 0xFF, "ERROR_RDS_BUSY", "RDS busy" },
};

static const uci_lookup_entry_t k_data_transfer_status_table[] = {
    { UCI_DATA_TRANSFER_STATUS_REPETITION_OK, "REPETITION_OK", "Frame already delivered during an earlier repetition" },
    { UCI_DATA_TRANSFER_STATUS_OK, "OK", "Data transfer completed successfully" },
    { UCI_DATA_TRANSFER_STATUS_ERROR_DATA_TRANSFER, "ERROR_DATA_TRANSFER", "Unspecified transmission problem" },
    { UCI_DATA_TRANSFER_STATUS_ERROR_NO_CREDIT_AVAILABLE, "ERROR_NO_CREDIT_AVAILABLE", "Peer reported no credits available" },
    { UCI_DATA_TRANSFER_STATUS_ERROR_REJECTED, "ERROR_REJECTED", "Peer rejected the data frame" },
    { UCI_DATA_TRANSFER_STATUS_SESSION_TYPE_NOT_SUPPORTED, "SESSION_TYPE_NOT_SUPPORTED", "Session configuration does not support data transfer" },
    { UCI_DATA_TRANSFER_STATUS_ERROR_DATA_TRANSFER_IS_ONGOING, "ERROR_DATA_TRANSFER_IS_ONGOING", "Another transfer is still in progress" },
    { UCI_DATA_TRANSFER_STATUS_INVALID_FORMAT, "INVALID_FORMAT", "Frame format violated data transfer rules" },
};

static const uci_lookup_entry_t k_uci_status_table[] = {
    { UCI_STATUS_OK, "OK", "Operation completed successfully" },
    { UCI_STATUS_REJECTED, "REJECTED", "Request rejected by device" },
    { UCI_STATUS_FAILED, "FAILED", "Generic failure occurred" },
    { UCI_STATUS_SYNTAX_ERROR, "SYNTAX_ERROR", "Malformed request encountered" },
    { UCI_STATUS_INVALID_PARAM, "INVALID_PARAM", "Invalid parameter provided" },
    { UCI_STATUS_INVALID_RANGE, "INVALID_RANGE", "Parameter out of allowed range" },
    { UCI_STATUS_INVALID_MSG_SIZE, "INVALID_MSG_SIZE", "Message size not supported" },
    { UCI_STATUS_UNKNOWN_GID, "UNKNOWN_GID", "Unsupported group identifier" },
    { UCI_STATUS_UNKNOWN_OID, "UNKNOWN_OID", "Unsupported opcode identifier" },
    { UCI_STATUS_READ_ONLY, "READ_ONLY", "Attempt to set a read-only value" },
    { UCI_STATUS_COMMAND_RETRY, "COMMAND_RETRY", "Command should be retried" },
    { UCI_STATUS_UNKNOWN, "STATUS_RFU_0B", "Reserved FiRa status" },
    { UCI_STATUS_NOT_APPLICABLE, "STATUS_RFU_0C", "Reserved FiRa status" },
    { UCI_STATUS_SESSION_NOT_EXIST, "SESSION_NOT_EXIST", "Referenced session does not exist" },
    { UCI_STATUS_SESSION_DUPLICATE, "SESSION_DUPLICATE", "Session already exists" },
    { UCI_STATUS_SESSION_ACTIVE, "SESSION_ACTIVE", "Session already active" },
    { UCI_STATUS_MAX_SESSIONS_EXCEEDED, "MAX_SESSIONS_EXCEEDED", "No more sessions can be created" },
    { UCI_STATUS_SESSION_NOT_CONFIGURED, "SESSION_NOT_CONFIGURED", "Session lacks configuration" },
    { UCI_STATUS_ACTIVE_SESSIONS_ONGOING, "ACTIVE_SESSIONS_ONGOING", "Conflicting active session detected" },
    { UCI_STATUS_MULTICAST_LIST_FULL, "MULTICAST_LIST_FULL", "Multicast list reached capacity" },
    { UCI_STATUS_ADDRESS_NOT_FOUND, "ADDRESS_NOT_FOUND", "Target address not found" },
    { UCI_STATUS_ADDRESS_ALREADY_PRESENT, "ADDRESS_ALREADY_PRESENT", "Address already present" },
    { UCI_STATUS_ERROR_UWB_INITIATION_TIME_TOO_OLD, "UWB_INITIATION_TIME_TOO_OLD", "Initiation time too old" },
    { UCI_STATUS_OK_NEGATIVE_DISTANCE_REPORT, "OK_NEGATIVE_DISTANCE_REPORT", "Negative distance reported" },
    { UCI_STATUS_RANGING_TX_FAILED, "RANGING_TX_FAILED", "Transmission failed during ranging" },
    { UCI_STATUS_RANGING_RX_TIMEOUT, "RANGING_RX_TIMEOUT", "Timed out waiting for ranging frame" },
    { UCI_STATUS_RANGING_RX_PHY_DEC_FAILED, "RANGING_RX_PHY_DEC_FAILED", "PHY decode failure" },
    { UCI_STATUS_RANGING_RX_PHY_TOA_FAILED, "RANGING_RX_PHY_TOA_FAILED", "PHY ToA extraction failure" },
    { UCI_STATUS_RANGING_RX_PHY_STS_FAILED, "RANGING_RX_PHY_STS_FAILED", "PHY STS validation failure" },
    { UCI_STATUS_RANGING_RX_MAC_DEC_FAILED, "RANGING_RX_MAC_DEC_FAILED", "MAC decode failure" },
    { UCI_STATUS_RANGING_RX_MAC_IE_DEC_FAILED, "RANGING_RX_MAC_IE_DEC_FAILED", "MAC IE decode failure" },
    { UCI_STATUS_RANGING_RX_MAC_IE_MISSING, "RANGING_RX_MAC_IE_MISSING", "Expected MAC IE missing" },
    { UCI_STATUS_ERROR_ROUND_INDEX_NOT_ACTIVATED, "ROUND_INDEX_NOT_ACTIVATED", "Requested round index not active" },
    { UCI_STATUS_ERROR_NUMBER_OF_ACTIVE_RANGING_ROUNDS_EXCEEDED, "ACTIVE_RANGING_ROUNDS_EXCEEDED", "Too many active ranging rounds" },
    { UCI_STATUS_ERROR_DL_TDOA_DEVICE_ADDRESS_NOT_MATCHING_IN_REPLY_TIME_LIST, "DEVICE_ADDRESS_MISMATCH_REPLY_TIME_LIST", "Device address mismatch in reply-time list" },
    { 0x2B, "STATUS_RFU_2B", "Reserved FiRa status" },
    { 0x2C, "STATUS_RFU_2C", "Reserved FiRa status" },
    { 0x2D, "STATUS_RFU_2D", "Reserved FiRa status" },
    { 0x2E, "STATUS_RFU_2E", "Reserved FiRa status" },
    { 0x2F, "STATUS_RFU_2F", "Reserved FiRa status" },
    { 0x30, "STATUS_RFU_30", "Reserved FiRa status" },
    { 0x31, "STATUS_RFU_31", "Reserved FiRa status" },
    { 0x32, "STATUS_RFU_32", "Reserved FiRa status" },
    { 0x33, "STATUS_RFU_33", "Reserved FiRa status" },
    { 0x34, "STATUS_RFU_34", "Reserved FiRa status" },
    { 0x35, "STATUS_RFU_35", "Reserved FiRa status" },
    { 0x36, "STATUS_RFU_36", "Reserved FiRa status" },
    { 0x37, "STATUS_RFU_37", "Reserved FiRa status" },
    { 0x38, "STATUS_RFU_38", "Reserved FiRa status" },
    { 0x39, "STATUS_RFU_39", "Reserved FiRa status" },
    { 0x3A, "STATUS_RFU_3A", "Reserved FiRa status" },
    { 0x3B, "STATUS_RFU_3B", "Reserved FiRa status" },
    { 0x3C, "STATUS_RFU_3C", "Reserved FiRa status" },
    { 0x3D, "STATUS_RFU_3D", "Reserved FiRa status" },
    { 0x3E, "STATUS_RFU_3E", "Reserved FiRa status" },
    { 0x3F, "STATUS_RFU_3F", "Reserved FiRa status" },
    { 0x40, "STATUS_RFU_40", "Reserved FiRa status" },
    { 0x41, "STATUS_RFU_41", "Reserved FiRa status" },
    { 0x42, "STATUS_RFU_42", "Reserved FiRa status" },
    { 0x43, "STATUS_RFU_43", "Reserved FiRa status" },
    { 0x44, "STATUS_RFU_44", "Reserved FiRa status" },
    { 0x45, "STATUS_RFU_45", "Reserved FiRa status" },
    { 0x46, "STATUS_RFU_46", "Reserved FiRa status" },
    { 0x47, "STATUS_RFU_47", "Reserved FiRa status" },
    { 0x48, "STATUS_RFU_48", "Reserved FiRa status" },
    { 0x49, "STATUS_RFU_49", "Reserved FiRa status" },
    { 0x4A, "STATUS_RFU_4A", "Reserved FiRa status" },
    { 0x4B, "STATUS_RFU_4B", "Reserved FiRa status" },
    { 0x4C, "STATUS_RFU_4C", "Reserved FiRa status" },
    { 0x4D, "STATUS_RFU_4D", "Reserved FiRa status" },
    { 0x4E, "STATUS_RFU_4E", "Reserved FiRa status" },
    { 0x4F, "STATUS_RFU_4F", "Reserved FiRa status" },
    { 0x50, "ERROR_SE_BUSY", "Secure element busy" },
    { 0x51, "ERROR_CCC_LIFE_CYCLE", "CCC lifecycle error" },
    { 0xFF, "STATUS_VENDOR_SPECIFIC", "Vendor-specific status code" },
};

static const uci_lookup_entry_t* find_lookup_entry(const uci_lookup_entry_t* table,
                                                   size_t count,
                                                   unsigned char value) {
    for (size_t i = 0; i < count; i++) {
        if (table[i].value == value) {
            return &table[i];
        }
    }
    return NULL;
}

const uci_lookup_entry_t* uci_lookup_device_state(unsigned char device_state) {
    return find_lookup_entry(k_device_state_table,
                             sizeof(k_device_state_table) / sizeof(k_device_state_table[0]),
                             device_state);
}

const uci_lookup_entry_t* uci_lookup_status(unsigned char status) {
    return find_lookup_entry(k_uci_status_table,
                             sizeof(k_uci_status_table) / sizeof(k_uci_status_table[0]),
                             status);
}

const uci_lookup_entry_t* uci_lookup_session_state(unsigned char session_state) {
    return find_lookup_entry(k_session_state_table,
                             sizeof(k_session_state_table) / sizeof(k_session_state_table[0]),
                             session_state);
}

const uci_lookup_entry_t* uci_lookup_session_reason(unsigned char reason_code) {
    return find_lookup_entry(k_session_reason_table,
                             sizeof(k_session_reason_table) / sizeof(k_session_reason_table[0]),
                             reason_code);
}

const uci_lookup_entry_t* uci_lookup_data_transfer_status(unsigned char status) {
    return find_lookup_entry(k_data_transfer_status_table,
                             sizeof(k_data_transfer_status_table) / sizeof(k_data_transfer_status_table[0]),
                             status);
}

const char* uci_device_state_to_string(unsigned char device_state) {
    const uci_lookup_entry_t* entry = uci_lookup_device_state(device_state);
    return entry ? entry->label : "UNKNOWN";
}

const char* uci_status_to_string(unsigned char status) {
    const uci_lookup_entry_t* entry = uci_lookup_status(status);
    return entry ? entry->label : "UNKNOWN";
}

const char* uci_status_description(unsigned char status) {
    const uci_lookup_entry_t* entry = uci_lookup_status(status);
    return entry ? entry->description : "Unknown status code";
}

const char* uci_session_state_to_string(unsigned char session_state) {
    const uci_lookup_entry_t* entry = uci_lookup_session_state(session_state);
    return entry ? entry->label : "UNKNOWN";
}

const char* uci_session_reason_to_string(unsigned char reason_code) {
    const uci_lookup_entry_t* entry = uci_lookup_session_reason(reason_code);
    return entry ? entry->label : "UNKNOWN";
}

const char* uci_session_type_to_string(unsigned char session_type) {
    switch (session_type) {
        case FIRA_RANGING_SESSION:
            return "FIRA_RANGING_SESSION";
        case FIRA_RANGING_AND_IN_BAND_DATA_SESSION:
            return "FIRA_RANGING_AND_IN_BAND_DATA_SESSION";
        case FIRA_DATA_TRANSFER_SESSION:
            return "FIRA_DATA_TRANSFER_SESSION";
        case FIRA_RANGING_ONLY_PHASE:
            return "FIRA_RANGING_ONLY_PHASE";
        case FIRA_IN_BAND_DATA_PHASE:
            return "FIRA_IN_BAND_DATA_PHASE";
        case FIRA_RANGING_WITH_DATA_PHASE:
            return "FIRA_RANGING_WITH_DATA_PHASE";
        case CCC_RANGING_SESSION:
            return "CCC_RANGING_SESSION";
        case DEVICE_TEST_MODE:
            return "DEVICE_TEST_MODE";
        default:
            return "UNKNOWN";
    }
}

unsigned char* create_uci_packet(
    unsigned char mt, 
    unsigned char pbf, 
    unsigned char gid, 
    unsigned char oid,
    const unsigned char* payload,
    size_t payload_len,
    size_t* packet_len) {
    
    *packet_len = sizeof(struct uci_packet_header) + payload_len;
    unsigned char* packet = malloc(*packet_len);
    if (!packet) {
        return NULL;
    }
    
    struct uci_packet_header* header = (struct uci_packet_header*)packet;
    create_uci_header(header, mt, pbf, gid, oid, (unsigned char)payload_len);
    
    if (payload && payload_len > 0) {
        memcpy(packet + sizeof(struct uci_packet_header), payload, payload_len);
    }
    
    return packet;
}

void create_uci_header(
    struct uci_packet_header* header,
    unsigned char mt,
    unsigned char pbf,
    unsigned char gid,
    unsigned char oid,
    unsigned char payload_len) {
    if (!header) {
        return;
    }

    set_header_values_safe(header, mt, pbf, gid, oid, payload_len);
}

unsigned char* create_session_init_packet(
    uint32_t session_id,
    unsigned char session_type,
    size_t* packet_len) {
    
    unsigned char payload[5];
    write_u32_le(payload, session_id);      // session_id in little-endian
    payload[4] = session_type;
    
    return create_uci_packet(COMMAND, COMPLETE, SESSION_CONFIG, SESSION_INIT, 
                            payload, sizeof(payload), packet_len);
}

unsigned char* create_session_deinit_packet(
    uint32_t session_id,
    size_t* packet_len) {
    
    unsigned char payload[4];
    write_u32_le(payload, session_id);      // session_id in little-endian
    
    return create_uci_packet(COMMAND, COMPLETE, SESSION_CONFIG, SESSION_DEINIT, 
                            payload, sizeof(payload), packet_len);
}

unsigned char* create_session_start_packet(
    uint32_t session_id,
    size_t* packet_len) {
    
    unsigned char payload[4];
    write_u32_le(payload, session_id);      // session_id in little-endian
    
    return create_uci_packet(COMMAND, COMPLETE, SESSION_CONTROL, SESSION_START, 
                            payload, sizeof(payload), packet_len);
}

unsigned char* create_session_stop_packet(
    uint32_t session_id,
    size_t* packet_len) {
    
    unsigned char payload[4];
    write_u32_le(payload, session_id);      // session_id in little-endian
    
    return create_uci_packet(COMMAND, COMPLETE, SESSION_CONTROL, SESSION_STOP, 
                            payload, sizeof(payload), packet_len);
}

unsigned char* create_get_session_state_packet(
    uint32_t session_id,
    size_t* packet_len) {
    
    unsigned char payload[4];
    write_u32_le(payload, session_id);      // session_id in little-endian
    
    return create_uci_packet(COMMAND, COMPLETE, SESSION_CONFIG, SESSION_GET_STATE, 
                            payload, sizeof(payload), packet_len);
}

unsigned char* create_get_device_info_packet(
    size_t* packet_len) {
    
    return create_uci_packet(COMMAND, COMPLETE, CORE, CORE_DEVICE_INFO, 
                            NULL, 0, packet_len);
}

unsigned char* create_device_reset_packet(
    uint8_t reset_config,
    size_t* packet_len) {
    
    unsigned char payload[] = {reset_config};
    return create_uci_packet(COMMAND, COMPLETE, CORE, CORE_DEVICE_RESET, 
                            payload, sizeof(payload), packet_len);
}

unsigned char* create_get_caps_info_packet(
    size_t* packet_len) {
    
    return create_uci_packet(COMMAND, COMPLETE, CORE, CORE_GET_CAPS_INFO, 
                            NULL, 0, packet_len);
}

unsigned char* create_set_config_packet(
    uint8_t num_configs,
    const unsigned char* configs,
    size_t configs_len,
    size_t* packet_len) {

    unsigned char payload[256];

    if (configs_len > 0 && !configs) {
        return NULL;
    }

    if (configs_len + 1 > sizeof(payload)) {
        return NULL;
    }

    payload[0] = num_configs;
    memcpy(payload + 1, configs, configs_len);
    
    return create_uci_packet(COMMAND, COMPLETE, CORE, CORE_SET_CONFIG, 
                            payload, configs_len + 1, packet_len);
}

unsigned char* create_get_config_packet(
    uint8_t num_configs,
    const unsigned char* config_ids,
    size_t config_ids_len,
    size_t* packet_len) {

    unsigned char payload[256];

    if (config_ids_len > 0 && !config_ids) {
        return NULL;
    }

    if (config_ids_len + 1 > sizeof(payload)) {
        return NULL;
    }

    payload[0] = num_configs;
    memcpy(payload + 1, config_ids, config_ids_len);

    return create_uci_packet(COMMAND, COMPLETE, CORE, CORE_GET_CONFIG,
                             payload, config_ids_len + 1, packet_len);
}

size_t uci_build_data_message_snd_payload(unsigned char *buffer,
                                          size_t capacity,
                                          uint32_t session_identifier,
                                          uint64_t destination_address,
                                          uint16_t sequence_number,
                                          const unsigned char *app_data,
                                          size_t app_data_len) {
    if (!buffer) {
        return 0;
    }

    if (capacity < UCI_DATA_MESSAGE_SND_HEADER) {
        return 0;
    }

    if (app_data_len > 0 && !app_data) {
        return 0;
    }

    if (app_data_len > 0xFFFF) {
        return 0;
    }

    if (UCI_DATA_MESSAGE_SND_HEADER + app_data_len > capacity) {
        return 0;
    }

    struct uci_payload_builder builder;
    uci_payload_builder_init(&builder, buffer, capacity);

    if (uci_payload_builder_put_u32_le(&builder, session_identifier) < 0) {
        return 0;
    }
    if (uci_payload_builder_put_u64_le(&builder, destination_address) < 0) {
        return 0;
    }
    if (uci_payload_builder_put_u16_le(&builder, sequence_number) < 0) {
        return 0;
    }
    if (uci_payload_builder_put_u16_le(&builder, (uint16_t)app_data_len) < 0) {
        return 0;
    }
    if (uci_payload_builder_put_mem(&builder, app_data, app_data_len) < 0) {
        return 0;
    }

    return uci_payload_builder_length(&builder);
}
