#include <stdint.h>
#include <stdio.h>

#include "../include/uci_decode_utils.h"
#include "../include/uci_packet_utils.h"
#include "../include/uci_plain_decoders_internal.h"

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
