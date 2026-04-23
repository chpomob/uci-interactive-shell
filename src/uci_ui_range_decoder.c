#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <strings.h>
#include <inttypes.h>
#include <stdarg.h>
#include "../include/uci.h"
#include "../include/uci_ui.h"
#include "../include/uci_ui_packet_decoder.h"
#include "../include/uci_packet_analyzer.h"
#include "../include/uci_packet_utils.h"
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
