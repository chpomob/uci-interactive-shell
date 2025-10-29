#include "uci_packet_utils.h"
#include <stdlib.h>
#include <string.h>

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
