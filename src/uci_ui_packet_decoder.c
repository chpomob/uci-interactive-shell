#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdarg.h>
#include "../include/uci.h"
#include "../include/uci_ui.h"
#include "../include/uci_ui_packet_decoder.h"
#include "../include/uci_packet_analyzer.h"
#include "../include/uci_packet_utils.h"
#include "../include/uci_config_manager.h"

#ifndef UCI_DIAG_REPORT_AOAS
#define UCI_DIAG_REPORT_AOAS 0x01
#endif
#ifndef UCI_DIAG_REPORT_EXTRA_STATUS
#define UCI_DIAG_REPORT_EXTRA_STATUS 0x03
#endif
#ifndef UCI_DIAG_REPORT_CFO_Q26
#define UCI_DIAG_REPORT_CFO_Q26 0x04
#endif
#ifndef UCI_DIAG_REPORT_EMITTER_SHORT_ADDR
#define UCI_DIAG_REPORT_EMITTER_SHORT_ADDR 0x05
#endif
#ifndef UCI_DIAG_REPORT_SEGMENT_METRICS
#define UCI_DIAG_REPORT_SEGMENT_METRICS 0x06
#endif
#ifndef UCI_DIAG_REPORT_CIRS
#define UCI_DIAG_REPORT_CIRS 0x07
#endif
#ifndef UCI_DIAG_REPORT_CONFIDENCE_METRICS
#define UCI_DIAG_REPORT_CONFIDENCE_METRICS 0x08
#endif
#ifndef UCI_DIAG_REPORT_SEGMENT_CONFIDENCE_RAW_DATA
#define UCI_DIAG_REPORT_SEGMENT_CONFIDENCE_RAW_DATA 0x09
#endif

static void print_lookup_line_with_indent(int indent,
                                          const char* label,
                                          unsigned char value,
                                          const uci_lookup_entry_t* entry,
                                          const char* color) {
    if (ui_color_enabled) {
        printf("%*s%s%s%s: 0x%02X ", indent, "", ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, label, value);
        if (entry) {
            const char* entry_color = color ? color : ANSI_COLOR_BRIGHT_GREEN;
            printf("%s(%s)%s", entry_color, entry->label, ANSI_RESET);
            if (entry->description && entry->description[0] != '\0') {
                printf(" - %s", entry->description);
            }
        } else {
            printf("%s(UNKNOWN)%s", ANSI_COLOR_YELLOW, ANSI_RESET);
        }
        printf("\n");
    } else {
        if (entry) {
            printf("%*s%s: 0x%02X (%s)\n", indent, "", label, value, entry->label);
            if (entry->description && entry->description[0] != '\0') {
                printf("%*s%s Description: %s\n", indent, "", label, entry->description);
            }
        } else {
            printf("%*s%s: 0x%02X (UNKNOWN)\n", indent, "", label, value);
        }
    }
}

static void print_lookup_line(const char* label,
                              unsigned char value,
                              const uci_lookup_entry_t* entry,
                              const char* color) {
    print_lookup_line_with_indent(4, label, value, entry, color);
}

static void ui_print_status_lookup_line_internal(const char* label,
                                                 unsigned char status_code,
                                                 int indent);

static void ui_print_session_state_lookup_line_internal(const char* label,
                                                        unsigned char session_state,
                                                        int indent);

static void ui_print_session_reason_lookup_line_internal(const char* label,
                                                         unsigned char reason_code,
                                                         int indent);

static void print_additional_payload_bytes(const unsigned char* payload,
                                           int payload_len,
                                           int start_index) {
    if (payload_len <= start_index) {
        return;
    }

    if (ui_color_enabled) {
        printf("    %s%sAdditional Payload Bytes:%s ", ANSI_COLOR_BRIGHT_BLACK, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("    Additional Payload Bytes: ");
    }

    for (int i = start_index; i < payload_len; i++) {
        printf("%02X ", payload[i]);
    }
    printf("\n");
}

static void print_field_line_color(int indent,
                                   const char* label,
                                   const char* label_color,
                                   const char* fmt,
                                   ...) {
    char value_buf[256];
    va_list args;
    va_start(args, fmt);
    vsnprintf(value_buf, sizeof(value_buf), fmt, args);
    va_end(args);

    if (ui_color_enabled) {
        const char* color = label_color ? label_color : ANSI_COLOR_BRIGHT_GREEN;
        printf("%*s%s%s%s: %s%s\n",
               indent,
               "",
               color,
               ANSI_BOLD,
               label,
               ANSI_RESET,
               value_buf);
    } else {
        printf("%*s%s: %s\n", indent, "", label, value_buf);
    }
}

static void print_error_line(int indent, const char* fmt, ...) {
    char value_buf[256];
    va_list args;
    va_start(args, fmt);
    vsnprintf(value_buf, sizeof(value_buf), fmt, args);
    va_end(args);

    if (ui_color_enabled) {
        printf("%*s%s%sError:%s %s\n",
               indent,
               "",
               ANSI_COLOR_RED,
               ANSI_BOLD,
               ANSI_RESET,
               value_buf);
    } else {
        printf("%*sError: %s\n", indent, "", value_buf);
    }
}

static void print_warning_line(int indent, const char* fmt, ...) {
    char value_buf[256];
    va_list args;
    va_start(args, fmt);
    vsnprintf(value_buf, sizeof(value_buf), fmt, args);
    va_end(args);

    if (ui_color_enabled) {
        printf("%*s%s%sWarning:%s %s\n",
               indent,
               "",
               ANSI_COLOR_YELLOW,
               ANSI_BOLD,
               ANSI_RESET,
               value_buf);
    } else {
        printf("%*sWarning: %s\n", indent, "", value_buf);
    }
}

static void print_measurement_header(int index) {
    if (ui_color_enabled) {
        printf("    %s%sMeasurement %d:%s\n",
               ANSI_COLOR_BRIGHT_CYAN,
               ANSI_BOLD,
               index,
               ANSI_RESET);
    } else {
        printf("    Measurement %d:\n", index);
    }
}

static void format_mac_address(const unsigned char* mac,
                               int mac_len,
                               char* out,
                               size_t out_size) {
    if (mac_len <= 0 || out_size == 0) {
        if (out_size > 0) {
            out[0] = '\0';
        }
        return;
    }

    if (mac_len == 2) {
        snprintf(out, out_size, "0x%04X", ui_read_u16_le(mac));
        return;
    }

    size_t pos = 0;
    for (int i = mac_len - 1; i >= 0; --i) {
        if (pos >= out_size) {
            break;
        }
        int written = snprintf(out + pos, out_size - pos, "%02X", mac[i]);
        if (written < 0) {
            break;
        }
        pos += (size_t)written;
        if (i > 0 && pos < out_size) {
            out[pos++] = ':';
        }
    }
    if (pos < out_size) {
        out[pos] = '\0';
    } else {
        out[out_size - 1] = '\0';
    }
}

static void print_mac_address_line(int indent,
                                   const unsigned char* mac,
                                   int mac_len) {
    char mac_str[48];
    format_mac_address(mac, mac_len, mac_str, sizeof(mac_str));
    print_field_line_color(indent, "MAC Address", ANSI_COLOR_BRIGHT_GREEN, "%s", mac_str);
}

static uint64_t read_le_bytes(const unsigned char* data, int length) {
    if (length <= 0) {
        return 0;
    }

    uint64_t value = 0;
    int capped_length = (length > 8) ? 8 : length;
    for (int i = capped_length - 1; i >= 0; --i) {
        value = (value << 8) | data[i];
    }
    return value;
}

static double decode_q_signed(uint16_t raw, int fractional_bits) {
    int16_t signed_value = (int16_t)raw;
    return (double)signed_value / (double)(1 << fractional_bits);
}

static double decode_q_unsigned(uint16_t raw, int fractional_bits) {
    return (double)raw / (double)(1 << fractional_bits);
}

static const char* map_timestamp_confidence_level(uint8_t value) {
    switch (value & 0x07) {
        case 0: return "No FOM";
        case 1: return "20%";
        case 2: return "55%";
        case 3: return "75%";
        case 4: return "85%";
        case 5: return "92%";
        case 6: return "97%";
        case 7: return "99%";
        default: return "Unknown";
    }
}

static const char* map_ranging_measurement_type(uint8_t type) {
    switch (type) {
        case 0x00: return "OWR UL-TDoA";
        case 0x01: return "TWR";
        case 0x02: return "OWR DL-TDoA";
        case 0x03: return "OWR AoA";
        case 0x04: return "CCC Controller";
        case 0x05: return "CCC Controlee";
        case 0x06: return "OWR DL-TDoA v2";
        default:   return "Unknown";
    }
}

static const char* map_ranging_msg_type(uint8_t type) {
    switch (type) {
        case 0x00: return "Poll DTM";
        case 0x01: return "Response DTM";
        case 0x02: return "Final DTM";
        default:   return "Unknown";
    }
}

typedef struct {
    const char* time_reference;
    int tx_time_size_bits;
    int rx_time_size_bits;
    const char* anchor_location_type;
    int anchor_location_size_bytes;
    uint8_t active_ranging_count;
} ranging_msg_control_t;

static void parse_ranging_msg_control(uint16_t word, ranging_msg_control_t* out) {
    static const char* k_time_options[] = { "Local", "Common" };
    static const char* k_anchor_types[] = { "Unknown", "WGS-84", "Relative", "RFU" };
    static const int k_anchor_sizes[] = { 0, 12, 10, 0 };

    out->time_reference = k_time_options[word & 0x01];

    uint8_t tx_flag = (word >> 1) & 0x03;
    out->tx_time_size_bits = (tx_flag == 0) ? 40 : (tx_flag == 1) ? 64 : 0;

    uint8_t rx_flag = (word >> 3) & 0x03;
    out->rx_time_size_bits = (rx_flag == 0) ? 40 : (rx_flag == 1) ? 64 : 0;

    uint8_t loc_idx = (word >> 5) & 0x03;
    out->anchor_location_type = k_anchor_types[loc_idx];
    out->anchor_location_size_bytes = k_anchor_sizes[loc_idx];

    out->active_ranging_count = (word >> 7) & 0x0F;
}

typedef struct {
    const char* time_reference;
    bool active_ranging_round_present;
    bool location_present;
} ranging_msg_control_v2_t;

static void parse_ranging_msg_control_v2(uint32_t word,
                                         ranging_msg_control_v2_t* out) {
    out->time_reference = (word & 0x01) ? "Common" : "Local";
    out->active_ranging_round_present = ((word >> 1) & 0x01) != 0;
    out->location_present = ((word >> 2) & 0x01) != 0;
}

static void print_hex_data(int indent,
                           const char* label,
                           const unsigned char* data,
                           int length,
                           int max_bytes) {
    if (length <= 0) {
        print_field_line_color(indent, label, ANSI_COLOR_BRIGHT_BLACK, "(none)");
        return;
    }

    char buffer[512];
    int pos = 0;
    int bytes_to_show = (max_bytes > 0 && length > max_bytes) ? max_bytes : length;

    for (int i = 0; i < bytes_to_show; i++) {
        if (pos >= (int)sizeof(buffer) - 4) {
            break;
        }
        pos += snprintf(buffer + pos, sizeof(buffer) - (size_t)pos, "%02X%s",
                        data[i], (i + 1 < bytes_to_show) ? " " : "");
    }

    if (bytes_to_show < length && pos < (int)sizeof(buffer) - 5) {
        snprintf(buffer + pos, sizeof(buffer) - (size_t)pos, " ...");
    }

    print_field_line_color(indent, label, ANSI_COLOR_BRIGHT_BLACK, "%s", buffer);
}

static void print_timestamp_confidence_analysis(int indent, uint8_t nlos_byte) {
    bool hw_error = (nlos_byte & 0x01) != 0;
    uint8_t reserved = (nlos_byte >> 1) & 0x1F;
    uint8_t ts_conf = (nlos_byte >> 5) & 0x07;

    print_field_line_color(indent,
                           "First Path Detection Error",
                           ANSI_COLOR_BRIGHT_GREEN,
                           "%s",
                           hw_error ? "Yes" : "No");
    print_field_line_color(indent,
                           "Timestamp Confidence",
                           ANSI_COLOR_BRIGHT_GREEN,
                           "%s (code %u)",
                           map_timestamp_confidence_level(ts_conf),
                           ts_conf);
    if (reserved != 0) {
        print_field_line_color(indent,
                               "Reserved Flags",
                               ANSI_COLOR_BRIGHT_BLACK,
                               "0x%02X",
                               reserved);
    }
}

static bool decode_range_measurement_twr(const unsigned char* payload,
                                         int payload_len,
                                         int* offset,
                                         int mac_len,
                                         int measurement_index) {
    (void)measurement_index;
    int cursor = *offset;
    int required = (mac_len == 2) ? 20 : 26;

    if (cursor + required > payload_len) {
        print_error_line(6, "Measurement truncated (need %d bytes, have %d)",
                         required, payload_len - cursor);
        return false;
    }

    const unsigned char* mac_ptr = &payload[cursor];
    cursor += mac_len;

    unsigned char status = payload[cursor++];
    unsigned char nlos = payload[cursor++];
    uint16_t distance = ui_read_u16_le(&payload[cursor]);
    cursor += 2;
    double aoa_azimuth = decode_q_signed(ui_read_u16_le(&payload[cursor]), 7);
    cursor += 2;
    unsigned char aoa_azimuth_fom = payload[cursor++];
    double aoa_elevation = decode_q_signed(ui_read_u16_le(&payload[cursor]), 7);
    cursor += 2;
    unsigned char aoa_elevation_fom = payload[cursor++];
    double dst_aoa_azimuth = decode_q_signed(ui_read_u16_le(&payload[cursor]), 7);
    cursor += 2;
    unsigned char dst_aoa_azimuth_fom = payload[cursor++];
    double dst_aoa_elevation = decode_q_signed(ui_read_u16_le(&payload[cursor]), 7);
    cursor += 2;
    unsigned char dst_aoa_elevation_fom = payload[cursor++];
    unsigned char slot_index = payload[cursor++];
    double rssi_dbm = (double)(int8_t)payload[cursor++];

    print_mac_address_line(6, mac_ptr, mac_len);
    ui_print_status_lookup_line_internal("Status", status, 6);
    print_field_line_color(6, "NLOS Flags", ANSI_COLOR_BRIGHT_GREEN, "0x%02X", nlos);
    print_timestamp_confidence_analysis(8, nlos);
    print_field_line_color(6, "Distance", ANSI_COLOR_BRIGHT_GREEN, "%u cm", distance);
    print_field_line_color(6,
                           "AoA Azimuth",
                           ANSI_COLOR_BRIGHT_GREEN,
                           "%.2f° (FoM %u%%)",
                           aoa_azimuth,
                           aoa_azimuth_fom);
    print_field_line_color(6,
                           "AoA Elevation",
                           ANSI_COLOR_BRIGHT_GREEN,
                           "%.2f° (FoM %u%%)",
                           aoa_elevation,
                           aoa_elevation_fom);
    print_field_line_color(6,
                           "Dest AoA Azimuth",
                           ANSI_COLOR_BRIGHT_GREEN,
                           "%.2f° (FoM %u%%)",
                           dst_aoa_azimuth,
                           dst_aoa_azimuth_fom);
    print_field_line_color(6,
                           "Dest AoA Elevation",
                           ANSI_COLOR_BRIGHT_GREEN,
                           "%.2f° (FoM %u%%)",
                           dst_aoa_elevation,
                           dst_aoa_elevation_fom);
    print_field_line_color(6, "Slot Index", ANSI_COLOR_BRIGHT_GREEN, "%u", slot_index);
    print_field_line_color(6, "RSSI", ANSI_COLOR_BRIGHT_GREEN, "%.1f dBm", rssi_dbm);

    *offset = cursor;
    return true;
}

static bool decode_range_measurement_owr_aoa(const unsigned char* payload,
                                             int payload_len,
                                             int* offset,
                                             int mac_len,
                                             int measurement_index) {
    (void)measurement_index;
    int cursor = *offset;
    int required = mac_len + 11;

    if (cursor + required > payload_len) {
        print_error_line(6, "OWR AoA measurement truncated (need %d bytes, have %d)",
                         required, payload_len - cursor);
        return false;
    }

    const unsigned char* mac_ptr = &payload[cursor];
    cursor += mac_len;

    unsigned char status = payload[cursor++];
    unsigned char nlos_flag = payload[cursor++];
    unsigned char frame_seq = payload[cursor++];
    uint16_t block_index = ui_read_u16_le(&payload[cursor]);
    cursor += 2;
    double aoa_azimuth = decode_q_signed(ui_read_u16_le(&payload[cursor]), 7);
    cursor += 2;
    unsigned char aoa_azimuth_fom = payload[cursor++];
    double aoa_elevation = decode_q_signed(ui_read_u16_le(&payload[cursor]), 7);
    cursor += 2;
    unsigned char aoa_elevation_fom = payload[cursor++];

    print_mac_address_line(6, mac_ptr, mac_len);
    ui_print_status_lookup_line_internal("Status", status, 6);
    print_field_line_color(6,
                           "Is NLOS",
                           ANSI_COLOR_BRIGHT_GREEN,
                           "%s",
                           nlos_flag ? "Yes" : "No");
    print_field_line_color(6,
                           "Frame Sequence",
                           ANSI_COLOR_BRIGHT_GREEN,
                           "%u",
                           frame_seq);
    print_field_line_color(6,
                           "Block Index",
                           ANSI_COLOR_BRIGHT_GREEN,
                           "%u",
                           block_index);
    print_field_line_color(6,
                           "AoA Azimuth",
                           ANSI_COLOR_BRIGHT_GREEN,
                           "%.2f° (FoM %u%%)",
                           aoa_azimuth,
                           aoa_azimuth_fom);
    print_field_line_color(6,
                           "AoA Elevation",
                           ANSI_COLOR_BRIGHT_GREEN,
                           "%.2f° (FoM %u%%)",
                           aoa_elevation,
                           aoa_elevation_fom);

    *offset = cursor;
    return true;
}

static bool decode_range_measurement_owr_ul_tdoa(const unsigned char* payload,
                                                int payload_len,
                                                int* offset,
                                                int mac_len,
                                                int measurement_index) {
    (void)measurement_index;
    int cursor = *offset;

    if (cursor + mac_len + 12 > payload_len) {
        print_error_line(6, "OWR UL-TDoA measurement truncated");
        return false;
    }

    const unsigned char* mac_ptr = &payload[cursor];
    cursor += mac_len;

    unsigned char status = payload[cursor++];
    uint16_t control_word = ui_read_u16_le(&payload[cursor]);
    cursor += 2;
    ranging_msg_control_t ctrl;
    parse_ranging_msg_control(control_word, &ctrl);

    unsigned char frame_type = payload[cursor++];
    unsigned char nlos_flag = payload[cursor++];
    double aoa_azimuth = decode_q_signed(ui_read_u16_le(&payload[cursor]), 7);
    cursor += 2;
    unsigned char aoa_azimuth_fom = payload[cursor++];
    double aoa_elevation = decode_q_signed(ui_read_u16_le(&payload[cursor]), 7);
    cursor += 2;
    unsigned char aoa_elevation_fom = payload[cursor++];
    uint32_t frame_number = ui_read_u32_le(&payload[cursor]);
    cursor += 4;

    int rx_time_bytes = ctrl.rx_time_size_bits / 8;
    uint64_t rx_time = 0;
    if (rx_time_bytes > 0) {
        if (cursor + rx_time_bytes > payload_len) {
            print_error_line(6, "OWR UL-TDoA rx_time exceeds payload");
            return false;
        }
        rx_time = read_le_bytes(&payload[cursor], rx_time_bytes);
        cursor += rx_time_bytes;
    }

    print_mac_address_line(6, mac_ptr, mac_len);
    ui_print_status_lookup_line_internal("Status", status, 6);
    print_field_line_color(6,
                           "Message Control",
                           ANSI_COLOR_BRIGHT_GREEN,
                           "%s time, TxTime=%s, RxTime=%s, AnchorLoc=%s",
                           ctrl.time_reference,
                           ctrl.tx_time_size_bits ? (ctrl.tx_time_size_bits == 40 ? "40 bits" : "64 bits") : "not provided",
                           ctrl.rx_time_size_bits ? (ctrl.rx_time_size_bits == 40 ? "40 bits" : "64 bits") : "not provided",
                           ctrl.anchor_location_type);
    const char* frame_type_str = (frame_type == 0) ? "UT-Tag" : (frame_type == 1) ? "UT-Synchronization Anchor" : "Unknown";
    print_field_line_color(6,
                           "Frame Type",
                           ANSI_COLOR_BRIGHT_GREEN,
                           "%s (0x%02X)",
                           frame_type_str,
                           frame_type);
    print_field_line_color(6,
                           "Is NLOS",
                           ANSI_COLOR_BRIGHT_GREEN,
                           "%s",
                           nlos_flag ? "Yes" : "No");
    print_field_line_color(6,
                           "AoA Azimuth",
                           ANSI_COLOR_BRIGHT_GREEN,
                           "%.2f° (FoM %u%%)",
                           aoa_azimuth,
                           aoa_azimuth_fom);
    print_field_line_color(6,
                           "AoA Elevation",
                           ANSI_COLOR_BRIGHT_GREEN,
                           "%.2f° (FoM %u%%)",
                           aoa_elevation,
                           aoa_elevation_fom);
    print_field_line_color(6,
                           "Frame Number",
                           ANSI_COLOR_BRIGHT_GREEN,
                           "0x%08X (%u)",
                           frame_number,
                           frame_number);
    if (rx_time_bytes > 0) {
        print_field_line_color(6,
                               "Rx Time",
                               ANSI_COLOR_BRIGHT_GREEN,
                               "%llu ticks (%d bytes)",
                               (unsigned long long)rx_time,
                               rx_time_bytes);
    }

    *offset = cursor;
    return true;
}

static bool decode_range_measurement_owr_dl_tdoa(const unsigned char* payload,
                                                 int payload_len,
                                                 int* offset,
                                                 int mac_len,
                                                 int measurement_index) {
    (void)measurement_index;
    int cursor = *offset;

    if (cursor + mac_len + 20 > payload_len) {
        print_error_line(6, "OWR DL-TDoA measurement truncated");
        return false;
    }

    const unsigned char* mac_ptr = &payload[cursor];
    cursor += mac_len;

    unsigned char status = payload[cursor++];
    unsigned char msg_type = payload[cursor++];
    uint16_t control_word = ui_read_u16_le(&payload[cursor]);
    cursor += 2;
    ranging_msg_control_t ctrl;
    parse_ranging_msg_control(control_word, &ctrl);

    uint16_t block_index = ui_read_u16_le(&payload[cursor]);
    cursor += 2;
    unsigned char round_index = payload[cursor++];
    bool nlos = payload[cursor++] != 0;
    double aoa_azimuth = decode_q_signed(ui_read_u16_le(&payload[cursor]), 7);
    cursor += 2;
    unsigned char aoa_azimuth_fom = payload[cursor++];
    double aoa_elevation = decode_q_signed(ui_read_u16_le(&payload[cursor]), 7);
    cursor += 2;
    unsigned char aoa_elevation_fom = payload[cursor++];
    double rssi_dbm = -decode_q_unsigned(payload[cursor++], 1);

    int tx_time_bytes = ctrl.tx_time_size_bits / 8;
    uint64_t tx_time = 0;
    if (tx_time_bytes > 0) {
        if (cursor + tx_time_bytes > payload_len) {
            print_error_line(6, "Tx time exceeds payload");
            return false;
        }
        tx_time = read_le_bytes(&payload[cursor], tx_time_bytes);
        cursor += tx_time_bytes;
    }

    int rx_time_bytes = ctrl.rx_time_size_bits / 8;
    uint64_t rx_time = 0;
    if (rx_time_bytes > 0) {
        if (cursor + rx_time_bytes > payload_len) {
            print_error_line(6, "Rx time exceeds payload");
            return false;
        }
        rx_time = read_le_bytes(&payload[cursor], rx_time_bytes);
        cursor += rx_time_bytes;
    }

    if (cursor + 12 > payload_len) {
        print_error_line(6, "CFO/reply time fields truncated");
        return false;
    }

    double anchor_cfo = decode_q_signed(ui_read_u16_le(&payload[cursor]), 10);
    cursor += 2;
    double cfo = decode_q_signed(ui_read_u16_le(&payload[cursor]), 10);
    cursor += 2;
    uint32_t initiator_reply_time = ui_read_u32_le(&payload[cursor]);
    cursor += 4;
    uint32_t responder_reply_time = ui_read_u32_le(&payload[cursor]);
    cursor += 4;
    if (cursor + 2 > payload_len) {
        print_error_line(6, "ToF field truncated");
        return false;
    }
    uint16_t tof = ui_read_u16_le(&payload[cursor]);
    cursor += 2;

    const unsigned char* anchor_loc_data = NULL;
    int anchor_loc_bytes = ctrl.anchor_location_size_bytes;
    if (anchor_loc_bytes > 0) {
        if (cursor + anchor_loc_bytes > payload_len) {
            print_error_line(6, "Anchor location truncated");
            return false;
        }
        anchor_loc_data = &payload[cursor];
        cursor += anchor_loc_bytes;
    }

    const unsigned char* active_rounds = NULL;
    uint8_t active_count = ctrl.active_ranging_count;
    if (active_count > 0) {
        if (cursor + active_count > payload_len) {
            print_error_line(6, "Active rounds truncated");
            return false;
        }
        active_rounds = &payload[cursor];
        cursor += active_count;
    }

    print_mac_address_line(6, mac_ptr, mac_len);
    ui_print_status_lookup_line_internal("Status", status, 6);
    print_field_line_color(6,
                           "Message Type",
                           ANSI_COLOR_BRIGHT_GREEN,
                           "%s (0x%02X)",
                           map_ranging_msg_type(msg_type),
                           msg_type);
    print_field_line_color(6,
                           "Message Control",
                           ANSI_COLOR_BRIGHT_GREEN,
                           "%s time, TxTime=%s, RxTime=%s, AnchorLoc=%s, ActiveRoundsBytes=%u",
                           ctrl.time_reference,
                           ctrl.tx_time_size_bits ? (ctrl.tx_time_size_bits == 40 ? "40 bits" : "64 bits") : "not provided",
                           ctrl.rx_time_size_bits ? (ctrl.rx_time_size_bits == 40 ? "40 bits" : "64 bits") : "not provided",
                           ctrl.anchor_location_type,
                           active_count);
    print_field_line_color(6,
                           "Block Index",
                           ANSI_COLOR_BRIGHT_GREEN,
                           "%u",
                           block_index);
    print_field_line_color(6,
                           "Round Index",
                           ANSI_COLOR_BRIGHT_GREEN,
                           "%u",
                           round_index);
    print_field_line_color(6,
                           "Is NLOS",
                           ANSI_COLOR_BRIGHT_GREEN,
                           "%s",
                           nlos ? "Yes" : "No");
    print_field_line_color(6,
                           "AoA Azimuth",
                           ANSI_COLOR_BRIGHT_GREEN,
                           "%.2f° (FoM %u%%)",
                           aoa_azimuth,
                           aoa_azimuth_fom);
    print_field_line_color(6,
                           "AoA Elevation",
                           ANSI_COLOR_BRIGHT_GREEN,
                           "%.2f° (FoM %u%%)",
                           aoa_elevation,
                           aoa_elevation_fom);
    print_field_line_color(6, "RSSI", ANSI_COLOR_BRIGHT_GREEN, "%.1f dBm", rssi_dbm);
    if (tx_time_bytes > 0) {
        print_field_line_color(6,
                               "Tx Time",
                               ANSI_COLOR_BRIGHT_GREEN,
                               "%llu ticks (%d bytes)",
                               (unsigned long long)tx_time,
                               tx_time_bytes);
    }
    if (rx_time_bytes > 0) {
        print_field_line_color(6,
                               "Rx Time",
                               ANSI_COLOR_BRIGHT_GREEN,
                               "%llu ticks (%d bytes)",
                               (unsigned long long)rx_time,
                               rx_time_bytes);
    }
    print_field_line_color(6,
                           "Anchor CFO",
                           ANSI_COLOR_BRIGHT_GREEN,
                           "%.4f ppm",
                           anchor_cfo);
    print_field_line_color(6,
                           "CFO",
                           ANSI_COLOR_BRIGHT_GREEN,
                           "%.4f ppm",
                           cfo);
    print_field_line_color(6,
                           "Initiator Reply Time",
                           ANSI_COLOR_BRIGHT_GREEN,
                           "%u ticks",
                           initiator_reply_time);
    print_field_line_color(6,
                           "Responder Reply Time",
                           ANSI_COLOR_BRIGHT_GREEN,
                           "%u ticks",
                           responder_reply_time);
    print_field_line_color(6,
                           "Time of Flight",
                           ANSI_COLOR_BRIGHT_GREEN,
                           "%u ticks",
                           tof);
    if (anchor_loc_bytes > 0) {
        print_hex_data(6,
                       "Anchor Location",
                       anchor_loc_data,
                       anchor_loc_bytes,
                       anchor_loc_bytes);
    }
    if (active_count > 0) {
        print_hex_data(6,
                       "Active Rounds",
                       active_rounds,
                       active_count,
                       active_count);
    }

    *offset = cursor;
    return true;
}

static bool decode_range_measurement_ccc_controller(const unsigned char* payload,
                                                    int payload_len,
                                                    int* offset,
                                                    int measurement_index) {
    (void)measurement_index;
    int cursor = *offset;
    const int required = 1 + 1 + 1 + 14 + 1 + 1 + 10;

    if (cursor + required > payload_len) {
        print_error_line(6, "CCC controller measurement truncated");
        return false;
    }

    cursor += 1; // Empty/reserved byte
    unsigned char status = payload[cursor++];
    unsigned char sts_quality = payload[cursor++];
    cursor += 14; // Empty block
    unsigned char slot_index = payload[cursor++];
    unsigned char rssi = payload[cursor++];
    cursor += 10; // RFU

    ui_print_status_lookup_line_internal("Status", status, 6);
    print_field_line_color(6, "STS Quality", ANSI_COLOR_BRIGHT_GREEN, "%u", sts_quality);
    print_field_line_color(6, "Slot Index", ANSI_COLOR_BRIGHT_GREEN, "%u", slot_index);
    print_field_line_color(6, "RSSI", ANSI_COLOR_BRIGHT_GREEN, "%u", rssi);

    *offset = cursor;
    return true;
}

static bool decode_range_measurement_ccc_controlee(const unsigned char* payload,
                                                   int payload_len,
                                                   int* offset,
                                                   int measurement_index) {
    (void)measurement_index;
    int cursor = *offset;
    const int required = 1 + 1 + 2 + 4 + 2 + 1 + 1 + 11;

    if (cursor + required > payload_len) {
        print_error_line(6, "CCC controlee measurement truncated");
        return false;
    }

    unsigned char status = payload[cursor++];
    unsigned char slot_index = payload[cursor++];
    uint16_t rr_index = ui_read_u16_le(&payload[cursor]);
    cursor += 2;
    uint32_t sts_index = ui_read_u32_le(&payload[cursor]);
    cursor += 4;
    uint16_t distance = ui_read_u16_le(&payload[cursor]);
    cursor += 2;
    unsigned char uncertainty_anchor = payload[cursor++];
    unsigned char uncertainty_initiator = payload[cursor++];
    cursor += 11; // RFU

    ui_print_status_lookup_line_internal("Status", status, 6);
    print_field_line_color(6, "Slot Index", ANSI_COLOR_BRIGHT_GREEN, "%u", slot_index);
    print_field_line_color(6, "RR Index", ANSI_COLOR_BRIGHT_GREEN, "%u", rr_index);
    print_field_line_color(6, "STS Index", ANSI_COLOR_BRIGHT_GREEN, "%u", sts_index);
    print_field_line_color(6, "Distance", ANSI_COLOR_BRIGHT_GREEN, "%u cm", distance);
    print_field_line_color(6,
                           "Timestamp Uncertainty (Anchor)",
                           ANSI_COLOR_BRIGHT_GREEN,
                           "%u",
                           uncertainty_anchor);
    print_field_line_color(6,
                           "Timestamp Uncertainty (Initiator)",
                           ANSI_COLOR_BRIGHT_GREEN,
                           "%u",
                           uncertainty_initiator);

    *offset = cursor;
    return true;
}

static bool decode_range_measurement_owr_dl_tdoa_v2(const unsigned char* payload,
                                                    int payload_len,
                                                    int* offset,
                                                    int mac_len,
                                                    int measurement_index) {
    (void)measurement_index;
    int cursor = *offset;

    if (cursor + 2 > payload_len) {
        print_error_line(6, "OWR DL-TDoA v2 measurement truncated (size field)");
        return false;
    }

    uint16_t measurement_size = ui_read_u16_le(&payload[cursor]);
    cursor += 2;
    int measurement_end = cursor + measurement_size;
    if (measurement_end > payload_len) {
        print_error_line(6,
                         "OWR DL-TDoA v2 measurement exceeds payload (need %d bytes, have %d)",
                         measurement_size,
                         payload_len - cursor);
        return false;
    }

    if (cursor + mac_len + 18 > measurement_end) {
        print_error_line(6, "OWR DL-TDoA v2 measurement too small");
        return false;
    }

    const unsigned char* mac_ptr = &payload[cursor];
    cursor += mac_len;

    unsigned char status = payload[cursor++];
    unsigned char msg_type = payload[cursor++];
    uint16_t block_index = ui_read_u16_le(&payload[cursor]);
    cursor += 2;
    unsigned char round_index = payload[cursor++];
    bool nlos = payload[cursor++] != 0;
    double aoa_azimuth = decode_q_signed(ui_read_u16_le(&payload[cursor]), 7);
    cursor += 2;
    unsigned char aoa_azimuth_fom = payload[cursor++];
    double aoa_elevation = decode_q_signed(ui_read_u16_le(&payload[cursor]), 7);
    cursor += 2;
    unsigned char aoa_elevation_fom = payload[cursor++];
    double rssi_dbm = -decode_q_unsigned(payload[cursor++], 1);
    double anchor_cfo = decode_q_signed(ui_read_u16_le(&payload[cursor]), 10);
    cursor += 2;
    double cfo = decode_q_signed(ui_read_u16_le(&payload[cursor]), 10);
    cursor += 2;
    uint32_t initiator_reply_time = ui_read_u32_le(&payload[cursor]);
    cursor += 4;
    uint32_t responder_reply_time = ui_read_u32_le(&payload[cursor]);
    cursor += 4;
    uint16_t tof = ui_read_u16_le(&payload[cursor]);
    cursor += 2;

    if (cursor >= measurement_end) {
        print_error_line(6, "OWR DL-TDoA v2 measurement missing Rx time size");
        return false;
    }

    unsigned char rx_time_size = payload[cursor++];
    uint64_t rx_time = 0;
    if (rx_time_size > 0) {
        if (cursor + rx_time_size > measurement_end) {
            print_error_line(6, "Rx time exceeds measurement size");
            return false;
        }
        rx_time = read_le_bytes(&payload[cursor], rx_time_size);
        cursor += rx_time_size;
    }

    if (cursor + 4 > measurement_end) {
        print_error_line(6, "OWR DL-TDoA v2 missing message control field");
        return false;
    }

    uint32_t control_word = ui_read_u32_le(&payload[cursor]);
    cursor += 4;
    ranging_msg_control_v2_t ctrl;
    parse_ranging_msg_control_v2(control_word, &ctrl);

    if (cursor >= measurement_end) {
        print_error_line(6, "OWR DL-TDoA v2 missing Tx time size");
        return false;
    }

    unsigned char tx_time_size = payload[cursor++];
    uint64_t tx_time = 0;
    if (tx_time_size > 0) {
        if (cursor + tx_time_size > measurement_end) {
            print_error_line(6, "Tx time exceeds measurement size");
            return false;
        }
        tx_time = read_le_bytes(&payload[cursor], tx_time_size);
        cursor += tx_time_size;
    }

    const unsigned char* active_rounds = NULL;
    unsigned char active_rounds_len = 0;
    if (ctrl.active_ranging_round_present) {
        if (cursor >= measurement_end) {
            print_error_line(6, "OWR DL-TDoA v2 missing active rounds length");
            return false;
        }
        active_rounds_len = payload[cursor++];
        if (cursor + active_rounds_len > measurement_end) {
            print_error_line(6, "Active rounds exceed measurement size");
            return false;
        }
        active_rounds = &payload[cursor];
        cursor += active_rounds_len;
    }

    const unsigned char* anchor_loc_data = NULL;
    unsigned char anchor_loc_len = 0;
    const char* anchor_loc_type = "Not provided";
    if (ctrl.location_present) {
        if (cursor + 2 > measurement_end) {
            print_error_line(6, "OWR DL-TDoA v2 missing anchor location metadata");
            return false;
        }
        anchor_loc_len = payload[cursor++];
        unsigned char loc_type_code = payload[cursor++];
        static const char* k_anchor_types_v2[] = {
            "Unknown",
            "WGS-84",
            "Relative",
            "WGS-84-Z",
            "Relative-Z",
            "Relative-Gravity",
            "Relative-Gravity-Z",
            "RFU"
        };
        if (loc_type_code < 7) {
            anchor_loc_type = k_anchor_types_v2[loc_type_code + 1];
        } else {
            anchor_loc_type = "Unknown";
        }
        if (cursor + anchor_loc_len > measurement_end) {
            print_error_line(6, "Anchor location exceeds measurement size");
            return false;
        }
        anchor_loc_data = &payload[cursor];
        cursor += anchor_loc_len;
    }

    if (cursor > measurement_end) {
        print_warning_line(6, "OWR DL-TDoA v2 measurement parsed past expected size");
        cursor = measurement_end;
    }

    print_field_line_color(6,
                           "Measurement Size",
                           ANSI_COLOR_BRIGHT_GREEN,
                           "%u bytes",
                           measurement_size);
    print_mac_address_line(6, mac_ptr, mac_len);
    ui_print_status_lookup_line_internal("Status", status, 6);
    print_field_line_color(6,
                           "Message Type",
                           ANSI_COLOR_BRIGHT_GREEN,
                           "%s (0x%02X)",
                           map_ranging_msg_type(msg_type),
                           msg_type);
    print_field_line_color(6,
                           "Block Index",
                           ANSI_COLOR_BRIGHT_GREEN,
                           "%u",
                           block_index);
    print_field_line_color(6,
                           "Round Index",
                           ANSI_COLOR_BRIGHT_GREEN,
                           "%u",
                           round_index);
    print_field_line_color(6,
                           "Is NLOS",
                           ANSI_COLOR_BRIGHT_GREEN,
                           "%s",
                           nlos ? "Yes" : "No");
    print_field_line_color(6,
                           "AoA Azimuth",
                           ANSI_COLOR_BRIGHT_GREEN,
                           "%.2f° (FoM %u%%)",
                           aoa_azimuth,
                           aoa_azimuth_fom);
    print_field_line_color(6,
                           "AoA Elevation",
                           ANSI_COLOR_BRIGHT_GREEN,
                           "%.2f° (FoM %u%%)",
                           aoa_elevation,
                           aoa_elevation_fom);
    print_field_line_color(6, "RSSI", ANSI_COLOR_BRIGHT_GREEN, "%.1f dBm", rssi_dbm);
    print_field_line_color(6,
                           "Anchor CFO",
                           ANSI_COLOR_BRIGHT_GREEN,
                           "%.4f ppm",
                           anchor_cfo);
    print_field_line_color(6, "CFO", ANSI_COLOR_BRIGHT_GREEN, "%.4f ppm", cfo);
    print_field_line_color(6,
                           "Initiator Reply Time",
                           ANSI_COLOR_BRIGHT_GREEN,
                           "%u ticks",
                           initiator_reply_time);
    print_field_line_color(6,
                           "Responder Reply Time",
                           ANSI_COLOR_BRIGHT_GREEN,
                           "%u ticks",
                           responder_reply_time);
    print_field_line_color(6,
                           "Time of Flight",
                           ANSI_COLOR_BRIGHT_GREEN,
                           "%u ticks",
                           tof);
    if (rx_time_size > 0) {
        print_field_line_color(6,
                               "Rx Time",
                               ANSI_COLOR_BRIGHT_GREEN,
                               "%llu ticks (%u bytes)",
                               (unsigned long long)rx_time,
                               rx_time_size);
    }
    if (tx_time_size > 0) {
        print_field_line_color(6,
                               "Tx Time",
                               ANSI_COLOR_BRIGHT_GREEN,
                               "%llu ticks (%u bytes)",
                               (unsigned long long)tx_time,
                               tx_time_size);
    }
    print_field_line_color(6,
                           "Message Control",
                           ANSI_COLOR_BRIGHT_GREEN,
                           "%s time, ActiveRounds=%s, LocationPresent=%s",
                           ctrl.time_reference,
                           ctrl.active_ranging_round_present ? "Yes" : "No",
                           ctrl.location_present ? "Yes" : "No");
    if (active_rounds_len > 0) {
        print_hex_data(6, "Active Rounds", active_rounds, active_rounds_len, active_rounds_len);
    }
    if (anchor_loc_data && anchor_loc_len > 0) {
        print_field_line_color(6,
                               "Anchor Location Type",
                               ANSI_COLOR_BRIGHT_GREEN,
                               "%s",
                               anchor_loc_type);
        print_hex_data(6,
                       "Anchor Location",
                       anchor_loc_data,
                       anchor_loc_len,
                       anchor_loc_len);
    }

    *offset = measurement_end;
    return true;
}

static bool decode_range_measurement_unknown(int measurement_index,
                                             uint8_t measurement_type) {
    print_warning_line(6,
                       "Measurement %d uses unsupported type 0x%02X",
                       measurement_index,
                       measurement_type);
    return false;
}

static void ui_print_status_lookup_line_internal(const char* label,
                                                 unsigned char status_code,
                                                 int indent) {
    const uci_lookup_entry_t* entry = uci_lookup_status(status_code);

    const char* status_color = ANSI_COLOR_BRIGHT_YELLOW;
    if (entry && entry->label) {
        if (strncasecmp(entry->label, "OK", 2) == 0 || strstr(entry->label, "SUCCESS") != NULL) {
            status_color = ANSI_COLOR_BRIGHT_GREEN;
        } else if (strstr(entry->label, "ERROR") != NULL || strstr(entry->label, "FAILED") != NULL) {
            status_color = ANSI_COLOR_RED;
        } else if (strstr(entry->label, "RFU") != NULL || strstr(entry->label, "RESERVED") != NULL) {
            status_color = ANSI_COLOR_BRIGHT_BLACK;
        } else {
            status_color = ANSI_COLOR_BRIGHT_CYAN;
        }
    } else if (status_code == UCI_STATUS_OK) {
        status_color = ANSI_COLOR_BRIGHT_GREEN;
    }

    print_lookup_line_with_indent(indent, label, status_code, entry, status_color);
}

void ui_print_status_lookup_line(const char* label, unsigned char status_code) {
    ui_print_status_lookup_line_internal(label, status_code, 4);
}

static void ui_print_session_state_lookup_line_internal(const char* label,
                                                        unsigned char session_state,
                                                        int indent) {
    const uci_lookup_entry_t* entry = uci_lookup_session_state(session_state);

    const char* state_color = ANSI_COLOR_RED;
    switch (session_state) {
        case SESSION_STATE_INIT:
            state_color = ANSI_COLOR_BRIGHT_BLUE;
            break;
        case SESSION_STATE_DEINIT:
            state_color = ANSI_COLOR_YELLOW;
            break;
        case SESSION_STATE_ACTIVE:
            state_color = ANSI_COLOR_BRIGHT_GREEN;
            break;
        case SESSION_STATE_IDLE:
            state_color = ANSI_COLOR_BRIGHT_CYAN;
            break;
        default:
            state_color = ANSI_COLOR_RED;
            break;
    }

    print_lookup_line_with_indent(indent, label, session_state, entry, state_color);
}

void ui_print_session_state_lookup_line(const char* label, unsigned char session_state) {
    ui_print_session_state_lookup_line_internal(label, session_state, 4);
}

static void ui_print_session_reason_lookup_line_internal(const char* label,
                                                         unsigned char reason_code,
                                                         int indent) {
    const uci_lookup_entry_t* entry = uci_lookup_session_reason(reason_code);

    const char* reason_color = ANSI_COLOR_BRIGHT_CYAN;
    if (entry && entry->label && strstr(entry->label, "ERROR") != NULL) {
        reason_color = ANSI_COLOR_RED;
    }

    print_lookup_line_with_indent(indent, label, reason_code, entry, reason_color);
}

void ui_print_session_reason_lookup_line(const char* label, unsigned char reason_code) {
    ui_print_session_reason_lookup_line_internal(label, reason_code, 4);
}

// Function declaration for enhanced error analysis from packet_analyzer.c
void enhanced_error_analysis(unsigned char status_code);

// Enhanced UI version of analyze_uci_packet - now just calls unified analyzer
void ui_analyze_uci_packet(unsigned char* packet, size_t packet_len) {
    // Call unified analyzer which respects ui_color_enabled
    uci_analyze_packet_core(packet, packet_len);
}


// Enhanced packet decoding functions with UI enhancements

void ui_decode_core_device_info_rsp(const unsigned char* payload, int payload_len) {
    if (ui_color_enabled) {
        printf("  %s%sCORE_DEVICE_INFO Response:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  CORE_DEVICE_INFO Response:\n");
    }

    if (payload_len < 10) {
        if (ui_color_enabled) {
            printf("    %s%sError: Payload too short (%d bytes, need at least 10)%s\n",
                   ANSI_COLOR_RED, ANSI_BOLD, payload_len, ANSI_RESET);
        } else {
            printf("    Error: Payload too short (%d bytes, need at least 10)\n", payload_len);
        }
        return;
    }

    // Per FiRa UCI spec (GetDeviceInfoRsp):
    // Byte 0: status
    // Bytes 1-2: uci_version (16-bit)
    // Bytes 3-4: mac_version (16-bit)
    // Bytes 5-6: phy_version (16-bit)
    // Bytes 7-8: uci_test_version (16-bit)
    // Byte 9: vendor_spec_info count
    // Bytes 10+: vendor_spec_info array

    uint8_t status = payload[0];
    uint16_t uci_version = ui_read_u16_le(payload + 1);
    uint16_t mac_version = ui_read_u16_le(payload + 3);
    uint16_t phy_version = ui_read_u16_le(payload + 5);
    uint16_t uci_test_version = ui_read_u16_le(payload + 7);
    uint8_t vendor_info_count = payload[9];

    if (ui_color_enabled) {
        printf("    %s%sStatus:%s 0x%02X", ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, status);
        switch(status) {
            case UCI_STATUS_OK: printf(" %s(OK)%s\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET); break;
            case UCI_STATUS_REJECTED: printf(" %s(REJECTED)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            case UCI_STATUS_FAILED: printf(" %s(FAILED)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            default: printf(" %s(UNKNOWN)%s\n", ANSI_COLOR_YELLOW, ANSI_RESET); break;
        }
        printf("    %s%sUCI Version:%s 0x%04X\n", ANSI_COLOR_BRIGHT_CYAN, ANSI_BOLD, ANSI_RESET, uci_version);
        printf("    %s%sMAC Version:%s 0x%04X\n", ANSI_COLOR_BRIGHT_CYAN, ANSI_BOLD, ANSI_RESET, mac_version);
        printf("    %s%sPHY Version:%s 0x%04X\n", ANSI_COLOR_BRIGHT_CYAN, ANSI_BOLD, ANSI_RESET, phy_version);
        printf("    %s%sUCI Test Version:%s 0x%04X\n", ANSI_COLOR_BRIGHT_CYAN, ANSI_BOLD, ANSI_RESET, uci_test_version);
        printf("    %s%sVendor Info Count:%s %u\n", ANSI_COLOR_BRIGHT_CYAN, ANSI_BOLD, ANSI_RESET, vendor_info_count);
    } else {
        printf("    Status: 0x%02X", status);
        switch(status) {
            case UCI_STATUS_OK: printf(" (OK)\n"); break;
            case UCI_STATUS_REJECTED: printf(" (REJECTED)\n"); break;
            case UCI_STATUS_FAILED: printf(" (FAILED)\n"); break;
            default: printf(" (UNKNOWN)\n"); break;
        }
        printf("    UCI Version: 0x%04X\n", uci_version);
        printf("    MAC Version: 0x%04X\n", mac_version);
        printf("    PHY Version: 0x%04X\n", phy_version);
        printf("    UCI Test Version: 0x%04X\n", uci_test_version);
        printf("    Vendor Info Count: %u\n", vendor_info_count);
    }

    // Display vendor-specific info if present
    if (vendor_info_count > 0 && payload_len >= 10 + vendor_info_count) {
        if (ui_color_enabled) {
            printf("    %s%sVendor Spec Info:%s ", ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_RESET);
        } else {
            printf("    Vendor Spec Info: ");
        }
        for (int i = 0; i < vendor_info_count; i++) {
            printf("0x%02X ", payload[10 + i]);
        }
        printf("\n");
    }
}

void ui_decode_core_get_caps_info_rsp(const unsigned char* payload, int payload_len) {
    if (ui_color_enabled) {
        printf("  %s%sCORE_GET_CAPS_INFO Response:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
        printf("  %s%sDevice Capabilities Information:%s\n", ANSI_COLOR_BRIGHT_CYAN, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  CORE_GET_CAPS_INFO Response:\n");
        printf("  Device Capabilities Information:\n");
    }
    
    if (payload_len < 2) {
        if (ui_color_enabled) {
            printf("  %s%sError: Payload too short (%d bytes, need at least 2)%s\n", 
                   ANSI_COLOR_RED, ANSI_BOLD, payload_len, ANSI_RESET);
        } else {
            printf("  Error: Payload too short (%d bytes, need at least 2)\n", payload_len);
        }
        return;
    }
    
    unsigned char status = payload[0];
    unsigned char num_tlvs = payload[1];
    
    if (ui_color_enabled) {
        printf("  %sStatus:%s 0x%02X", ANSI_COLOR_BRIGHT_YELLOW, ANSI_RESET, status);
        switch(status) {
            case UCI_STATUS_OK: printf(" %s(OK)%s\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET); break;
            case UCI_STATUS_REJECTED: printf(" %s(REJECTED)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            case UCI_STATUS_FAILED: printf(" %s(FAILED)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            case UCI_STATUS_INVALID_PARAM: printf(" %s(INVALID_PARAM)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            default: printf(" %s(UNKNOWN)%s\n", ANSI_COLOR_YELLOW, ANSI_RESET); break;
        }
        printf("  %sNumber of Capability TLVs:%s %d\n", ANSI_COLOR_BRIGHT_YELLOW, ANSI_RESET, num_tlvs);
    } else {
        printf("  Status: 0x%02X", status);
        switch(status) {
            case UCI_STATUS_OK: printf(" (OK)\n"); break;
            case UCI_STATUS_REJECTED: printf(" (REJECTED)\n"); break;
            case UCI_STATUS_FAILED: printf(" (FAILED)\n"); break;
            case UCI_STATUS_INVALID_PARAM: printf(" (INVALID_PARAM)\n"); break;
            default: printf(" (UNKNOWN)\n"); break;
        }
        printf("  Number of Capability TLVs: %d\n", num_tlvs);
    }
    
    if (num_tlvs == 0) {
        if (ui_color_enabled) {
            printf("  %s%sNo capabilities reported%s\n", ANSI_COLOR_YELLOW, ANSI_BOLD, ANSI_RESET);
        } else {
            printf("  No capabilities reported\n");
        }
        return;
    }
    
    int offset = 2;
    for (int i = 0; i < num_tlvs; i++) {
        if (offset + 2 > payload_len) {
            if (ui_color_enabled) {
                printf("  %s%sError: Incomplete TLV at index %d%s\n", ANSI_COLOR_RED, ANSI_BOLD, i, ANSI_RESET);
            } else {
                printf("  Error: Incomplete TLV at index %d\n", i);
            }
            return;
        }
        
        CapTlvType tlv_type = (CapTlvType)payload[offset];
        unsigned char tlv_len = payload[offset + 1];
        offset += 2;
        
        // Print TLV header with color coding
        if (ui_color_enabled) {
            printf("  %sCapability TLV %d:%s\n", ANSI_COLOR_BRIGHT_CYAN, i, ANSI_RESET);
            printf("    %sType:%s 0x%02X", ANSI_COLOR_BRIGHT_YELLOW, ANSI_RESET, tlv_type);
        } else {
            printf("  Capability TLV %d:\n", i);
            printf("    Type: 0x%02X", tlv_type);
        }
        
        // Print descriptive name for TLV type
        switch(tlv_type) {
            case SUPPORTED_V1_FIRA_PHY_VERSION_RANGE_V2_MAX_MESSAGE_SIZE: 
                if (ui_color_enabled) printf(" %s(FIRA_PHY_VERSION_RANGE)%s\n", ANSI_COLOR_BRIGHT_BLUE, ANSI_RESET); 
                else printf(" (FIRA_PHY_VERSION_RANGE)\n"); 
                break;
            case SUPPORTED_V1_FIRA_MAC_VERSION_RANGE_V2_MAX_DATA_PAYLOAD_SIZE: 
                if (ui_color_enabled) printf(" %s(FIRA_MAC_VERSION_RANGE)%s\n", ANSI_COLOR_BRIGHT_BLUE, ANSI_RESET); 
                else printf(" (FIRA_MAC_VERSION_RANGE)\n"); 
                break;
            case SUPPORTED_V1_DEVICE_ROLES_V2_FIRA_PHY_VERSION_RANGE: 
                if (ui_color_enabled) printf(" %s(DEVICE_ROLES)%s\n", ANSI_COLOR_BRIGHT_BLUE, ANSI_RESET); 
                else printf(" (DEVICE_ROLES)\n"); 
                break;
            case SUPPORTED_V1_RANGING_METHOD_V2_FIRA_MAC_VERSION_RANGE: 
                if (ui_color_enabled) printf(" %s(RANGING_METHOD)%s\n", ANSI_COLOR_BRIGHT_BLUE, ANSI_RESET); 
                else printf(" (RANGING_METHOD)\n"); 
                break;
            case SUPPORTED_V1_STS_CONFIG_V2_DEVICE_TYPE: 
                if (ui_color_enabled) printf(" %s(STS_CONFIG)%s\n", ANSI_COLOR_BRIGHT_BLUE, ANSI_RESET); 
                else printf(" (STS_CONFIG)\n"); 
                break;
            case SUPPORTED_V1_MULTI_NODE_MODES_V2_DEVICE_ROLES: 
                if (ui_color_enabled) printf(" %s(MULTI_NODE_MODES)%s\n", ANSI_COLOR_BRIGHT_BLUE, ANSI_RESET); 
                else printf(" (MULTI_NODE_MODES)\n"); 
                break;
            case SUPPORTED_V1_AOA_V2_AOA_SUPPORT: 
                if (ui_color_enabled) printf(" %s(AOA_SUPPORT)%s\n", ANSI_COLOR_BRIGHT_BLUE, ANSI_RESET); 
                else printf(" (AOA_SUPPORT)\n"); 
                break;
            case SUPPORTED_V1_EXTENDED_MAC_ADDRESS_V2_EXTENDED_MAC_ADDRESS: 
                if (ui_color_enabled) printf(" %s(EXTENDED_MAC_ADDRESS)%s\n", ANSI_COLOR_BRIGHT_BLUE, ANSI_RESET); 
                else printf(" (EXTENDED_MAC_ADDRESS)\n"); 
                break;
            case CCC_SUPPORTED_CHANNELS: 
                if (ui_color_enabled) printf(" %s(CCC_CHANNELS)%s\n", ANSI_COLOR_BRIGHT_BLUE, ANSI_RESET); 
                else printf(" (CCC_CHANNELS)\n"); 
                break;
            case CCC_SUPPORTED_VERSIONS: 
                if (ui_color_enabled) printf(" %s(CCC_VERSIONS)%s\n", ANSI_COLOR_BRIGHT_BLUE, ANSI_RESET); 
                else printf(" (CCC_VERSIONS)\n"); 
                break;
            case RADAR_SUPPORT: 
                if (ui_color_enabled) printf(" %s(RADAR_SUPPORT)%s\n", ANSI_COLOR_BRIGHT_BLUE, ANSI_RESET); 
                else printf(" (RADAR_SUPPORT)\n"); 
                break;
            case SUPPORTED_POWER_STATS: 
                if (ui_color_enabled) printf(" %s(POWER_STATS)%s\n", ANSI_COLOR_BRIGHT_BLUE, ANSI_RESET); 
                else printf(" (POWER_STATS)\n"); 
                break;
            case SUPPORTED_DIAGNOSTICS: 
                if (ui_color_enabled) printf(" %s(DIAGNOSTICS)%s\n", ANSI_COLOR_BRIGHT_BLUE, ANSI_RESET); 
                else printf(" (DIAGNOSTICS)\n"); 
                break;
            case SUPPORTED_MAX_RANGING_SESSION_NUMBER: 
                if (ui_color_enabled) printf(" %s(MAX_RANGING_SESSIONS)%s\n", ANSI_COLOR_BRIGHT_BLUE, ANSI_RESET); 
                else printf(" (MAX_RANGING_SESSIONS)\n"); 
                break;
            default:
                if (ui_color_enabled) printf(" %s(UNKNOWN)%s\n", ANSI_COLOR_YELLOW, ANSI_RESET);
                else printf(" (UNKNOWN)\n");
                break;
        }
        
        if (ui_color_enabled) {
            printf("    %sLength:%s %d bytes\n", ANSI_COLOR_BRIGHT_YELLOW, ANSI_RESET, tlv_len);
        } else {
            printf("    Length: %d bytes\n", tlv_len);
        }
        
        if (offset + tlv_len > payload_len) {
            if (ui_color_enabled) {
                printf("    %s%sError: Incomplete TLV value%s\n", ANSI_COLOR_RED, ANSI_BOLD, ANSI_RESET);
            } else {
                printf("    Error: Incomplete TLV value\n");
            }
            return;
        }
        
        // Print value with hex dump and interpretation
        if (ui_color_enabled) {
            printf("    %sValue:%s ", ANSI_COLOR_BRIGHT_YELLOW, ANSI_RESET);
        } else {
            printf("    Value: ");
        }
        
        for (int j = 0; j < tlv_len; j++) {
            if (ui_color_enabled) {
                printf("%s%02X%s ", ANSI_COLOR_BRIGHT_GREEN, payload[offset + j], ANSI_RESET);
            } else {
                printf("%02X ", payload[offset + j]);
            }
        }
        printf("\n");
        
        // Interpret value based on TLV type and length
        if (tlv_len > 0) {
            if (ui_color_enabled) {
                printf("    %sInterpreted:%s ", ANSI_COLOR_BRIGHT_YELLOW, ANSI_RESET);
            } else {
                printf("    Interpreted: ");
            }
            
            if (tlv_len == 1) {
                unsigned char val = payload[offset];
                printf("%u", val);
                // Special interpretations for boolean-like values
                if (tlv_type == SUPPORTED_V1_AOA_V2_AOA_SUPPORT ||
                    tlv_type == SUPPORTED_V2_DT_TAG_BLOCK_SHIPPING ||
                    tlv_type == RADAR_SUPPORT) {
                    if (ui_color_enabled) {
                        printf(" %s(%s)%s", 
                               val ? ANSI_COLOR_BRIGHT_GREEN : ANSI_COLOR_RED,
                               val ? "SUPPORTED" : "NOT_SUPPORTED",
                               ANSI_RESET);
                    } else {
                        printf(" (%s)", val ? "SUPPORTED" : "NOT_SUPPORTED");
                    }
                }
            } else if (tlv_len == 2) {
                unsigned short val = payload[offset] | (payload[offset + 1] << 8); // Little-endian
                printf("%u", val);
            } else if (tlv_len == 4) {
                unsigned int val = payload[offset] | 
                                  (payload[offset + 1] << 8) | 
                                  (payload[offset + 2] << 16) | 
                                  (payload[offset + 3] << 24); // Little-endian
                printf("%u", val);
            } else {
                // For longer values, show as hex dump with interpretation
                printf("(complex value)");
            }
            printf("\n");
        }
        
        offset += tlv_len;
    }
}

void ui_decode_core_set_config_rsp(const unsigned char* payload, int payload_len) {
    if (ui_color_enabled) {
        printf("  %s%sCORE_SET_CONFIG Response:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  CORE_SET_CONFIG Response:\n");
    }

    if (payload_len < 2) {
        if (ui_color_enabled) {
            printf("    %s%sError: Payload too short (%d bytes, need at least 2)%s\n",
                   ANSI_COLOR_RED, ANSI_BOLD, payload_len, ANSI_RESET);
        } else {
            printf("    Error: Payload too short (%d bytes, need at least 2)\n", payload_len);
        }
        return;
    }

    unsigned char status = payload[0];
    unsigned char num_configs = payload[1];

    // Use enhanced error analysis
    if (ui_color_enabled) {
        printf("    %s%sStatus:%s 0x%02X", ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, status);
        switch(status) {
            case UCI_STATUS_OK: printf(" %s(OK)%s\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET); break;
            case UCI_STATUS_REJECTED: printf(" %s(REJECTED)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            case UCI_STATUS_FAILED: printf(" %s(FAILED)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            case UCI_STATUS_INVALID_PARAM: printf(" %s(INVALID_PARAM)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            default: printf(" %s(UNKNOWN)%s\n", ANSI_COLOR_YELLOW, ANSI_RESET); break;
        }
        // Call enhanced analysis for more detailed information
        if (status != UCI_STATUS_OK) {
            enhanced_error_analysis(status);
        }
        printf("    %s%sNumber of Config Status:%s %d\n",
               ANSI_COLOR_BRIGHT_CYAN, ANSI_BOLD, ANSI_RESET, num_configs);
    } else {
        printf("    Status: 0x%02X", status);
        switch(status) {
            case UCI_STATUS_OK: printf(" (OK)\n"); break;
            case UCI_STATUS_REJECTED: printf(" (REJECTED)\n"); break;
            case UCI_STATUS_FAILED: printf(" (FAILED)\n"); break;
            case UCI_STATUS_INVALID_PARAM: printf(" (INVALID_PARAM)\n"); break;
            default: printf(" (UNKNOWN)\n"); break;
        }
        // Call enhanced analysis for more detailed information
        if (status != UCI_STATUS_OK) {
            enhanced_error_analysis(status);
        }
        printf("    Number of Config Status: %d\n", num_configs);
    }

    if (num_configs > 0 && payload_len >= 2) {
        int offset = 2;
        for (int i = 0; i < num_configs && offset + 2 <= payload_len; i++) {
            unsigned char cfg_id = payload[offset];
            unsigned char cfg_status = payload[offset + 1];
            
            // Get human-readable parameter name
            const char* param_name = uci_config_get_device_param_name((DeviceConfigId)cfg_id);

            if (ui_color_enabled) {
                printf("    %s%sConfig[%d]:%s ID=0x%02X", 
                       ANSI_COLOR_BRIGHT_CYAN, ANSI_BOLD, i, ANSI_RESET, cfg_id);
                if (param_name) {
                    printf(" (%s)", param_name);
                }
                printf(", Status=0x%02X", cfg_status);
                switch(cfg_status) {
                    case UCI_STATUS_OK: printf(" %s(OK)%s\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET); break;
                    case UCI_STATUS_REJECTED: printf(" %s(REJECTED)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
                    case UCI_STATUS_FAILED: printf(" %s(FAILED)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
                    case UCI_STATUS_INVALID_PARAM: printf(" %s(INVALID_PARAM)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
                    default: printf(" %s(UNKNOWN)%s\n", ANSI_COLOR_YELLOW, ANSI_RESET); break;
                }
            } else {
                printf("    Config[%d]: ID=0x%02X", i, cfg_id);
                if (param_name) {
                    printf(" (%s)", param_name);
                }
                printf(", Status=0x%02X", cfg_status);
                switch(cfg_status) {
                    case UCI_STATUS_OK: printf(" (OK)\n"); break;
                    case UCI_STATUS_REJECTED: printf(" (REJECTED)\n"); break;
                    case UCI_STATUS_FAILED: printf(" (FAILED)\n"); break;
                    case UCI_STATUS_INVALID_PARAM: printf(" (INVALID_PARAM)\n"); break;
                    default: printf(" (UNKNOWN)\n"); break;
                }
            }
            offset += 2;
        }
    }
}

void ui_decode_core_get_config_rsp(const unsigned char* payload, int payload_len) {
    if (ui_color_enabled) {
        printf("  %s%sCORE_GET_CONFIG Response:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  CORE_GET_CONFIG Response:\n");
    }

    if (payload_len < 2) {
        if (ui_color_enabled) {
            printf("    %s%sError: Payload too short (%d bytes, need at least 2)%s\n",
                   ANSI_COLOR_RED, ANSI_BOLD, payload_len, ANSI_RESET);
        } else {
            printf("    Error: Payload too short (%d bytes, need at least 2)\n", payload_len);
        }
        return;
    }

    unsigned char status = payload[0];
    unsigned char num_tlvs = payload[1];

    if (ui_color_enabled) {
        printf("    %s%sStatus:%s 0x%02X", ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, status);
        switch(status) {
            case UCI_STATUS_OK: printf(" %s(OK)%s\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET); break;
            case UCI_STATUS_REJECTED: printf(" %s(REJECTED)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            case UCI_STATUS_FAILED: printf(" %s(FAILED)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            case UCI_STATUS_INVALID_PARAM: printf(" %s(INVALID_PARAM)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            default: printf(" %s(UNKNOWN)%s\n", ANSI_COLOR_YELLOW, ANSI_RESET); break;
        }
        printf("    %s%sNumber of TLVs:%s %d\n",
               ANSI_COLOR_BRIGHT_CYAN, ANSI_BOLD, ANSI_RESET, num_tlvs);
    } else {
        printf("    Status: 0x%02X", status);
        switch(status) {
            case UCI_STATUS_OK: printf(" (OK)\n"); break;
            case UCI_STATUS_REJECTED: printf(" (REJECTED)\n"); break;
            case UCI_STATUS_FAILED: printf(" (FAILED)\n"); break;
            case UCI_STATUS_INVALID_PARAM: printf(" (INVALID_PARAM)\n"); break;
            default: printf(" (UNKNOWN)\n"); break;
        }
        printf("    Number of TLVs: %d\n", num_tlvs);
    }

    if (num_tlvs > 0 && payload_len >= 2) {
        int offset = 2;
        for (int i = 0; i < num_tlvs && offset + 2 <= payload_len; i++) {
            unsigned char cfg_id = payload[offset];
            unsigned char cfg_len = payload[offset + 1];
            offset += 2;

            // Get human-readable parameter name
            const char* param_name = uci_config_get_device_param_name((DeviceConfigId)cfg_id);
            
            if (ui_color_enabled) {
                printf("    %s%sTLV %d:%s\n", ANSI_COLOR_BRIGHT_CYAN, ANSI_BOLD, i, ANSI_RESET);
                printf("      %s%sConfig ID:%s 0x%02X", ANSI_COLOR_YELLOW, ANSI_BOLD, ANSI_RESET, cfg_id);
                if (param_name) {
                    printf(" (%s)", param_name);
                }
                switch(cfg_id) {
                    case DEVICE_STATE: printf(" %s(DEVICE_STATE)%s\n", ANSI_COLOR_CYAN, ANSI_RESET); break;
                    case LOW_POWER_MODE: printf(" %s(LOW_POWER_MODE)%s\n", ANSI_COLOR_CYAN, ANSI_RESET); break;
                    default: 
                        if (!param_name) {
                            printf(" %s(UNKNOWN)%s\n", ANSI_COLOR_YELLOW, ANSI_RESET);
                        } else {
                            printf("\n");
                        }
                        break;
                }
                printf("      %s%sLength:%s %d\n", ANSI_COLOR_YELLOW, ANSI_BOLD, ANSI_RESET, cfg_len);
            } else {
                printf("    TLV %d:\n", i);
                printf("      Config ID: 0x%02X", cfg_id);
                if (param_name) {
                    printf(" (%s)", param_name);
                }
                switch(cfg_id) {
                    case DEVICE_STATE: printf(" (DEVICE_STATE)\n"); break;
                    case LOW_POWER_MODE: printf(" (LOW_POWER_MODE)\n"); break;
                    default: 
                        if (!param_name) {
                            printf(" (UNKNOWN)\n");
                        } else {
                            printf("\n");
                        }
                        break;
                }
                printf("      Length: %d\n", cfg_len);
            }

            if (offset + cfg_len <= payload_len) {
                if (ui_color_enabled) {
                    printf("      %s%sValue:%s ", ANSI_COLOR_YELLOW, ANSI_BOLD, ANSI_RESET);
                    for (int j = 0; j < cfg_len; j++) {
                        printf("%s%02X%s ", ANSI_COLOR_BRIGHT_WHITE, payload[offset + j], ANSI_RESET);
                    }
                    printf("\n");
                } else {
                    printf("      Value: ");
                    for (int j = 0; j < cfg_len; j++) {
                        printf("%02X ", payload[offset + j]);
                    }
                    printf("\n");
                }

                // Interpret common values with enhanced interpretation
                if (param_name) {
                    if (ui_color_enabled) {
                        printf("        %s%sInterpreted:%s ", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
                    } else {
                        printf("        Interpreted: ");
                    }
                    
                    // Handle specific parameter interpretations
                    if (strcasecmp(param_name, "device_state") == 0 && cfg_len == 1) {
                        unsigned char state = payload[offset];
                        if (ui_color_enabled) {
                            switch(state) {
                                case DEVICE_STATE_READY: printf("%sREADY%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, state); break;
                                case DEVICE_STATE_ACTIVE: printf("%sACTIVE%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, state); break;
                                case DEVICE_STATE_ERROR: printf("%sERROR%s (0x%02X)\n", ANSI_COLOR_RED, ANSI_RESET, state); break;
                                default: printf("%sUNKNOWN%s (0x%02X)\n", ANSI_COLOR_YELLOW, ANSI_RESET, state); break;
                            }
                        } else {
                            switch(state) {
                                case DEVICE_STATE_READY: printf("READY (0x%02X)\n", state); break;
                                case DEVICE_STATE_ACTIVE: printf("ACTIVE (0x%02X)\n", state); break;
                                case DEVICE_STATE_ERROR: printf("ERROR (0x%02X)\n", state); break;
                                default: printf("UNKNOWN (0x%02X)\n", state); break;
                            }
                        }
                    } else if (strcasecmp(param_name, "low_power_mode") == 0 && cfg_len == 1) {
                        unsigned char lpm = payload[offset];
                        if (ui_color_enabled) {
                            printf("%s%s%s (0x%02X)\n",
                                   lpm ? ANSI_COLOR_BRIGHT_GREEN : ANSI_COLOR_YELLOW,
                                   lpm ? "ON" : "OFF", ANSI_RESET, lpm);
                        } else {
                            printf("%s (0x%02X)\n", lpm ? "ON" : "OFF", lpm);
                        }
                    } else if (cfg_len <= 8) {
                        // For numeric values, try to display as integer
                        uint64_t value = 0;
                        for (int j = 0; j < cfg_len && j < 8; j++) {
                            value |= ((uint64_t)payload[offset + j]) << (j * 8);
                        }
                        
                        // Get parameter range information
                        uint64_t min_val, max_val;
                        if (uci_config_get_device_param_range((DeviceConfigId)cfg_id, &min_val, &max_val) == 0) {
                            if (ui_color_enabled) {
                                if (value >= min_val && value <= max_val) {
                                    printf("%s%" PRIu64 "%s (0x%02X", ANSI_COLOR_BRIGHT_GREEN, value, ANSI_RESET, payload[offset]);
                                    for (int j = 1; j < cfg_len; j++) {
                                        printf(" %02X", payload[offset + j]);
                                    }
                                    printf(") [Range: %" PRIu64 "-%" PRIu64 "]\n", min_val, max_val);
                                } else {
                                    printf("%s%" PRIu64 "%s (0x%02X", ANSI_COLOR_YELLOW, value, ANSI_RESET, payload[offset]);
                                    for (int j = 1; j < cfg_len; j++) {
                                        printf(" %02X", payload[offset + j]);
                                    }
                                    printf(") [OUT OF RANGE: %" PRIu64 "-%" PRIu64 "]\n", min_val, max_val);
                                }
                            } else {
                                if (value >= min_val && value <= max_val) {
                                    printf("%" PRIu64 " (0x%02X", value, payload[offset]);
                                    for (int j = 1; j < cfg_len; j++) {
                                        printf(" %02X", payload[offset + j]);
                                    }
                                    printf(") [Range: %" PRIu64 "-%" PRIu64 "]\n", min_val, max_val);
                                } else {
                                    printf("%" PRIu64 " (0x%02X", value, payload[offset]);
                                    for (int j = 1; j < cfg_len; j++) {
                                        printf(" %02X", payload[offset + j]);
                                    }
                                    printf(") [OUT OF RANGE: %" PRIu64 "-%" PRIu64 "]\n", min_val, max_val);
                                }
                            }
                        } else {
                            if (ui_color_enabled) {
                                printf("%s%" PRIu64 "%s (0x%02X", ANSI_COLOR_BRIGHT_WHITE, value, ANSI_RESET, payload[offset]);
                                for (int j = 1; j < cfg_len; j++) {
                                    printf(" %02X", payload[offset + j]);
                                }
                                printf(")\n");
                            } else {
                                printf("%" PRIu64 " (0x%02X", value, payload[offset]);
                                for (int j = 1; j < cfg_len; j++) {
                                    printf(" %02X", payload[offset + j]);
                                }
                                printf(")\n");
                            }
                        }
                    } else {
                        // For longer values, just show hex
                        if (ui_color_enabled) {
                            printf("%s", ANSI_COLOR_BRIGHT_WHITE);
                        }
                        for (int j = 0; j < cfg_len; j++) {
                            printf("%02X ", payload[offset + j]);
                        }
                        if (ui_color_enabled) {
                            printf("%s\n", ANSI_RESET);
                        } else {
                            printf("\n");
                        }
                    }
                } else if (cfg_id == DEVICE_STATE && cfg_len == 1) {
                    unsigned char state = payload[offset];
                    if (ui_color_enabled) {
                        printf("        %s%sInterpreted:%s ", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
                        switch(state) {
                            case DEVICE_STATE_READY: printf("%sREADY%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, state); break;
                            case DEVICE_STATE_ACTIVE: printf("%sACTIVE%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, state); break;
                            case DEVICE_STATE_ERROR: printf("%sERROR%s (0x%02X)\n", ANSI_COLOR_RED, ANSI_RESET, state); break;
                            default: printf("%sUNKNOWN%s (0x%02X)\n", ANSI_COLOR_YELLOW, ANSI_RESET, state); break;
                        }
                    } else {
                        printf("        Interpreted: ");
                        switch(state) {
                            case DEVICE_STATE_READY: printf("READY (0x%02X)\n", state); break;
                            case DEVICE_STATE_ACTIVE: printf("ACTIVE (0x%02X)\n", state); break;
                            case DEVICE_STATE_ERROR: printf("ERROR (0x%02X)\n", state); break;
                            default: printf("UNKNOWN (0x%02X)\n", state); break;
                        }
                    }
                } else if (cfg_id == LOW_POWER_MODE && cfg_len == 1) {
                    unsigned char lpm = payload[offset];
                    if (ui_color_enabled) {
                        printf("        %s%sInterpreted:%s %s%s%s (0x%02X)\n",
                               ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET,
                               lpm ? ANSI_COLOR_BRIGHT_GREEN : ANSI_COLOR_YELLOW,
                               lpm ? "ON" : "OFF", ANSI_RESET, lpm);
                    } else {
                        printf("        Interpreted: %s (0x%02X)\n", lpm ? "ON" : "OFF", lpm);
                    }
                }
            } else {
                if (ui_color_enabled) {
                    printf("      %s%sValue: TRUNCATED%s (expected %d bytes, only %d available)\n",
                           ANSI_COLOR_RED, ANSI_BOLD, ANSI_RESET, cfg_len, payload_len - offset);
                } else {
                    printf("      Value: TRUNCATED (expected %d bytes, only %d available)\n",
                           cfg_len, payload_len - offset);
                }
            }

            offset += cfg_len;
        }
    }
}

void ui_decode_core_device_reset_rsp(const unsigned char* payload, int payload_len) {
    if (ui_color_enabled) {
        printf("  %s%sCORE_DEVICE_RESET Response:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  CORE_DEVICE_RESET Response:\n");
    }

    if (payload_len < 1) {
        if (ui_color_enabled) {
            printf("    %s%sError: Payload too short (%d bytes, need at least 1)%s\n",
                   ANSI_COLOR_RED, ANSI_BOLD, payload_len, ANSI_RESET);
        } else {
            printf("    Error: Payload too short (%d bytes, need at least 1)\n", payload_len);
        }
        return;
    }

    unsigned char status = payload[0];

    if (ui_color_enabled) {
        printf("    %s%sStatus:%s 0x%02X", ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, status);
        switch(status) {
            case UCI_STATUS_OK: printf(" %s(OK)%s\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET); break;
            case UCI_STATUS_REJECTED: printf(" %s(REJECTED)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            case UCI_STATUS_FAILED: printf(" %s(FAILED)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            default: printf(" %s(UNKNOWN)%s\n", ANSI_COLOR_YELLOW, ANSI_RESET); break;
        }
    } else {
        printf("    Status: 0x%02X", status);
        switch(status) {
            case UCI_STATUS_OK: printf(" (OK)\n"); break;
            case UCI_STATUS_REJECTED: printf(" (REJECTED)\n"); break;
            case UCI_STATUS_FAILED: printf(" (FAILED)\n"); break;
            default: printf(" (UNKNOWN)\n"); break;
        }
    }
}

void ui_decode_core_device_suspend_rsp(const unsigned char* payload, int payload_len) {
    if (ui_color_enabled) {
        printf("  %s%sCORE_DEVICE_SUSPEND Response:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  CORE_DEVICE_SUSPEND Response:\n");
    }

    if (payload_len < 1) {
        if (ui_color_enabled) {
            printf("    %s%sError: Payload too short (%d bytes, need at least 1)%s\n",
                   ANSI_COLOR_RED, ANSI_BOLD, payload_len, ANSI_RESET);
        } else {
            printf("    Error: Payload too short (%d bytes, need at least 1)\n", payload_len);
        }
        return;
    }

    unsigned char status = payload[0];

    if (ui_color_enabled) {
        printf("    %s%sStatus:%s 0x%02X", ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, status);
        switch(status) {
            case UCI_STATUS_OK: printf(" %s(OK)%s\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET); break;
            case UCI_STATUS_REJECTED: printf(" %s(REJECTED)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            case UCI_STATUS_FAILED: printf(" %s(FAILED)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            default: printf(" %s(UNKNOWN)%s\n", ANSI_COLOR_YELLOW, ANSI_RESET); break;
        }
    } else {
        printf("    Status: 0x%02X", status);
        switch(status) {
            case UCI_STATUS_OK: printf(" (OK)\n"); break;
            case UCI_STATUS_REJECTED: printf(" (REJECTED)\n"); break;
            case UCI_STATUS_FAILED: printf(" (FAILED)\n"); break;
            default: printf(" (UNKNOWN)\n"); break;
        }
    }
}

void ui_decode_core_query_uwbs_timestamp_rsp(const unsigned char* payload, int payload_len) {
    if (ui_color_enabled) {
        printf("  %s%sCORE_QUERY_UWBS_TIMESTAMP Response:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  CORE_QUERY_UWBS_TIMESTAMP Response:\n");
    }

    if (payload_len < 9) {
        if (ui_color_enabled) {
            printf("    %s%sError: Payload too short (%d bytes, need at least 9)%s\n",
                   ANSI_COLOR_RED, ANSI_BOLD, payload_len, ANSI_RESET);
        } else {
            printf("    Error: Payload too short (%d bytes, need at least 9)\n", payload_len);
        }
        return;
    }

    unsigned char status = payload[0];
    unsigned long long timestamp = ui_read_u64_le(payload + 1);

    if (ui_color_enabled) {
        printf("    %s%sStatus:%s 0x%02X", ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, status);
        switch(status) {
            case UCI_STATUS_OK:
                printf(" %s(OK)%s\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET);
                printf("    %s%sTimestamp:%s %llu\n",
                       ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_RESET, timestamp);
                break;
            case UCI_STATUS_REJECTED: printf(" %s(REJECTED)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            case UCI_STATUS_FAILED: printf(" %s(FAILED)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            default: printf(" %s(UNKNOWN)%s\n", ANSI_COLOR_YELLOW, ANSI_RESET); break;
        }
    } else {
        printf("    Status: 0x%02X", status);
        switch(status) {
            case UCI_STATUS_OK:
                printf(" (OK)\n");
                printf("    Timestamp: %llu\n", timestamp);
                break;
            case UCI_STATUS_REJECTED: printf(" (REJECTED)\n"); break;
            case UCI_STATUS_FAILED: printf(" (FAILED)\n"); break;
            default: printf(" (UNKNOWN)\n"); break;
        }
    }
}

void ui_decode_core_get_state_rsp(const unsigned char* payload, int payload_len) {
    (void)payload;
    (void)payload_len;
    if (ui_color_enabled) {
        printf("  %s%sCORE_GET_STATE Response:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  CORE_GET_STATE Response:\n");
    }
}

void ui_decode_core_set_active_rsp(const unsigned char* payload, int payload_len) {
    (void)payload;
    (void)payload_len;
    if (ui_color_enabled) {
        printf("  %s%sCORE_SET_ACTIVE Response:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  CORE_SET_ACTIVE Response:\n");
    }
}

void ui_decode_core_set_ready_rsp(const unsigned char* payload, int payload_len) {
    (void)payload;
    (void)payload_len;
    if (ui_color_enabled) {
        printf("  %s%sCORE_SET_READY Response:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  CORE_SET_READY Response:\n");
    }
}

void ui_decode_core_device_ready_rsp(const unsigned char* payload, int payload_len) {
    (void)payload;
    (void)payload_len;
    if (ui_color_enabled) {
        printf("  %s%sCORE_DEVICE_READY Response:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  CORE_DEVICE_READY Response:\n");
    }
}

void ui_decode_core_get_caps_rsp(const unsigned char* payload, int payload_len) {
    (void)payload;
    (void)payload_len;
    if (ui_color_enabled) {
        printf("  %s%sCORE_GET_CAPS Response:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  CORE_GET_CAPS Response:\n");
    }
}

void ui_decode_core_set_power_rsp(const unsigned char* payload, int payload_len) {
    (void)payload;
    (void)payload_len;
    if (ui_color_enabled) {
        printf("  %s%sCORE_SET_POWER Response:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  CORE_SET_POWER Response:\n");
    }
}

void ui_decode_core_get_power_rsp(const unsigned char* payload, int payload_len) {
    (void)payload;
    (void)payload_len;
    if (ui_color_enabled) {
        printf("  %s%sCORE_GET_POWER Response:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  CORE_GET_POWER Response:\n");
    }
}

void ui_decode_core_device_on_rsp(const unsigned char* payload, int payload_len) {
    (void)payload;
    (void)payload_len;
    if (ui_color_enabled) {
        printf("  %s%sCORE_DEVICE_ON Response:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  CORE_DEVICE_ON Response:\n");
    }
}

void ui_decode_core_device_off_rsp(const unsigned char* payload, int payload_len) {
    (void)payload;
    (void)payload_len;
    if (ui_color_enabled) {
        printf("  %s%sCORE_DEVICE_OFF Response:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  CORE_DEVICE_OFF Response:\n");
    }
}

void ui_decode_core_device_suspend_cmd_rsp(const unsigned char* payload, int payload_len) {
    (void)payload;
    (void)payload_len;
    if (ui_color_enabled) {
        printf("  %s%sCORE_DEVICE_SUSPEND_CMD Response:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  CORE_DEVICE_SUSPEND_CMD Response:\n");
    }
}

void ui_decode_core_device_status_ntf(const unsigned char* payload, int payload_len) {
    if (ui_color_enabled) {
        printf("  %s%sCORE_DEVICE_STATUS_NTF:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  CORE_DEVICE_STATUS_NTF:\n");
    }

    if (payload_len < 1) {
        if (ui_color_enabled) {
            printf("    %s%sError: Payload too short (%d bytes, need at least 1)%s\n",
                   ANSI_COLOR_RED, ANSI_BOLD, payload_len, ANSI_RESET);
        } else {
            printf("    Error: Payload too short (%d bytes, need at least 1)\n", payload_len);
        }
        return;
    }

    unsigned char device_state = payload[0];
    const uci_lookup_entry_t* entry = uci_lookup_device_state(device_state);

    const char* state_color = ANSI_COLOR_YELLOW;
    if (device_state == DEVICE_STATE_READY || device_state == DEVICE_STATE_ACTIVE) {
        state_color = ANSI_COLOR_BRIGHT_GREEN;
    } else if (device_state == DEVICE_STATE_ERROR) {
        state_color = ANSI_COLOR_RED;
    }

    print_lookup_line("Device State", device_state, entry, state_color);

    print_additional_payload_bytes(payload, payload_len, 1);
}

void ui_decode_core_generic_error_ntf(const unsigned char* payload, int payload_len) {
    if (ui_color_enabled) {
        printf("  %s%sCORE_GENERIC_ERROR_NTF:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  CORE_GENERIC_ERROR_NTF:\n");
    }

    if (payload_len < 1) {
        if (ui_color_enabled) {
            printf("    %s%sError: Payload too short (%d bytes, need at least 1)%s\n",
                   ANSI_COLOR_RED, ANSI_BOLD, payload_len, ANSI_RESET);
        } else {
            printf("    Error: Payload too short (%d bytes, need at least 1)\n", payload_len);
        }
        return;
    }

    unsigned char status = payload[0];
    const uci_lookup_entry_t* entry = uci_lookup_status(status);

    const char* status_color = (status == UCI_STATUS_OK) ? ANSI_COLOR_BRIGHT_GREEN : ANSI_COLOR_RED;

    print_lookup_line("Status Code", status, entry, status_color);

    enhanced_error_analysis(status);

    print_additional_payload_bytes(payload, payload_len, 1);
}

void ui_decode_session_init_cmd(const unsigned char* payload, int payload_len) {
    if (ui_color_enabled) {
        printf("  %s%sSESSION_INIT_CMD - Session Initialization Command:%s\n",
               ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  SESSION_INIT_CMD - Session Initialization Command:\n");
    }

    if (payload_len < 5) {
        if (ui_color_enabled) {
            printf("    %s%sERROR:%s Payload too short (%d bytes, need at least 5)\n",
                   ANSI_COLOR_RED, ANSI_BOLD, ANSI_RESET, payload_len);
        } else {
            printf("    ERROR: Payload too short (%d bytes, need at least 5)\n", payload_len);
        }
        return;
    }

    unsigned int session_id = (unsigned int)payload[0] |
                             ((unsigned int)payload[1] << 8) |
                             ((unsigned int)payload[2] << 16) |
                             ((unsigned int)payload[3] << 24);
    unsigned char session_type = payload[4];

    if (ui_color_enabled) {
        printf("    %s%sSession ID:%s 0x%08X\n",
               ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_RESET, session_id);
    } else {
        printf("    Session ID: 0x%08X\n", session_id);
    }

    const char* type_name = uci_session_type_to_string(session_type);

    if (ui_color_enabled) {
        printf("    %s%sSession Type:%s 0x%02X (%s%s%s)\n",
               ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_RESET, session_type,
               ANSI_COLOR_CYAN, type_name, ANSI_RESET);
    } else {
        printf("    Session Type: 0x%02X (%s)\n", session_type, type_name);
    }
}

void ui_decode_session_init_rsp(const unsigned char* payload, int payload_len) {
    if (ui_color_enabled) {
        printf("  %s%sSESSION_INIT Response:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  SESSION_INIT Response:\n");
    }

    if (payload_len < 5) {
        if (ui_color_enabled) {
            printf("    %s%sError: Payload too short (%d bytes, need at least 5)%s\n",
                   ANSI_COLOR_RED, ANSI_BOLD, payload_len, ANSI_RESET);
        } else {
            printf("    Error: Payload too short (%d bytes, need at least 5)\n", payload_len);
        }
        return;
    }

    unsigned char status = payload[0];
    unsigned int session_handle = ui_read_u32_le(payload + 1);

    if (ui_color_enabled) {
        printf("    %s%sStatus:%s 0x%02X", ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, status);
        switch(status) {
            case UCI_STATUS_OK: printf(" %s(OK)%s\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET); break;
            case UCI_STATUS_REJECTED: printf(" %s(REJECTED)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            case UCI_STATUS_FAILED: printf(" %s(FAILED)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            case UCI_STATUS_INVALID_PARAM: printf(" %s(INVALID_PARAM)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            case UCI_STATUS_SESSION_DUPLICATE: printf(" %s(SESSION_DUPLICATE)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            default: printf(" %s(UNKNOWN)%s\n", ANSI_COLOR_YELLOW, ANSI_RESET); break;
        }
        printf("    %s%sSession Handle:%s 0x%08X\n",
               ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_RESET, session_handle);
    } else {
        printf("    Status: 0x%02X", status);
        switch(status) {
            case UCI_STATUS_OK: printf(" (OK)\n"); break;
            case UCI_STATUS_REJECTED: printf(" (REJECTED)\n"); break;
            case UCI_STATUS_FAILED: printf(" (FAILED)\n"); break;
            case UCI_STATUS_INVALID_PARAM: printf(" (INVALID_PARAM)\n"); break;
            case UCI_STATUS_SESSION_DUPLICATE: printf(" (SESSION_DUPLICATE)\n"); break;
            default: printf(" (UNKNOWN)\n"); break;
        }
        printf("    Session Handle: 0x%08X\n", session_handle);
    }
}

void ui_decode_session_deinit_rsp(const unsigned char* payload, int payload_len) {
    if (ui_color_enabled) {
        printf("  %s%sSESSION_DEINIT Response:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  SESSION_DEINIT Response:\n");
    }

    if (payload_len < 1) {
        if (ui_color_enabled) {
            printf("    %s%sError: Payload too short (%d bytes, need at least 1)%s\n",
                   ANSI_COLOR_RED, ANSI_BOLD, payload_len, ANSI_RESET);
        } else {
            printf("    Error: Payload too short (%d bytes, need at least 1)\n", payload_len);
        }
        return;
    }

    unsigned char status = payload[0];

    if (ui_color_enabled) {
        printf("    %s%sStatus:%s 0x%02X", ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, status);
        switch(status) {
            case UCI_STATUS_OK: printf(" %s(OK)%s\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET); break;
            case UCI_STATUS_REJECTED: printf(" %s(REJECTED)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            case UCI_STATUS_FAILED: printf(" %s(FAILED)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            case UCI_STATUS_INVALID_PARAM: printf(" %s(INVALID_PARAM)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            case UCI_STATUS_SESSION_NOT_EXIST: printf(" %s(SESSION_NOT_EXIST)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            default: printf(" %s(UNKNOWN)%s\n", ANSI_COLOR_YELLOW, ANSI_RESET); break;
        }
    } else {
        printf("    Status: 0x%02X", status);
        switch(status) {
            case UCI_STATUS_OK: printf(" (OK)\n"); break;
            case UCI_STATUS_REJECTED: printf(" (REJECTED)\n"); break;
            case UCI_STATUS_FAILED: printf(" (FAILED)\n"); break;
            case UCI_STATUS_INVALID_PARAM: printf(" (INVALID_PARAM)\n"); break;
            case UCI_STATUS_SESSION_NOT_EXIST: printf(" (SESSION_NOT_EXIST)\n"); break;
            default: printf(" (UNKNOWN)\n"); break;
        }
    }
}

void ui_decode_session_set_app_config_rsp(const unsigned char* payload, int payload_len) {
    if (ui_color_enabled) {
        printf("  %s%sSESSION_SET_APP_CONFIG Response:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  SESSION_SET_APP_CONFIG Response:\n");
    }

    if (payload_len < 2) {
        if (ui_color_enabled) {
            printf("    %s%sError: Payload too short (%d bytes, need at least 2)%s\n",
                   ANSI_COLOR_RED, ANSI_BOLD, payload_len, ANSI_RESET);
        } else {
            printf("    Error: Payload too short (%d bytes, need at least 2)\n", payload_len);
        }
        return;
    }

    unsigned char status = payload[0];
    unsigned char num_configs = payload[1];

    if (ui_color_enabled) {
        printf("    %s%sStatus:%s 0x%02X", ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, status);
        switch(status) {
            case UCI_STATUS_OK: printf(" %s(OK)%s\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET); break;
            case UCI_STATUS_REJECTED: printf(" %s(REJECTED)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            case UCI_STATUS_FAILED: printf(" %s(FAILED)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            case UCI_STATUS_INVALID_PARAM: printf(" %s(INVALID_PARAM)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            case UCI_STATUS_INVALID_RANGE: printf(" %s(INVALID_RANGE)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            case UCI_STATUS_INVALID_MSG_SIZE: printf(" %s(INVALID_MSG_SIZE)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            default: printf(" %s(UNKNOWN)%s\n", ANSI_COLOR_YELLOW, ANSI_RESET); break;
        }
        // Call enhanced analysis for more detailed information
        if (status != UCI_STATUS_OK) {
            enhanced_error_analysis(status);
        }
        printf("    %s%sNumber of Config Status:%s %d\n",
               ANSI_COLOR_BRIGHT_CYAN, ANSI_BOLD, ANSI_RESET, num_configs);
    } else {
        printf("    Status: 0x%02X", status);
        switch(status) {
            case UCI_STATUS_OK: printf(" (OK)\n"); break;
            case UCI_STATUS_REJECTED: printf(" (REJECTED)\n"); break;
            case UCI_STATUS_FAILED: printf(" (FAILED)\n"); break;
            case UCI_STATUS_INVALID_PARAM: printf(" (INVALID_PARAM)\n"); break;
            case UCI_STATUS_INVALID_RANGE: printf(" (INVALID_RANGE)\n"); break;
            case UCI_STATUS_INVALID_MSG_SIZE: printf(" (INVALID_MSG_SIZE)\n"); break;
            default: printf(" (UNKNOWN)\n"); break;
        }
        // Call enhanced analysis for more detailed information
        if (status != UCI_STATUS_OK) {
            enhanced_error_analysis(status);
        }
        printf("    Number of Config Status: %d\n", num_configs);
    }

    // Parse config status entries (each is 2 bytes: config_id + status)
    int offset = 2;
    for (int i = 0; i < num_configs && offset + 2 <= payload_len; i++) {
        unsigned char cfg_id = payload[offset];
        unsigned char cfg_status = payload[offset + 1];
        
        // Get human-readable parameter name
        const char* param_name = uci_config_get_app_param_name((AppConfigTlvType)cfg_id);

        if (ui_color_enabled) {
            printf("    %s%sConfig[%d]:%s ID=0x%02X", 
                   ANSI_COLOR_BRIGHT_CYAN, ANSI_BOLD, i, ANSI_RESET, cfg_id);
            if (param_name) {
                printf(" (%s)", param_name);
            }
            printf(", Status=0x%02X", cfg_status);
            switch(cfg_status) {
                case UCI_STATUS_OK: printf(" %s(OK)%s\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET); break;
                case UCI_STATUS_REJECTED: printf(" %s(REJECTED)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
                case UCI_STATUS_FAILED: printf(" %s(FAILED)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
                default: printf(" %s(UNKNOWN)%s\n", ANSI_COLOR_YELLOW, ANSI_RESET); break;
            }
        } else {
            printf("    Config[%d]: ID=0x%02X", i, cfg_id);
            if (param_name) {
                printf(" (%s)", param_name);
            }
            printf(", Status=0x%02X", cfg_status);
            switch(cfg_status) {
                case UCI_STATUS_OK: printf(" (OK)\n"); break;
                case UCI_STATUS_REJECTED: printf(" (REJECTED)\n"); break;
                case UCI_STATUS_FAILED: printf(" (FAILED)\n"); break;
                default: printf(" (UNKNOWN)\n"); break;
            }
        }
        offset += 2;
    }
}

void ui_decode_session_get_app_config_rsp(const unsigned char* payload, int payload_len) {
    if (ui_color_enabled) {
        printf("  %s%sSESSION_GET_APP_CONFIG Response:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  SESSION_GET_APP_CONFIG Response:\n");
    }

    if (payload_len < 2) {
        if (ui_color_enabled) {
            printf("    %s%sError: Payload too short (%d bytes, need at least 2)%s\n",
                   ANSI_COLOR_RED, ANSI_BOLD, payload_len, ANSI_RESET);
        } else {
            printf("    Error: Payload too short (%d bytes, need at least 2)\n", payload_len);
        }
        return;
    }

    unsigned char status = payload[0];
    unsigned char num_tlvs = payload[1];

    if (ui_color_enabled) {
        printf("    %s%sStatus:%s 0x%02X", ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, status);
        switch(status) {
            case UCI_STATUS_OK: printf(" %s(OK)%s\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET); break;
            case UCI_STATUS_REJECTED: printf(" %s(REJECTED)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            case UCI_STATUS_FAILED: printf(" %s(FAILED)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            case UCI_STATUS_INVALID_PARAM: printf(" %s(INVALID_PARAM)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            default: printf(" %s(UNKNOWN)%s\n", ANSI_COLOR_YELLOW, ANSI_RESET); break;
        }
        // Call enhanced analysis for more detailed information
        if (status != UCI_STATUS_OK) {
            enhanced_error_analysis(status);
        }
        printf("    %s%sNumber of TLVs:%s %d\n",
               ANSI_COLOR_BRIGHT_CYAN, ANSI_BOLD, ANSI_RESET, num_tlvs);
    } else {
        printf("    Status: 0x%02X", status);
        switch(status) {
            case UCI_STATUS_OK: printf(" (OK)\n"); break;
            case UCI_STATUS_REJECTED: printf(" (REJECTED)\n"); break;
            case UCI_STATUS_FAILED: printf(" (FAILED)\n"); break;
            case UCI_STATUS_INVALID_PARAM: printf(" (INVALID_PARAM)\n"); break;
            default: printf(" (UNKNOWN)\n"); break;
        }
        // Call enhanced analysis for more detailed information
        if (status != UCI_STATUS_OK) {
            enhanced_error_analysis(status);
        }
        printf("    Number of TLVs: %d\n", num_tlvs);
    }

    // Parse TLV entries (each TLV: config_id (1 byte) + length (1 byte) + value bytes)
    int offset = 2;
    for (int i = 0; i < num_tlvs && offset + 2 <= payload_len; i++) {
        unsigned char cfg_id = payload[offset];
        unsigned char cfg_len = payload[offset + 1];

        if (offset + 2 + cfg_len > payload_len) {
            if (ui_color_enabled) {
                printf("    %s%sWarning: TLV[%d] extends beyond payload%s\n",
                       ANSI_COLOR_YELLOW, ANSI_BOLD, i, ANSI_RESET);
            } else {
                printf("    Warning: TLV[%d] extends beyond payload\n", i);
            }
            break;
        }

        // Get human-readable parameter name
        const char* param_name = uci_config_get_app_param_name((AppConfigTlvType)cfg_id);
        
        if (ui_color_enabled) {
            printf("    %s%sTLV[%d]:%s Config ID=0x%02X", 
                   ANSI_COLOR_BRIGHT_CYAN, ANSI_BOLD, i, ANSI_RESET, cfg_id);
            if (param_name) {
                printf(" (%s)", param_name);
            }
            printf(", Length=%d bytes\n", cfg_len);
            printf("      %s%sValue:%s ", ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_RESET);
        } else {
            printf("    TLV[%d]: Config ID=0x%02X", i, cfg_id);
            if (param_name) {
                printf(" (%s)", param_name);
            }
            printf(", Length=%d bytes\n", cfg_len);
            printf("      Value: ");
        }

        // Print hex value
        for (int j = 0; j < cfg_len; j++) {
            printf("0x%02X ", payload[offset + 2 + j]);
        }
        printf("\n");

        // Try to interpret common parameter values
        if (param_name) {
            if (ui_color_enabled) {
                printf("      %s%sInterpreted:%s ", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
            } else {
                printf("      Interpreted: ");
            }
            
            // Handle specific parameter interpretations
            if (strcasecmp(param_name, "device_type") == 0 && cfg_len == 1) {
                unsigned char value = payload[offset + 2];
                if (ui_color_enabled) {
                    switch(value) {
                        case 0x00: printf("%sCONTROLEE%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, value); break;
                        case 0x01: printf("%sCONTROLLER%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, value); break;
                        default: printf("%sUNKNOWN%s (0x%02X)\n", ANSI_COLOR_YELLOW, ANSI_RESET, value); break;
                    }
                } else {
                    switch(value) {
                        case 0x00: printf("CONTROLEE (0x%02X)\n", value); break;
                        case 0x01: printf("CONTROLLER (0x%02X)\n", value); break;
                        default: printf("UNKNOWN (0x%02X)\n", value); break;
                    }
                }
            } else if (strcasecmp(param_name, "device_role") == 0 && cfg_len == 1) {
                unsigned char value = payload[offset + 2];
                if (ui_color_enabled) {
                    switch(value) {
                        case 0x00: printf("%sRESPONDER%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, value); break;
                        case 0x01: printf("%sINITIATOR%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, value); break;
                        case 0x05: printf("%sADVERTISER%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, value); break;
                        case 0x06: printf("%sOBSERVER%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, value); break;
                        case 0x07: printf("%sDT_ANCHOR%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, value); break;
                        case 0x08: printf("%sDT_TAG%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, value); break;
                        default: printf("%sUNKNOWN%s (0x%02X)\n", ANSI_COLOR_YELLOW, ANSI_RESET, value); break;
                    }
                } else {
                    switch(value) {
                        case 0x00: printf("RESPONDER (0x%02X)\n", value); break;
                        case 0x01: printf("INITIATOR (0x%02X)\n", value); break;
                        case 0x05: printf("ADVERTISER (0x%02X)\n", value); break;
                        case 0x06: printf("OBSERVER (0x%02X)\n", value); break;
                        case 0x07: printf("DT_ANCHOR (0x%02X)\n", value); break;
                        case 0x08: printf("DT_TAG (0x%02X)\n", value); break;
                        default: printf("UNKNOWN (0x%02X)\n", value); break;
                    }
                }
            } else if (strcasecmp(param_name, "channel_number") == 0 && cfg_len == 1) {
                unsigned char value = payload[offset + 2];
                if (ui_color_enabled) {
                    switch(value) {
                        case 5: printf("%sChannel 5%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, value); break;
                        case 9: printf("%sChannel 9%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, value); break;
                        default: printf("%sUNKNOWN%s (0x%02X)\n", ANSI_COLOR_YELLOW, ANSI_RESET, value); break;
                    }
                } else {
                    switch(value) {
                        case 5: printf("Channel 5 (0x%02X)\n", value); break;
                        case 9: printf("Channel 9 (0x%02X)\n", value); break;
                        default: printf("UNKNOWN (0x%02X)\n", value); break;
                    }
                }
            } else if (strcasecmp(param_name, "no_of_controlee") == 0 && cfg_len == 1) {
                unsigned char value = payload[offset + 2];
                if (ui_color_enabled) {
                    printf("%s%u controlees%s (0x%02X)\n",
                           ANSI_COLOR_BRIGHT_GREEN, value, ANSI_RESET, value);
                } else {
                    printf("%u controlees (0x%02X)\n", value, value);
                }
            } else if (strcasecmp(param_name, "device_mac_address") == 0 && cfg_len == 2) {
                unsigned int value = (unsigned int)payload[offset + 2] |
                                     ((unsigned int)payload[offset + 3] << 8);
                if (ui_color_enabled) {
                    printf("%sSHORT%s 0x%04X\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, value);
                } else {
                    printf("SHORT 0x%04X\n", value);
                }
            } else if (strcasecmp(param_name, "dst_mac_address") == 0 && cfg_len >= 2 && (cfg_len % 2) == 0) {
                int address_count = cfg_len / 2;
                if (ui_color_enabled) {
                    printf("%s%d addresses%s: ", ANSI_COLOR_BRIGHT_GREEN, address_count, ANSI_RESET);
                } else {
                    printf("%d addresses: ", address_count);
                }
                for (int addr_idx = 0; addr_idx < address_count; addr_idx++) {
                    unsigned int value = (unsigned int)payload[offset + 2 + (addr_idx * 2)] |
                                         ((unsigned int)payload[offset + 3 + (addr_idx * 2)] << 8);
                    if (addr_idx > 0) {
                        printf(", ");
                    }
                    printf("0x%04X", value);
                }
                printf("\n");
            } else if (strcasecmp(param_name, "mac_address_mode") == 0 && cfg_len == 1) {
                unsigned char value = payload[offset + 2];
                if (ui_color_enabled) {
                    switch(value) {
                        case 0: printf("%sSHORT%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, value); break;
                        case 1: printf("%sEXTENDED%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, value); break;
                        default: printf("%sUNKNOWN%s (0x%02X)\n", ANSI_COLOR_YELLOW, ANSI_RESET, value); break;
                    }
                } else {
                    switch(value) {
                        case 0: printf("SHORT (0x%02X)\n", value); break;
                        case 1: printf("EXTENDED (0x%02X)\n", value); break;
                        default: printf("UNKNOWN (0x%02X)\n", value); break;
                    }
                }
            } else if (strcasecmp(param_name, "slot_duration") == 0 && cfg_len == 2) {
                unsigned int value = (unsigned int)payload[offset + 2] |
                                     ((unsigned int)payload[offset + 3] << 8);
                if (ui_color_enabled) {
                    printf("%s%u RSTU%s (0x%04X)\n", ANSI_COLOR_BRIGHT_GREEN, value, ANSI_RESET, value);
                } else {
                    printf("%u RSTU (0x%04X)\n", value, value);
                }
            } else if (strcasecmp(param_name, "ranging_duration") == 0 && cfg_len == 4) {
                unsigned int value = ui_read_u32_le(&payload[offset + 2]);
                if (ui_color_enabled) {
                    printf("%s%u ms%s (0x%08X)\n", ANSI_COLOR_BRIGHT_GREEN, value, ANSI_RESET, value);
                } else {
                    printf("%u ms (0x%08X)\n", value, value);
                }
            } else if (strcasecmp(param_name, "multi_node_mode") == 0 && cfg_len == 1) {
                unsigned char value = payload[offset + 2];
                if (ui_color_enabled) {
                    switch(value) {
                        case 0x00: printf("%sUNICAST%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, value); break;
                        case 0x01: printf("%sONE_TO_MANY%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, value); break;
                        case 0x02: printf("%sMANY_TO_MANY%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, value); break;
                        default: printf("%sUNKNOWN%s (0x%02X)\n", ANSI_COLOR_YELLOW, ANSI_RESET, value); break;
                    }
                } else {
                    switch(value) {
                        case 0x00: printf("UNICAST (0x%02X)\n", value); break;
                        case 0x01: printf("ONE_TO_MANY (0x%02X)\n", value); break;
                        case 0x02: printf("MANY_TO_MANY (0x%02X)\n", value); break;
                        default: printf("UNKNOWN (0x%02X)\n", value); break;
                    }
                }
            } else if (strcasecmp(param_name, "sts_config") == 0 && cfg_len == 1) {
                unsigned char value = payload[offset + 2];
                if (ui_color_enabled) {
                    switch(value) {
                        case 0: printf("%sSTATIC_STS%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, value); break;
                        case 1: printf("%sDYNAMIC_STS%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, value); break;
                        case 2: printf("%sDYNAMIC_STS_RESPONDER_SUBSESSION%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, value); break;
                        case 3: printf("%sPROVISIONED_STS%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, value); break;
                        case 4: printf("%sPROVISIONED_STS_RESPONDER_SUBSESSION%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, value); break;
                        default: printf("%sUNKNOWN%s (0x%02X)\n", ANSI_COLOR_YELLOW, ANSI_RESET, value); break;
                    }
                } else {
                    switch(value) {
                        case 0: printf("STATIC_STS (0x%02X)\n", value); break;
                        case 1: printf("DYNAMIC_STS (0x%02X)\n", value); break;
                        case 2: printf("DYNAMIC_STS_RESPONDER_SUBSESSION (0x%02X)\n", value); break;
                        case 3: printf("PROVISIONED_STS (0x%02X)\n", value); break;
                        case 4: printf("PROVISIONED_STS_RESPONDER_SUBSESSION (0x%02X)\n", value); break;
                        default: printf("UNKNOWN (0x%02X)\n", value); break;
                    }
                }
            } else if (strcasecmp(param_name, "aoa_result_req") == 0 && cfg_len == 1) {
                unsigned char value = payload[offset + 2];
                if (ui_color_enabled) {
                    switch(value) {
                        case 0: printf("%sNO_AOA_REPORT%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, value); break;
                        case 1: printf("%sAOA_ELEVATION%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, value); break;
                        case 2: printf("%sAOA_AZIMUTH%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, value); break;
                        case 3: printf("%sAOA_ELEVATION_AND_AZIMUTH%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, value); break;
                        default: printf("%sUNKNOWN%s (0x%02X)\n", ANSI_COLOR_YELLOW, ANSI_RESET, value); break;
                    }
                } else {
                    switch(value) {
                        case 0: printf("NO_AOA_REPORT (0x%02X)\n", value); break;
                        case 1: printf("AOA_ELEVATION (0x%02X)\n", value); break;
                        case 2: printf("AOA_AZIMUTH (0x%02X)\n", value); break;
                        case 3: printf("AOA_ELEVATION_AND_AZIMUTH (0x%02X)\n", value); break;
                        default: printf("UNKNOWN (0x%02X)\n", value); break;
                    }
                }
            } else if (strcasecmp(param_name, "ranging_round_usage") == 0 && cfg_len == 1) {
                unsigned char value = payload[offset + 2];
                if (ui_color_enabled) {
                    switch(value) {
                        case 0x01: printf("%sSS_TWR_DEFERRED%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, value); break;
                        case 0x02: printf("%sDS_TWR_DEFERRED%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, value); break;
                        case 0x03: printf("%sSS_TWR_NON_DEFERRED%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, value); break;
                        case 0x04: printf("%sDS_TWR_NON_DEFERRED%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, value); break;
                        case 0x05: printf("%sOWR_DL_TDOA%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, value); break;
                        case 0x06: printf("%sOWR_AOA%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, value); break;
                        case 0x07: printf("%sESS_TWR_CONTENTION_BASED%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, value); break;
                        case 0x08: printf("%sADS_TWR_CONTENTION_BASED%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, value); break;
                        default: printf("%sUNKNOWN%s (0x%02X)\n", ANSI_COLOR_YELLOW, ANSI_RESET, value); break;
                    }
                } else {
                    switch(value) {
                        case 0x01: printf("SS_TWR_DEFERRED (0x%02X)\n", value); break;
                        case 0x02: printf("DS_TWR_DEFERRED (0x%02X)\n", value); break;
                        case 0x03: printf("SS_TWR_NON_DEFERRED (0x%02X)\n", value); break;
                        case 0x04: printf("DS_TWR_NON_DEFERRED (0x%02X)\n", value); break;
                        case 0x05: printf("OWR_DL_TDOA (0x%02X)\n", value); break;
                        case 0x06: printf("OWR_AOA (0x%02X)\n", value); break;
                        case 0x07: printf("ESS_TWR_CONTENTION_BASED (0x%02X)\n", value); break;
                        case 0x08: printf("ADS_TWR_CONTENTION_BASED (0x%02X)\n", value); break;
                        default: printf("UNKNOWN (0x%02X)\n", value); break;
                    }
                }
            } else if (strcasecmp(param_name, "mac_fcs_type") == 0 && cfg_len == 1) {
                unsigned char value = payload[offset + 2];
                if (ui_color_enabled) {
                    switch(value) {
                        case 0: printf("%sCRC16%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, value); break;
                        case 1: printf("%sCRC32%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, value); break;
                        default: printf("%sUNKNOWN%s (0x%02X)\n", ANSI_COLOR_YELLOW, ANSI_RESET, value); break;
                    }
                } else {
                    switch(value) {
                        case 0: printf("CRC16 (0x%02X)\n", value); break;
                        case 1: printf("CRC32 (0x%02X)\n", value); break;
                        default: printf("UNKNOWN (0x%02X)\n", value); break;
                    }
                }
            } else if (strcasecmp(param_name, "rframe_config") == 0 && cfg_len == 1) {
                unsigned char value = payload[offset + 2];
                if (ui_color_enabled) {
                    switch(value) {
                        case 0: printf("%sSP0%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, value); break;
                        case 1: printf("%sSP1%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, value); break;
                        case 2: printf("%sSP2%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, value); break;
                        case 3: printf("%sSP3%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, value); break;
                        default: printf("%sUNKNOWN%s (0x%02X)\n", ANSI_COLOR_YELLOW, ANSI_RESET, value); break;
                    }
                } else {
                    switch(value) {
                        case 0: printf("SP0 (0x%02X)\n", value); break;
                        case 1: printf("SP1 (0x%02X)\n", value); break;
                        case 2: printf("SP2 (0x%02X)\n", value); break;
                        case 3: printf("SP3 (0x%02X)\n", value); break;
                        default: printf("UNKNOWN (0x%02X)\n", value); break;
                    }
                }
            } else if (strcasecmp(param_name, "rssi_reporting") == 0 && cfg_len == 1) {
                unsigned char value = payload[offset + 2];
                if (ui_color_enabled) {
                    switch(value) {
                        case 0: printf("%sDISABLED%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, value); break;
                        case 1: printf("%sENABLED%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, value); break;
                        default: printf("%sUNKNOWN%s (0x%02X)\n", ANSI_COLOR_YELLOW, ANSI_RESET, value); break;
                    }
                } else {
                    switch(value) {
                        case 0: printf("DISABLED (0x%02X)\n", value); break;
                        case 1: printf("ENABLED (0x%02X)\n", value); break;
                        default: printf("UNKNOWN (0x%02X)\n", value); break;
                    }
                }
            } else if (strcasecmp(param_name, "link_layer_mode") == 0 && cfg_len == 1) {
                unsigned char value = payload[offset + 2];
                if (ui_color_enabled) {
                    switch(value) {
                        case 0: printf("%sBASIC%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, value); break;
                        case 1: printf("%sEXTENDED%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, value); break;
                        default: printf("%sUNKNOWN%s (0x%02X)\n", ANSI_COLOR_YELLOW, ANSI_RESET, value); break;
                    }
                } else {
                    switch(value) {
                        case 0: printf("BASIC (0x%02X)\n", value); break;
                        case 1: printf("EXTENDED (0x%02X)\n", value); break;
                        default: printf("UNKNOWN (0x%02X)\n", value); break;
                    }
                }
            } else if (strcasecmp(param_name, "tx_adaptive_payload_power") == 0 && cfg_len == 1) {
                unsigned char value = payload[offset + 2];
                if (ui_color_enabled) {
                    switch(value) {
                        case 0: printf("%sDISABLED%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, value); break;
                        case 1: printf("%sENABLED%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, value); break;
                        default: printf("%sUNKNOWN%s (0x%02X)\n", ANSI_COLOR_YELLOW, ANSI_RESET, value); break;
                    }
                } else {
                    switch(value) {
                        case 0: printf("DISABLED (0x%02X)\n", value); break;
                        case 1: printf("ENABLED (0x%02X)\n", value); break;
                        default: printf("UNKNOWN (0x%02X)\n", value); break;
                    }
                }
            } else if (strcasecmp(param_name, "scheduled_mode") == 0 && cfg_len == 1) {
                unsigned char value = payload[offset + 2];
                if (ui_color_enabled) {
                    switch(value) {
                        case 0: printf("%sCONT%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, value); break;
                        case 1: printf("%sSCHEDULED%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, value); break;
                        default: printf("%sUNKNOWN%s (0x%02X)\n", ANSI_COLOR_YELLOW, ANSI_RESET, value); break;
                    }
                } else {
                    switch(value) {
                        case 0: printf("CONT (0x%02X)\n", value); break;
                        case 1: printf("SCHEDULED (0x%02X)\n", value); break;
                        default: printf("UNKNOWN (0x%02X)\n", value); break;
                    }
                }
            } else if (strcasecmp(param_name, "key_rotation") == 0 && cfg_len == 1) {
                unsigned char value = payload[offset + 2];
                if (ui_color_enabled) {
                    switch(value) {
                        case 0: printf("%sDISABLED%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, value); break;
                        case 1: printf("%sENABLED%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, value); break;
                        default: printf("%sUNKNOWN%s (0x%02X)\n", ANSI_COLOR_YELLOW, ANSI_RESET, value); break;
                    }
                } else {
                    switch(value) {
                        case 0: printf("DISABLED (0x%02X)\n", value); break;
                        case 1: printf("ENABLED (0x%02X)\n", value); break;
                        default: printf("UNKNOWN (0x%02X)\n", value); break;
                    }
                }
            } else if (strcasecmp(param_name, "prf_mode") == 0 && cfg_len == 1) {
                unsigned char value = payload[offset + 2];
                if (ui_color_enabled) {
                    switch(value) {
                        case 0: printf("%sBPRF%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, value); break;
                        case 1: printf("%sHPRF_124_8M%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, value); break;
                        case 2: printf("%sHPRF_249_6M%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, value); break;
                        default: printf("%sUNKNOWN%s (0x%02X)\n", ANSI_COLOR_YELLOW, ANSI_RESET, value); break;
                    }
                } else {
                    switch(value) {
                        case 0: printf("BPRF (0x%02X)\n", value); break;
                        case 1: printf("HPRF_124_8M (0x%02X)\n", value); break;
                        case 2: printf("HPRF_249_6M (0x%02X)\n", value); break;
                        default: printf("UNKNOWN (0x%02X)\n", value); break;
                    }
                }
            } else if (strcasecmp(param_name, "hopping_mode") == 0 && cfg_len == 1) {
                unsigned char value = payload[offset + 2];
                if (ui_color_enabled) {
                    switch(value) {
                        case 0: printf("%sDISABLED%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, value); break;
                        case 1: printf("%sENABLED%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, value); break;
                        default: printf("%sUNKNOWN%s (0x%02X)\n", ANSI_COLOR_YELLOW, ANSI_RESET, value); break;
                    }
                } else {
                    switch(value) {
                        case 0: printf("DISABLED (0x%02X)\n", value); break;
                        case 1: printf("ENABLED (0x%02X)\n", value); break;
                        default: printf("UNKNOWN (0x%02X)\n", value); break;
                    }
                }
            } else if (cfg_len <= 8) {
                // For numeric values, try to display as integer
                uint64_t value = 0;
                for (int j = 0; j < cfg_len && j < 8; j++) {
                    value |= ((uint64_t)payload[offset + 2 + j]) << (j * 8);
                }
                
                // Get parameter range information
                uint64_t min_val, max_val;
                if (uci_config_get_app_param_range((AppConfigTlvType)cfg_id, &min_val, &max_val) == 0) {
                    if (ui_color_enabled) {
                        if (value >= min_val && value <= max_val) {
                            printf("%s%" PRIu64 "%s (0x%02X", ANSI_COLOR_BRIGHT_GREEN, value, ANSI_RESET, payload[offset + 2]);
                            for (int j = 1; j < cfg_len; j++) {
                                printf(" %02X", payload[offset + 2 + j]);
                            }
                            printf(") [Range: %" PRIu64 "-%" PRIu64 "]\n", min_val, max_val);
                        } else {
                            printf("%s%" PRIu64 "%s (0x%02X", ANSI_COLOR_YELLOW, value, ANSI_RESET, payload[offset + 2]);
                            for (int j = 1; j < cfg_len; j++) {
                                printf(" %02X", payload[offset + 2 + j]);
                            }
                            printf(") [OUT OF RANGE: %" PRIu64 "-%" PRIu64 "]\n", min_val, max_val);
                        }
                    } else {
                        if (value >= min_val && value <= max_val) {
                            printf("%" PRIu64 " (0x%02X", value, payload[offset + 2]);
                            for (int j = 1; j < cfg_len; j++) {
                                printf(" %02X", payload[offset + 2 + j]);
                            }
                            printf(") [Range: %" PRIu64 "-%" PRIu64 "]\n", min_val, max_val);
                        } else {
                            printf("%" PRIu64 " (0x%02X", value, payload[offset + 2]);
                            for (int j = 1; j < cfg_len; j++) {
                                printf(" %02X", payload[offset + 2 + j]);
                            }
                            printf(") [OUT OF RANGE: %" PRIu64 "-%" PRIu64 "]\n", min_val, max_val);
                        }
                    }
                } else {
                    if (ui_color_enabled) {
                        printf("%s%" PRIu64 "%s (0x%02X", ANSI_COLOR_BRIGHT_WHITE, value, ANSI_RESET, payload[offset + 2]);
                        for (int j = 1; j < cfg_len; j++) {
                            printf(" %02X", payload[offset + 2 + j]);
                        }
                        printf(")\n");
                    } else {
                        printf("%" PRIu64 " (0x%02X", value, payload[offset + 2]);
                        for (int j = 1; j < cfg_len; j++) {
                            printf(" %02X", payload[offset + 2 + j]);
                        }
                        printf(")\n");
                    }
                }
            } else {
                // For longer values, just show hex
                if (ui_color_enabled) {
                    printf("%s", ANSI_COLOR_BRIGHT_WHITE);
                }
                for (int j = 0; j < cfg_len; j++) {
                    printf("%02X ", payload[offset + 2 + j]);
                }
                if (ui_color_enabled) {
                    printf("%s\n", ANSI_RESET);
                } else {
                    printf("\n");
                }
            }
        }

        offset += 2 + cfg_len;
    }
}

void ui_decode_session_get_count_rsp(const unsigned char* payload, int payload_len) {
    if (ui_color_enabled) {
        printf("  %s%sSESSION_GET_COUNT Response:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  SESSION_GET_COUNT Response:\n");
    }

    if (payload_len < 2) {
        if (ui_color_enabled) {
            printf("    %s%sError: Payload too short (%d bytes, need at least 2)%s\n",
                   ANSI_COLOR_RED, ANSI_BOLD, payload_len, ANSI_RESET);
        } else {
            printf("    Error: Payload too short (%d bytes, need at least 2)\n", payload_len);
        }
        return;
    }

    unsigned char status = payload[0];
    unsigned char count = payload[1];

    if (ui_color_enabled) {
        printf("    %s%sStatus:%s 0x%02X", ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, status);
        switch(status) {
            case UCI_STATUS_OK: printf(" %s(OK)%s\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET); break;
            case UCI_STATUS_REJECTED: printf(" %s(REJECTED)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            case UCI_STATUS_FAILED: printf(" %s(FAILED)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            default: printf(" %s(UNKNOWN)%s\n", ANSI_COLOR_YELLOW, ANSI_RESET); break;
        }
        printf("    %s%sSession Count:%s %d\n",
               ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_RESET, count);
    } else {
        printf("    Status: 0x%02X", status);
        switch(status) {
            case UCI_STATUS_OK: printf(" (OK)\n"); break;
            case UCI_STATUS_REJECTED: printf(" (REJECTED)\n"); break;
            case UCI_STATUS_FAILED: printf(" (FAILED)\n"); break;
            default: printf(" (UNKNOWN)\n"); break;
        }
        printf("    Session Count: %d\n", count);
    }
}

void ui_decode_session_get_state_rsp(const unsigned char* payload, int payload_len) {
    if (ui_color_enabled) {
        printf("  %s%sSESSION_GET_STATE Response:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  SESSION_GET_STATE Response:\n");
    }

    if (payload_len < 2) {
        if (ui_color_enabled) {
            printf("    %s%sError: Payload too short (%d bytes, need at least 2)%s\n",
                   ANSI_COLOR_RED, ANSI_BOLD, payload_len, ANSI_RESET);
        } else {
            printf("    Error: Payload too short (%d bytes, need at least 2)\n", payload_len);
        }
        return;
    }

    unsigned char status = payload[0];
    unsigned char state = payload[1];

    if (ui_color_enabled) {
        printf("    %s%sStatus:%s 0x%02X", ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, status);
        switch(status) {
            case UCI_STATUS_OK: printf(" %s(OK)%s\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET); break;
            case UCI_STATUS_REJECTED: printf(" %s(REJECTED)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            case UCI_STATUS_FAILED: printf(" %s(FAILED)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            case UCI_STATUS_SESSION_NOT_EXIST: printf(" %s(SESSION_NOT_EXIST)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            default: printf(" %s(UNKNOWN)%s\n", ANSI_COLOR_YELLOW, ANSI_RESET); break;
        }
        printf("    %s%sSession State:%s 0x%02X", ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_RESET, state);
        switch(state) {
            case SESSION_STATE_INIT: printf(" %s(INIT)%s\n", ANSI_COLOR_CYAN, ANSI_RESET); break;
            case SESSION_STATE_DEINIT: printf(" %s(DEINIT)%s\n", ANSI_COLOR_CYAN, ANSI_RESET); break;
            case SESSION_STATE_ACTIVE: printf(" %s(ACTIVE)%s\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET); break;
            case SESSION_STATE_IDLE: printf(" %s(IDLE)%s\n", ANSI_COLOR_CYAN, ANSI_RESET); break;
            default: printf(" %s(UNKNOWN)%s\n", ANSI_COLOR_YELLOW, ANSI_RESET); break;
        }
    } else {
        printf("    Status: 0x%02X", status);
        switch(status) {
            case UCI_STATUS_OK: printf(" (OK)\n"); break;
            case UCI_STATUS_REJECTED: printf(" (REJECTED)\n"); break;
            case UCI_STATUS_FAILED: printf(" (FAILED)\n"); break;
            case UCI_STATUS_SESSION_NOT_EXIST: printf(" (SESSION_NOT_EXIST)\n"); break;
            default: printf(" (UNKNOWN)\n"); break;
        }
        printf("    Session State: 0x%02X", state);
        switch(state) {
            case SESSION_STATE_INIT: printf(" (INIT)\n"); break;
            case SESSION_STATE_DEINIT: printf(" (DEINIT)\n"); break;
            case SESSION_STATE_ACTIVE: printf(" (ACTIVE)\n"); break;
            case SESSION_STATE_IDLE: printf(" (IDLE)\n"); break;
            default: printf(" (UNKNOWN)\n"); break;
        }
    }
}

void ui_decode_session_update_controller_multicast_list_rsp(const unsigned char* payload, int payload_len) {
    if (ui_color_enabled) {
        printf("  %s%sSESSION_UPDATE_CONTROLLER_MULTICAST_LIST Response:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  SESSION_UPDATE_CONTROLLER_MULTICAST_LIST Response:\n");
    }

    if (payload_len < 2) {
        if (ui_color_enabled) {
            printf("    %s%sError: Payload too short (%d bytes, need at least 2)%s\n",
                   ANSI_COLOR_RED, ANSI_BOLD, payload_len, ANSI_RESET);
        } else {
            printf("    Error: Payload too short (%d bytes, need at least 2)\n", payload_len);
        }
        return;
    }

    unsigned char status = payload[0];
    unsigned char processed = payload[1];
    int offset = 2;

    if (ui_color_enabled) {
        printf("    %s%sStatus:%s 0x%02X", ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, status);
        switch (status) {
            case UCI_STATUS_OK: printf(" %s(OK)%s\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET); break;
            case UCI_STATUS_REJECTED: printf(" %s(REJECTED)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            case UCI_STATUS_FAILED: printf(" %s(FAILED)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            case UCI_STATUS_INVALID_PARAM: printf(" %s(INVALID_PARAM)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            default: printf(" %s(UNKNOWN)%s\n", ANSI_COLOR_YELLOW, ANSI_RESET); break;
        }
        if (status != UCI_STATUS_OK) {
            enhanced_error_analysis(status);
        }
        printf("    %s%sEntries Processed:%s %u\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_RESET, processed);
    } else {
        printf("    Status: 0x%02X", status);
        switch (status) {
            case UCI_STATUS_OK: printf(" (OK)\n"); break;
            case UCI_STATUS_REJECTED: printf(" (REJECTED)\n"); break;
            case UCI_STATUS_FAILED: printf(" (FAILED)\n"); break;
            case UCI_STATUS_INVALID_PARAM: printf(" (INVALID_PARAM)\n"); break;
            default: printf(" (UNKNOWN)\n"); break;
        }
        if (status != UCI_STATUS_OK) {
            enhanced_error_analysis(status);
        }
        printf("    Entries Processed: %u\n", processed);
    }

    for (unsigned char i = 0; i < processed; i++) {
        if (offset + 7 > payload_len) {
            if (ui_color_enabled) {
                printf("    %s%sError: Truncated entry %u in response%s\n",
                       ANSI_COLOR_RED, ANSI_BOLD, i, ANSI_RESET);
            } else {
                printf("    Error: Truncated entry %u in response\n", i);
            }
            return;
        }

        unsigned short short_address = ui_read_u16_le(&payload[offset]);
        offset += 2;
        unsigned int subsession_id = ui_read_u32_le(&payload[offset]);
        offset += 4;
        unsigned char entry_status = payload[offset++];

        if (ui_color_enabled) {
            printf("    %sEntry[%u]:%s Short=0x%04X, Subsession=0x%08X, Status=0x%02X",
                   ANSI_COLOR_BRIGHT_CYAN, i, ANSI_RESET, short_address, subsession_id, entry_status);
            switch (entry_status) {
                case UCI_STATUS_OK: printf(" %s(OK)%s\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET); break;
                case UCI_STATUS_INVALID_PARAM: printf(" %s(INVALID_PARAM)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
                case UCI_STATUS_MULTICAST_LIST_FULL: printf(" %s(LIST_FULL)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
                case UCI_STATUS_ADDRESS_ALREADY_PRESENT: printf(" %s(ALREADY_PRESENT)%s\n", ANSI_COLOR_YELLOW, ANSI_RESET); break;
                case UCI_STATUS_ADDRESS_NOT_FOUND: printf(" %s(NOT_FOUND)%s\n", ANSI_COLOR_YELLOW, ANSI_RESET); break;
                default: printf(" %s(UNKNOWN)%s\n", ANSI_COLOR_YELLOW, ANSI_RESET); break;
            }
        } else {
            printf("    Entry[%u]: Short=0x%04X, Subsession=0x%08X, Status=0x%02X",
                   i, short_address, subsession_id, entry_status);
            switch (entry_status) {
                case UCI_STATUS_OK: printf(" (OK)\n"); break;
                case UCI_STATUS_INVALID_PARAM: printf(" (INVALID_PARAM)\n"); break;
                case UCI_STATUS_MULTICAST_LIST_FULL: printf(" (LIST_FULL)\n"); break;
                case UCI_STATUS_ADDRESS_ALREADY_PRESENT: printf(" (ALREADY_PRESENT)\n"); break;
                case UCI_STATUS_ADDRESS_NOT_FOUND: printf(" (NOT_FOUND)\n"); break;
                default: printf(" (UNKNOWN)\n"); break;
            }
        }
    }
}

void ui_decode_session_update_active_rounds_dt_tag_rsp(const unsigned char* payload, int payload_len) {
    if (ui_color_enabled) {
        printf("  %s%sSESSION_UPDATE_ACTIVE_ROUNDS_DT_TAG Response:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  SESSION_UPDATE_ACTIVE_ROUNDS_DT_TAG Response:\n");
    }

    if (payload_len < 2) {
        if (ui_color_enabled) {
            printf("    %s%sError: Payload too short (%d bytes, need at least 2)%s\n",
                   ANSI_COLOR_RED, ANSI_BOLD, payload_len, ANSI_RESET);
        } else {
            printf("    Error: Payload too short (%d bytes, need at least 2)\n", payload_len);
        }
        return;
    }

    unsigned char status = payload[0];
    unsigned char stored_count = payload[1];

    if (ui_color_enabled) {
        printf("    %s%sStatus:%s 0x%02X", ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, status);
        switch (status) {
            case UCI_STATUS_OK: printf(" %s(OK)%s\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET); break;
            case UCI_STATUS_INVALID_PARAM: printf(" %s(INVALID_PARAM)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            case UCI_STATUS_INVALID_MSG_SIZE: printf(" %s(INVALID_MSG_SIZE)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            default: printf(" %s(UNKNOWN)%s\n", ANSI_COLOR_YELLOW, ANSI_RESET); break;
        }
        if (status != UCI_STATUS_OK) {
            enhanced_error_analysis(status);
        }
        printf("    %s%sStored Round Indices:%s %u\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_RESET, stored_count);
    } else {
        printf("    Status: 0x%02X", status);
        switch (status) {
            case UCI_STATUS_OK: printf(" (OK)\n"); break;
            case UCI_STATUS_INVALID_PARAM: printf(" (INVALID_PARAM)\n"); break;
            case UCI_STATUS_INVALID_MSG_SIZE: printf(" (INVALID_MSG_SIZE)\n"); break;
            default: printf(" (UNKNOWN)\n"); break;
        }
        if (status != UCI_STATUS_OK) {
            enhanced_error_analysis(status);
        }
        printf("    Stored Round Indices: %u\n", stored_count);
    }

    if (stored_count == 0) {
        return;
    }

    if (payload_len < 2 + stored_count) {
        if (ui_color_enabled) {
            printf("    %s%sError: Declared %u indices but payload shorter than expected%s\n",
                   ANSI_COLOR_RED, ANSI_BOLD, stored_count, ANSI_RESET);
        } else {
            printf("    Error: Declared %u indices but payload shorter than expected\n", stored_count);
        }
        return;
    }

    if (ui_color_enabled) {
        printf("    %sRound Indices:%s ", ANSI_COLOR_BRIGHT_CYAN, ANSI_RESET);
    } else {
        printf("    Round Indices: ");
    }

    for (unsigned char i = 0; i < stored_count; i++) {
        unsigned char value = payload[2 + i];
        if (ui_color_enabled) {
            printf("%s0x%02X%s", ANSI_COLOR_BRIGHT_GREEN, value, ANSI_RESET);
        } else {
            printf("0x%02X", value);
        }
        if (i + 1 < stored_count) {
            printf(" ");
        }
    }
    printf("\n");
}

void ui_decode_session_data_transfer_phase_config_rsp(const unsigned char* payload, int payload_len) {
    if (ui_color_enabled) {
        printf("  %s%sSESSION_DATA_TRANSFER_PHASE_CONFIG Response:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  SESSION_DATA_TRANSFER_PHASE_CONFIG Response:\n");
    }

    if (payload_len < 1) {
        if (ui_color_enabled) {
            printf("    %s%sError: Payload too short (%d bytes, need at least 1)%s\n",
                   ANSI_COLOR_RED, ANSI_BOLD, payload_len, ANSI_RESET);
        } else {
            printf("    Error: Payload too short (%d bytes, need at least 1)\n", payload_len);
        }
        return;
    }

    unsigned char status = payload[0];

    if (ui_color_enabled) {
        printf("    %s%sStatus:%s 0x%02X", ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, status);
        switch (status) {
            case UCI_STATUS_OK: printf(" %s(OK)%s\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET); break;
            case UCI_STATUS_INVALID_PARAM: printf(" %s(INVALID_PARAM)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            case UCI_STATUS_INVALID_MSG_SIZE: printf(" %s(INVALID_MSG_SIZE)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            default: printf(" %s(UNKNOWN)%s\n", ANSI_COLOR_YELLOW, ANSI_RESET); break;
        }
    } else {
        printf("    Status: 0x%02X", status);
        switch (status) {
            case UCI_STATUS_OK: printf(" (OK)\n"); break;
            case UCI_STATUS_INVALID_PARAM: printf(" (INVALID_PARAM)\n"); break;
            case UCI_STATUS_INVALID_MSG_SIZE: printf(" (INVALID_MSG_SIZE)\n"); break;
            default: printf(" (UNKNOWN)\n"); break;
        }
    }

    if (status != UCI_STATUS_OK) {
        enhanced_error_analysis(status);
    }
}

void ui_decode_session_query_data_size_in_ranging_rsp(const unsigned char* payload, int payload_len) {
    if (ui_color_enabled) {
        printf("  %s%sSESSION_QUERY_DATA_SIZE_IN_RANGING Response:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  SESSION_QUERY_DATA_SIZE_IN_RANGING Response:\n");
    }

    if (payload_len < 3) {  // Need at least 3 bytes: status(1) + max_data_size(2)
        if (ui_color_enabled) {
            printf("    %s%sError: Payload too short (%d bytes, need at least 3)%s\n",
                   ANSI_COLOR_RED, ANSI_BOLD, payload_len, ANSI_RESET);
        } else {
            printf("    Error: Payload too short (%d bytes, need at least 3)\n", payload_len);
        }
        return;
    }

    // Extract fields
    unsigned char status = payload[0];
    unsigned short max_data_size = (unsigned short)payload[1] |
                                  ((unsigned short)payload[2] << 8);

    // Display decoded information
    if (ui_color_enabled) {
        printf("    %s%sStatus:%s 0x%02X", ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, status);
        switch(status) {
            case UCI_STATUS_OK: printf(" %s(OK)%s\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET); break;
            case UCI_STATUS_REJECTED: printf(" %s(REJECTED)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            case UCI_STATUS_FAILED: printf(" %s(FAILED)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            default: printf(" %s(UNKNOWN)%s\n", ANSI_COLOR_YELLOW, ANSI_RESET); break;
        }
        printf("    %s%sMax Data Size:%s %u bytes\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_RESET, max_data_size);
    } else {
        printf("    Status: 0x%02X", status);
        switch(status) {
            case UCI_STATUS_OK: printf(" (OK)\n"); break;
            case UCI_STATUS_REJECTED: printf(" (REJECTED)\n"); break;
            case UCI_STATUS_FAILED: printf(" (FAILED)\n"); break;
            default: printf(" (UNKNOWN)\n"); break;
        }
        printf("    Max Data Size: %u bytes\n", max_data_size);
    }
}

void ui_decode_session_set_hybrid_controller_config_rsp(const unsigned char* payload, int payload_len) {
    if (ui_color_enabled) {
        printf("  %s%sSESSION_SET_HYBRID_CONTROLLER_CONFIG Response:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  SESSION_SET_HYBRID_CONTROLLER_CONFIG Response:\n");
    }

    if (payload_len < 1) {  // Need at least 1 byte for status
        if (ui_color_enabled) {
            printf("    %s%sError: Payload too short (%d bytes, need at least 1)%s\n",
                   ANSI_COLOR_RED, ANSI_BOLD, payload_len, ANSI_RESET);
        } else {
            printf("    Error: Payload too short (%d bytes, need at least 1)\n", payload_len);
        }
        return;
    }

    // Extract status
    unsigned char status = payload[0];

    // Display decoded information
    if (ui_color_enabled) {
        printf("    %s%sStatus:%s 0x%02X", ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, status);
        switch(status) {
            case UCI_STATUS_OK: printf(" %s(OK)%s\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET); break;
            case UCI_STATUS_REJECTED: printf(" %s(REJECTED)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            case UCI_STATUS_FAILED: printf(" %s(FAILED)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            default: printf(" %s(UNKNOWN)%s\n", ANSI_COLOR_YELLOW, ANSI_RESET); break;
        }
    } else {
        printf("    Status: 0x%02X", status);
        switch(status) {
            case UCI_STATUS_OK: printf(" (OK)\n"); break;
            case UCI_STATUS_REJECTED: printf(" (REJECTED)\n"); break;
            case UCI_STATUS_FAILED: printf(" (FAILED)\n"); break;
            default: printf(" (UNKNOWN)\n"); break;
        }
    }
}

void ui_decode_session_set_hybrid_controlee_config_rsp(const unsigned char* payload, int payload_len) {
    if (ui_color_enabled) {
        printf("  %s%sSESSION_SET_HYBRID_CONTROLEE_CONFIG Response:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  SESSION_SET_HYBRID_CONTROLEE_CONFIG Response:\n");
    }

    if (payload_len < 1) {  // Need at least 1 byte for status
        if (ui_color_enabled) {
            printf("    %s%sError: Payload too short (%d bytes, need at least 1)%s\n",
                   ANSI_COLOR_RED, ANSI_BOLD, payload_len, ANSI_RESET);
        } else {
            printf("    Error: Payload too short (%d bytes, need at least 1)\n", payload_len);
        }
        return;
    }

    // Extract status
    unsigned char status = payload[0];

    // Display decoded information
    if (ui_color_enabled) {
        printf("    %s%sStatus:%s 0x%02X", ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, status);
        switch(status) {
            case UCI_STATUS_OK: printf(" %s(OK)%s\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET); break;
            case UCI_STATUS_REJECTED: printf(" %s(REJECTED)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            case UCI_STATUS_FAILED: printf(" %s(FAILED)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            default: printf(" %s(UNKNOWN)%s\n", ANSI_COLOR_YELLOW, ANSI_RESET); break;
        }
    } else {
        printf("    Status: 0x%02X", status);
        switch(status) {
            case UCI_STATUS_OK: printf(" (OK)\n"); break;
            case UCI_STATUS_REJECTED: printf(" (REJECTED)\n"); break;
            case UCI_STATUS_FAILED: printf(" (FAILED)\n"); break;
            default: printf(" (UNKNOWN)\n"); break;
        }
    }
}

void ui_decode_session_logical_link_create_rsp(const unsigned char* payload, int payload_len) {
    if (ui_color_enabled) {
        printf("  %s%sSESSION_LOGICAL_LINK_CREATE Response:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  SESSION_LOGICAL_LINK_CREATE Response:\n");
    }

    if (payload_len < 3) {
        if (ui_color_enabled) {
            printf("    %s%sError: Payload too short (%d bytes, need at least 3)%s\n",
                   ANSI_COLOR_RED, ANSI_BOLD, payload_len, ANSI_RESET);
        } else {
            printf("    Error: Payload too short (%d bytes, need at least 3)\n", payload_len);
        }
        return;
    }

    unsigned char status = payload[0];
    unsigned char link_id = payload[1];
    unsigned char credit = payload[2];

    if (ui_color_enabled) {
        printf("    %s%sStatus:%s 0x%02X", ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, status);
        switch (status) {
            case UCI_STATUS_OK: printf(" %s(OK)%s\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET); break;
            case UCI_STATUS_MULTICAST_LIST_FULL: printf(" %s(NO_SLOTS)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            case UCI_STATUS_INVALID_PARAM: printf(" %s(INVALID_PARAM)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            default: printf(" %s(UNKNOWN)%s\n", ANSI_COLOR_YELLOW, ANSI_RESET); break;
        }
        printf("    %s%sLogical Link ID:%s 0x%02X\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_RESET, link_id);
        printf("    %s%sInitial Credit:%s %u\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_RESET, credit);
    } else {
        printf("    Status: 0x%02X", status);
        switch (status) {
            case UCI_STATUS_OK: printf(" (OK)\n"); break;
            case UCI_STATUS_MULTICAST_LIST_FULL: printf(" (NO_SLOTS)\n"); break;
            case UCI_STATUS_INVALID_PARAM: printf(" (INVALID_PARAM)\n"); break;
            default: printf(" (UNKNOWN)\n"); break;
        }
        printf("    Logical Link ID: 0x%02X\n", link_id);
        printf("    Initial Credit: %u\n", credit);
    }
}

void ui_decode_session_logical_link_close_rsp(const unsigned char* payload, int payload_len) {
    if (ui_color_enabled) {
        printf("  %s%sSESSION_LOGICAL_LINK_CLOSE Response:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  SESSION_LOGICAL_LINK_CLOSE Response:\n");
    }

    if (payload_len < 2) {
        if (ui_color_enabled) {
            printf("    %s%sError: Payload too short (%d bytes, need at least 2)%s\n",
                   ANSI_COLOR_RED, ANSI_BOLD, payload_len, ANSI_RESET);
        } else {
            printf("    Error: Payload too short (%d bytes, need at least 2)\n", payload_len);
        }
        return;
    }

    unsigned char status = payload[0];
    unsigned char link_id = payload[1];

    if (ui_color_enabled) {
        printf("    %s%sStatus:%s 0x%02X", ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, status);
        switch (status) {
            case UCI_STATUS_OK: printf(" %s(OK)%s\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET); break;
            case UCI_STATUS_INVALID_PARAM: printf(" %s(INVALID_PARAM)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            default: printf(" %s(UNKNOWN)%s\n", ANSI_COLOR_YELLOW, ANSI_RESET); break;
        }
        printf("    %s%sLogical Link ID:%s 0x%02X\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_RESET, link_id);
    } else {
        printf("    Status: 0x%02X", status);
        switch (status) {
            case UCI_STATUS_OK: printf(" (OK)\n"); break;
            case UCI_STATUS_INVALID_PARAM: printf(" (INVALID_PARAM)\n"); break;
            default: printf(" (UNKNOWN)\n"); break;
        }
        printf("    Logical Link ID: 0x%02X\n", link_id);
    }
}

void ui_decode_session_logical_link_get_param_rsp(const unsigned char* payload, int payload_len) {
    if (ui_color_enabled) {
        printf("  %s%sSESSION_LOGICAL_LINK_GET_PARAM Response:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  SESSION_LOGICAL_LINK_GET_PARAM Response:\n");
    }

    if (payload_len < 4) {
        if (ui_color_enabled) {
            printf("    %s%sError: Payload too short (%d bytes, need at least 4)%s\n",
                   ANSI_COLOR_RED, ANSI_BOLD, payload_len, ANSI_RESET);
        } else {
            printf("    Error: Payload too short (%d bytes, need at least 4)\n", payload_len);
        }
        return;
    }

    unsigned char status = payload[0];
    unsigned char link_id = payload[1];
    unsigned char mode = payload[2];
    unsigned char credit = payload[3];

    if (ui_color_enabled) {
        printf("    %s%sStatus:%s 0x%02X", ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, status);
        switch (status) {
            case UCI_STATUS_OK: printf(" %s(OK)%s\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET); break;
            case UCI_STATUS_INVALID_PARAM: printf(" %s(INVALID_PARAM)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            default: printf(" %s(UNKNOWN)%s\n", ANSI_COLOR_YELLOW, ANSI_RESET); break;
        }
        printf("    %s%sLogical Link ID:%s 0x%02X\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_RESET, link_id);
        printf("    %s%sMode:%s 0x%02X\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_RESET, mode);
        printf("    %s%sCredits:%s %u\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_RESET, credit);
    } else {
        printf("    Status: 0x%02X", status);
        switch (status) {
            case UCI_STATUS_OK: printf(" (OK)\n"); break;
            case UCI_STATUS_INVALID_PARAM: printf(" (INVALID_PARAM)\n"); break;
            default: printf(" (UNKNOWN)\n"); break;
        }
        printf("    Logical Link ID: 0x%02X\n", link_id);
        printf("    Mode: 0x%02X\n", mode);
        printf("    Credits: %u\n", credit);
    }
}

void ui_decode_session_start_rsp(const unsigned char* payload, int payload_len) {
    if (ui_color_enabled) {
        printf("  %s%sSESSION_START Response:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  SESSION_START Response:\n");
    }

    if (payload_len < 1) {
        if (ui_color_enabled) {
            printf("    %s%sError: Payload too short (%d bytes, need at least 1)%s\n",
                   ANSI_COLOR_RED, ANSI_BOLD, payload_len, ANSI_RESET);
        } else {
            printf("    Error: Payload too short (%d bytes, need at least 1)\n", payload_len);
        }
        return;
    }

    unsigned char status = payload[0];

    if (ui_color_enabled) {
        printf("    %s%sStatus:%s 0x%02X", ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, status);
        switch(status) {
            case UCI_STATUS_OK: printf(" %s(OK)%s\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET); break;
            case UCI_STATUS_REJECTED: printf(" %s(REJECTED)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            case UCI_STATUS_FAILED: printf(" %s(FAILED)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            case UCI_STATUS_INVALID_PARAM: printf(" %s(INVALID_PARAM)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            case UCI_STATUS_SESSION_NOT_EXIST: printf(" %s(SESSION_NOT_EXIST)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            case UCI_STATUS_SESSION_ACTIVE: printf(" %s(SESSION_ACTIVE)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            default: printf(" %s(UNKNOWN)%s\n", ANSI_COLOR_YELLOW, ANSI_RESET); break;
        }
    } else {
        printf("    Status: 0x%02X", status);
        switch(status) {
            case UCI_STATUS_OK: printf(" (OK)\n"); break;
            case UCI_STATUS_REJECTED: printf(" (REJECTED)\n"); break;
            case UCI_STATUS_FAILED: printf(" (FAILED)\n"); break;
            case UCI_STATUS_INVALID_PARAM: printf(" (INVALID_PARAM)\n"); break;
            case UCI_STATUS_SESSION_NOT_EXIST: printf(" (SESSION_NOT_EXIST)\n"); break;
            case UCI_STATUS_SESSION_ACTIVE: printf(" (SESSION_ACTIVE)\n"); break;
            default: printf(" (UNKNOWN)\n"); break;
        }
    }
}

void ui_decode_session_stop_rsp(const unsigned char* payload, int payload_len) {
    if (ui_color_enabled) {
        printf("  %s%sSESSION_STOP Response:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  SESSION_STOP Response:\n");
    }

    if (payload_len < 1) {
        if (ui_color_enabled) {
            printf("    %s%sError: Payload too short (%d bytes, need at least 1)%s\n",
                   ANSI_COLOR_RED, ANSI_BOLD, payload_len, ANSI_RESET);
        } else {
            printf("    Error: Payload too short (%d bytes, need at least 1)\n", payload_len);
        }
        return;
    }

    unsigned char status = payload[0];

    if (ui_color_enabled) {
        printf("    %s%sStatus:%s 0x%02X", ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, status);
        switch(status) {
            case UCI_STATUS_OK: printf(" %s(OK)%s\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET); break;
            case UCI_STATUS_REJECTED: printf(" %s(REJECTED)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            case UCI_STATUS_FAILED: printf(" %s(FAILED)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            case UCI_STATUS_INVALID_PARAM: printf(" %s(INVALID_PARAM)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            case UCI_STATUS_SESSION_NOT_EXIST: printf(" %s(SESSION_NOT_EXIST)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            default: printf(" %s(UNKNOWN)%s\n", ANSI_COLOR_YELLOW, ANSI_RESET); break;
        }
    } else {
        printf("    Status: 0x%02X", status);
        switch(status) {
            case UCI_STATUS_OK: printf(" (OK)\n"); break;
            case UCI_STATUS_REJECTED: printf(" (REJECTED)\n"); break;
            case UCI_STATUS_FAILED: printf(" (FAILED)\n"); break;
            case UCI_STATUS_INVALID_PARAM: printf(" (INVALID_PARAM)\n"); break;
            case UCI_STATUS_SESSION_NOT_EXIST: printf(" (SESSION_NOT_EXIST)\n"); break;
            default: printf(" (UNKNOWN)\n"); break;
        }
    }
}

void ui_decode_session_get_ranging_count_rsp(const unsigned char* payload, int payload_len) {
    if (ui_color_enabled) {
        printf("  %s%sSESSION_GET_RANGING_COUNT Response:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  SESSION_GET_RANGING_COUNT Response:\n");
    }

    if (payload_len < 5) {
        if (ui_color_enabled) {
            printf("    %s%sError: Payload too short (%d bytes, need at least 5)%s\n",
                   ANSI_COLOR_RED, ANSI_BOLD, payload_len, ANSI_RESET);
        } else {
            printf("    Error: Payload too short (%d bytes, need at least 5)\n", payload_len);
        }
        return;
    }

    unsigned char status = payload[0];
    unsigned int count = ui_read_u32_le(payload + 1);

    if (ui_color_enabled) {
        printf("    %s%sStatus:%s 0x%02X", ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, status);
        switch(status) {
            case UCI_STATUS_OK: printf(" %s(OK)%s\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET); break;
            case UCI_STATUS_REJECTED: printf(" %s(REJECTED)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            case UCI_STATUS_FAILED: printf(" %s(FAILED)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            case UCI_STATUS_INVALID_PARAM: printf(" %s(INVALID_PARAM)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            case UCI_STATUS_SESSION_NOT_EXIST: printf(" %s(SESSION_NOT_EXIST)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            default: printf(" %s(UNKNOWN)%s\n", ANSI_COLOR_YELLOW, ANSI_RESET); break;
        }
        printf("    %s%sRanging Count:%s %u\n",
               ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_RESET, count);
    } else {
        printf("    Status: 0x%02X", status);
        switch(status) {
            case UCI_STATUS_OK: printf(" (OK)\n"); break;
            case UCI_STATUS_REJECTED: printf(" (REJECTED)\n"); break;
            case UCI_STATUS_FAILED: printf(" (FAILED)\n"); break;
            case UCI_STATUS_INVALID_PARAM: printf(" (INVALID_PARAM)\n"); break;
            case UCI_STATUS_SESSION_NOT_EXIST: printf(" (SESSION_NOT_EXIST)\n"); break;
            default: printf(" (UNKNOWN)\n"); break;
        }
        printf("    Ranging Count: %u\n", count);
    }
}

void ui_decode_session_status_ntf(const unsigned char* payload, int payload_len) {
    if (ui_color_enabled) {
        printf("  %s%sSESSION_STATUS_NTF:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  SESSION_STATUS_NTF:\n");
    }

    if (payload_len < 6) {
        if (ui_color_enabled) {
            printf("    %s%sError: Payload too short (%d bytes, need at least 6)%s\n",
                   ANSI_COLOR_RED, ANSI_BOLD, payload_len, ANSI_RESET);
        } else {
            printf("    Error: Payload too short (%d bytes, need at least 6)\n", payload_len);
        }
        return;
    }

    uint32_t session_token = ui_read_u32_le(payload);
    unsigned char session_state = payload[4];
    unsigned char reason_code = payload[5];

    const uci_lookup_entry_t* state_entry = uci_lookup_session_state(session_state);
    const uci_lookup_entry_t* reason_entry = uci_lookup_session_reason(reason_code);

    const char* state_color = ANSI_COLOR_YELLOW;
    switch (session_state) {
        case SESSION_STATE_INIT:
            state_color = ANSI_COLOR_BRIGHT_BLUE;
            break;
        case SESSION_STATE_DEINIT:
            state_color = ANSI_COLOR_YELLOW;
            break;
        case SESSION_STATE_ACTIVE:
            state_color = ANSI_COLOR_BRIGHT_GREEN;
            break;
        case SESSION_STATE_IDLE:
            state_color = ANSI_COLOR_BRIGHT_CYAN;
            break;
        default:
            state_color = ANSI_COLOR_RED;
            break;
    }

    if (ui_color_enabled) {
        printf("    %s%sSession Token:%s 0x%08X\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_RESET, session_token);
    } else {
        printf("    Session Token: 0x%08X\n", session_token);
    }

    print_lookup_line("Session State", session_state, state_entry, state_color);
    print_lookup_line("Reason", reason_code, reason_entry, ANSI_COLOR_BRIGHT_CYAN);

    print_additional_payload_bytes(payload, payload_len, 6);
}

void ui_decode_session_data_credit_ntf(const unsigned char* payload, int payload_len) {
    if (ui_color_enabled) {
        printf("  %s%sSESSION_DATA_CREDIT_NTF:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  SESSION_DATA_CREDIT_NTF:\n");
    }

    if (payload_len < 5) {
        if (ui_color_enabled) {
            printf("    %s%sError: Payload too short (%d bytes, need at least 5)%s\n",
                   ANSI_COLOR_RED, ANSI_BOLD, payload_len, ANSI_RESET);
        } else {
            printf("    Error: Payload too short (%d bytes, need at least 5)\n", payload_len);
        }
        return;
    }

    uint32_t session_token = ui_read_u32_le(payload);
    unsigned char credit_flag = payload[4];
    const char* availability_text = (credit_flag != 0) ? "Credits Available" : "No Credits Available";

    if (ui_color_enabled) {
        printf("    %s%sSession Token:%s 0x%08X\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_RESET, session_token);
        printf("    %s%sCredit Availability:%s 0x%02X %s(%s)%s\n",
               ANSI_COLOR_BRIGHT_YELLOW,
               ANSI_BOLD,
               ANSI_RESET,
               credit_flag,
               credit_flag != 0 ? ANSI_COLOR_BRIGHT_GREEN : ANSI_COLOR_RED,
               availability_text,
               ANSI_RESET);
    } else {
        printf("    Session Token: 0x%08X\n", session_token);
        printf("    Credit Availability: 0x%02X (%s)\n", credit_flag, availability_text);
    }

    print_additional_payload_bytes(payload, payload_len, 5);
}

void ui_decode_session_data_transfer_status_ntf(const unsigned char* payload, int payload_len) {
    if (ui_color_enabled) {
        printf("  %s%sSESSION_DATA_TRANSFER_STATUS_NTF:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  SESSION_DATA_TRANSFER_STATUS_NTF:\n");
    }

    if (payload_len < 8) {
        if (ui_color_enabled) {
            printf("    %s%sError: Payload too short (%d bytes, need at least 8)%s\n",
                   ANSI_COLOR_RED, ANSI_BOLD, payload_len, ANSI_RESET);
        } else {
            printf("    Error: Payload too short (%d bytes, need at least 8)\n", payload_len);
        }
        return;
    }

    uint32_t session_token = ui_read_u32_le(payload);
    uint16_t sequence_number = ui_read_u16_le(&payload[4]);
    unsigned char status = payload[6];
    unsigned char tx_count = payload[7];

    const uci_lookup_entry_t* status_entry = uci_lookup_data_transfer_status(status);

    const char* status_color = (status == UCI_DATA_TRANSFER_STATUS_REPETITION_OK ||
                                status == UCI_DATA_TRANSFER_STATUS_OK)
                                   ? ANSI_COLOR_BRIGHT_GREEN
                                   : ANSI_COLOR_RED;

    if (ui_color_enabled) {
        printf("    %s%sSession Token:%s 0x%08X\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_RESET, session_token);
        printf("    %s%sUCI Sequence Number:%s %u\n", ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, sequence_number);
    } else {
        printf("    Session Token: 0x%08X\n", session_token);
        printf("    UCI Sequence Number: %u\n", sequence_number);
    }

    print_lookup_line("Transfer Status", status, status_entry, status_color);

    if (ui_color_enabled) {
        printf("    %s%sTX Attempt Count:%s %u\n", ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, tx_count);
    } else {
        printf("    TX Attempt Count: %u\n", tx_count);
    }

    print_additional_payload_bytes(payload, payload_len, 8);
}

void ui_decode_session_logical_link_uwbs_create_ntf(const unsigned char* payload, int payload_len) {
    if (ui_color_enabled) {
        printf("  %s%sSESSION_LOGICAL_LINK_UWBS_CREATE_NTF:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  SESSION_LOGICAL_LINK_UWBS_CREATE_NTF:\n");
    }

    if (payload_len < 6) {
        if (ui_color_enabled) {
            printf("    %s%sError: Payload too short (%d bytes, need at least 6)%s\n",
                   ANSI_COLOR_RED, ANSI_BOLD, payload_len, ANSI_RESET);
        } else {
            printf("    Error: Payload too short (%d bytes, need at least 6)\n", payload_len);
        }
        return;
    }

    uint32_t session_handle = ui_read_u32_le(payload);
    unsigned char link_id = payload[4];
    unsigned char credit = payload[5];

    if (ui_color_enabled) {
        printf("    %s%sSession Handle:%s 0x%08X\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_RESET, session_handle);
        printf("    %s%sLogical Link ID:%s 0x%02X\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_RESET, link_id);
        printf("    %s%sCredits:%s %u\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_RESET, credit);
    } else {
        printf("    Session Handle: 0x%08X\n", session_handle);
        printf("    Logical Link ID: 0x%02X\n", link_id);
        printf("    Credits: %u\n", credit);
    }
}

void ui_decode_session_logical_link_uwbs_close_ntf(const unsigned char* payload, int payload_len) {
    if (ui_color_enabled) {
        printf("  %s%sSESSION_LOGICAL_LINK_UWBS_CLOSE_NTF:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  SESSION_LOGICAL_LINK_UWBS_CLOSE_NTF:\n");
    }

    if (payload_len < 6) {
        if (ui_color_enabled) {
            printf("    %s%sError: Payload too short (%d bytes, need at least 6)%s\n",
                   ANSI_COLOR_RED, ANSI_BOLD, payload_len, ANSI_RESET);
        } else {
            printf("    Error: Payload too short (%d bytes, need at least 6)\n", payload_len);
        }
        return;
    }

    uint32_t session_handle = ui_read_u32_le(payload);
    unsigned char link_id = payload[4];
    unsigned char reason = payload[5];

    if (ui_color_enabled) {
        printf("    %s%sSession Handle:%s 0x%08X\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_RESET, session_handle);
        printf("    %s%sLogical Link ID:%s 0x%02X\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_RESET, link_id);
        printf("    %s%sReason:%s 0x%02X\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_RESET, reason);
    } else {
        printf("    Session Handle: 0x%08X\n", session_handle);
        printf("    Logical Link ID: 0x%02X\n", link_id);
        printf("    Reason: 0x%02X\n", reason);
    }
}

void ui_decode_session_info_ntf(const unsigned char* payload, int payload_len) {
    ui_decode_range_data_ntf(payload, payload_len);
}

void ui_decode_range_data_ntf(const unsigned char* payload, int payload_len) {
    if (ui_color_enabled) {
        printf("  %s%sRANGE_DATA_NTF (SESSION_INFO_NTF):%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  RANGE_DATA_NTF (SESSION_INFO_NTF):\n");
    }

    if (payload_len < 25) {
        print_error_line(4, "Payload too short for RANGE_DATA_NTF (%d bytes)", payload_len);
        return;
    }

    uint32_t sequence_number = ui_read_u32_le(&payload[0]);
    uint32_t session_handle = ui_read_u32_le(&payload[4]);
    uint8_t reserved_byte = payload[8];
    uint32_t ranging_interval_ms = ui_read_u32_le(&payload[9]);
    uint8_t measurement_type = payload[13];
    uint8_t mac_addressing_mode = payload[15];
    uint32_t primary_session_id = ui_read_u32_le(&payload[16]);
    uint8_t measurement_count = payload[24];

    const char* measurement_type_desc = map_ranging_measurement_type(measurement_type);
    const char* mac_mode_desc = mac_addressing_mode ? "Extended Address (8 bytes)" : "Short Address (2 bytes)";
    int mac_len = mac_addressing_mode ? 8 : 2;

    print_field_line_color(4,
                           "Sequence Number",
                           ANSI_COLOR_BRIGHT_YELLOW,
                           "%u",
                           sequence_number);
    print_field_line_color(4,
                           "Session Handle",
                           ANSI_COLOR_BRIGHT_YELLOW,
                           "0x%08X",
                           session_handle);
    print_field_line_color(4,
                           "Reserved Byte",
                           ANSI_COLOR_BRIGHT_YELLOW,
                           "0x%02X",
                           reserved_byte);
    print_field_line_color(4,
                           "Current Ranging Interval",
                           ANSI_COLOR_BRIGHT_YELLOW,
                           "%u ms",
                           ranging_interval_ms);
    print_field_line_color(4,
                           "Measurement Type",
                           ANSI_COLOR_BRIGHT_YELLOW,
                           "%s (0x%02X)",
                           measurement_type_desc,
                           measurement_type);
    print_field_line_color(4,
                           "MAC Addressing Mode",
                           ANSI_COLOR_BRIGHT_YELLOW,
                           "%s",
                           mac_mode_desc);
    print_field_line_color(4,
                           "Primary Session ID",
                           ANSI_COLOR_BRIGHT_YELLOW,
                           "0x%08X",
                           primary_session_id);
    print_field_line_color(4,
                           "Measurement Count",
                           ANSI_COLOR_BRIGHT_YELLOW,
                           "%u",
                           measurement_count);

    int offset = 25;
    for (uint8_t i = 0; i < measurement_count; i++) {
        if (offset >= payload_len) {
            print_warning_line(4,
                               "No data left while expecting measurement %u",
                               i + 1);
            break;
        }

        print_measurement_header(i + 1);

        bool parsed = false;
        switch (measurement_type) {
            case 0x00:
                parsed = decode_range_measurement_owr_ul_tdoa(payload, payload_len, &offset, mac_len, i + 1);
                break;
            case 0x01:
                parsed = decode_range_measurement_twr(payload, payload_len, &offset, mac_len, i + 1);
                break;
            case 0x02:
                parsed = decode_range_measurement_owr_dl_tdoa(payload, payload_len, &offset, mac_len, i + 1);
                break;
            case 0x03:
                parsed = decode_range_measurement_owr_aoa(payload, payload_len, &offset, mac_len, i + 1);
                break;
            case 0x04:
                parsed = decode_range_measurement_ccc_controller(payload, payload_len, &offset, i + 1);
                break;
            case 0x05:
                parsed = decode_range_measurement_ccc_controlee(payload, payload_len, &offset, i + 1);
                break;
            case 0x06:
                parsed = decode_range_measurement_owr_dl_tdoa_v2(payload, payload_len, &offset, mac_len, i + 1);
                break;
            default:
                parsed = decode_range_measurement_unknown(i + 1, measurement_type);
                offset = payload_len;
                break;
        }

        if (!parsed) {
            print_warning_line(4, "Stopping measurement parsing after entry %u", i + 1);
            break;
        }
    }

    if (offset < payload_len) {
        print_hex_data(4,
                       "Trailing Vendor Data",
                       &payload[offset],
                       payload_len - offset,
                       32);
    }
}

void ui_decode_android_range_diagnostics_ntf(const unsigned char* payload, int payload_len) {
    if (ui_color_enabled) {
        printf("  %s%sANDROID_RANGE_DIAGNOSTICS_NTF:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  ANDROID_RANGE_DIAGNOSTICS_NTF:\n");
    }

    if (payload_len < 9) {
        print_error_line(4, "Payload too short for diagnostics notification (%d bytes)", payload_len);
        return;
    }

    uint32_t session_token = ui_read_u32_le(&payload[0]);
    uint32_t sequence_number = ui_read_u32_le(&payload[4]);
    uint8_t report_count = payload[8];
    int offset = 9;

    print_field_line_color(4,
                           "Session Token",
                           ANSI_COLOR_BRIGHT_YELLOW,
                           "0x%08X",
                           session_token);
    print_field_line_color(4,
                           "Sequence Number",
                           ANSI_COLOR_BRIGHT_YELLOW,
                           "%u",
                           sequence_number);
    print_field_line_color(4,
                           "Frame Report Count",
                           ANSI_COLOR_BRIGHT_YELLOW,
                           "%u",
                           report_count);

    for (uint8_t report_idx = 0; report_idx < report_count; ++report_idx) {
        if (offset + 4 > payload_len) {
            print_error_line(4, "Diagnostics report %u truncated", report_idx + 1);
            return;
        }

        uint8_t msg_id = payload[offset++];
        uint8_t action = payload[offset++];
        uint8_t antenna_set = payload[offset++];
        uint8_t field_count = payload[offset++];

        if (ui_color_enabled) {
            printf("    %s%sFrame Report %u:%s\n",
                   ANSI_COLOR_BRIGHT_CYAN,
                   ANSI_BOLD,
                   report_idx + 1,
                   ANSI_RESET);
        } else {
            printf("    Frame Report %u:\n", report_idx + 1);
        }

        print_field_line_color(6,
                               "Message ID",
                               ANSI_COLOR_BRIGHT_GREEN,
                               "0x%02X",
                               msg_id);
        print_field_line_color(6,
                               "Action",
                               ANSI_COLOR_BRIGHT_GREEN,
                               "0x%02X",
                               action);
        print_field_line_color(6,
                               "Antenna Set",
                               ANSI_COLOR_BRIGHT_GREEN,
                               "0x%02X",
                               antenna_set);
        print_field_line_color(6,
                               "Field Count",
                               ANSI_COLOR_BRIGHT_GREEN,
                               "%u",
                               field_count);

        for (uint8_t field_idx = 0; field_idx < field_count; ++field_idx) {
            if (offset + 3 > payload_len) {
                print_error_line(6,
                                 "Report %u field %u truncated",
                                 report_idx + 1,
                                 field_idx + 1);
                return;
            }

            uint8_t field_id = payload[offset++];
            uint16_t field_size = ui_read_u16_le(&payload[offset]);
            offset += 2;

            if (offset + field_size > payload_len) {
                print_error_line(6,
                                 "Report %u field %u exceeds payload",
                                 report_idx + 1,
                                 field_idx + 1);
                return;
            }

            const unsigned char* field_data = &payload[offset];
            offset += field_size;

            switch (field_id) {
                case UCI_DIAG_REPORT_AOAS: {
                    if (field_size % 8 != 0) {
                        print_warning_line(6, "AoA field has unexpected length %u", field_size);
                    }
                    uint16_t entry_count = field_size / 8;
                    print_field_line_color(6,
                                           "AoA Measurements",
                                           ANSI_COLOR_BRIGHT_GREEN,
                                           "%u entries",
                                           entry_count);
                    for (uint16_t idx = 0; idx < entry_count; ++idx) {
                        const unsigned char* entry = &field_data[idx * 8];
                        int16_t tdoa = (int16_t)ui_read_u16_le(&entry[0]);
                        int16_t pdoa = (int16_t)ui_read_u16_le(&entry[2]);
                        int16_t aoa = (int16_t)ui_read_u16_le(&entry[4]);
                        uint8_t fom = entry[6];
                        uint8_t type = entry[7];
                        print_field_line_color(8,
                                               "AoA Entry",
                                               ANSI_COLOR_BRIGHT_GREEN,
                                               "TDoA=%d, PDoA=%d, AoA=%d, FoM=%u, Type=0x%02X",
                                               tdoa,
                                               pdoa,
                                               aoa,
                                               fom,
                                               type);
                    }
                    break;
                }
                case UCI_DIAG_REPORT_EXTRA_STATUS: {
                    if (field_size >= 2) {
                        uint16_t extra = ui_read_u16_le(field_data);
                        print_field_line_color(6,
                                               "Extra Status",
                                               ANSI_COLOR_BRIGHT_GREEN,
                                               "0x%04X",
                                               extra);
                    } else {
                        print_warning_line(6, "Extra status too short (%u bytes)", field_size);
                    }
                    break;
                }
                case UCI_DIAG_REPORT_CFO_Q26: {
                    if (field_size >= 4) {
                        int32_t cfo_raw = (int32_t)ui_read_u32_le(field_data);
                        print_field_line_color(6,
                                               "CFO Q26",
                                               ANSI_COLOR_BRIGHT_GREEN,
                                               "%d",
                                               cfo_raw);
                    } else {
                        print_warning_line(6, "CFO Q26 too short (%u bytes)", field_size);
                    }
                    break;
                }
                case UCI_DIAG_REPORT_EMITTER_SHORT_ADDR: {
                    if (field_size >= 2) {
                        uint16_t short_addr = ui_read_u16_le(field_data);
                        print_field_line_color(6,
                                               "Emitter Short Address",
                                               ANSI_COLOR_BRIGHT_GREEN,
                                               "0x%04X",
                                               short_addr);
                    } else {
                        print_warning_line(6,
                                           "Emitter short address too short (%u bytes)",
                                           field_size);
                    }
                    break;
                }
                case UCI_DIAG_REPORT_SEGMENT_METRICS: {
                    if (field_size % 17 != 0) {
                        print_warning_line(6,
                                           "Segment metrics unexpected size %u",
                                           field_size);
                    }
                    uint16_t metric_count = field_size / 17;
                    print_field_line_color(6,
                                           "Segment Metrics",
                                           ANSI_COLOR_BRIGHT_GREEN,
                                           "%u entries",
                                           metric_count);
                    for (uint16_t idx = 0; idx < metric_count; ++idx) {
                        const unsigned char* entry = &field_data[idx * 17];
                        uint8_t receiver_segment = entry[0];
                        uint16_t noise_value = ui_read_u16_le(&entry[1]);
                        double rsl_q8 = decode_q_unsigned(ui_read_u16_le(&entry[3]), 8);
                        uint16_t fp_index = ui_read_u16_le(&entry[5]);
                        double fp_rsl = decode_q_unsigned(ui_read_u16_le(&entry[7]), 8);
                        double fp_ns = decode_q_unsigned(ui_read_u16_le(&entry[9]), 6);
                        uint16_t pp_index = ui_read_u16_le(&entry[11]);
                        double pp_rsl = decode_q_unsigned(ui_read_u16_le(&entry[13]), 8);
                        double pp_ns = decode_q_unsigned(ui_read_u16_le(&entry[15]), 6);
                        print_field_line_color(8,
                                               "Metric",
                                               ANSI_COLOR_BRIGHT_GREEN,
                                               "seg=%u noise=%u rsl=%.2f fp_idx=%u fp_rsl=%.2f fp_ns=%.2f pp_idx=%u pp_rsl=%.2f pp_ns=%.2f",
                                               receiver_segment,
                                               noise_value,
                                               rsl_q8,
                                               fp_index,
                                               fp_rsl,
                                               fp_ns,
                                               pp_index,
                                               pp_rsl,
                                               pp_ns);
                    }
                    break;
                }
                case UCI_DIAG_REPORT_CIRS: {
                    uint16_t remaining = field_size;
                    const unsigned char* cursor_ptr = field_data;
                    uint8_t cir_index = 0;
                    while (remaining >= 4) {
                        uint8_t receiver_segment = cursor_ptr[0];
                        uint8_t fpath_tap_offset = cursor_ptr[1];
                        uint8_t tap_count = cursor_ptr[2];
                        uint8_t tap_size = cursor_ptr[3];
                        cursor_ptr += 4;
                        remaining -= 4;
                        uint16_t taps_bytes = (uint16_t)tap_count * tap_size;
                        if (remaining < taps_bytes) {
                            print_warning_line(8,
                                               "CIR entry %u truncated",
                                               cir_index + 1);
                            break;
                        }
                        print_field_line_color(8,
                                               "CIR Entry",
                                               ANSI_COLOR_BRIGHT_GREEN,
                                               "seg=%u fp_offset=%u taps=%u size=%u",
                                               receiver_segment,
                                               fpath_tap_offset,
                                               tap_count,
                                               tap_size);
                        cursor_ptr += taps_bytes;
                        remaining -= taps_bytes;
                        cir_index++;
                    }
                    if (remaining > 0) {
                        print_hex_data(8, "CIR Residual", cursor_ptr, remaining, remaining);
                    }
                    break;
                }
                case UCI_DIAG_REPORT_CONFIDENCE_METRICS:
                case UCI_DIAG_REPORT_SEGMENT_CONFIDENCE_RAW_DATA:
                default:
                    print_hex_data(6,
                                   "Diagnostics Field",
                                   field_data,
                                   field_size,
                                   32);
                    break;
            }
        }
    }

    if (offset < payload_len) {
        print_hex_data(4,
                       "Trailing Diagnostics Data",
                       &payload[offset],
                       payload_len - offset,
                       32);
    }
}

// Android vendor command response decoders

// Decode ANDROID_GET_POWER_STATS response
void ui_decode_android_get_power_stats_rsp(const unsigned char* payload, int payload_len) {
    if (ui_color_enabled) {
        printf("  %s%sANDROID_GET_POWER_STATS Response:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  ANDROID_GET_POWER_STATS Response:\n");
    }

    if (payload_len < 17) {
        if (ui_color_enabled) {
            printf("    %s%sError: Payload too short (%d bytes, need at least 17)%s\n",
                   ANSI_COLOR_RED, ANSI_BOLD, payload_len, ANSI_RESET);
        } else {
            printf("    Error: Payload too short (%d bytes, need at least 17)\n", payload_len);
        }
        return;
    }

    unsigned char status = payload[0];

    if (ui_color_enabled) {
        printf("    %s%sStatus:%s 0x%02X", ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, status);
        switch(status) {
            case UCI_STATUS_OK: printf(" %s(OK)%s\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET); break;
            case UCI_STATUS_REJECTED: printf(" %s(REJECTED)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            case UCI_STATUS_FAILED: printf(" %s(FAILED)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            case UCI_STATUS_INVALID_PARAM: printf(" %s(INVALID_PARAM)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            default: printf(" %s(UNKNOWN)%s\n", ANSI_COLOR_YELLOW, ANSI_RESET); break;
        }
        // Call enhanced analysis for more detailed information
        if (status != UCI_STATUS_OK) {
            enhanced_error_analysis(status);
        }
        printf("    %s%sPower Statistics Data:%s %d bytes\n",
               ANSI_COLOR_BRIGHT_CYAN, ANSI_BOLD, ANSI_RESET, payload_len - 1);
    } else {
        printf("    Status: 0x%02X", status);
        switch(status) {
            case UCI_STATUS_OK: printf(" (OK)\n"); break;
            case UCI_STATUS_REJECTED: printf(" (REJECTED)\n"); break;
            case UCI_STATUS_FAILED: printf(" (FAILED)\n"); break;
            case UCI_STATUS_INVALID_PARAM: printf(" (INVALID_PARAM)\n"); break;
            default: printf(" (UNKNOWN)\n"); break;
        }
        // Call enhanced analysis for more detailed information
        if (status != UCI_STATUS_OK) {
            enhanced_error_analysis(status);
        }
        printf("    Power Statistics Data: %d bytes\n", payload_len - 1);
    }

    // Display power statistics data in hex format
    if (payload_len > 1) {
        if (ui_color_enabled) {
            printf("    %s%sRaw Data:%s ", ANSI_COLOR_BRIGHT_WHITE, ANSI_BOLD, ANSI_RESET);
        } else {
            printf("    Raw Data: ");
        }
        for (int i = 1; i < payload_len && i < 33; i++) {
            if (ui_color_enabled) {
                printf("%s%02X%s ", ANSI_COLOR_BRIGHT_WHITE, payload[i], ANSI_RESET);
            } else {
                printf("%02X ", payload[i]);
            }
        }
        if (payload_len > 33) {
            if (ui_color_enabled) {
                printf("%s... (and %d more bytes)%s", ANSI_COLOR_BRIGHT_BLACK, payload_len - 33, ANSI_RESET);
            } else {
                printf("... (and %d more bytes)", payload_len - 33);
            }
        }
        printf("\n");
    }
}

// Decode ANDROID_SET_COUNTRY_CODE response
void ui_decode_android_set_country_code_rsp(const unsigned char* payload, int payload_len) {
    if (ui_color_enabled) {
        printf("  %s%sANDROID_SET_COUNTRY_CODE Response:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  ANDROID_SET_COUNTRY_CODE Response:\n");
    }

    if (payload_len < 1) {
        if (ui_color_enabled) {
            printf("    %s%sError: Payload too short (%d bytes, need at least 1)%s\n",
                   ANSI_COLOR_RED, ANSI_BOLD, payload_len, ANSI_RESET);
        } else {
            printf("    Error: Payload too short (%d bytes, need at least 1)\n", payload_len);
        }
        return;
    }

    unsigned char status = payload[0];

    if (ui_color_enabled) {
        printf("    %s%sStatus:%s 0x%02X", ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, status);
        switch(status) {
            case UCI_STATUS_OK: printf(" %s(OK)%s\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET); break;
            case UCI_STATUS_REJECTED: printf(" %s(REJECTED)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            case UCI_STATUS_FAILED: printf(" %s(FAILED)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            case UCI_STATUS_INVALID_PARAM: printf(" %s(INVALID_PARAM)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            default: printf(" %s(UNKNOWN)%s\n", ANSI_COLOR_YELLOW, ANSI_RESET); break;
        }
        // Call enhanced analysis for more detailed information
        if (status != UCI_STATUS_OK) {
            enhanced_error_analysis(status);
        }
    } else {
        printf("    Status: 0x%02X", status);
        switch(status) {
            case UCI_STATUS_OK: printf(" (OK)\n"); break;
            case UCI_STATUS_REJECTED: printf(" (REJECTED)\n"); break;
            case UCI_STATUS_FAILED: printf(" (FAILED)\n"); break;
            case UCI_STATUS_INVALID_PARAM: printf(" (INVALID_PARAM)\n"); break;
            default: printf(" (UNKNOWN)\n"); break;
        }
        // Call enhanced analysis for more detailed information
        if (status != UCI_STATUS_OK) {
            enhanced_error_analysis(status);
        }
    }
}

// Decode ANDROID_RADAR_SET_APP_CONFIG response
void ui_decode_android_radar_set_app_config_rsp(const unsigned char* payload, int payload_len) {
    if (ui_color_enabled) {
        printf("  %s%sANDROID_RADAR_SET_APP_CONFIG Response:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  ANDROID_RADAR_SET_APP_CONFIG Response:\n");
    }

    if (payload_len < 2) {
        if (ui_color_enabled) {
            printf("    %s%sError: Payload too short (%d bytes, need at least 2)%s\n",
                   ANSI_COLOR_RED, ANSI_BOLD, payload_len, ANSI_RESET);
        } else {
            printf("    Error: Payload too short (%d bytes, need at least 2)\n", payload_len);
        }
        return;
    }

    unsigned char status = payload[0];
    unsigned char num_configs = payload[1];

    if (ui_color_enabled) {
        printf("    %s%sStatus:%s 0x%02X", ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, status);
        switch(status) {
            case UCI_STATUS_OK: printf(" %s(OK)%s\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET); break;
            case UCI_STATUS_REJECTED: printf(" %s(REJECTED)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            case UCI_STATUS_FAILED: printf(" %s(FAILED)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            case UCI_STATUS_INVALID_PARAM: printf(" %s(INVALID_PARAM)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            default: printf(" %s(UNKNOWN)%s\n", ANSI_COLOR_YELLOW, ANSI_RESET); break;
        }
        // Call enhanced analysis for more detailed information
        if (status != UCI_STATUS_OK) {
            enhanced_error_analysis(status);
        }
        printf("    %s%sNumber of Config Status:%s %d\n",
               ANSI_COLOR_BRIGHT_CYAN, ANSI_BOLD, ANSI_RESET, num_configs);
    } else {
        printf("    Status: 0x%02X", status);
        switch(status) {
            case UCI_STATUS_OK: printf(" (OK)\n"); break;
            case UCI_STATUS_REJECTED: printf(" (REJECTED)\n"); break;
            case UCI_STATUS_FAILED: printf(" (FAILED)\n"); break;
            case UCI_STATUS_INVALID_PARAM: printf(" (INVALID_PARAM)\n"); break;
            default: printf(" (UNKNOWN)\n"); break;
        }
        // Call enhanced analysis for more detailed information
        if (status != UCI_STATUS_OK) {
            enhanced_error_analysis(status);
        }
        printf("    Number of Config Status: %d\n", num_configs);
    }

    // Parse config status entries (each is 2 bytes: config_id + status)
    int offset = 2;
    for (int i = 0; i < num_configs && offset + 2 <= payload_len; i++) {
        unsigned char cfg_id = payload[offset];
        unsigned char cfg_status = payload[offset + 1];
        
        // Get human-readable parameter name
        const char* param_name = uci_config_get_app_param_name((AppConfigTlvType)cfg_id);

        if (ui_color_enabled) {
            printf("    %s%sConfig[%d]:%s ID=0x%02X", 
                   ANSI_COLOR_BRIGHT_CYAN, ANSI_BOLD, i, ANSI_RESET, cfg_id);
            if (param_name) {
                printf(" (%s)", param_name);
            }
            printf(", Status=0x%02X", cfg_status);
            switch(cfg_status) {
                case UCI_STATUS_OK: printf(" %s(OK)%s\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET); break;
                case UCI_STATUS_REJECTED: printf(" %s(REJECTED)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
                case UCI_STATUS_FAILED: printf(" %s(FAILED)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
                default: printf(" %s(UNKNOWN)%s\n", ANSI_COLOR_YELLOW, ANSI_RESET); break;
            }
        } else {
            printf("    Config[%d]: ID=0x%02X", i, cfg_id);
            if (param_name) {
                printf(" (%s)", param_name);
            }
            printf(", Status=0x%02X", cfg_status);
            switch(cfg_status) {
                case UCI_STATUS_OK: printf(" (OK)\n"); break;
                case UCI_STATUS_REJECTED: printf(" (REJECTED)\n"); break;
                case UCI_STATUS_FAILED: printf(" (FAILED)\n"); break;
                default: printf(" (UNKNOWN)\n"); break;
            }
        }
        offset += 2;
    }
}

// Decode ANDROID_RADAR_GET_APP_CONFIG response
void ui_decode_android_radar_get_app_config_rsp(const unsigned char* payload, int payload_len) {
    if (ui_color_enabled) {
        printf("  %s%sANDROID_RADAR_GET_APP_CONFIG Response:%s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("  ANDROID_RADAR_GET_APP_CONFIG Response:\n");
    }

    if (payload_len < 2) {
        if (ui_color_enabled) {
            printf("    %s%sError: Payload too short (%d bytes, need at least 2)%s\n",
                   ANSI_COLOR_RED, ANSI_BOLD, payload_len, ANSI_RESET);
        } else {
            printf("    Error: Payload too short (%d bytes, need at least 2)\n", payload_len);
        }
        return;
    }

    unsigned char status = payload[0];
    unsigned char num_tlvs = payload[1];

    if (ui_color_enabled) {
        printf("    %s%sStatus:%s 0x%02X", ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, status);
        switch(status) {
            case UCI_STATUS_OK: printf(" %s(OK)%s\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET); break;
            case UCI_STATUS_REJECTED: printf(" %s(REJECTED)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            case UCI_STATUS_FAILED: printf(" %s(FAILED)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            case UCI_STATUS_INVALID_PARAM: printf(" %s(INVALID_PARAM)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
            default: printf(" %s(UNKNOWN)%s\n", ANSI_COLOR_YELLOW, ANSI_RESET); break;
        }
        // Call enhanced analysis for more detailed information
        if (status != UCI_STATUS_OK) {
            enhanced_error_analysis(status);
        }
        printf("    %s%sNumber of TLVs:%s %d\n",
               ANSI_COLOR_BRIGHT_CYAN, ANSI_BOLD, ANSI_RESET, num_tlvs);
    } else {
        printf("    Status: 0x%02X", status);
        switch(status) {
            case UCI_STATUS_OK: printf(" (OK)\n"); break;
            case UCI_STATUS_REJECTED: printf(" (REJECTED)\n"); break;
            case UCI_STATUS_FAILED: printf(" (FAILED)\n"); break;
            case UCI_STATUS_INVALID_PARAM: printf(" (INVALID_PARAM)\n"); break;
            default: printf(" (UNKNOWN)\n"); break;
        }
        // Call enhanced analysis for more detailed information
        if (status != UCI_STATUS_OK) {
            enhanced_error_analysis(status);
        }
        printf("    Number of TLVs: %d\n", num_tlvs);
    }

    // Parse TLV entries (each TLV: config_id (1 byte) + length (1 byte) + value bytes)
    int offset = 2;
    for (int i = 0; i < num_tlvs && offset + 2 <= payload_len; i++) {
        unsigned char cfg_id = payload[offset];
        unsigned char cfg_len = payload[offset + 1];

        if (offset + 2 + cfg_len > payload_len) {
            if (ui_color_enabled) {
                printf("    %s%sWarning: TLV[%d] extends beyond payload%s\n",
                       ANSI_COLOR_YELLOW, ANSI_BOLD, i, ANSI_RESET);
            } else {
                printf("    Warning: TLV[%d] extends beyond payload\n", i);
            }
            break;
        }

        // Get human-readable parameter name
        const char* param_name = uci_config_get_app_param_name((AppConfigTlvType)cfg_id);
        
        if (ui_color_enabled) {
            printf("    %s%sTLV[%d]:%s Config ID=0x%02X", 
                   ANSI_COLOR_BRIGHT_CYAN, ANSI_BOLD, i, ANSI_RESET, cfg_id);
            if (param_name) {
                printf(" (%s)", param_name);
            }
            printf(", Length=%d bytes\n", cfg_len);
            printf("      %s%sValue:%s ", ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_RESET);
        } else {
            printf("    TLV[%d]: Config ID=0x%02X", i, cfg_id);
            if (param_name) {
                printf(" (%s)", param_name);
            }
            printf(", Length=%d bytes\n", cfg_len);
            printf("      Value: ");
        }

        // Print hex value
        for (int j = 0; j < cfg_len; j++) {
            printf("0x%02X ", payload[offset + 2 + j]);
        }
        printf("\n");

        // Try to interpret common parameter values
        if (param_name) {
            if (ui_color_enabled) {
                printf("      %s%sInterpreted:%s ", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET);
            } else {
                printf("      Interpreted: ");
            }
            
            // Handle specific parameter interpretations
            if (strcasecmp(param_name, "device_type") == 0 && cfg_len == 1) {
                unsigned char value = payload[offset + 2];
                if (ui_color_enabled) {
                    switch(value) {
                        case 0x00: printf("%sCONTROLEE%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, value); break;
                        case 0x01: printf("%sCONTROLLER%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, value); break;
                        default: printf("%sUNKNOWN%s (0x%02X)\n", ANSI_COLOR_YELLOW, ANSI_RESET, value); break;
                    }
                } else {
                    switch(value) {
                        case 0x00: printf("CONTROLEE (0x%02X)\n", value); break;
                        case 0x01: printf("CONTROLLER (0x%02X)\n", value); break;
                        default: printf("UNKNOWN (0x%02X)\n", value); break;
                    }
                }
            } else if (strcasecmp(param_name, "device_role") == 0 && cfg_len == 1) {
                unsigned char value = payload[offset + 2];
                if (ui_color_enabled) {
                    switch(value) {
                        case 0x00: printf("%sRESPONDER%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, value); break;
                        case 0x01: printf("%sINITIATOR%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, value); break;
                        case 0x05: printf("%sADVERTISER%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, value); break;
                        case 0x06: printf("%sOBSERVER%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, value); break;
                        case 0x07: printf("%sDT_ANCHOR%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, value); break;
                        case 0x08: printf("%sDT_TAG%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, value); break;
                        default: printf("%sUNKNOWN%s (0x%02X)\n", ANSI_COLOR_YELLOW, ANSI_RESET, value); break;
                    }
                } else {
                    switch(value) {
                        case 0x00: printf("RESPONDER (0x%02X)\n", value); break;
                        case 0x01: printf("INITIATOR (0x%02X)\n", value); break;
                        case 0x05: printf("ADVERTISER (0x%02X)\n", value); break;
                        case 0x06: printf("OBSERVER (0x%02X)\n", value); break;
                        case 0x07: printf("DT_ANCHOR (0x%02X)\n", value); break;
                        case 0x08: printf("DT_TAG (0x%02X)\n", value); break;
                        default: printf("UNKNOWN (0x%02X)\n", value); break;
                    }
                }
            } else if (strcasecmp(param_name, "channel_number") == 0 && cfg_len == 1) {
                unsigned char value = payload[offset + 2];
                if (ui_color_enabled) {
                    switch(value) {
                        case 5: printf("%sChannel 5%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, value); break;
                        case 9: printf("%sChannel 9%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, value); break;
                        default: printf("%sUNKNOWN%s (0x%02X)\n", ANSI_COLOR_YELLOW, ANSI_RESET, value); break;
                    }
                } else {
                    switch(value) {
                        case 5: printf("Channel 5 (0x%02X)\n", value); break;
                        case 9: printf("Channel 9 (0x%02X)\n", value); break;
                        default: printf("UNKNOWN (0x%02X)\n", value); break;
                    }
                }
            } else if (strcasecmp(param_name, "multi_node_mode") == 0 && cfg_len == 1) {
                unsigned char value = payload[offset + 2];
                if (ui_color_enabled) {
                    switch(value) {
                        case 0x00: printf("%sUNICAST%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, value); break;
                        case 0x01: printf("%sONE_TO_MANY%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, value); break;
                        case 0x02: printf("%sMANY_TO_MANY%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, value); break;
                        default: printf("%sUNKNOWN%s (0x%02X)\n", ANSI_COLOR_YELLOW, ANSI_RESET, value); break;
                    }
                } else {
                    switch(value) {
                        case 0x00: printf("UNICAST (0x%02X)\n", value); break;
                        case 0x01: printf("ONE_TO_MANY (0x%02X)\n", value); break;
                        case 0x02: printf("MANY_TO_MANY (0x%02X)\n", value); break;
                        default: printf("UNKNOWN (0x%02X)\n", value); break;
                    }
                }
            } else if (strcasecmp(param_name, "sts_config") == 0 && cfg_len == 1) {
                unsigned char value = payload[offset + 2];
                if (ui_color_enabled) {
                    switch(value) {
                        case 0: printf("%sSTATIC_STS%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, value); break;
                        case 1: printf("%sDYNAMIC_STS%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, value); break;
                        case 2: printf("%sDYNAMIC_STS_RESPONDER_SUBSESSION%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, value); break;
                        case 3: printf("%sPROVISIONED_STS%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, value); break;
                        case 4: printf("%sPROVISIONED_STS_RESPONDER_SUBSESSION%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, value); break;
                        default: printf("%sUNKNOWN%s (0x%02X)\n", ANSI_COLOR_YELLOW, ANSI_RESET, value); break;
                    }
                } else {
                    switch(value) {
                        case 0: printf("STATIC_STS (0x%02X)\n", value); break;
                        case 1: printf("DYNAMIC_STS (0x%02X)\n", value); break;
                        case 2: printf("DYNAMIC_STS_RESPONDER_SUBSESSION (0x%02X)\n", value); break;
                        case 3: printf("PROVISIONED_STS (0x%02X)\n", value); break;
                        case 4: printf("PROVISIONED_STS_RESPONDER_SUBSESSION (0x%02X)\n", value); break;
                        default: printf("UNKNOWN (0x%02X)\n", value); break;
                    }
                }
            } else if (strcasecmp(param_name, "aoa_result_req") == 0 && cfg_len == 1) {
                unsigned char value = payload[offset + 2];
                if (ui_color_enabled) {
                    switch(value) {
                        case 0: printf("%sNO_AOA_REPORT%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, value); break;
                        case 1: printf("%sAOA_ELEVATION%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, value); break;
                        case 2: printf("%sAOA_AZIMUTH%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, value); break;
                        case 3: printf("%sAOA_ELEVATION_AND_AZIMUTH%s (0x%02X)\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, value); break;
                        default: printf("%sUNKNOWN%s (0x%02X)\n", ANSI_COLOR_YELLOW, ANSI_RESET, value); break;
                    }
                } else {
                    switch(value) {
                        case 0: printf("NO_AOA_REPORT (0x%02X)\n", value); break;
                        case 1: printf("AOA_ELEVATION (0x%02X)\n", value); break;
                        case 2: printf("AOA_AZIMUTH (0x%02X)\n", value); break;
                        case 3: printf("AOA_ELEVATION_AND_AZIMUTH (0x%02X)\n", value); break;
                        default: printf("UNKNOWN (0x%02X)\n", value); break;
                    }
                }
            } else if (cfg_len <= 8) {
                // For numeric values, try to display as integer
                uint64_t value = 0;
                for (int j = 0; j < cfg_len && j < 8; j++) {
                    value |= ((uint64_t)payload[offset + 2 + j]) << (j * 8);
                }
                
                // Get parameter range information
                uint64_t min_val, max_val;
                if (uci_config_get_app_param_range((AppConfigTlvType)cfg_id, &min_val, &max_val) == 0) {
                    if (ui_color_enabled) {
                        if (value >= min_val && value <= max_val) {
                            printf("%s%" PRIu64 "%s (0x%02X", ANSI_COLOR_BRIGHT_GREEN, value, ANSI_RESET, payload[offset + 2]);
                            for (int j = 1; j < cfg_len; j++) {
                                printf(" %02X", payload[offset + 2 + j]);
                            }
                            printf(") [Range: %" PRIu64 "-%" PRIu64 "]\n", min_val, max_val);
                        } else {
                            printf("%s%" PRIu64 "%s (0x%02X", ANSI_COLOR_YELLOW, value, ANSI_RESET, payload[offset + 2]);
                            for (int j = 1; j < cfg_len; j++) {
                                printf(" %02X", payload[offset + 2 + j]);
                            }
                            printf(") [OUT OF RANGE: %" PRIu64 "-%" PRIu64 "]\n", min_val, max_val);
                        }
                    } else {
                        if (value >= min_val && value <= max_val) {
                            printf("%" PRIu64 " (0x%02X", value, payload[offset + 2]);
                            for (int j = 1; j < cfg_len; j++) {
                                printf(" %02X", payload[offset + 2 + j]);
                            }
                            printf(") [Range: %" PRIu64 "-%" PRIu64 "]\n", min_val, max_val);
                        } else {
                            printf("%" PRIu64 " (0x%02X", value, payload[offset + 2]);
                            for (int j = 1; j < cfg_len; j++) {
                                printf(" %02X", payload[offset + 2 + j]);
                            }
                            printf(") [OUT OF RANGE: %" PRIu64 "-%" PRIu64 "]\n", min_val, max_val);
                        }
                    }
                } else {
                    if (ui_color_enabled) {
                        printf("%s%" PRIu64 "%s (0x%02X", ANSI_COLOR_BRIGHT_WHITE, value, ANSI_RESET, payload[offset + 2]);
                        for (int j = 1; j < cfg_len; j++) {
                            printf(" %02X", payload[offset + 2 + j]);
                        }
                        printf(")\n");
                    } else {
                        printf("%" PRIu64 " (0x%02X", value, payload[offset + 2]);
                        for (int j = 1; j < cfg_len; j++) {
                            printf(" %02X", payload[offset + 2 + j]);
                        }
                        printf(")\n");
                    }
                }
            } else {
                // For longer values, just show hex
                if (ui_color_enabled) {
                    printf("%s", ANSI_COLOR_BRIGHT_WHITE);
                }
                for (int j = 0; j < cfg_len; j++) {
                    printf("%02X ", payload[offset + 2 + j]);
                }
                if (ui_color_enabled) {
                    printf("%s\n", ANSI_RESET);
                } else {
                    printf("\n");
                }
            }
        }

        offset += 2 + cfg_len;
    }
}
