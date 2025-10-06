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
    
    header->first_byte = gid | (pbf << 4) | (mt << 5);
    header->second_byte = oid & 0x3F;  // opcode occupies lower 6 bits
    header->reserved2 = 0;
    header->payload_len = payload_len;
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