#ifndef UCI_H
#define UCI_H

#include "uci_pdl.h"
#include "uci_utils.h"  // Include utilities for secure operations
#include "uci_types.h"  // Include standardized type definitions
#include "uci_logging.h"  // Include logging system
#include <stddef.h>

// UCI Packet Header - Cherry-compatible common packet header encoding.
// For COMMAND/RESPONSE/NOTIFICATION:
// Byte 0: [GID:4][PBF:1][MT:3]
// Byte 1: [Opcode:6][R:2]
// Byte 2: Reserved
// Byte 3: Payload Length (8-bit)
// For DATA and SE_TESTING:
// Byte 2-3: Payload Length (16-bit little-endian)
struct uci_packet_header {
    uci_uint8 first_byte;   // GID | (PBF << 4) | (MT << 5)
    uci_uint8 second_byte;  // Opcode in bits[5:0], reserved bits[7:6]
    uci_uint8 third_byte;   // Reserved for control packets, length low byte for DATA/SE
    uci_uint8 fourth_byte;  // Length byte for control, length high byte for DATA/SE
};

#define UCI_MAX_CONTROL_PAYLOAD_SIZE 255
#define UCI_MAX_DATA_PACKET_PAYLOAD_SIZE 255
#define UCI_MAX_DATA_MESSAGE_PAYLOAD_SIZE 0xFFFFu
#define UCI_DATA_MESSAGE_SND_HEADER 16
#define UCI_MAX_APPLICATION_DATA_PAYLOAD_SIZE \
    (UCI_MAX_DATA_MESSAGE_PAYLOAD_SIZE - UCI_DATA_MESSAGE_SND_HEADER)
#define UCI_MAX_APPLICATION_DATA_FIRST_SEGMENT \
    (UCI_MAX_DATA_PACKET_PAYLOAD_SIZE - UCI_DATA_MESSAGE_SND_HEADER)

enum uci_data_packet_format {
    DATA_PACKET_FORMAT_SEND = 0x01,
    DATA_PACKET_FORMAT_RECEIVE = 0x02,
    DATA_PACKET_FORMAT_LL_SEND = 0x03,
    DATA_PACKET_FORMAT_LL_RECEIVE = 0x04,
    DATA_PACKET_FORMAT_RADAR = 0x0F,
};

// UCI Session context structure for tracking session state
#define MAX_SESSIONS 10
#define MAX_SESSION_CONFIGS 32
#define MAX_SESSION_CONFIG_VALUE_SIZE 255
#define MAX_MULTICAST_CONTROLEES 16
#define MAX_DT_TAG_ROUNDS 32
#define MAX_LOGICAL_LINKS 8

typedef struct {
    uci_uint16 short_address;
    uci_uint32 subsession_id;
    uci_uint8 key_len;
    uci_uint8 key[32];
} uci_multicast_entry;

typedef struct {
    uci_uint8 cfg_id;
    uci_uint8 length;
    uci_uint8 value[MAX_SESSION_CONFIG_VALUE_SIZE];
    uci_uint8 in_use;
} uci_session_config_entry;

typedef struct {
    uci_uint8 link_id;
    uci_uint8 mode;
    uci_uint8 credit;
    uci_uint8 active;
} uci_logical_link_entry;

struct uci_session {
    uci_uint32 session_id;
    SessionType session_type;
    uci_uint8 session_state;  // Using uci_uint8 to avoid direct dependency
    uci_uint8 is_allocated;  // 1 if session slot is in use, 0 otherwise
    uci_uint32 session_handle;  // Simulated UWBS-generated session handle
    uci_uint16 ranging_count; // Tracks completed ranging rounds
    uci_session_config_entry configs[MAX_SESSION_CONFIGS];
    int num_configs;            // Number of stored configurations
    uci_multicast_entry multicast_entries[MAX_MULTICAST_CONTROLEES];
    uci_uint8 multicast_count;
    uci_uint8 dt_tag_round_indexes[MAX_DT_TAG_ROUNDS];
    uci_uint8 dt_tag_round_count;
    uci_uint8 dtp_repetition;
    uci_uint8 dtp_control;
    uci_uint8 dtp_size;
    uci_uint8 dtp_payload[64];
    uci_uint8 dtp_payload_len;
    uci_logical_link_entry logical_links[MAX_LOGICAL_LINKS];
    uci_uint8 logical_link_count;
    uint16_t last_data_sequence;
    uint16_t last_data_length;
    uint64_t last_data_destination;
    uci_uint8 last_data_preview[64];
    uci_uint8 last_data_preview_len;
};

// Global session storage
extern struct uci_session uci_sessions[MAX_SESSIONS];

// Helper functions to properly set up and decode the header
static inline uci_uint8 uci_pack_first_byte(uci_uint8 message_type,
                                            uci_uint8 packet_boundary,
                                            uci_uint8 group_id) {
    return (uci_uint8)((group_id & 0x0F) |
                       ((packet_boundary & 0x01) << 4) |
                       ((message_type & 0x07) << 5));
}

static inline uci_uint8 uci_pack_second_byte(uci_uint8 opcode_id) {
    return (uci_uint8)(opcode_id & 0x3F);  // opcode occupies lower 6 bits
}

static inline int uci_mt_uses_u16_payload_length(uci_uint8 message_type) {
    return message_type == DATA;
}

static inline uci_uint16 uci_get_message_max_payload_length(uci_uint8 message_type) {
    return uci_mt_uses_u16_payload_length(message_type) ? 0xFFFFu : UCI_MAX_CONTROL_PAYLOAD_SIZE;
}

static inline uci_uint16 uci_get_payload_length_from_header_bytes(uci_uint8 message_type,
                                                                  uci_uint8 third_byte,
                                                                  uci_uint8 fourth_byte) {
    if (uci_mt_uses_u16_payload_length(message_type)) {
        return (uci_uint16)((((uci_uint16)fourth_byte) << 8) | third_byte);
    }

    return fourth_byte;
}

static inline void uci_set_payload_length_in_header_bytes(uci_uint8 message_type,
                                                          uci_uint16 payload_length,
                                                          uci_uint8 *third_byte,
                                                          uci_uint8 *fourth_byte) {
    if (!third_byte || !fourth_byte) {
        return;
    }

    if (uci_mt_uses_u16_payload_length(message_type)) {
        *third_byte = (uci_uint8)(payload_length & 0xFF);
        *fourth_byte = (uci_uint8)((payload_length >> 8) & 0xFF);
    } else {
        *third_byte = 0;
        *fourth_byte = (uci_uint8)(payload_length & 0xFF);
    }
}

static inline uci_error_t set_header_values_safe(struct uci_packet_header *header,
                                                 uci_uint8 message_type,
                                                 uci_uint8 packet_boundary,
                                                 uci_uint8 group_id,
                                                 uci_uint8 opcode_id,
                                                 uci_uint16 payload_length) {
    if (!header) {
        return UCI_ERROR_INVALID_PARAM;
    }

    if (payload_length > uci_get_message_max_payload_length(message_type)) {
        return UCI_ERROR_INVALID_PARAM;
    }
    
    header->first_byte = uci_pack_first_byte(message_type, packet_boundary, group_id);
    header->second_byte = uci_pack_second_byte(opcode_id);
    uci_set_payload_length_in_header_bytes(message_type,
                                           payload_length,
                                           &header->third_byte,
                                           &header->fourth_byte);
    
    return UCI_SUCCESS;
}

// Helper functions to extract header values
static inline uci_uint8 get_gid(const struct uci_packet_header *header) {
    return header->first_byte & 0x0F;
}

static inline uci_uint8 get_pbf(const struct uci_packet_header *header) {
    return (header->first_byte >> 4) & 0x01;
}

static inline uci_uint8 get_mt(const struct uci_packet_header *header) {
    return (header->first_byte >> 5) & 0x07;
}

static inline uci_uint8 get_opcode(const struct uci_packet_header *header) {
    return header->second_byte & 0x3F;
}

static inline uci_uint8 get_reserved_opcode_bits(const struct uci_packet_header *header) {
    return (header->second_byte >> 6) & 0x03;
}

typedef struct {
    uci_uint8 message_type;
    uci_uint8 packet_boundary;
    uci_uint8 group_id;
    uci_uint8 opcode_id;
    uci_uint8 reserved_opcode_bits;
    uci_uint16 payload_length;
} uci_header_fields_t;

static inline uci_error_t uci_extract_header_fields_safe(const struct uci_packet_header *header,
                                                         uci_header_fields_t *out_fields) {
    if (!header || !out_fields) {
        return UCI_ERROR_INVALID_PARAM;
    }

    out_fields->message_type = get_mt(header);
    out_fields->packet_boundary = get_pbf(header);
    out_fields->group_id = get_gid(header);
    out_fields->opcode_id = get_opcode(header);
    out_fields->reserved_opcode_bits = get_reserved_opcode_bits(header);
    out_fields->payload_length = uci_get_payload_length_from_header_bytes(out_fields->message_type,
                                                                          header->third_byte,
                                                                          header->fourth_byte);
    
    return UCI_SUCCESS;
}

// UCI packet analysis function
void analyze_uci_packet(unsigned char* packet, size_t packet_len);

// Hardware mode functions
void uci_enable_hardware_mode(const char* device_path);
void uci_disable_hardware_mode();
int uci_is_hardware_mode_enabled();
const char* uci_get_hardware_device_path();

// Payload decoding functions
// CORE group decoders
void decode_core_device_info_rsp(unsigned char* payload, int payload_len);
void decode_core_get_caps_info_rsp(unsigned char* payload, int payload_len);
void decode_core_set_config_rsp(unsigned char* payload, int payload_len);
void decode_core_get_config_rsp(unsigned char* payload, int payload_len);
void decode_core_device_reset_rsp(unsigned char* payload, int payload_len);
void decode_core_device_suspend_rsp(unsigned char* payload, int payload_len);
void decode_core_query_uwbs_timestamp_rsp(unsigned char* payload, int payload_len);

// SESSION_CONFIG group decoders
void decode_session_init_cmd(unsigned char* payload, int payload_len);
void decode_session_init_rsp(unsigned char* payload, int payload_len);
void decode_session_deinit_rsp(unsigned char* payload, int payload_len);
void decode_session_set_app_config_rsp(unsigned char* payload, int payload_len);
void decode_session_get_app_config_rsp(unsigned char* payload, int payload_len);
void decode_session_get_count_rsp(unsigned char* payload, int payload_len);
void decode_session_get_state_rsp(unsigned char* payload, int payload_len);

// SESSION_CONTROL group decoders
void decode_session_start_rsp(unsigned char* payload, int payload_len);
void decode_session_stop_rsp(unsigned char* payload, int payload_len);
void decode_session_get_ranging_count_rsp(unsigned char* payload, int payload_len);

// CORE notification decoders
void decode_core_device_status_ntf(unsigned char* payload, int payload_len);
void decode_core_generic_error_ntf(unsigned char* payload, int payload_len);

// SESSION_CONFIG notification decoders
void decode_session_status_ntf(unsigned char* payload, int payload_len);

// SESSION_CONTROL notification decoders
void decode_session_info_ntf(unsigned char* payload, int payload_len);
void decode_session_data_credit_ntf(unsigned char* payload, int payload_len);
void decode_session_data_transfer_status_ntf(unsigned char* payload, int payload_len);

// VENDOR_ANDROID notification decoders
void decode_android_range_diagnostics_ntf(unsigned char* payload, int payload_len);

// RANGING_DATA notification decoders
void decode_range_data_ntf(unsigned char* payload, int payload_len);

void uci_send_data_message(uint32_t identifier,
                           uint64_t destination_address,
                           uint16_t sequence_number,
                           const unsigned char *app_data,
                           size_t app_data_len);

#include "uci_command_utils.h"
#endif // UCI_H
