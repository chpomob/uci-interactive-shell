#include "../include/uci_packet_structures.h"
#include "../include/uci_packet_utils.h"
#include "../include/uci.h"
#include "../include/uci_functions.h"  // Include for send_uci_command
#include <stdlib.h>
#include <string.h>

// Helper function to create and send a UCI packet using mutualized structures
unsigned char* uci_create_packet_from_struct(uint8_t mt, uint8_t pbf, uint8_t gid, uint8_t oid, 
                                             const void* payload_struct, size_t* packet_len) {
    if (!payload_struct || !packet_len) {
        return NULL;
    }

    // Get the context for this command
    const uci_packet_ctx_t* ctx = uci_get_packet_context(gid, oid);
    if (!ctx || !ctx->serialize_fn) {
        return NULL; // Unsupported command or missing serialization function
    }

    // First validate the payload structure
    if (ctx->validate_fn) {
        uci_error_t validation_result = ctx->validate_fn(payload_struct);
        if (validation_result != UCI_SUCCESS) {
            return NULL; // Invalid payload data
        }
    }

    // Calculate the minimum payload size based on the format
    size_t payload_size = ctx->format_def->max_size;
    unsigned char* payload_buffer = (unsigned char*)malloc(payload_size);
    if (!payload_buffer) {
        return NULL;
    }

    // Serialize the structure to the payload buffer
    uci_error_t serialize_result = ctx->serialize_fn(payload_struct, payload_buffer, payload_size);
    if (serialize_result != UCI_SUCCESS) {
        free(payload_buffer);
        return NULL;
    }

    // Create the full UCI packet using the existing utility function
    unsigned char* packet = create_uci_packet(mt, pbf, gid, oid, payload_buffer, payload_size, packet_len);

    // Calculate actual payload size based on format
    size_t actual_payload_size = ctx->format_def->min_size;
    if (gid == SESSION_CONFIG && oid == SESSION_DATA_TRANSFER_PHASE_CONFIG) {
        // Special case for DTP config which has variable size
        const uci_session_dtp_config_payload_t* dtp_payload = (const uci_session_dtp_config_payload_t*)payload_struct;
        actual_payload_size = 7 + dtp_payload->dtp_size;
    } else if (gid == SESSION_CONFIG && (oid == SESSION_SET_HYBRID_CONTROLLER_CONFIG || oid == SESSION_SET_HYBRID_CONTROLEE_CONFIG)) {
        // Special case for HUS config which has variable size
        const uci_session_hus_config_payload_t* hus_payload = (const uci_session_hus_config_payload_t*)payload_struct;
        actual_payload_size = 12 + hus_payload->hus_config_length;
    } else if (gid == SESSION_CONFIG && (oid == SESSION_SET_APP_CONFIG || oid == SESSION_GET_APP_CONFIG)) {
        // Special case for app config which has variable size
        const uci_session_app_config_payload_t* app_payload = (const uci_session_app_config_payload_t*)payload_struct;
        // This would need additional TLV handling
        actual_payload_size = 5; // Just the header for now
    }

    // Now create packet with the correct payload size
    free(payload_buffer);
    payload_buffer = (unsigned char*)malloc(actual_payload_size);
    if (!payload_buffer) {
        free(packet);
        return NULL;
    }

    serialize_result = ctx->serialize_fn(payload_struct, payload_buffer, actual_payload_size);
    if (serialize_result != UCI_SUCCESS) {
        free(payload_buffer);
        if (packet) free(packet);
        return NULL;
    }

    // Create the final packet with correct size
    unsigned char* final_packet = create_uci_packet(mt, pbf, gid, oid, payload_buffer, actual_payload_size, packet_len);
    free(payload_buffer);
    
    return final_packet;
}

// Helper function to decode a UCI packet payload into a mutualized structure
uci_error_t uci_decode_packet_to_struct(const unsigned char* packet, size_t packet_len, void* payload_struct, 
                                        uint8_t gid, uint8_t oid) {
    if (!packet || !payload_struct || packet_len < sizeof(struct uci_packet_header)) {
        return UCI_ERROR_INVALID_PARAM;
    }

    // Get the context for this command
    const uci_packet_ctx_t* ctx = uci_get_packet_context(gid, oid);
    if (!ctx || !ctx->deserialize_fn) {
        return UCI_ERROR_UNSUPPORTED_OPERATION; // Unsupported command or missing deserialization function
    }

    // Extract payload from the packet
    size_t payload_len = packet_len - sizeof(struct uci_packet_header);
    const unsigned char* payload = packet + sizeof(struct uci_packet_header);

    // Validate payload length based on format
    if (payload_len < ctx->format_def->min_size) {
        return UCI_ERROR_MALFORMED_PACKET;
    }

    // Deserialize the payload into the structure
    uci_error_t result = ctx->deserialize_fn(payload, payload_len, payload_struct);
    if (result != UCI_SUCCESS) {
        return result;
    }

    // Optionally validate the deserialized structure
    if (ctx->validate_fn) {
        result = ctx->validate_fn(payload_struct);
        if (result != UCI_SUCCESS) {
            return result;
        }
    }

    return UCI_SUCCESS;
}

// Helper functions for creating specific packet structures
uci_error_t uci_create_session_init_struct(uci_session_init_payload_t* out_struct, 
                                           uint32_t session_id, uint8_t session_type) {
    if (!out_struct) {
        return UCI_ERROR_INVALID_PARAM;
    }

    out_struct->session_id = session_id;
    out_struct->session_type = session_type;

    return uci_validate_session_init(out_struct);
}

uci_error_t uci_create_session_deinit_struct(uci_session_deinit_payload_t* out_struct, 
                                             uint32_t session_id) {
    if (!out_struct) {
        return UCI_ERROR_INVALID_PARAM;
    }

    out_struct->session_id = session_id;

    return uci_validate_session_deinit(out_struct);
}

uci_error_t uci_create_session_control_struct(uci_session_control_payload_t* out_struct, 
                                              uint32_t session_id) {
    if (!out_struct) {
        return UCI_ERROR_INVALID_PARAM;
    }

    out_struct->session_id = session_id;

    return uci_validate_session_control(out_struct);
}

uci_error_t uci_create_session_get_ranging_count_struct(uci_session_get_ranging_count_payload_t* out_struct, 
                                                        uint32_t session_id, uint16_t ranging_count) {
    if (!out_struct) {
        return UCI_ERROR_INVALID_PARAM;
    }

    out_struct->session_id = session_id;
    out_struct->ranging_count = ranging_count;

    return uci_validate_session_get_ranging_count(out_struct);
}

uci_error_t uci_create_session_logical_link_struct(uci_session_logical_link_payload_t* out_struct, 
                                                   uint32_t session_id, uint8_t link_id, uint8_t mode) {
    if (!out_struct) {
        return UCI_ERROR_INVALID_PARAM;
    }

    out_struct->session_id = session_id;
    out_struct->link_id = link_id;
    out_struct->mode = mode;

    return uci_validate_session_logical_link(out_struct);
}

// Helper function to print payload information for debugging
void uci_print_payload_struct(uint8_t gid, uint8_t oid, const void* payload_struct) {
    if (!payload_struct) {
        printf("  Payload struct: NULL\n");
        return;
    }

    switch (gid) {
        case SESSION_CONFIG:
            switch (oid) {
                case SESSION_INIT:
                {
                    const uci_session_init_payload_t* p = (const uci_session_init_payload_t*)payload_struct;
                    printf("  Session ID: 0x%08X, Type: %d\n", p->session_id, p->session_type);
                    break;
                }
                case SESSION_DEINIT:
                {
                    const uci_session_deinit_payload_t* p = (const uci_session_deinit_payload_t*)payload_struct;
                    printf("  Session ID: 0x%08X\n", p->session_id);
                    break;
                }
                case SESSION_GET_STATE:
                {
                    const uci_session_get_state_payload_t* p = (const uci_session_get_state_payload_t*)payload_struct;
                    printf("  Session ID: 0x%08X\n", p->session_id);
                    break;
                }
                case SESSION_SET_APP_CONFIG:
                case SESSION_GET_APP_CONFIG:
                {
                    const uci_session_app_config_payload_t* p = (const uci_session_app_config_payload_t*)payload_struct;
                    printf("  Session ID: 0x%08X, Num TLVs: %d\n", p->session_id, p->num_tlvs);
                    break;
                }
                case SESSION_LOGICAL_LINK_CREATE:
                case SESSION_LOGICAL_LINK_CLOSE:
                {
                    const uci_session_logical_link_payload_t* p = (const uci_session_logical_link_payload_t*)payload_struct;
                    printf("  Session ID: 0x%08X, Link ID: %d, Mode: %d\n", 
                           p->session_id, p->link_id, p->mode);
                    break;
                }
                default:
                    printf("  Payload for GID:0x%02X OID:0x%02X - format not implemented\n", gid, oid);
                    break;
            }
            break;
            
        case SESSION_CONTROL:
            switch (oid) {
                case SESSION_START:
                case SESSION_STOP:
                {
                    const uci_session_control_payload_t* p = (const uci_session_control_payload_t*)payload_struct;
                    printf("  Session ID: 0x%08X\n", p->session_id);
                    break;
                }
                case SESSION_GET_RANGING_COUNT:
                {
                    const uci_session_get_ranging_count_payload_t* p = (const uci_session_get_ranging_count_payload_t*)payload_struct;
                    printf("  Session ID: 0x%08X, Ranging Count: %d\n", p->session_id, p->ranging_count);
                    break;
                }
                case SESSION_LOGICAL_LINK_GET_PARAM:
                {
                    const uci_session_logical_link_get_param_payload_t* p = (const uci_session_logical_link_get_param_payload_t*)payload_struct;
                    printf("  Session ID: 0x%08X, Link ID: %d, Param ID: %d\n", 
                           p->session_id, p->link_id, p->param_id);
                    break;
                }
                default:
                    printf("  Payload for GID:0x%02X OID:0x%02X - format not implemented\n", gid, oid);
                    break;
            }
            break;
            
        default:
            printf("  Payload for GID:0x%02X OID:0x%02X - format not implemented\n", gid, oid);
            break;
    }
}

// Function to integrate with the existing send_uci_command function using mutualized structures
void uci_send_command_with_struct(uint8_t mt, uint8_t pbf, uint8_t gid, uint8_t oid, 
                                  const void* payload_struct) {
    // First validate the structure
    const uci_packet_ctx_t* ctx = uci_get_packet_context(gid, oid);
    if (!ctx || !ctx->serialize_fn) {
        printf("Error: Command not supported or missing serialization function (GID:0x%02X, OID:0x%02X)\n", gid, oid);
        return;
    }

    if (ctx->validate_fn) {
        uci_error_t validation_result = ctx->validate_fn(payload_struct);
        if (validation_result != UCI_SUCCESS) {
            printf("Error: Payload validation failed (result: %d)\n", validation_result);
            return;
        }
    }

    // Create the packet
    size_t packet_len;
    unsigned char* packet = uci_create_packet_from_struct(mt, pbf, gid, oid, payload_struct, &packet_len);
    if (!packet) {
        printf("Error: Failed to create UCI packet\n");
        return;
    }

    // Extract payload from the constructed packet for sending
    unsigned char* payload = packet + sizeof(struct uci_packet_header);
    int payload_len = (int)(packet_len - sizeof(struct uci_packet_header));
    
    // Send using the existing function
    send_uci_command(mt, pbf, gid, oid, payload, payload_len);

    // Clean up
    free(packet);
}