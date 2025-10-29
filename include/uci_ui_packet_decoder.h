#ifndef UCI_UI_PACKET_DECODER_H
#define UCI_UI_PACKET_DECODER_H

#include <stdint.h>
#include <stddef.h>

// Enhanced UI packet analysis function
void ui_analyze_uci_packet(unsigned char* packet, size_t packet_len);

// Enhanced packet decoding functions with UI enhancements
void ui_decode_core_device_info_rsp(unsigned char* payload, int payload_len);
void ui_decode_core_get_caps_info_rsp(unsigned char* payload, int payload_len);
void ui_decode_core_set_config_rsp(unsigned char* payload, int payload_len);
void ui_decode_core_get_config_rsp(unsigned char* payload, int payload_len);
void ui_decode_core_device_reset_rsp(unsigned char* payload, int payload_len);
void ui_decode_core_device_suspend_rsp(unsigned char* payload, int payload_len);
void ui_decode_core_query_uwbs_timestamp_rsp(unsigned char* payload, int payload_len);

// Additional core response decoders (previously implemented but not declared)
void ui_decode_core_get_state_rsp(unsigned char* payload, int payload_len);
void ui_decode_core_set_active_rsp(unsigned char* payload, int payload_len);
void ui_decode_core_set_ready_rsp(unsigned char* payload, int payload_len);
void ui_decode_core_device_ready_rsp(unsigned char* payload, int payload_len);
void ui_decode_core_get_caps_rsp(unsigned char* payload, int payload_len);
void ui_decode_core_set_power_rsp(unsigned char* payload, int payload_len);
void ui_decode_core_get_power_rsp(unsigned char* payload, int payload_len);
void ui_decode_core_device_on_rsp(unsigned char* payload, int payload_len);
void ui_decode_core_device_off_rsp(unsigned char* payload, int payload_len);
void ui_decode_core_device_suspend_cmd_rsp(unsigned char* payload, int payload_len);

void ui_decode_session_init_rsp(unsigned char* payload, int payload_len);
void ui_decode_session_init_cmd(unsigned char* payload, int payload_len);
void ui_decode_session_deinit_rsp(unsigned char* payload, int payload_len);
void ui_decode_session_set_app_config_rsp(unsigned char* payload, int payload_len);
void ui_decode_session_get_app_config_rsp(unsigned char* payload, int payload_len);
void ui_decode_session_get_count_rsp(unsigned char* payload, int payload_len);
void ui_decode_session_get_state_rsp(unsigned char* payload, int payload_len);
void ui_decode_session_update_controller_multicast_list_rsp(unsigned char* payload, int payload_len);
void ui_decode_session_update_active_rounds_dt_tag_rsp(unsigned char* payload, int payload_len);
void ui_decode_session_data_transfer_phase_config_rsp(unsigned char* payload, int payload_len);
void ui_decode_session_query_data_size_in_ranging_rsp(unsigned char* payload, int payload_len);
void ui_decode_session_set_hybrid_controller_config_rsp(unsigned char* payload, int payload_len);
void ui_decode_session_set_hybrid_controlee_config_rsp(unsigned char* payload, int payload_len);
void ui_decode_session_logical_link_create_rsp(unsigned char* payload, int payload_len);
void ui_decode_session_logical_link_close_rsp(unsigned char* payload, int payload_len);
void ui_decode_session_logical_link_get_param_rsp(unsigned char* payload, int payload_len);

void ui_decode_session_start_rsp(unsigned char* payload, int payload_len);
void ui_decode_session_stop_rsp(unsigned char* payload, int payload_len);
void ui_decode_session_get_ranging_count_rsp(unsigned char* payload, int payload_len);

void ui_decode_core_device_status_ntf(unsigned char* payload, int payload_len);
void ui_decode_core_generic_error_ntf(unsigned char* payload, int payload_len);
void ui_decode_session_status_ntf(unsigned char* payload, int payload_len);
void ui_decode_session_data_credit_ntf(unsigned char* payload, int payload_len);
void ui_decode_session_data_transfer_status_ntf(unsigned char* payload, int payload_len);
void ui_decode_session_info_ntf(unsigned char* payload, int payload_len);
void ui_decode_range_data_ntf(unsigned char* payload, int payload_len);
void ui_decode_android_range_diagnostics_ntf(unsigned char* payload, int payload_len);
void ui_decode_session_logical_link_uwbs_create_ntf(unsigned char* payload, int payload_len);
void ui_decode_session_logical_link_uwbs_close_ntf(unsigned char* payload, int payload_len);

// Android vendor command response decoders
void ui_decode_android_get_power_stats_rsp(unsigned char* payload, int payload_len);
void ui_decode_android_set_country_code_rsp(unsigned char* payload, int payload_len);
void ui_decode_android_radar_set_app_config_rsp(unsigned char* payload, int payload_len);
void ui_decode_android_radar_get_app_config_rsp(unsigned char* payload, int payload_len);

// Shared lookup helpers exposed for enhanced analysis overlays
void ui_print_status_lookup_line(const char* label, unsigned char status_code);
void ui_print_session_state_lookup_line(const char* label, unsigned char session_state);
void ui_print_session_reason_lookup_line(const char* label, unsigned char reason_code);

// Utility functions for reading values in little-endian format
static inline uint16_t ui_read_u16_le(const unsigned char* buffer) {
    return (uint16_t)buffer[0] | ((uint16_t)buffer[1] << 8);
}

static inline uint32_t ui_read_u32_le(const unsigned char* buffer) {
    return (uint32_t)buffer[0] | ((uint32_t)buffer[1] << 8) |
           ((uint32_t)buffer[2] << 16) | ((uint32_t)buffer[3] << 24);
}

static inline uint64_t ui_read_u64_le(const unsigned char* buffer) {
    return (uint64_t)buffer[0] | ((uint64_t)buffer[1] << 8) |
           ((uint64_t)buffer[2] << 16) | ((uint64_t)buffer[3] << 24) |
           ((uint64_t)buffer[4] << 32) | ((uint64_t)buffer[5] << 40) |
           ((uint64_t)buffer[6] << 48) | ((uint64_t)buffer[7] << 56);
}

#endif // UCI_UI_PACKET_DECODER_H
