#include "../include/uci_packet_structures.h"
#include "../include/uci_packet_utils.h"
#include <string.h>

// Define field definitions for session init command
static const uci_field_def_t session_init_fields[] = {
    { FIELD_TYPE_U32, 0, 4, "session_id", 0, 0xFF, 1 },
    { FIELD_TYPE_U8,  4, 1, "session_type", 0, 5, 1 }  // Limited by SessionType enum
};

// Define field definitions for session deinit command
static const uci_field_def_t session_deinit_fields[] = {
    { FIELD_TYPE_U32, 0, 4, "session_id", 0, 0xFF, 1 }
};

// Define field definitions for session control commands (start/stop)
static const uci_field_def_t session_control_fields[] = {
    { FIELD_TYPE_U32, 0, 4, "session_id", 0, 0xFF, 1 }
};

// Define field definitions for session get state command
static const uci_field_def_t session_get_state_fields[] = {
    { FIELD_TYPE_U32, 0, 4, "session_id", 0, 0xFF, 1 }
};

// Define field definitions for session get ranging count response
static const uci_field_def_t session_get_ranging_count_fields[] = {
    { FIELD_TYPE_U32, 0, 4, "session_id", 0, 0xFF, 1 },
    { FIELD_TYPE_U16, 4, 2, "ranging_count", 0, 0xFF, 1 }
};

// Define field definitions for app config operations
static const uci_field_def_t session_app_config_fields[] = {
    { FIELD_TYPE_U32, 0, 4, "session_id", 0, 0xFF, 1 },
    { FIELD_TYPE_U8,  4, 1, "num_tlvs", 0, 32, 1 }  // Limited by max config entries
    // Additional TLV data follows
};

// Define field definitions for logical link operations
static const uci_field_def_t session_logical_link_fields[] = {
    { FIELD_TYPE_U32, 0, 4, "session_id", 0, 0xFF, 1 },
    { FIELD_TYPE_U8,  4, 1, "link_id", 0, 7, 1 },   // Limited by MAX_LOGICAL_LINKS
    { FIELD_TYPE_U8,  5, 1, "mode", 0, 2, 1 }       // 0-2 for different modes
};

// Define field definitions for logical link get param
static const uci_field_def_t session_logical_link_get_param_fields[] = {
    { FIELD_TYPE_U32, 0, 4, "session_id", 0, 0xFF, 1 },
    { FIELD_TYPE_U8,  4, 1, "link_id", 0, 7, 1 },
    { FIELD_TYPE_U8,  5, 1, "param_id", 0, 0xFF, 1 }
};

// Define field definitions for multicast list operations
static const uci_field_def_t session_multicast_fields[] = {
    { FIELD_TYPE_U32, 0,  4, "session_id", 0, 0xFF, 1 },
    { FIELD_TYPE_U8,  4,  1, "action", 0, 3, 1 },      // UpdateMulticastListAction values
    { FIELD_TYPE_U8,  5,  1, "address_type", 0, 1, 1 }, // 0=short, 1=extended
    { FIELD_TYPE_BYTES, 6,  8, "address", 0, 0xFF, 1 },
    { FIELD_TYPE_U8,  14, 1, "subsession_id", 0, 0xFF, 0 },
    { FIELD_TYPE_U8,  15, 1, "key_length", 0, 32, 0 },
    { FIELD_TYPE_BYTES, 16, 32, "key", 0, 0xFF, 0 }
};

// Define field definitions for data transfer phase config
static const uci_field_def_t session_dtp_config_fields[] = {
    { FIELD_TYPE_U32, 0,  4, "session_id", 0, 0xFF, 1 },
    { FIELD_TYPE_U8,  4,  1, "dtp_repetition", 0, 0xFF, 1 },
    { FIELD_TYPE_U8,  5,  1, "dtp_control", 0, 0xFF, 1 },
    { FIELD_TYPE_U8,  6,  1, "dtp_size", 0, 64, 1 },
    { FIELD_TYPE_BYTES, 7, 64, "dtp_payload", 0, 0xFF, 0 }
};

// Define field definitions for hybrid usage support
static const uci_field_def_t session_hus_config_fields[] = {
    { FIELD_TYPE_U32, 0,   4, "session_id", 0, 0xFF, 1 },
    { FIELD_TYPE_U32, 4,   4, "hus_primary_session_id", 0, 0xFF, 1 },
    { FIELD_TYPE_U8,  8,   1, "hus_role", 0, 1, 1 },  // 0=controller, 1=controlee
    { FIELD_TYPE_U8,  9,   1, "reserved", 0, 0, 0 },
    { FIELD_TYPE_U16, 10,  2, "hus_config_length", 0, 250, 1 },
    { FIELD_TYPE_BYTES, 12, 250, "hus_config_data", 0, 0xFF, 0 }
};

// Define format definitions
static const uci_payload_format_def_t session_init_format = {
    .fields = session_init_fields,
    .num_fields = sizeof(session_init_fields) / sizeof(session_init_fields[0]),
    .min_size = 5,
    .max_size = 5
};

static const uci_payload_format_def_t session_deinit_format = {
    .fields = session_deinit_fields,
    .num_fields = sizeof(session_deinit_fields) / sizeof(session_deinit_fields[0]),
    .min_size = 4,
    .max_size = 4
};

static const uci_payload_format_def_t session_control_format = {
    .fields = session_control_fields,
    .num_fields = sizeof(session_control_fields) / sizeof(session_control_fields[0]),
    .min_size = 4,
    .max_size = 4
};

static const uci_payload_format_def_t session_get_state_format = {
    .fields = session_get_state_fields,
    .num_fields = sizeof(session_get_state_fields) / sizeof(session_get_state_fields[0]),
    .min_size = 4,
    .max_size = 4
};

static const uci_payload_format_def_t session_get_ranging_count_format = {
    .fields = session_get_ranging_count_fields,
    .num_fields = sizeof(session_get_ranging_count_fields) / sizeof(session_get_ranging_count_fields[0]),
    .min_size = 6,
    .max_size = 6
};

static const uci_payload_format_def_t session_app_config_format = {
    .fields = session_app_config_fields,
    .num_fields = sizeof(session_app_config_fields) / sizeof(session_app_config_fields[0]),
    .min_size = 5,
    .max_size = 255  // Max payload size
};

static const uci_payload_format_def_t session_logical_link_format = {
    .fields = session_logical_link_fields,
    .num_fields = sizeof(session_logical_link_fields) / sizeof(session_logical_link_fields[0]),
    .min_size = 6,
    .max_size = 6
};

static const uci_payload_format_def_t session_logical_link_get_param_format = {
    .fields = session_logical_link_get_param_fields,
    .num_fields = sizeof(session_logical_link_get_param_fields) / sizeof(session_logical_link_get_param_fields[0]),
    .min_size = 6,
    .max_size = 6
};

static const uci_payload_format_def_t session_multicast_format = {
    .fields = session_multicast_fields,
    .num_fields = sizeof(session_multicast_fields) / sizeof(session_multicast_fields[0]),
    .min_size = 16,  // Minimum when no key provided
    .max_size = 64   // Maximum with key and address
};

static const uci_payload_format_def_t session_dtp_config_format = {
    .fields = session_dtp_config_fields,
    .num_fields = sizeof(session_dtp_config_fields) / sizeof(session_dtp_config_fields[0]),
    .min_size = 7,   // Without full payload
    .max_size = 71   // With full payload
};

static const uci_payload_format_def_t session_hus_config_format = {
    .fields = session_hus_config_fields,
    .num_fields = sizeof(session_hus_config_fields) / sizeof(session_hus_config_fields[0]),
    .min_size = 12,  // Minimum without config data
    .max_size = 262  // Maximum with config data
};

// Serialization functions
uci_error_t uci_serialize_session_init(const void* src, unsigned char* dst, size_t dst_size) {
    if (!src || !dst) {
        return UCI_ERROR_INVALID_PARAM;
    }

    const uci_session_init_payload_t* payload = (const uci_session_init_payload_t*)src;
    
    if (dst_size < 5) {
        return UCI_ERROR_BUFFER_OVERFLOW;
    }

    write_u32_le(dst, payload->session_id);
    dst[4] = payload->session_type;

    return UCI_SUCCESS;
}

uci_error_t uci_serialize_session_deinit(const void* src, unsigned char* dst, size_t dst_size) {
    if (!src || !dst) {
        return UCI_ERROR_INVALID_PARAM;
    }

    const uci_session_deinit_payload_t* payload = (const uci_session_deinit_payload_t*)src;
    
    if (dst_size < 4) {
        return UCI_ERROR_BUFFER_OVERFLOW;
    }

    write_u32_le(dst, payload->session_id);

    return UCI_SUCCESS;
}

uci_error_t uci_serialize_session_control(const void* src, unsigned char* dst, size_t dst_size) {
    if (!src || !dst) {
        return UCI_ERROR_INVALID_PARAM;
    }

    const uci_session_control_payload_t* payload = (const uci_session_control_payload_t*)src;
    
    if (dst_size < 4) {
        return UCI_ERROR_BUFFER_OVERFLOW;
    }

    write_u32_le(dst, payload->session_id);

    return UCI_SUCCESS;
}

uci_error_t uci_serialize_session_get_state(const void* src, unsigned char* dst, size_t dst_size) {
    if (!src || !dst) {
        return UCI_ERROR_INVALID_PARAM;
    }

    const uci_session_get_state_payload_t* payload = (const uci_session_get_state_payload_t*)src;
    
    if (dst_size < 4) {
        return UCI_ERROR_BUFFER_OVERFLOW;
    }

    write_u32_le(dst, payload->session_id);

    return UCI_SUCCESS;
}

uci_error_t uci_serialize_session_get_ranging_count(const void* src, unsigned char* dst, size_t dst_size) {
    if (!src || !dst) {
        return UCI_ERROR_INVALID_PARAM;
    }

    const uci_session_get_ranging_count_payload_t* payload = (const uci_session_get_ranging_count_payload_t*)src;
    
    if (dst_size < 6) {
        return UCI_ERROR_BUFFER_OVERFLOW;
    }

    write_u32_le(dst, payload->session_id);
    write_u16_le(dst + 4, payload->ranging_count);

    return UCI_SUCCESS;
}

uci_error_t uci_serialize_session_app_config(const void* src, unsigned char* dst, size_t dst_size) {
    if (!src || !dst) {
        return UCI_ERROR_INVALID_PARAM;
    }

    const uci_session_app_config_payload_t* payload = (const uci_session_app_config_payload_t*)src;
    
    if (dst_size < 5) {
        return UCI_ERROR_BUFFER_OVERFLOW;
    }

    write_u32_le(dst, payload->session_id);
    dst[4] = payload->num_tlvs;

    return UCI_SUCCESS;
}

uci_error_t uci_serialize_session_logical_link(const void* src, unsigned char* dst, size_t dst_size) {
    if (!src || !dst) {
        return UCI_ERROR_INVALID_PARAM;
    }

    const uci_session_logical_link_payload_t* payload = (const uci_session_logical_link_payload_t*)src;
    
    if (dst_size < 6) {
        return UCI_ERROR_BUFFER_OVERFLOW;
    }

    write_u32_le(dst, payload->session_id);
    dst[4] = payload->link_id;
    dst[5] = payload->mode;

    return UCI_SUCCESS;
}

uci_error_t uci_serialize_session_multicast(const void* src, unsigned char* dst, size_t dst_size) {
    if (!src || !dst) {
        return UCI_ERROR_INVALID_PARAM;
    }

    const uci_session_multicast_payload_t* payload = (const uci_session_multicast_payload_t*)src;
    
    size_t required_size = 16 + (payload->key_length > 0 ? payload->key_length : 0);
    if (dst_size < required_size) {
        return UCI_ERROR_BUFFER_OVERFLOW;
    }

    write_u32_le(dst, payload->session_id);
    dst[4] = payload->action;
    dst[5] = payload->address_type;
    
    for (int i = 0; i < 8; i++) {
        dst[6 + i] = payload->address[i];
    }
    
    dst[14] = payload->subsession_id;
    dst[15] = payload->key_length;
    
    if (payload->key_length > 0 && payload->key_length <= 32) {
        for (int i = 0; i < payload->key_length; i++) {
            dst[16 + i] = payload->key[i];
        }
    }

    return UCI_SUCCESS;
}

uci_error_t uci_serialize_session_dtp_config(const void* src, unsigned char* dst, size_t dst_size) {
    if (!src || !dst) {
        return UCI_ERROR_INVALID_PARAM;
    }

    const uci_session_dtp_config_payload_t* payload = (const uci_session_dtp_config_payload_t*)src;
    
    size_t required_size = 7 + (payload->dtp_size > 0 ? payload->dtp_size : 0);
    if (dst_size < required_size) {
        return UCI_ERROR_BUFFER_OVERFLOW;
    }

    write_u32_le(dst, payload->session_id);
    dst[4] = payload->dtp_repetition;
    dst[5] = payload->dtp_control;
    dst[6] = payload->dtp_size;
    
    if (payload->dtp_size > 0 && payload->dtp_size <= 64) {
        for (int i = 0; i < payload->dtp_size; i++) {
            dst[7 + i] = payload->dtp_payload[i];
        }
    }

    return UCI_SUCCESS;
}

uci_error_t uci_serialize_session_hus_config(const void* src, unsigned char* dst, size_t dst_size) {
    if (!src || !dst) {
        return UCI_ERROR_INVALID_PARAM;
    }

    const uci_session_hus_config_payload_t* payload = (const uci_session_hus_config_payload_t*)src;
    
    size_t required_size = 12 + (payload->hus_config_length > 0 ? payload->hus_config_length : 0);
    if (dst_size < required_size) {
        return UCI_ERROR_BUFFER_OVERFLOW;
    }

    write_u32_le(dst, payload->session_id);
    write_u32_le(dst + 4, payload->hus_primary_session_id);
    dst[8] = payload->hus_role;
    dst[9] = payload->reserved;
    write_u16_le(dst + 10, payload->hus_config_length);
    
    if (payload->hus_config_length > 0 && payload->hus_config_length <= 250) {
        for (int i = 0; i < payload->hus_config_length; i++) {
            dst[12 + i] = payload->hus_config_data[i];
        }
    }

    return UCI_SUCCESS;
}

// Deserialization functions
uci_error_t uci_deserialize_session_init(const unsigned char* src, size_t src_size, void* dst) {
    if (!src || !dst || src_size < 5) {
        return UCI_ERROR_INVALID_PARAM;
    }

    uci_session_init_payload_t* payload = (uci_session_init_payload_t*)dst;
    
    payload->session_id = read_u32_le(src);
    payload->session_type = src[4];
    
    return UCI_SUCCESS;
}

uci_error_t uci_deserialize_session_deinit(const unsigned char* src, size_t src_size, void* dst) {
    if (!src || !dst || src_size < 4) {
        return UCI_ERROR_INVALID_PARAM;
    }

    uci_session_deinit_payload_t* payload = (uci_session_deinit_payload_t*)dst;
    
    payload->session_id = read_u32_le(src);
    
    return UCI_SUCCESS;
}

uci_error_t uci_deserialize_session_control(const unsigned char* src, size_t src_size, void* dst) {
    if (!src || !dst || src_size < 4) {
        return UCI_ERROR_INVALID_PARAM;
    }

    uci_session_control_payload_t* payload = (uci_session_control_payload_t*)dst;
    
    payload->session_id = read_u32_le(src);
    
    return UCI_SUCCESS;
}

uci_error_t uci_deserialize_session_get_state(const unsigned char* src, size_t src_size, void* dst) {
    if (!src || !dst || src_size < 4) {
        return UCI_ERROR_INVALID_PARAM;
    }

    uci_session_get_state_payload_t* payload = (uci_session_get_state_payload_t*)dst;
    
    payload->session_id = read_u32_le(src);
    
    return UCI_SUCCESS;
}

uci_error_t uci_deserialize_session_get_ranging_count(const unsigned char* src, size_t src_size, void* dst) {
    if (!src || !dst || src_size < 6) {
        return UCI_ERROR_INVALID_PARAM;
    }

    uci_session_get_ranging_count_payload_t* payload = (uci_session_get_ranging_count_payload_t*)dst;
    
    payload->session_id = read_u32_le(src);
    payload->ranging_count = read_u16_le(src + 4);
    
    return UCI_SUCCESS;
}

uci_error_t uci_deserialize_session_app_config(const unsigned char* src, size_t src_size, void* dst) {
    if (!src || !dst || src_size < 5) {
        return UCI_ERROR_INVALID_PARAM;
    }

    uci_session_app_config_payload_t* payload = (uci_session_app_config_payload_t*)dst;
    
    payload->session_id = read_u32_le(src);
    payload->num_tlvs = src[4];
    
    return UCI_SUCCESS;
}

uci_error_t uci_deserialize_session_logical_link(const unsigned char* src, size_t src_size, void* dst) {
    if (!src || !dst || src_size < 6) {
        return UCI_ERROR_INVALID_PARAM;
    }

    uci_session_logical_link_payload_t* payload = (uci_session_logical_link_payload_t*)dst;
    
    payload->session_id = read_u32_le(src);
    payload->link_id = src[4];
    payload->mode = src[5];
    
    return UCI_SUCCESS;
}

uci_error_t uci_deserialize_session_multicast(const unsigned char* src, size_t src_size, void* dst) {
    if (!src || !dst || src_size < 16) {
        return UCI_ERROR_INVALID_PARAM;
    }

    uci_session_multicast_payload_t* payload = (uci_session_multicast_payload_t*)dst;
    
    payload->session_id = read_u32_le(src);
    payload->action = src[4];
    payload->address_type = src[5];
    
    for (int i = 0; i < 8; i++) {
        payload->address[i] = src[6 + i];
    }
    
    payload->subsession_id = src[14];
    payload->key_length = src[15];
    
    if (payload->key_length > 0 && payload->key_length <= 32 && 
        src_size >= (size_t)(16 + payload->key_length)) {
        for (int i = 0; i < payload->key_length; i++) {
            payload->key[i] = src[16 + i];
        }
    }
    
    return UCI_SUCCESS;
}

uci_error_t uci_deserialize_session_dtp_config(const unsigned char* src, size_t src_size, void* dst) {
    if (!src || !dst || src_size < 7) {
        return UCI_ERROR_INVALID_PARAM;
    }

    uci_session_dtp_config_payload_t* payload = (uci_session_dtp_config_payload_t*)dst;
    
    payload->session_id = read_u32_le(src);
    payload->dtp_repetition = src[4];
    payload->dtp_control = src[5];
    payload->dtp_size = src[6];
    
    if (payload->dtp_size > 0 && payload->dtp_size <= 64 && 
        src_size >= (size_t)(7 + payload->dtp_size)) {
        for (int i = 0; i < payload->dtp_size; i++) {
            payload->dtp_payload[i] = src[7 + i];
        }
    }
    
    return UCI_SUCCESS;
}

uci_error_t uci_deserialize_session_logical_link_get_param(const unsigned char* src, size_t src_size, void* dst) {
    if (!src || !dst || src_size < 6) {
        return UCI_ERROR_INVALID_PARAM;
    }

    uci_session_logical_link_get_param_payload_t* payload = (uci_session_logical_link_get_param_payload_t*)dst;
    
    payload->session_id = read_u32_le(src);
    payload->link_id = src[4];
    payload->param_id = src[5];
    
    return UCI_SUCCESS;
}

uci_error_t uci_deserialize_session_hus_config(const unsigned char* src, size_t src_size, void* dst) {
    if (!src || !dst || src_size < 12) {
        return UCI_ERROR_INVALID_PARAM;
    }

    uci_session_hus_config_payload_t* payload = (uci_session_hus_config_payload_t*)dst;
    
    payload->session_id = read_u32_le(src);
    payload->hus_primary_session_id = read_u32_le(src + 4);
    payload->hus_role = src[8];
    payload->reserved = src[9];
    payload->hus_config_length = read_u16_le(src + 10);
    
    if (payload->hus_config_length > 0 && payload->hus_config_length <= 250 && 
        src_size >= (size_t)(12 + payload->hus_config_length)) {
        for (int i = 0; i < payload->hus_config_length; i++) {
            payload->hus_config_data[i] = src[12 + i];
        }
    }
    
    return UCI_SUCCESS;
}

// Validation functions
uci_error_t uci_validate_session_init(const void* data) {
    if (!data) {
        return UCI_ERROR_INVALID_PARAM;
    }

    const uci_session_init_payload_t* payload = (const uci_session_init_payload_t*)data;
    
    // Validate session ID range
    if (payload->session_id >= MAX_SESSIONS) {
        return UCI_ERROR_INVALID_PARAM;
    }
    
    // Validate session type
    if (payload->session_type > FIRA_IN_BAND_DATA_PHASE) {
        return UCI_ERROR_INVALID_PARAM;
    }
    
    return UCI_SUCCESS;
}

uci_error_t uci_validate_session_deinit(const void* data) {
    if (!data) {
        return UCI_ERROR_INVALID_PARAM;
    }

    const uci_session_deinit_payload_t* payload = (const uci_session_deinit_payload_t*)data;
    
    if (payload->session_id >= MAX_SESSIONS) {
        return UCI_ERROR_INVALID_PARAM;
    }
    
    return UCI_SUCCESS;
}

uci_error_t uci_validate_session_control(const void* data) {
    if (!data) {
        return UCI_ERROR_INVALID_PARAM;
    }

    const uci_session_control_payload_t* payload = (const uci_session_control_payload_t*)data;
    
    if (payload->session_id >= MAX_SESSIONS) {
        return UCI_ERROR_INVALID_PARAM;
    }
    
    return UCI_SUCCESS;
}

uci_error_t uci_validate_session_get_state(const void* data) {
    return uci_validate_session_control(data);  // Same validation as control
}

uci_error_t uci_validate_session_get_ranging_count(const void* data) {
    return uci_validate_session_control(data);  // Same validation as control
}

uci_error_t uci_validate_session_app_config(const void* data) {
    if (!data) {
        return UCI_ERROR_INVALID_PARAM;
    }

    const uci_session_app_config_payload_t* payload = (const uci_session_app_config_payload_t*)data;
    
    if (payload->session_id >= MAX_SESSIONS) {
        return UCI_ERROR_INVALID_PARAM;
    }
    
    if (payload->num_tlvs > 32) {  // Max config entries
        return UCI_ERROR_INVALID_PARAM;
    }
    
    return UCI_SUCCESS;
}

uci_error_t uci_validate_session_logical_link(const void* data) {
    if (!data) {
        return UCI_ERROR_INVALID_PARAM;
    }

    const uci_session_logical_link_payload_t* payload = (const uci_session_logical_link_payload_t*)data;
    
    if (payload->session_id >= MAX_SESSIONS) {
        return UCI_ERROR_INVALID_PARAM;
    }
    
    if (payload->link_id >= MAX_LOGICAL_LINKS) {
        return UCI_ERROR_INVALID_PARAM;
    }
    
    if (payload->mode > 2) {  // Validate mode range
        return UCI_ERROR_INVALID_PARAM;
    }
    
    return UCI_SUCCESS;
}

uci_error_t uci_validate_session_multicast(const void* data) {
    if (!data) {
        return UCI_ERROR_INVALID_PARAM;
    }

    const uci_session_multicast_payload_t* payload = (const uci_session_multicast_payload_t*)data;
    
    if (payload->session_id >= MAX_SESSIONS) {
        return UCI_ERROR_INVALID_PARAM;
    }
    
    if (payload->action > MULTICAST_ACTION_ADD_LONG_KEY) {
        return UCI_ERROR_INVALID_PARAM;
    }
    
    if (payload->key_length > 32) {
        return UCI_ERROR_INVALID_PARAM;
    }
    
    return UCI_SUCCESS;
}

uci_error_t uci_validate_session_dtp_config(const void* data) {
    if (!data) {
        return UCI_ERROR_INVALID_PARAM;
    }

    const uci_session_dtp_config_payload_t* payload = (const uci_session_dtp_config_payload_t*)data;
    
    if (payload->session_id >= MAX_SESSIONS) {
        return UCI_ERROR_INVALID_PARAM;
    }
    
    if (payload->dtp_size > 64) {
        return UCI_ERROR_INVALID_PARAM;
    }
    
    return UCI_SUCCESS;
}

uci_error_t uci_validate_session_hus_config(const void* data) {
    if (!data) {
        return UCI_ERROR_INVALID_PARAM;
    }

    const uci_session_hus_config_payload_t* payload = (const uci_session_hus_config_payload_t*)data;
    
    if (payload->session_id >= MAX_SESSIONS) {
        return UCI_ERROR_INVALID_PARAM;
    }
    
    if (payload->hus_role > 1) {  // Only 0/1 for controller/controlee
        return UCI_ERROR_INVALID_PARAM;
    }
    
    if (payload->hus_config_length > 250) {
        return UCI_ERROR_INVALID_PARAM;
    }
    
    return UCI_SUCCESS;
}

// Context definitions for each command
static const uci_packet_ctx_t packet_contexts[] = {
    // Session Config Group
    { COMMAND, COMPLETE, SESSION_CONFIG, SESSION_INIT,
      &session_init_format, uci_serialize_session_init, uci_deserialize_session_init, uci_validate_session_init },
    { RESPONSE, COMPLETE, SESSION_CONFIG, SESSION_INIT,
      &session_init_format, uci_serialize_session_init, uci_deserialize_session_init, uci_validate_session_init },
    { COMMAND, COMPLETE, SESSION_CONFIG, SESSION_DEINIT,
      &session_deinit_format, uci_serialize_session_deinit, uci_deserialize_session_deinit, uci_validate_session_deinit },
    { RESPONSE, COMPLETE, SESSION_CONFIG, SESSION_DEINIT,
      &session_deinit_format, uci_serialize_session_deinit, uci_deserialize_session_deinit, uci_validate_session_deinit },
    { COMMAND, COMPLETE, SESSION_CONFIG, SESSION_GET_STATE,
      &session_get_state_format, uci_serialize_session_get_state, uci_deserialize_session_get_state, uci_validate_session_get_state },
    { RESPONSE, COMPLETE, SESSION_CONFIG, SESSION_GET_STATE,
      &session_get_state_format, uci_serialize_session_get_state, uci_deserialize_session_get_state, uci_validate_session_get_state },
    { COMMAND, COMPLETE, SESSION_CONFIG, SESSION_SET_APP_CONFIG,
      &session_app_config_format, uci_serialize_session_app_config, uci_deserialize_session_app_config, uci_validate_session_app_config },
    { RESPONSE, COMPLETE, SESSION_CONFIG, SESSION_SET_APP_CONFIG,
      &session_app_config_format, uci_serialize_session_app_config, uci_deserialize_session_app_config, uci_validate_session_app_config },
    { COMMAND, COMPLETE, SESSION_CONFIG, SESSION_GET_APP_CONFIG,
      &session_app_config_format, uci_serialize_session_app_config, uci_deserialize_session_app_config, uci_validate_session_app_config },
    { RESPONSE, COMPLETE, SESSION_CONFIG, SESSION_GET_APP_CONFIG,
      &session_app_config_format, uci_serialize_session_app_config, uci_deserialize_session_app_config, uci_validate_session_app_config },
    { COMMAND, COMPLETE, SESSION_CONFIG, SESSION_UPDATE_CONTROLLER_MULTICAST_LIST,
      &session_multicast_format, uci_serialize_session_multicast, uci_deserialize_session_multicast, uci_validate_session_multicast },
    { RESPONSE, COMPLETE, SESSION_CONFIG, SESSION_UPDATE_CONTROLLER_MULTICAST_LIST,
      &session_multicast_format, uci_serialize_session_multicast, uci_deserialize_session_multicast, uci_validate_session_multicast },
    { COMMAND, COMPLETE, SESSION_CONFIG, SESSION_DATA_TRANSFER_PHASE_CONFIG,
      &session_dtp_config_format, uci_serialize_session_dtp_config, uci_deserialize_session_dtp_config, uci_validate_session_dtp_config },
    { RESPONSE, COMPLETE, SESSION_CONFIG, SESSION_DATA_TRANSFER_PHASE_CONFIG,
      &session_dtp_config_format, uci_serialize_session_dtp_config, uci_deserialize_session_dtp_config, uci_validate_session_dtp_config },
    { COMMAND, COMPLETE, SESSION_CONFIG, SESSION_SET_HYBRID_CONTROLLER_CONFIG,
      &session_hus_config_format, uci_serialize_session_hus_config, uci_deserialize_session_hus_config, uci_validate_session_hus_config },
    { RESPONSE, COMPLETE, SESSION_CONFIG, SESSION_SET_HYBRID_CONTROLLER_CONFIG,
      &session_hus_config_format, uci_serialize_session_hus_config, uci_deserialize_session_hus_config, uci_validate_session_hus_config },
    { COMMAND, COMPLETE, SESSION_CONFIG, SESSION_SET_HYBRID_CONTROLEE_CONFIG,
      &session_hus_config_format, uci_serialize_session_hus_config, uci_deserialize_session_hus_config, uci_validate_session_hus_config },
    { RESPONSE, COMPLETE, SESSION_CONFIG, SESSION_SET_HYBRID_CONTROLEE_CONFIG,
      &session_hus_config_format, uci_serialize_session_hus_config, uci_deserialize_session_hus_config, uci_validate_session_hus_config },
    
    // Session Control Group
    { COMMAND, COMPLETE, SESSION_CONTROL, SESSION_START,
      &session_control_format, uci_serialize_session_control, uci_deserialize_session_control, uci_validate_session_control },
    { RESPONSE, COMPLETE, SESSION_CONTROL, SESSION_START,
      &session_control_format, uci_serialize_session_control, uci_deserialize_session_control, uci_validate_session_control },
    { COMMAND, COMPLETE, SESSION_CONTROL, SESSION_STOP,
      &session_control_format, uci_serialize_session_control, uci_deserialize_session_control, uci_validate_session_control },
    { RESPONSE, COMPLETE, SESSION_CONTROL, SESSION_STOP,
      &session_control_format, uci_serialize_session_control, uci_deserialize_session_control, uci_validate_session_control },
    { RESPONSE, COMPLETE, SESSION_CONTROL, SESSION_GET_RANGING_COUNT,
      &session_get_ranging_count_format, uci_serialize_session_get_ranging_count, uci_deserialize_session_get_ranging_count, uci_validate_session_get_ranging_count },
    { COMMAND, COMPLETE, SESSION_CONTROL, SESSION_LOGICAL_LINK_CREATE,
      &session_logical_link_format, uci_serialize_session_logical_link, uci_deserialize_session_logical_link, uci_validate_session_logical_link },
    { RESPONSE, COMPLETE, SESSION_CONTROL, SESSION_LOGICAL_LINK_CREATE,
      &session_logical_link_format, uci_serialize_session_logical_link, uci_deserialize_session_logical_link, uci_validate_session_logical_link },
    { COMMAND, COMPLETE, SESSION_CONTROL, SESSION_LOGICAL_LINK_CLOSE,
      &session_logical_link_format, uci_serialize_session_logical_link, uci_deserialize_session_logical_link, uci_validate_session_logical_link },
    { RESPONSE, COMPLETE, SESSION_CONTROL, SESSION_LOGICAL_LINK_CLOSE,
      &session_logical_link_format, uci_serialize_session_logical_link, uci_deserialize_session_logical_link, uci_validate_session_logical_link },
    { COMMAND, COMPLETE, SESSION_CONTROL, SESSION_LOGICAL_LINK_GET_PARAM,
      &session_logical_link_get_param_format, uci_serialize_session_logical_link, uci_deserialize_session_logical_link_get_param, uci_validate_session_logical_link },
    { RESPONSE, COMPLETE, SESSION_CONTROL, SESSION_LOGICAL_LINK_GET_PARAM,
      &session_logical_link_get_param_format, uci_serialize_session_logical_link, uci_deserialize_session_logical_link_get_param, uci_validate_session_logical_link },
};

// Get payload format based on GID and OID
const uci_payload_format_def_t* uci_get_payload_format(uint8_t gid, uint8_t oid) {
    // Linear search through contexts to find matching format
    for (size_t i = 0; i < sizeof(packet_contexts) / sizeof(packet_contexts[0]); i++) {
        if (packet_contexts[i].gid == gid && packet_contexts[i].oid == oid) {
            return packet_contexts[i].format_def;
        }
    }
    return NULL; // Format not defined
}

// Get packet context based on GID and OID
const uci_packet_ctx_t* uci_get_packet_context(uint8_t gid, uint8_t oid) {
    // Linear search through contexts
    for (size_t i = 0; i < sizeof(packet_contexts) / sizeof(packet_contexts[0]); i++) {
        if (packet_contexts[i].gid == gid && packet_contexts[i].oid == oid) {
            return &packet_contexts[i];
        }
    }
    return NULL; // Context not defined
}