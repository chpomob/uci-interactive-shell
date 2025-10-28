#ifndef UCI_PACKET_STRUCTURES_H
#define UCI_PACKET_STRUCTURES_H

#include <stdint.h>
#include <stddef.h>
#include "uci_pdl.h"
#include "uci_utils.h"

// Forward declaration
struct uci_payload_format_def;

// Type definitions for field types
typedef enum {
    FIELD_TYPE_U8,
    FIELD_TYPE_U16,
    FIELD_TYPE_U32,
    FIELD_TYPE_U64,
    FIELD_TYPE_BYTES,
    FIELD_TYPE_ARRAY,
    FIELD_TYPE_TLV
} uci_field_type_t;

// Definition for a single field in a payload format
typedef struct {
    uci_field_type_t field_type;
    size_t offset;
    size_t size;
    const char* name;
    uint8_t min_val;  // For validation
    uint8_t max_val;  // For validation
    uint8_t required; // Whether field is required
} uci_field_def_t;

// Definition for a complete payload format structure
typedef struct {
    const uci_field_def_t* fields;
    size_t num_fields;
    size_t min_size;
    size_t max_size;
} uci_payload_format_def_t;

// Structure for session init command/response
typedef struct {
    uint32_t session_id;
    uint8_t session_type;
} uci_session_init_payload_t;

// Structure for session deinit command/response
typedef struct {
    uint32_t session_id;
} uci_session_deinit_payload_t;

// Structure for session start/stop commands/responses
typedef struct {
    uint32_t session_id;
} uci_session_control_payload_t;

// Structure for session get state command/response
typedef struct {
    uint32_t session_id;
} uci_session_get_state_payload_t;

// Structure for session get ranging count response
typedef struct {
    uint32_t session_id;
    uint16_t ranging_count;
} uci_session_get_ranging_count_payload_t;

// Structure for set/get app config
typedef struct {
    uint32_t session_id;
    uint8_t num_tlvs;
    // Followed by TLV data in actual payload
} uci_session_app_config_payload_t;

// Structure for logical link operations
typedef struct {
    uint32_t session_id;
    uint8_t link_id;
    uint8_t mode;
} uci_session_logical_link_payload_t;

// Structure for logical link get param
typedef struct {
    uint32_t session_id;
    uint8_t link_id;
    uint8_t param_id;  // What parameter to get
} uci_session_logical_link_get_param_payload_t;

// Structure for controller multicast list
typedef struct {
    uint32_t session_id;
    uint8_t action;
    uint8_t address_type;
    uint8_t address[8];  // 8 bytes for extended address
    uint8_t subsession_id;
    uint8_t key_length;
    uint8_t key[32];  // Max key length
} uci_session_multicast_payload_t;

// Structure for data transfer phase config
typedef struct {
    uint32_t session_id;
    uint8_t dtp_repetition;
    uint8_t dtp_control;
    uint8_t dtp_size;
    uint8_t dtp_payload[64];  // Max payload size
} uci_session_dtp_config_payload_t;

// Structure for hybrid usage support (HUS)
typedef struct {
    uint32_t session_id;
    uint32_t hus_primary_session_id;
    uint8_t hus_role;
    uint8_t reserved;
    uint16_t hus_config_length;
    uint8_t hus_config_data[250];  // Max config data
} uci_session_hus_config_payload_t;

// Structure for range data notification
typedef struct {
    uint32_t session_token;
    uint8_t reason_code;
    uint8_t status;
    uint8_t nlos;
    uint16_t distance;
    uint16_t aoa_azimuth;
    uint8_t aoa_azimuth_fom;
    uint16_t aoa_elevation;
    uint8_t aoa_elevation_fom;
    uint16_t aoa_destination_azimuth;
    uint8_t aoa_destination_azimuth_fom;
    uint16_t aoa_destination_elevation;
    uint8_t aoa_destination_elevation_fom;
    uint8_t slot_index;
    uint8_t rssi;
    uint8_t resolvable_mac_used;
    uint8_t mac_address[8];
} uci_range_data_ntf_payload_t;

// Core command/response structures
typedef struct {
    uint8_t status;
    uint16_t uci_version;
    uint16_t mac_version;
    uint8_t mac_ext;
    uint16_t phy_version;
    uint16_t phy_ext;
    uint16_t vendor_id;
    uint16_t uci_version_spec;
} uci_core_device_info_rsp_t;

typedef struct {
    uint8_t status;
    uint32_t session_id;
} uci_core_generic_rsp_t;

// Function pointer types for serialization/deserialization
typedef uci_error_t (*uci_serialize_fn_t)(const void* src, unsigned char* dst, size_t dst_size);
typedef uci_error_t (*uci_deserialize_fn_t)(const unsigned char* src, size_t src_size, void* dst);
typedef uci_error_t (*uci_validate_fn_t)(const void* data);

// Unified packet processing context
typedef struct {
    uint8_t mt;           // Message type
    uint8_t pbf;          // Packet boundary flag
    uint8_t gid;          // Group ID
    uint8_t oid;          // Opcode ID
    const uci_payload_format_def_t* format_def;
    uci_serialize_fn_t serialize_fn;
    uci_deserialize_fn_t deserialize_fn;
    uci_validate_fn_t validate_fn;
} uci_packet_ctx_t;

// Exported function declarations
extern const uci_payload_format_def_t* uci_get_payload_format(uint8_t gid, uint8_t oid);
extern const uci_packet_ctx_t* uci_get_packet_context(uint8_t gid, uint8_t oid);

// Serialization functions
extern uci_error_t uci_serialize_session_init(const void* src, unsigned char* dst, size_t dst_size);
extern uci_error_t uci_serialize_session_deinit(const void* src, unsigned char* dst, size_t dst_size);
extern uci_error_t uci_serialize_session_control(const void* src, unsigned char* dst, size_t dst_size);
extern uci_error_t uci_serialize_session_get_state(const void* src, unsigned char* dst, size_t dst_size);
extern uci_error_t uci_serialize_session_get_ranging_count(const void* src, unsigned char* dst, size_t dst_size);
extern uci_error_t uci_serialize_session_app_config(const void* src, unsigned char* dst, size_t dst_size);
extern uci_error_t uci_serialize_session_logical_link(const void* src, unsigned char* dst, size_t dst_size);
extern uci_error_t uci_serialize_session_multicast(const void* src, unsigned char* dst, size_t dst_size);
extern uci_error_t uci_serialize_session_dtp_config(const void* src, unsigned char* dst, size_t dst_size);
extern uci_error_t uci_serialize_session_hus_config(const void* src, unsigned char* dst, size_t dst_size);

// Deserialization functions
extern uci_error_t uci_deserialize_session_init(const unsigned char* src, size_t src_size, void* dst);
extern uci_error_t uci_deserialize_session_deinit(const unsigned char* src, size_t src_size, void* dst);
extern uci_error_t uci_deserialize_session_control(const unsigned char* src, size_t src_size, void* dst);
extern uci_error_t uci_deserialize_session_get_state(const unsigned char* src, size_t src_size, void* dst);
extern uci_error_t uci_deserialize_session_get_ranging_count(const unsigned char* src, size_t src_size, void* dst);
extern uci_error_t uci_deserialize_session_app_config(const unsigned char* src, size_t src_size, void* dst);
extern uci_error_t uci_deserialize_session_logical_link(const unsigned char* src, size_t src_size, void* dst);
extern uci_error_t uci_deserialize_session_multicast(const unsigned char* src, size_t src_size, void* dst);
extern uci_error_t uci_deserialize_session_dtp_config(const unsigned char* src, size_t src_size, void* dst);
extern uci_error_t uci_deserialize_session_hus_config(const unsigned char* src, size_t src_size, void* dst);

// Validation functions
extern uci_error_t uci_validate_session_init(const void* data);
extern uci_error_t uci_validate_session_deinit(const void* data);
extern uci_error_t uci_validate_session_control(const void* data);
extern uci_error_t uci_validate_session_get_state(const void* data);
extern uci_error_t uci_validate_session_get_ranging_count(const void* data);
extern uci_error_t uci_validate_session_app_config(const void* data);
extern uci_error_t uci_validate_session_logical_link(const void* data);
extern uci_error_t uci_validate_session_multicast(const void* data);
extern uci_error_t uci_validate_session_dtp_config(const void* data);
extern uci_error_t uci_validate_session_hus_config(const void* data);

#endif // UCI_PACKET_STRUCTURES_H