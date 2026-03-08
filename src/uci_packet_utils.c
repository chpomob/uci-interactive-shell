#include "uci_packet_utils.h"
#include <stdlib.h>
#include <string.h>

const char* uci_device_state_to_string(unsigned char device_state) {
    switch (device_state) {
        case DEVICE_STATE_READY:
            return "READY";
        case DEVICE_STATE_ACTIVE:
            return "ACTIVE";
        case DEVICE_STATE_ERROR:
            return "ERROR";
        default:
            return "UNKNOWN";
    }
}

const char* uci_status_to_string(unsigned char status) {
    switch (status) {
        case UCI_STATUS_OK:
            return "OK";
        case UCI_STATUS_REJECTED:
            return "REJECTED";
        case UCI_STATUS_FAILED:
            return "FAILED";
        case UCI_STATUS_SYNTAX_ERROR:
            return "SYNTAX_ERROR";
        case UCI_STATUS_INVALID_PARAM:
            return "INVALID_PARAM";
        case UCI_STATUS_INVALID_RANGE:
            return "INVALID_RANGE";
        case UCI_STATUS_INVALID_MSG_SIZE:
            return "INVALID_MSG_SIZE";
        case UCI_STATUS_UNKNOWN_GID:
            return "UNKNOWN_GID";
        case UCI_STATUS_UNKNOWN_OID:
            return "UNKNOWN_OID";
        case UCI_STATUS_READ_ONLY:
            return "READ_ONLY";
        case UCI_STATUS_COMMAND_RETRY:
            return "COMMAND_RETRY";
        case UCI_STATUS_SESSION_NOT_EXIST:
            return "SESSION_NOT_EXIST";
        case UCI_STATUS_SESSION_DUPLICATE:
            return "SESSION_DUPLICATE";
        case UCI_STATUS_SESSION_ACTIVE:
            return "SESSION_ACTIVE";
        case UCI_STATUS_MAX_SESSIONS_EXCEEDED:
            return "MAX_SESSIONS_EXCEEDED";
        case UCI_STATUS_SESSION_NOT_CONFIGURED:
            return "SESSION_NOT_CONFIGURED";
        case UCI_STATUS_ACTIVE_SESSIONS_ONGOING:
            return "ACTIVE_SESSIONS_ONGOING";
        case UCI_STATUS_MULTICAST_LIST_FULL:
            return "MULTICAST_LIST_FULL";
        case UCI_STATUS_ADDRESS_NOT_FOUND:
            return "ADDRESS_NOT_FOUND";
        case UCI_STATUS_ADDRESS_ALREADY_PRESENT:
            return "ADDRESS_ALREADY_PRESENT";
        case UCI_STATUS_ERROR_UWB_INITIATION_TIME_TOO_OLD:
            return "UWB_INITIATION_TIME_TOO_OLD";
        case UCI_STATUS_OK_NEGATIVE_DISTANCE_REPORT:
            return "OK_NEGATIVE_DISTANCE_REPORT";
        case UCI_STATUS_RANGING_TX_FAILED:
            return "RANGING_TX_FAILED";
        case UCI_STATUS_RANGING_RX_TIMEOUT:
            return "RANGING_RX_TIMEOUT";
        case UCI_STATUS_RANGING_RX_PHY_DEC_FAILED:
            return "RANGING_RX_PHY_DEC_FAILED";
        case UCI_STATUS_RANGING_RX_PHY_TOA_FAILED:
            return "RANGING_RX_PHY_TOA_FAILED";
        case UCI_STATUS_RANGING_RX_PHY_STS_FAILED:
            return "RANGING_RX_PHY_STS_FAILED";
        case UCI_STATUS_RANGING_RX_MAC_DEC_FAILED:
            return "RANGING_RX_MAC_DEC_FAILED";
        case UCI_STATUS_RANGING_RX_MAC_IE_DEC_FAILED:
            return "RANGING_RX_MAC_IE_DEC_FAILED";
        case UCI_STATUS_RANGING_RX_MAC_IE_MISSING:
            return "RANGING_RX_MAC_IE_MISSING";
        case UCI_STATUS_ERROR_ROUND_INDEX_NOT_ACTIVATED:
            return "ROUND_INDEX_NOT_ACTIVATED";
        case UCI_STATUS_ERROR_NUMBER_OF_ACTIVE_RANGING_ROUNDS_EXCEEDED:
            return "ACTIVE_RANGING_ROUNDS_EXCEEDED";
        case UCI_STATUS_ERROR_DL_TDOA_DEVICE_ADDRESS_NOT_MATCHING_IN_REPLY_TIME_LIST:
            return "DEVICE_ADDRESS_MISMATCH_REPLY_TIME_LIST";
        default:
            return "UNKNOWN";
    }
}

const char* uci_status_description(unsigned char status) {
    switch (status) {
        case UCI_STATUS_OK:
            return "Operation completed successfully";
        case UCI_STATUS_REJECTED:
            return "Request rejected by device";
        case UCI_STATUS_FAILED:
            return "Generic failure status";
        case UCI_STATUS_SYNTAX_ERROR:
            return "Malformed request encountered";
        case UCI_STATUS_INVALID_PARAM:
            return "Invalid parameter provided";
        case UCI_STATUS_INVALID_RANGE:
            return "Parameter value out of range";
        case UCI_STATUS_INVALID_MSG_SIZE:
            return "Message size invalid";
        case UCI_STATUS_UNKNOWN_GID:
            return "Unknown Group ID";
        case UCI_STATUS_UNKNOWN_OID:
            return "Unknown Opcode ID";
        case UCI_STATUS_READ_ONLY:
            return "Attempt to set a read-only value";
        case UCI_STATUS_COMMAND_RETRY:
            return "Command should be retried";
        case UCI_STATUS_SESSION_DUPLICATE:
            return "Session ID already exists";
        case UCI_STATUS_SESSION_NOT_EXIST:
            return "Session does not exist";
        case UCI_STATUS_SESSION_ACTIVE:
            return "Session already active";
        default:
            return "Unknown status code";
    }
}

const char* uci_session_state_to_string(unsigned char session_state) {
    switch (session_state) {
        case SESSION_STATE_INIT:
            return "INIT";
        case SESSION_STATE_DEINIT:
            return "DEINIT";
        case SESSION_STATE_ACTIVE:
            return "ACTIVE";
        case SESSION_STATE_IDLE:
            return "IDLE";
        default:
            return "UNKNOWN";
    }
}

const char* uci_session_reason_to_string(unsigned char reason_code) {
    switch (reason_code) {
        case STATE_CHANGE_WITH_SESSION_MANAGEMENT_COMMANDS:
            return "STATE_CHANGE_WITH_SESSION_MANAGEMENT_COMMANDS";
        case MAX_RANGING_ROUND_RETRY_COUNT_REACHED:
            return "MAX_RANGING_ROUND_RETRY_COUNT_REACHED";
        case MAX_NUMBER_OF_MEASUREMENTS_REACHED:
            return "MAX_NUMBER_OF_MEASUREMENTS_REACHED";
        case SESSION_RESUMED_DUE_TO_INBAND_SIGNAL:
            return "SESSION_RESUMED_DUE_TO_INBAND_SIGNAL";
        case SESSION_SUSPENDED_DUE_TO_INBAND_SIGNAL:
            return "SESSION_SUSPENDED_DUE_TO_INBAND_SIGNAL";
        case SESSION_STOPPED_DUE_TO_INBAND_SIGNAL:
            return "SESSION_STOPPED_DUE_TO_INBAND_SIGNAL";
        default:
            return "UNKNOWN";
    }
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
