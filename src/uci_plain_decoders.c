#include <stdint.h>
#include <stdio.h>

#include "../include/uci.h"
#include "../include/uci_config_manager.h"
#include "../include/uci_decode_utils.h"
#include "../include/uci_packet_utils.h"
#include "../include/uci_plain_decoders_internal.h"

static double q8_8_to_double(int16_t raw) {
    return (double)raw / 256.0;
}

static double q6_9_to_double(int16_t raw) {
    return (double)raw / 512.0;
}

static inline uint32_t read_u32_be(const unsigned char* buffer) {
    return ((uint32_t)buffer[0] << 24) |
           ((uint32_t)buffer[1] << 16) |
           ((uint32_t)buffer[2] << 8) |
           ((uint32_t)buffer[3]);
}

void print_short_address_measurement(const unsigned char* data) {
    unsigned short mac_address = read_u16_le(&data[0]);
    unsigned char status = data[2];
    unsigned char nlos = data[3];
    unsigned short distance = read_u16_le(&data[4]);
    unsigned short aoa_azimuth = read_u16_le(&data[6]);
    unsigned char aoa_azimuth_fom = data[8];
    unsigned short aoa_elevation = read_u16_le(&data[9]);
    unsigned char aoa_elevation_fom = data[11];
    unsigned short aoa_destination_azimuth = read_u16_le(&data[12]);
    unsigned char aoa_destination_azimuth_fom = data[14];
    unsigned short aoa_destination_elevation = read_u16_le(&data[15]);
    unsigned char aoa_destination_elevation_fom = data[17];
    unsigned char slot_index = data[18];
    unsigned char rssi = data[19];

    printf("      MAC Address: 0x%04X\n", mac_address);
    uci_print_status_line("Status", status);
    printf("      NLOS: %s\n", nlos ? "YES" : "NO");
    printf("      Distance: %u cm\n", distance);
    printf("      AoA Azimuth: %u degrees (FoM: %u)\n", aoa_azimuth, aoa_azimuth_fom);
    printf("      AoA Elevation: %u degrees (FoM: %u)\n", aoa_elevation, aoa_elevation_fom);
    printf("      Destination AoA Azimuth: %u degrees (FoM: %u)\n", aoa_destination_azimuth, aoa_destination_azimuth_fom);
    printf("      Destination AoA Elevation: %u degrees (FoM: %u)\n", aoa_destination_elevation, aoa_destination_elevation_fom);
    printf("      Slot Index: %u\n", slot_index);
    printf("      RSSI: %d dBm\n", (int8_t)rssi);
}

void print_extended_address_measurement(const unsigned char* data) {
    uint64_t mac_address = read_u64_le(&data[0]);
    unsigned char status = data[8];
    unsigned char nlos = data[9];
    unsigned short distance = read_u16_le(&data[10]);
    unsigned short aoa_azimuth = read_u16_le(&data[12]);
    unsigned char aoa_azimuth_fom = data[14];
    unsigned short aoa_elevation = read_u16_le(&data[15]);
    unsigned char aoa_elevation_fom = data[17];
    unsigned short aoa_destination_azimuth = read_u16_le(&data[18]);
    unsigned char aoa_destination_azimuth_fom = data[20];
    unsigned short aoa_destination_elevation = read_u16_le(&data[21]);
    unsigned char aoa_destination_elevation_fom = data[23];
    unsigned char slot_index = data[24];
    unsigned char rssi = data[25];

    printf("      MAC Address: 0x%016llX\n", (unsigned long long)mac_address);
    uci_print_status_line("Status", status);
    printf("      NLOS: %s\n", nlos ? "YES" : "NO");
    printf("      Distance: %u cm\n", distance);
    printf("      AoA Azimuth: %u degrees (FoM: %u)\n", aoa_azimuth, aoa_azimuth_fom);
    printf("      AoA Elevation: %u degrees (FoM: %u)\n", aoa_elevation, aoa_elevation_fom);
    printf("      Destination AoA Azimuth: %u degrees (FoM: %u)\n", aoa_destination_azimuth, aoa_destination_azimuth_fom);
    printf("      Destination AoA Elevation: %u degrees (FoM: %u)\n", aoa_destination_elevation, aoa_destination_elevation_fom);
    printf("      Slot Index: %u\n", slot_index);
    printf("      RSSI: %d dBm\n", (int8_t)rssi);
}

void decode_range_vendor_data(const unsigned char* data, int length) {
    if (!data || length <= 0) {
        return;
    }

    int offset = 0;
    while (offset + 6 <= length) {
        unsigned char field_id = data[offset++];
        unsigned char sub_id = data[offset++];
        unsigned int value = read_u32_be(&data[offset]);
        offset += 4;

        printf("    Vendor Field 0x%02X/0x%02X: 0x%08X", field_id, sub_id, value);
        if (field_id == 0x01 && sub_id == 0x01) {
            printf(" (Distance: %u cm)", value);
        }
        printf("\n");

        /* Remaining vendor bytes after the first known TLV are often zeroed out. */
        int remaining = length - offset;
        int all_zero = 1;
        for (int i = 0; i < remaining; i++) {
            if (data[offset + i] != 0x00) {
                all_zero = 0;
                break;
            }
        }
        if (all_zero) {
            return;
        }
    }

    if (offset < length) {
        printf("    Unparsed Vendor Data (%d bytes): ", length - offset);
        for (int i = offset; i < length; i++) {
            printf("%02X ", data[i]);
        }
        printf("\n");
    }
}

static const char* get_frame_report_tlv_name(FrameReportTlvType tlv_type) {
    switch (tlv_type) {
        case FRAME_REPORT_TLV_RSSI: return "RSSI";
        case FRAME_REPORT_TLV_AOA: return "AOA";
        case FRAME_REPORT_TLV_CIR: return "CIR";
        case FRAME_REPORT_TLV_SEGMENT_METRICS: return "SEGMENT_METRICS";
        default: return NULL;
    }
}

static const char* get_segment_id_name(unsigned char segment_id) {
    switch (segment_id) {
        case 0: return "IPATOV";
        case 1: return "STS0";
        case 2: return "STS1";
        case 3: return "STS2";
        case 4: return "STS3";
        default: return "UNKNOWN";
    }
}

static void decode_frame_report_tlv(FrameReportTlvType tlv_type,
                                    const unsigned char* value,
                                    unsigned short length) {
    const char* name = get_frame_report_tlv_name(tlv_type);
    printf("        TLV Type: 0x%02X", tlv_type);
    if (name) {
        printf(" (%s)", name);
    } else {
        printf(" (UNKNOWN)");
    }
    printf(", Length: %u\n", length);

    if (!value || length == 0) {
        printf("          No data.\n");
        return;
    }

    switch (tlv_type) {
        case FRAME_REPORT_TLV_RSSI: {
            printf("          RSSI Samples (dBm): ");
            for (unsigned short i = 0; i < length; i++) {
                printf("%d ", (int8_t)value[i]);
            }
            printf("\n");
            break;
        }
        case FRAME_REPORT_TLV_AOA: {
            if (length % 8 != 0) {
                printf("          WARNING: AOA payload length %u is not a multiple of 8 bytes.\n", length);
            }
            unsigned short offset = 0;
            unsigned int measurement_idx = 0;
            while (offset + 8 <= length) {
                int16_t tdoa = (int16_t)read_u16_le(&value[offset]);
                int16_t pdoa = (int16_t)read_u16_le(&value[offset + 2]);
                int16_t aoa = (int16_t)read_u16_le(&value[offset + 4]);
                unsigned char fom = value[offset + 6];
                unsigned char meas_type = value[offset + 7];
                printf("          Measurement %u: TDOA=%d, PDOA=%d, AoA=%d, FoM=%u, Type=0x%02X\n",
                       measurement_idx++, tdoa, pdoa, aoa, fom, meas_type);
                offset += 8;
            }
            if (offset < length) {
                printf("          %u trailing bytes remain unparsed.\n", (unsigned int)(length - offset));
            }
            break;
        }
        case FRAME_REPORT_TLV_CIR: {
            unsigned short offset = 0;
            if (length < 1) {
                printf("          WARNING: CIR payload too short.\n");
                break;
            }
            unsigned char cir_entries = value[offset++];
            printf("          CIR Entries: %u\n", cir_entries);
            for (unsigned char entry = 0; entry < cir_entries; entry++) {
                if (offset + 16 > length) {
                    printf("          WARNING: CIR entry %u truncated (need at least 16 bytes, have %u).\n",
                           entry, (unsigned int)(length - offset));
                    break;
                }
                unsigned short first_path_index = read_u16_le(&value[offset]);
                unsigned short first_path_snr = read_u16_le(&value[offset + 2]);
                unsigned short first_path_ns = read_u16_le(&value[offset + 4]);
                unsigned short peak_path_index = read_u16_le(&value[offset + 6]);
                unsigned short peak_path_snr = read_u16_le(&value[offset + 8]);
                unsigned short peak_path_ns = read_u16_le(&value[offset + 10]);
                unsigned char first_path_sample_offset = value[offset + 12];
                unsigned char samples_number = value[offset + 13];
                unsigned short sample_window_len = read_u16_le(&value[offset + 14]);
                offset += 16;

                printf("          CIR %u: first_path_index=%u, first_path_snr=%u, first_path_ns=%u\n",
                       entry, first_path_index, first_path_snr, first_path_ns);
                printf("                   peak_path_index=%u, peak_path_snr=%u, peak_path_ns=%u\n",
                       peak_path_index, peak_path_snr, peak_path_ns);
                printf("                   first_path_sample_offset=%u, samples_number=%u, sample_window_len=%u\n",
                       first_path_sample_offset, samples_number, sample_window_len);

                if (offset + sample_window_len > length) {
                    printf("          WARNING: CIR sample window truncated (expected %u bytes, have %u).\n",
                           sample_window_len, (unsigned int)(length - offset));
                    break;
                }

                unsigned short preview = sample_window_len < 16 ? sample_window_len : 16;
                printf("                   Sample window preview: ");
                for (unsigned short i = 0; i < preview; i++) {
                    printf("%02X ", value[offset + i]);
                }
                if (sample_window_len > preview) {
                    printf("... (+%u bytes)", sample_window_len - preview);
                }
                printf("\n");

                offset += sample_window_len;
            }
            if (offset < length) {
                printf("          %u trailing bytes remain after CIR parsing.\n", (unsigned int)(length - offset));
            }
            break;
        }
        case FRAME_REPORT_TLV_SEGMENT_METRICS: {
            unsigned short offset = 0;
            unsigned int metric_index = 0;
            const unsigned short entry_size = 17;

            while (offset + entry_size <= length) {
                unsigned char ras = value[offset++];
                unsigned char segment_id = ras & 0x07;
                unsigned char receiver_is_controller = (ras >> 3) & 0x01;
                unsigned char receiver_id = (ras >> 4) & 0x0F;

                int16_t rf_noise_floor_raw = (int16_t)read_u16_le(&value[offset]);
                offset += 2;
                int16_t segment_rsl_raw = (int16_t)read_u16_le(&value[offset]);
                offset += 2;

                uint16_t first_path_index = read_u16_le(&value[offset]);
                offset += 2;
                int16_t first_path_rsl_raw = (int16_t)read_u16_le(&value[offset]);
                offset += 2;
                int16_t first_path_time_raw = (int16_t)read_u16_le(&value[offset]);
                offset += 2;

                uint16_t peak_path_index = read_u16_le(&value[offset]);
                offset += 2;
                int16_t peak_path_rsl_raw = (int16_t)read_u16_le(&value[offset]);
                offset += 2;
                int16_t peak_path_time_raw = (int16_t)read_u16_le(&value[offset]);
                offset += 2;

                printf("          Segment Metrics %u:\n", metric_index++);
                printf("            Segment: %s (ID=%u)\n", get_segment_id_name(segment_id), segment_id);
                printf("            Receiver: %s (ID=%u)\n",
                       receiver_is_controller ? "Controller" : "Controlee",
                       receiver_id);
                printf("            RF Noise Floor: %.2f dBm (raw 0x%04X)\n",
                       q8_8_to_double(rf_noise_floor_raw), (unsigned int)(uint16_t)rf_noise_floor_raw);
                printf("            Segment RSL: %.2f dBm (raw 0x%04X)\n",
                       q8_8_to_double(segment_rsl_raw), (unsigned int)(uint16_t)segment_rsl_raw);
                printf("            First Path -> index=%u, RSL=%.2f dBm, time=%.3f ns\n",
                       first_path_index,
                       q8_8_to_double(first_path_rsl_raw),
                       q6_9_to_double(first_path_time_raw));
                printf("            Peak  Path -> index=%u, RSL=%.2f dBm, time=%.3f ns\n",
                       peak_path_index,
                       q8_8_to_double(peak_path_rsl_raw),
                       q6_9_to_double(peak_path_time_raw));
            }

            if (offset < length) {
                printf("          WARNING: %u trailing bytes remain after parsing segment metrics.\n",
                       (unsigned int)(length - offset));
            }
            break;
        }
        default: {
            printf("          Raw bytes: ");
            unsigned short preview = length < 32 ? length : 32;
            for (unsigned short i = 0; i < preview; i++) {
                printf("%02X ", value[i]);
            }
            if (length > preview) {
                printf("... (+%u bytes)", length - preview);
            }
            printf("\n");
            break;
        }
    }
}

void decode_session_data_credit_ntf(unsigned char* payload, int payload_len) {
    printf("    SESSION_DATA_CREDIT_NTF - Data Credit Notification\n");

    if (payload_len < 5) {
        printf("      ERROR: Payload too short (%d bytes, need at least 5)\n", payload_len);
        return;
    }

    unsigned int session_token = read_u32_le(payload);
    unsigned char credit_availability = payload[4];

    printf("      Session Token: 0x%08X\n", session_token);
    printf("      Credit Availability: 0x%02X", credit_availability);
    if (credit_availability == 0x00) {
        printf(" (NOT_AVAILABLE)\n");
    } else {
        printf(" (AVAILABLE)\n");
    }
}

void decode_session_data_transfer_status_ntf(unsigned char* payload, int payload_len) {
    printf("    SESSION_DATA_TRANSFER_STATUS_NTF - Data Transfer Status Notification\n");

    if (payload_len < 6) {
        printf("      ERROR: Payload too short (%d bytes, need at least 6)\n", payload_len);
        return;
    }

    unsigned int session_token = read_u32_le(payload);
    unsigned short uci_sequence_number = read_u16_le(&payload[4]);

    printf("      Session Token: 0x%08X\n", session_token);
    printf("      UCI Sequence Number: %u\n", uci_sequence_number);

    if (payload_len >= 8) {
        unsigned char status = payload[6];
        unsigned char tx_count = payload[7];

        uci_print_data_transfer_status_line("Status", status);

        printf("      TX Count: %d\n", tx_count);
    }

    if (payload_len > 8) {
        printf("      Additional Data (%d bytes): ", payload_len - 8);
        for (int i = 8; i < payload_len; i++) {
            printf("%02X ", payload[i]);
        }
        printf("\n");
    }
}

void decode_core_device_info_rsp(unsigned char* payload, int payload_len) {
    printf("    CORE_DEVICE_INFO_RSP - Device Information Response\n");

    if (payload_len < 9) {
        printf("      ERROR: Payload too short (%d bytes, need at least 9)\n", payload_len);
        return;
    }

    unsigned char status = payload[0];
    unsigned short uci_version = read_u16_le(&payload[1]);
    unsigned short mac_version = read_u16_le(&payload[3]);
    unsigned short phy_version = read_u16_le(&payload[5]);
    unsigned short uci_test_version = read_u16_le(&payload[7]);

    uci_print_status_line("Status", status);

    printf("      UCI Version: 0x%04X\n", uci_version);
    printf("      MAC Version: 0x%04X\n", mac_version);
    printf("      PHY Version: 0x%04X\n", phy_version);
    printf("      UCI Test Version: 0x%04X\n", uci_test_version);

    if (payload_len > 9) {
        printf("      Vendor Specific Info (%d bytes): ", payload_len - 9);
        for (int i = 9; i < payload_len; i++) {
            printf("%02X ", payload[i]);
        }
        printf("\n");
    }
}

void decode_core_get_caps_info_rsp(unsigned char* payload, int payload_len) {
    printf("    CORE_GET_CAPS_INFO_RSP - Get Capabilities Information Response\n");

    if (payload_len < 2) {
        printf("      ERROR: Payload too short (%d bytes, need at least 2)\n", payload_len);
        return;
    }

    unsigned char status = payload[0];
    unsigned char num_tlvs = payload[1];

    uci_print_status_line("Status", status);

    printf("      Number of TLVs: %d\n", num_tlvs);

    if (num_tlvs > 0 && payload_len >= 2) {
        int offset = 2;
        for (int i = 0; i < num_tlvs && offset + 3 <= payload_len; i++) {
            CapTlvType tlv_type = (CapTlvType)payload[offset];
            unsigned char tlv_len = payload[offset + 1];
            offset += 2;

            printf("      TLV %d:\n", i);
            printf("        Type: 0x%02X", tlv_type);
            switch (tlv_type) {
                case SUPPORTED_V1_FIRA_PHY_VERSION_RANGE_V2_MAX_MESSAGE_SIZE: printf(" (PHY_VERSION_RANGE)\n"); break;
                case SUPPORTED_V1_FIRA_MAC_VERSION_RANGE_V2_MAX_DATA_PAYLOAD_SIZE: printf(" (MAC_VERSION_RANGE)\n"); break;
                case SUPPORTED_V1_DEVICE_ROLES_V2_FIRA_PHY_VERSION_RANGE: printf(" (DEVICE_ROLES)\n"); break;
                case SUPPORTED_V1_RANGING_METHOD_V2_FIRA_MAC_VERSION_RANGE: printf(" (RANGING_METHOD)\n"); break;
                case SUPPORTED_V1_STS_CONFIG_V2_DEVICE_TYPE: printf(" (STS_CONFIG)\n"); break;
                case SUPPORTED_V1_MULTI_NODE_MODES_V2_DEVICE_ROLES: printf(" (MULTI_NODE_MODES)\n"); break;
                case SUPPORTED_V1_RANGING_TIME_STRUCT_V2_RANGING_METHOD: printf(" (RANGING_TIME_STRUCT)\n"); break;
                case SUPPORTED_V1_SCHEDULED_MODE_V2_STS_CONFIG: printf(" (SCHEDULED_MODE)\n"); break;
                case SUPPORTED_V1_HOPPING_MODE_V2_MULTI_NODE_MODE: printf(" (HOPPING_MODE)\n"); break;
                case SUPPORTED_V1_BLOCK_STRIDING_V2_RANGING_TIME_STRUCT: printf(" (BLOCK_STRIDING)\n"); break;
                case SUPPORTED_V1_UWB_INITIATION_TIME_V2_SCHEDULE_MODE: printf(" (UWB_INITIATION_TIME)\n"); break;
                case SUPPORTED_V1_CHANNELS_V2_HOPPING_MODE: printf(" (CHANNELS)\n"); break;
                case SUPPORTED_V1_RFRAME_CONFIG_V2_BLOCK_STRIDING: printf(" (RFRAME_CONFIG)\n"); break;
                case SUPPORTED_V1_CC_CONSTRAINT_LENGTH_V2_UWB_INITIATION_TIME: printf(" (CC_CONSTRAINT_LENGTH)\n"); break;
                case SUPPORTED_V1_BPRF_PARAMETER_SETS_V2_CHANNELS: printf(" (BPRF_PARAMETER_SETS)\n"); break;
                case SUPPORTED_V1_HPRF_PARAMETER_SETS_V2_RFRAME_CONFIG: printf(" (HPRF_PARAMETER_SETS)\n"); break;
                case SUPPORTED_V1_AOA_V2_AOA_SUPPORT: printf(" (AOA)\n"); break;
                case SUPPORTED_V1_EXTENDED_MAC_ADDRESS_V2_EXTENDED_MAC_ADDRESS: printf(" (EXTENDED_MAC_ADDRESS)\n"); break;
                case SUPPORTED_V1_MAX_MESSAGE_SIZE_V2_ASSIGNED: printf(" (MAX_MESSAGE_SIZE)\n"); break;
                case SUPPORTED_V1_MAX_DATA_PACKET_PAYLOAD_SIZE_V2_SESSION_KEY_LENGTH: printf(" (MAX_DATA_PACKET_PAYLOAD_SIZE)\n"); break;
                case SUPPORTED_V2_EXTENDED_MAC_ADDRESS: printf(" (V2_EXTENDED_MAC_ADDRESS)\n"); break;
                case SUPPORTED_V2_ASSIGNED: printf(" (V2_ASSIGNED)\n"); break;
                case SUPPORTED_V2_SESSION_KEY_LENGTH: printf(" (V2_SESSION_KEY_LENGTH)\n"); break;
                case SUPPORTED_V2_DT_ANCHOR_MAX_ACTIVE_RR: printf(" (DT_ANCHOR_MAX_ACTIVE_RR)\n"); break;
                case SUPPORTED_V2_DT_TAG_MAX_ACTIVE_RR: printf(" (DT_TAG_MAX_ACTIVE_RR)\n"); break;
                case SUPPORTED_V2_DT_TAG_BLOCK_SHIPPING: printf(" (DT_TAG_BLOCK_SHIPPING)\n"); break;
                case SUPPORTED_V2_PSDU_LENGTH_SUPPORT: printf(" (PSDU_LENGTH_SUPPORT)\n"); break;
                case CCC_SUPPORTED_CHAPS_PER_SLOT: printf(" (CCC_CHAPS_PER_SLOT)\n"); break;
                case CCC_SUPPORTED_SYNC_CODES: printf(" (CCC_SYNC_CODES)\n"); break;
                case CCC_SUPPORTED_HOPPING_CONFIG_MODES_AND_SEQUENCES: printf(" (CCC_HOPPING_CONFIGS)\n"); break;
                case CCC_SUPPORTED_CHANNELS: printf(" (CCC_CHANNELS)\n"); break;
                case CCC_SUPPORTED_VERSIONS: printf(" (CCC_VERSIONS)\n"); break;
                case CCC_SUPPORTED_UWB_CONFIGS: printf(" (CCC_UWB_CONFIGS)\n"); break;
                case CCC_SUPPORTED_PULSE_SHAPE_COMBOS: printf(" (CCC_PULSE_SHAPES)\n"); break;
                case CCC_SUPPORTED_RAN_MULTIPLIER: printf(" (CCC_RAN_MULTIPLIER)\n"); break;
                case CCC_SUPPORTED_MAX_RANGING_SESSION_NUMBER: printf(" (CCC_MAX_RANGING_SESSIONS)\n"); break;
                case CCC_SUPPORTED_MIN_UWB_INITIATION_TIME_MS: printf(" (CCC_MIN_INIT_TIME_MS)\n"); break;
                case CCC_PRIORITIZED_CHANNEL_LIST: printf(" (CCC_PRIORITY_CHANNELS)\n"); break;
                case CCC_SUPPORTED_UWBS_MAX_PPM: printf(" (CCC_MAX_PPM)\n"); break;
                case ALIRO_SUPPORTED_MAC_MODES: printf(" (ALIRO_MAC_MODES)\n"); break;
                case RADAR_SUPPORT: printf(" (RADAR_SUPPORT)\n"); break;
                case SUPPORTED_POWER_STATS: printf(" (POWER_STATS)\n"); break;
                case SUPPORTED_AOA_RESULT_REQ_ANTENNA_INTERLEAVING: printf(" (AOA_REQ_ANTENNA_INTERLEAVE)\n"); break;
                case SUPPORTED_MIN_RANGING_INTERVAL_MS: printf(" (MIN_RANGING_INTERVAL_MS)\n"); break;
                case SUPPORTED_RANGE_DATA_NTF_CONFIG: printf(" (RANGE_DATA_NTF_CONFIG)\n"); break;
                case SUPPORTED_RSSI_REPORTING: printf(" (RSSI_REPORTING)\n"); break;
                case SUPPORTED_DIAGNOSTICS: printf(" (DIAGNOSTICS_SUPPORT)\n"); break;
                case SUPPORTED_MIN_SLOT_DURATION_RSTU: printf(" (MIN_SLOT_DURATION_RSTU)\n"); break;
                case SUPPORTED_MAX_RANGING_SESSION_NUMBER: printf(" (MAX_RANGING_SESSIONS)\n"); break;
                default: printf(" (UNKNOWN)\n"); break;
            }

            printf("        Length: %d\n", tlv_len);

            if (offset + tlv_len <= payload_len) {
                printf("        Value: ");
                for (int j = 0; j < tlv_len; j++) {
                    printf("%02X ", payload[offset + j]);
                }
                printf("\n");
            } else {
                printf("        Value: TRUNCATED (expected %d bytes, only %d available)\n", tlv_len, payload_len - offset);
            }

            offset += tlv_len;
        }
    }
}

void decode_core_set_config_rsp(unsigned char* payload, int payload_len) {
    printf("    CORE_SET_CONFIG_RSP - Set Configuration Response\n");

    if (payload_len < 2) {
        printf("      ERROR: Payload too short (%d bytes, need at least 2)\n", payload_len);
        return;
    }

    unsigned char status = payload[0];
    unsigned char num_configs = payload[1];

    uci_print_status_line("Status", status);

    printf("      Number of Config Status: %d\n", num_configs);

    if (num_configs > 0 && payload_len >= 2) {
        int offset = 2;
        for (int i = 0; i < num_configs && offset + 2 <= payload_len; i++) {
            DeviceConfigId cfg_id = (DeviceConfigId)payload[offset];
            unsigned char cfg_status = payload[offset + 1];
            offset += 2;

            printf("      Config %d:\n", i);
            printf("        Config ID: 0x%02X", cfg_id);
            switch (cfg_id) {
                case DEVICE_STATE: printf(" (DEVICE_STATE)\n"); break;
                case LOW_POWER_MODE: printf(" (LOW_POWER_MODE)\n"); break;
                default: printf(" (UNKNOWN)\n"); break;
            }

            uci_print_status_line("Status", cfg_status);
        }
    }
}

void decode_core_get_config_rsp(unsigned char* payload, int payload_len) {
    printf("    CORE_GET_CONFIG_RSP - Get Configuration Response\n");

    if (payload_len < 2) {
        printf("      ERROR: Payload too short (%d bytes, need at least 2)\n", payload_len);
        return;
    }

    unsigned char status = payload[0];
    unsigned char num_tlvs = payload[1];

    uci_print_status_line("Status", status);

    printf("      Number of TLVs: %d\n", num_tlvs);

    if (num_tlvs > 0 && payload_len >= 2) {
        int offset = 2;
        for (int i = 0; i < num_tlvs && offset + 3 <= payload_len; i++) {
            DeviceConfigId cfg_id = (DeviceConfigId)payload[offset];
            unsigned char cfg_len = payload[offset + 1];
            offset += 2;

            printf("      TLV %d:\n", i);
            printf("        Config ID: 0x%02X", cfg_id);
            switch (cfg_id) {
                case DEVICE_STATE: printf(" (DEVICE_STATE)\n"); break;
                case LOW_POWER_MODE: printf(" (LOW_POWER_MODE)\n"); break;
                default: printf(" (UNKNOWN)\n"); break;
            }

            printf("        Length: %d\n", cfg_len);

            if (offset + cfg_len <= payload_len) {
                printf("        Value: ");
                for (int j = 0; j < cfg_len; j++) {
                    printf("%02X ", payload[offset + j]);
                }
                printf("\n");

                if (cfg_id == DEVICE_STATE && cfg_len == 1) {
                    uci_print_device_state_line("Interpreted as Device State", payload[offset]);
                } else if (cfg_id == LOW_POWER_MODE && cfg_len == 1) {
                    unsigned char lpm = payload[offset];
                    printf("          Interpreted as Low Power Mode: %s (0x%02X)\n",
                           lpm ? "ON" : "OFF", lpm);
                }
            } else {
                printf("        Value: TRUNCATED (expected %d bytes, only %d available)\n", cfg_len, payload_len - offset);
            }

            offset += cfg_len;
        }
    }
}

void decode_core_device_reset_rsp(unsigned char* payload, int payload_len) {
    printf("    CORE_DEVICE_RESET_RSP - Device Reset Response\n");

    if (payload_len < 1) {
        printf("      ERROR: Payload too short (%d bytes, need at least 1)\n", payload_len);
        return;
    }

    uci_print_status_line("Status", payload[0]);
}

void decode_core_device_suspend_rsp(unsigned char* payload, int payload_len) {
    printf("    CORE_DEVICE_SUSPEND_RSP - Device Suspend Response\n");

    if (payload_len < 1) {
        printf("      ERROR: Payload too short (%d bytes, need at least 1)\n", payload_len);
        return;
    }

    uci_print_status_line("Status", payload[0]);
}

void decode_core_query_uwbs_timestamp_rsp(unsigned char* payload, int payload_len) {
    printf("    CORE_QUERY_UWBS_TIMESTAMP_RSP - Query UWBS Timestamp Response\n");

    if (payload_len < 9) {
        printf("      ERROR: Payload too short (%d bytes, need at least 9)\n", payload_len);
        return;
    }

    unsigned char status = payload[0];
    unsigned long long timestamp = read_u64_le(&payload[1]);

    uci_print_status_line("Status", status);
    if (status == UCI_STATUS_OK) {
        printf("      Timestamp: %llu\n", timestamp);
    }
}

void decode_session_init_cmd(unsigned char* payload, int payload_len) {
    printf("    SESSION_INIT_CMD - Session Initialization Command\n");

    if (payload_len < 5) {
        printf("      ERROR: Payload too short (%d bytes, need at least 5)\n", payload_len);
        return;
    }

    unsigned int session_id = read_u32_le(&payload[0]);
    unsigned char session_type = payload[4];
    const char* session_type_name = uci_session_type_to_string(session_type);

    printf("      Session ID: 0x%08X\n", session_id);
    printf("      Session Type: 0x%02X (%s)\n", session_type, session_type_name);
}

void decode_session_init_rsp(unsigned char* payload, int payload_len) {
    printf("    SESSION_INIT_RSP - Session Initialization Response\n");

    if (payload_len < 5) {
        printf("      ERROR: Payload too short (%d bytes, need at least 5)\n", payload_len);
        return;
    }

    unsigned char status = payload[0];
    unsigned int session_handle = read_u32_le(&payload[1]);

    uci_print_status_line("Status", status);

    printf("      Session Handle: 0x%08X\n", session_handle);
}

void decode_session_deinit_rsp(unsigned char* payload, int payload_len) {
    printf("    SESSION_DEINIT_RSP - Session Deinitialization Response\n");

    if (payload_len < 1) {
        printf("      ERROR: Payload too short (%d bytes, need at least 1)\n", payload_len);
        return;
    }

    uci_print_status_line("Status", payload[0]);
}

void decode_session_set_app_config_rsp(unsigned char* payload, int payload_len) {
    printf("    SESSION_SET_APP_CONFIG_RSP - Set Application Configuration Response\n");

    if (payload_len < 2) {
        printf("      ERROR: Payload too short (%d bytes, need at least 2)\n", payload_len);
        return;
    }

    unsigned char status = payload[0];
    unsigned char num_configs = payload[1];

    uci_print_status_line("Status", status);

    printf("      Number of Config Status: %d\n", num_configs);

    if (num_configs > 0 && payload_len >= 2) {
        int offset = 2;
        for (int i = 0; i < num_configs && offset + 2 <= payload_len; i++) {
            AppConfigTlvType cfg_id = (AppConfigTlvType)payload[offset];
            unsigned char cfg_status = payload[offset + 1];
            const char* cfg_name = uci_config_get_app_param_name(cfg_id);
            offset += 2;

            printf("      Config %d:\n", i);
            printf("        Config ID: 0x%02X", cfg_id);
            printf(" (%s)\n", cfg_name ? cfg_name : "UNKNOWN");

            uci_print_status_line("Status", cfg_status);
        }
    }
}

void decode_session_get_app_config_rsp(unsigned char* payload, int payload_len) {
    printf("    SESSION_GET_APP_CONFIG_RSP - Get Application Configuration Response\n");

    if (payload_len < 2) {
        printf("      ERROR: Payload too short (%d bytes, need at least 2)\n", payload_len);
        return;
    }

    unsigned char status = payload[0];
    unsigned char num_tlvs = payload[1];

    uci_print_status_line("Status", status);

    printf("      Number of TLVs: %d\n", num_tlvs);

    if (num_tlvs > 0 && payload_len >= 2) {
        int offset = 2;
        for (int i = 0; i < num_tlvs && offset + 3 <= payload_len; i++) {
            AppConfigTlvType cfg_id = (AppConfigTlvType)payload[offset];
            unsigned char cfg_len = payload[offset + 1];
            const char* cfg_name = uci_config_get_app_param_name(cfg_id);
            offset += 2;

            printf("      TLV %d:\n", i);
            printf("        Config ID: 0x%02X", cfg_id);
            printf(" (%s)\n", cfg_name ? cfg_name : "UNKNOWN");
            printf("        Length: %d\n", cfg_len);

            if (offset + cfg_len <= payload_len) {
                printf("        Value: ");
                for (int j = 0; j < cfg_len; j++) {
                    printf("%02X ", payload[offset + j]);
                }
                printf("\n");
            } else {
                printf("        Value: TRUNCATED (expected %d bytes, only %d available)\n", cfg_len, payload_len - offset);
            }

            offset += cfg_len;
        }
    }
}

void decode_range_data_ntf(unsigned char* payload, int payload_len) {
    printf("    RANGE_DATA_NTF - Range Data Notification\n");

    if (payload_len < 12) {
        printf("      ERROR: Payload too short (%d bytes, need at least 12)\n", payload_len);
        return;
    }

    unsigned int session_token = read_u32_le(&payload[0]);
    unsigned int sequence_number = read_u32_le(&payload[4]);
    unsigned int control_word = read_u32_le(&payload[8]);

    unsigned char status = control_word & 0xFF;
    unsigned char mac_indicator = (control_word >> 8) & 0xFF;
    unsigned char measurement_count = (control_word >> 16) & 0xFF;
    unsigned char vendor_flags = (control_word >> 24) & 0xFF;

    printf("      Session Token: 0x%08X\n", session_token);
    printf("      Sequence Number: %u\n", sequence_number);
    uci_print_status_line("Status", status);
    printf("      MAC Indicator: 0x%02X (%s)\n",
           mac_indicator, mac_indicator ? "EXTENDED_ADDRESS" : "SHORT_ADDRESS");
    printf("      Measurement Count: %u\n", measurement_count);
    if (vendor_flags) {
        printf("      Vendor Flags: 0x%02X\n", vendor_flags);
    }

    int offset = 12;
    for (unsigned int i = 0; i < measurement_count; i++) {
        printf("      Measurement %u:\n", i + 1);
        if (mac_indicator == 0) {
            if (offset + 20 > payload_len) {
                printf("        WARNING: Incomplete short-address measurement (need 20 bytes, have %d).\n",
                       payload_len - offset);
                return;
            }
            print_short_address_measurement(&payload[offset]);
            offset += 20;
        } else {
            if (offset + 26 > payload_len) {
                printf("        WARNING: Incomplete extended-address measurement (need 26 bytes, have %d).\n",
                       payload_len - offset);
                return;
            }
            print_extended_address_measurement(&payload[offset]);
            offset += 26;
        }
    }

    if (offset < payload_len) {
        printf("      Vendor-specific Range Data (%d bytes):\n", payload_len - offset);
        decode_range_vendor_data(&payload[offset], payload_len - offset);
    }
}

void decode_android_range_diagnostics_ntf(unsigned char* payload, int payload_len) {
    printf("    ANDROID_FIRA_RANGE_DIAGNOSTICS_NTF - Range Diagnostics Notification\n");

    if (payload_len < 9) {
        printf("      ERROR: Payload too short (%d bytes, need at least 9)\n", payload_len);
        return;
    }

    uint32_t session_token = read_u32_le(&payload[0]);
    uint32_t sequence_number = read_u32_le(&payload[4]);
    unsigned char report_count = payload[8];

    printf("      Session Token: 0x%08X\n", session_token);
    printf("      Sequence Number: %u\n", sequence_number);
    printf("      Frame Reports: %u\n", report_count);

    int offset = 9;
    for (unsigned char report_idx = 0; report_idx < report_count; report_idx++) {
        if (offset + 4 > payload_len) {
            printf("      WARNING: Incomplete frame report header at index %u (need 4 bytes, have %d).\n",
                   report_idx, payload_len - offset);
            return;
        }

        unsigned char uwb_msg_id = payload[offset];
        unsigned char action = payload[offset + 1];
        unsigned char antenna_set = payload[offset + 2];
        unsigned char tlv_count = payload[offset + 3];
        offset += 4;

        printf("      Frame Report %u:\n", report_idx);
        printf("        UWB Message ID: 0x%02X\n", uwb_msg_id);
        printf("        Action: 0x%02X\n", action);
        printf("        Antenna Set: 0x%02X\n", antenna_set);
        printf("        TLV Count: %u\n", tlv_count);

        for (unsigned char tlv_idx = 0; tlv_idx < tlv_count; tlv_idx++) {
            if (offset + 3 > payload_len) {
                printf("        WARNING: Incomplete frame report TLV header at index %u (need 3 bytes, have %d).\n",
                       tlv_idx, payload_len - offset);
                return;
            }

            FrameReportTlvType tlv_type = (FrameReportTlvType)payload[offset];
            unsigned short tlv_len = read_u16_le(&payload[offset + 1]);
            offset += 3;

            if (offset + tlv_len > payload_len) {
                printf("        WARNING: TLV 0x%02X length %u exceeds remaining payload (%d).\n",
                       tlv_type, tlv_len, payload_len - offset);
                return;
            }

            decode_frame_report_tlv(tlv_type, &payload[offset], tlv_len);
            offset += tlv_len;
        }
    }

    if (offset < payload_len) {
        printf("      NOTE: %d trailing diagnostic bytes remain after parsing.\n", payload_len - offset);
    }
}

void decode_session_get_count_rsp(unsigned char* payload, int payload_len) {
    printf("    SESSION_GET_COUNT_RSP - Get Session Count Response\n");

    if (payload_len < 2) {
        printf("      ERROR: Payload too short (%d bytes, need at least 2)\n", payload_len);
        return;
    }

    uci_print_status_line("Status", payload[0]);

    printf("      Session Count: %d\n", payload[1]);
}

void decode_session_get_state_rsp(unsigned char* payload, int payload_len) {
    printf("    SESSION_GET_STATE_RSP - Get Session State Response\n");

    if (payload_len < 2) {
        printf("      ERROR: Payload too short (%d bytes, need at least 2)\n", payload_len);
        return;
    }

    uci_print_status_line("Status", payload[0]);
    uci_print_session_state_line("Session State", payload[1]);
}

void decode_session_start_rsp(unsigned char* payload, int payload_len) {
    printf("    SESSION_START_RSP - Session Start Response\n");

    if (payload_len < 1) {
        printf("      ERROR: Payload too short (%d bytes, need at least 1)\n", payload_len);
        return;
    }

    uci_print_status_line("Status", payload[0]);
}

void decode_session_stop_rsp(unsigned char* payload, int payload_len) {
    printf("    SESSION_STOP_RSP - Session Stop Response\n");

    if (payload_len < 1) {
        printf("      ERROR: Payload too short (%d bytes, need at least 1)\n", payload_len);
        return;
    }

    uci_print_status_line("Status", payload[0]);
}

void decode_session_get_ranging_count_rsp(unsigned char* payload, int payload_len) {
    printf("    SESSION_GET_RANGING_COUNT_RSP - Get Ranging Count Response\n");

    if (payload_len < 3) {
        printf("      ERROR: Payload too short (%d bytes, need at least 3)\n", payload_len);
        return;
    }

    uci_print_status_line("Status", payload[0]);
    printf("      Ranging Count: %d\n", read_u16_le(&payload[1]));
}

void decode_core_device_status_ntf(unsigned char* payload, int payload_len) {
    printf("    CORE_DEVICE_STATUS_NTF - Device Status Notification\n");

    if (payload_len < 1) {
        printf("      ERROR: Payload too short (%d bytes, need at least 1)\n", payload_len);
        return;
    }

    uci_print_device_state_line("Device State", payload[0]);
}

void decode_core_generic_error_ntf(unsigned char* payload, int payload_len) {
    printf("    CORE_GENERIC_ERROR_NTF - Generic Error Notification\n");

    if (payload_len < 1) {
        printf("      ERROR: Payload too short (%d bytes, need at least 1)\n", payload_len);
        return;
    }

    uci_print_status_line("Error Status", payload[0]);
}

void decode_session_status_ntf(unsigned char* payload, int payload_len) {
    printf("    SESSION_STATUS_NTF - Session Status Notification\n");

    if (payload_len < 6) {
        printf("      ERROR: Payload too short (%d bytes, need at least 6)\n", payload_len);
        return;
    }

    unsigned int session_token = read_u32_le(payload);
    unsigned char session_state = payload[4];
    unsigned char reason_code = payload[5];

    printf("      Session Token: 0x%08X\n", session_token);
    uci_print_session_state_line("Session State", session_state);
    uci_print_session_reason_line("Reason Code", reason_code);
}

void decode_session_info_ntf(unsigned char* payload, int payload_len) {
    printf("    RANGE_DATA_NTF (SESSION_INFO_NTF) - Ranging Data Notification\n");

    if (payload_len < 25) {
        printf("      ERROR: Payload too short (%d bytes, need at least 25 for header)\n", payload_len);
        return;
    }

    unsigned int sequence_number = read_u32_le(&payload[0]);
    unsigned int session_handle = read_u32_le(&payload[4]);
    unsigned char reserved_byte = payload[8];
    unsigned int current_ranging_interval = read_u32_le(&payload[9]);
    unsigned char ranging_measurement_type = payload[13];
    unsigned char reserved1 = payload[14];
    unsigned char mac_address_indicator = payload[15];
    unsigned int hus_primary_session_id = read_u32_le(&payload[16]);
    unsigned int reserved2 = read_u32_le(&payload[20]);
    unsigned char measurement_count = payload[24];

    printf("      Sequence Number: %u\n", sequence_number);
    printf("      Session Handle: 0x%08X\n", session_handle);
    printf("      Reserved Byte: 0x%02X\n", reserved_byte);
    printf("      Current Ranging Interval: %u ms\n", current_ranging_interval);
    printf("      Ranging Measurement Type: 0x%02X", ranging_measurement_type);
    switch (ranging_measurement_type) {
        case 0x00: printf(" (ONE_WAY)\n"); break;
        case 0x01: printf(" (TWO_WAY)\n"); break;
        case 0x02: printf(" (DL_TDOA)\n"); break;
        case 0x03: printf(" (OWR_AOA)\n"); break;
        default: printf(" (UNKNOWN)\n"); break;
    }

    if (reserved1 || reserved2) {
        printf("      Reserved: 0x%02X 0x%08X\n", reserved1, reserved2);
    }
    printf("      MAC Address Indicator: 0x%02X", mac_address_indicator);
    if (mac_address_indicator == 0x00) {
        printf(" (SHORT_ADDRESS)\n");
    } else {
        printf(" (EXTENDED_ADDRESS)\n");
    }

    printf("      Primary Session ID: 0x%08X\n", hus_primary_session_id);
    printf("      Measurement Count: %u\n", measurement_count);

    int offset = 25;
    if (measurement_count > 0) {
        if (ranging_measurement_type == 0x01) {
            printf("      Number of Two-Way Measurements: %d\n", measurement_count);

            for (int i = 0; i < measurement_count && offset + 20 <= payload_len; i++) {
                printf("      Measurement %d:\n", i + 1);

                if (mac_address_indicator == 0x00) {
                    print_short_address_measurement(&payload[offset]);
                    offset += 20;
                } else if (offset + 26 <= payload_len) {
                    print_extended_address_measurement(&payload[offset]);
                    offset += 26;
                } else {
                    printf("        ERROR: Insufficient data for EXTENDED_ADDRESS measurement\n");
                    break;
                }
            }
        } else if (ranging_measurement_type == 0x03) {
            printf("      Number of OWR-AoA Measurements: %d\n", measurement_count);

            for (int i = 0; i < measurement_count && offset + 13 <= payload_len; i++) {
                printf("      OWR-AoA Measurement %d:\n", i + 1);

                if (mac_address_indicator == 0x00) {
                    unsigned short mac_address = read_u16_le(&payload[offset]);
                    unsigned char status = payload[offset + 2];
                    unsigned char nlos = payload[offset + 3];
                    unsigned char frame_sequence_number = payload[offset + 4];
                    unsigned short block_index = read_u16_le(&payload[offset + 5]);
                    unsigned short aoa_azimuth = read_u16_le(&payload[offset + 7]);
                    unsigned char aoa_azimuth_fom = payload[offset + 9];
                    unsigned short aoa_elevation = read_u16_le(&payload[offset + 10]);
                    unsigned char aoa_elevation_fom = payload[offset + 12];

                    printf("        MAC Address: 0x%04X\n", mac_address);
                    printf("        Status: 0x%02X", status);
                    if (status == 0x00) {
                        printf(" (OK)");
                    }
                    printf("\n");
                    printf("        NLOS: %s\n", nlos ? "YES" : "NO");
                    printf("        Frame Sequence Number: %u\n", frame_sequence_number);
                    printf("        Block Index: %u\n", block_index);
                    printf("        AoA Azimuth: %u degrees (FoM: %u)\n", aoa_azimuth, aoa_azimuth_fom);
                    printf("        AoA Elevation: %u degrees (FoM: %u)\n", aoa_elevation, aoa_elevation_fom);

                    offset += 13;
                } else if (offset + 19 <= payload_len) {
                    unsigned long long mac_address = read_u64_le(&payload[offset]);
                    unsigned char status = payload[offset + 8];
                    unsigned char nlos = payload[offset + 9];
                    unsigned char frame_sequence_number = payload[offset + 10];
                    unsigned short block_index = read_u16_le(&payload[offset + 11]);
                    unsigned short aoa_azimuth = read_u16_le(&payload[offset + 13]);
                    unsigned char aoa_azimuth_fom = payload[offset + 15];
                    unsigned short aoa_elevation = read_u16_le(&payload[offset + 16]);
                    unsigned char aoa_elevation_fom = payload[offset + 18];

                    printf("        MAC Address: 0x%016llX\n", mac_address);
                    printf("        Status: 0x%02X", status);
                    if (status == 0x00) {
                        printf(" (OK)");
                    }
                    printf("\n");
                    printf("        NLOS: %s\n", nlos ? "YES" : "NO");
                    printf("        Frame Sequence Number: %u\n", frame_sequence_number);
                    printf("        Block Index: %u\n", block_index);
                    printf("        AoA Azimuth: %u degrees (FoM: %u)\n", aoa_azimuth, aoa_azimuth_fom);
                    printf("        AoA Elevation: %u degrees (FoM: %u)\n", aoa_elevation, aoa_elevation_fom);

                    offset += 19;
                } else {
                    printf("        ERROR: Insufficient data for EXTENDED_ADDRESS OWR-AoA measurement\n");
                    break;
                }
            }
        } else {
            printf("      Unsupported ranging measurement type: 0x%02X\n", ranging_measurement_type);
        }
    }
}
