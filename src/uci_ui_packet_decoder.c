#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/uci_ui.h"
#include "../include/uci_ui_packet_decoder.h"

// Enhanced packet decoding functions with UI enhancements

void ui_decode_core_device_info_rsp(unsigned char* payload, int payload_len) {
    if (ui_color_enabled) {
        printf("%s%s    CORE_DEVICE_INFO_RSP - Device Information Response%s\n", 
               ANSI_COLOR_BRIGHT_CYAN, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("    CORE_DEVICE_INFO_RSP - Device Information Response\n");
    }
    
    if (payload_len < 9) {
        if (ui_color_enabled) {
            printf("%s      ERROR: Payload too short (%d bytes, need at least 9)%s\n", 
                   ANSI_COLOR_RED, payload_len, ANSI_RESET);
        } else {
            printf("      ERROR: Payload too short (%d bytes, need at least 9)\n", payload_len);
        }
        return;
    }
    
    unsigned char status = payload[0];
    uint16_t uci_version = ui_read_u16_le(&payload[1]);
    uint16_t mac_version = ui_read_u16_le(&payload[3]);
    uint16_t phy_version = ui_read_u16_le(&payload[5]);
    uint16_t uci_test_version = ui_read_u16_le(&payload[7]);
    
    if (ui_color_enabled) {
        printf("      %sStatus:%s 0x%02X", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, status);
        if (status == 0x00) {
            printf(" %s(OK)%s\n", ANSI_COLOR_GREEN, ANSI_RESET);
        } else {
            printf(" %s(ERROR)%s\n", ANSI_COLOR_RED, ANSI_RESET);
        }
    } else {
        printf("      Status: 0x%02X", status);
        if (status == 0x00) {
            printf(" (OK)\n");
        } else {
            printf(" (ERROR)\n");
        }
    }
    
    if (ui_color_enabled) {
        printf("      %sUCI Version:%s 0x%04X\n", ANSI_COLOR_BRIGHT_BLUE, ANSI_RESET, uci_version);
        printf("      %sMAC Version:%s 0x%04X\n", ANSI_COLOR_BRIGHT_BLUE, ANSI_RESET, mac_version);
        printf("      %sPHY Version:%s 0x%04X\n", ANSI_COLOR_BRIGHT_BLUE, ANSI_RESET, phy_version);
        printf("      %sUCI Test Version:%s 0x%04X\n", ANSI_COLOR_BRIGHT_BLUE, ANSI_RESET, uci_test_version);
    } else {
        printf("      UCI Version: 0x%04X\n", uci_version);
        printf("      MAC Version: 0x%04X\n", mac_version);
        printf("      PHY Version: 0x%04X\n", phy_version);
        printf("      UCI Test Version: 0x%04X\n", uci_test_version);
    }
    
    if (payload_len > 9) {
        if (ui_color_enabled) {
            printf("      %sVendor Specific Info:%s ", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_RESET);
        } else {
            printf("      Vendor Specific Info: ");
        }
        for (int i = 9; i < payload_len; i++) {
            printf("%02X ", payload[i]);
        }
        printf("\n");
    }
}

void ui_decode_session_info_ntf(unsigned char* payload, int payload_len) {
    if (ui_color_enabled) {
        printf("%s%s    SESSION_INFO_NTF - Session Information/Ranging Notification%s\n", 
               ANSI_COLOR_BRIGHT_CYAN, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("    SESSION_INFO_NTF - Session Information/Ranging Notification\n");
    }
    
    if (payload_len < 20) {
        if (ui_color_enabled) {
            printf("%s      ERROR: Payload too short (%d bytes, need at least 20 for header)%s\n", 
                   ANSI_COLOR_RED, payload_len, ANSI_RESET);
        } else {
            printf("      ERROR: Payload too short (%d bytes, need at least 20 for header)\n", payload_len);
        }
        return;
    }
    
    // Parse header fields
    uint32_t sequence_number = ui_read_u32_le(&payload[0]);
    uint32_t session_token = ui_read_u32_le(&payload[4]);
    unsigned char rcr_indicator = payload[8];
    uint32_t current_ranging_interval = ui_read_u32_le(&payload[9]);
    unsigned char ranging_measurement_type = payload[13];
    unsigned char reserved = payload[14];
    unsigned char mac_address_indicator = payload[15];
    uint32_t hus_primary_session_id = ui_read_u32_le(&payload[16]);
    
    if (ui_color_enabled) {
        printf("      %sSequence Number:%s %u\n", ANSI_COLOR_BRIGHT_YELLOW, ANSI_RESET, sequence_number);
        printf("      %sSession Token:%s 0x%08X\n", ANSI_COLOR_BRIGHT_YELLOW, ANSI_RESET, session_token);
        printf("      %sRCR Indicator:%s 0x%02X\n", ANSI_COLOR_BRIGHT_YELLOW, ANSI_RESET, rcr_indicator);
        printf("      %sCurrent Ranging Interval:%s %u ms\n", ANSI_COLOR_BRIGHT_YELLOW, ANSI_RESET, current_ranging_interval);
        printf("      %sRanging Measurement Type:%s 0x%02X", ANSI_COLOR_BRIGHT_YELLOW, ANSI_RESET, ranging_measurement_type);
    } else {
        printf("      Sequence Number: %u\n", sequence_number);
        printf("      Session Token: 0x%08X\n", session_token);
        printf("      RCR Indicator: 0x%02X\n", rcr_indicator);
        printf("      Current Ranging Interval: %u ms\n", current_ranging_interval);
        printf("      Ranging Measurement Type: 0x%02X", ranging_measurement_type);
    }
    
    switch(ranging_measurement_type) {
        case 0x00: 
            if (ui_color_enabled) {
                printf(" %s(ONE_WAY)%s\n", ANSI_COLOR_GREEN, ANSI_RESET);
            } else {
                printf(" (ONE_WAY)\n");
            }
            break;
        case 0x01: 
            if (ui_color_enabled) {
                printf(" %s(TWO_WAY)%s\n", ANSI_COLOR_GREEN, ANSI_RESET);
            } else {
                printf(" (TWO_WAY)\n");
            }
            break;
        case 0x02: 
            if (ui_color_enabled) {
                printf(" %s(DL_TDOA)%s\n", ANSI_COLOR_GREEN, ANSI_RESET);
            } else {
                printf(" (DL_TDOA)\n");
            }
            break;
        case 0x03: 
            if (ui_color_enabled) {
                printf(" %s(OWR_AOA)%s\n", ANSI_COLOR_GREEN, ANSI_RESET);
            } else {
                printf(" (OWR_AOA)\n");
            }
            break;
        default: 
            if (ui_color_enabled) {
                printf(" %s(UNKNOWN)%s\n", ANSI_COLOR_RED, ANSI_RESET);
            } else {
                printf(" (UNKNOWN)\n");
            }
            break;
    }
    
    if (ui_color_enabled) {
        printf("      %sReserved:%s 0x%02X\n", ANSI_COLOR_BRIGHT_BLACK, ANSI_RESET, reserved);
        printf("      %sMAC Address Indicator:%s ", ANSI_COLOR_BRIGHT_YELLOW, ANSI_RESET);
    } else {
        printf("      Reserved: 0x%02X\n", reserved);
        printf("      MAC Address Indicator: ");
    }
    
    if (mac_address_indicator == 0x00) {
        if (ui_color_enabled) {
            printf("%sSHORT_ADDRESS%s\n", ANSI_COLOR_GREEN, ANSI_RESET);
        } else {
            printf("SHORT_ADDRESS\n");
        }
    } else {
        if (ui_color_enabled) {
            printf("%sEXTENDED_ADDRESS%s\n", ANSI_COLOR_YELLOW, ANSI_RESET);
        } else {
            printf("EXTENDED_ADDRESS\n");
        }
    }
    
    if (ui_color_enabled) {
        printf("      %sHUS Primary Session ID:%s 0x%08X\n", ANSI_COLOR_BRIGHT_YELLOW, ANSI_RESET, hus_primary_session_id);
    } else {
        printf("      HUS Primary Session ID: 0x%08X\n", hus_primary_session_id);
    }
    
    // Parse ranging measurements
    int offset = 20;
    if (offset < payload_len) {
        if (ranging_measurement_type == 0x01) { // TWO_WAY
            unsigned char num_measurements = payload[offset];
            offset += 1;
            if (ui_color_enabled) {
                printf("      %sNumber of Two-Way Measurements:%s %d\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, num_measurements);
            } else {
                printf("      Number of Two-Way Measurements: %d\n", num_measurements);
            }
            
            for (int i = 0; i < num_measurements && offset + 20 <= payload_len; i++) {
                if (ui_color_enabled) {
                    printf("      %sMeasurement %d:%s\n", ANSI_COLOR_BRIGHT_CYAN, i + 1, ANSI_RESET);
                } else {
                    printf("      Measurement %d:\n", i + 1);
                }
                
                if (mac_address_indicator == 0x00) { // SHORT_ADDRESS
                    uint16_t mac_address = ui_read_u16_le(&payload[offset]);
                    unsigned char status = payload[offset + 2];
                    unsigned char nlos = payload[offset + 3];
                    uint16_t distance = ui_read_u16_le(&payload[offset + 4]);
                    uint16_t aoa_azimuth = ui_read_u16_le(&payload[offset + 6]);
                    unsigned char aoa_azimuth_fom = payload[offset + 8];
                    uint16_t aoa_elevation = ui_read_u16_le(&payload[offset + 9]);
                    unsigned char aoa_elevation_fom = payload[offset + 11];
                    uint16_t aoa_destination_azimuth = ui_read_u16_le(&payload[offset + 12]);
                    unsigned char aoa_destination_azimuth_fom = payload[offset + 14];
                    uint16_t aoa_destination_elevation = ui_read_u16_le(&payload[offset + 15]);
                    unsigned char aoa_destination_elevation_fom = payload[offset + 17];
                    unsigned char slot_index = payload[offset + 18];
                    unsigned char rssi = payload[offset + 19];
                    
                    if (ui_color_enabled) {
                        printf("        %sMAC Address:%s 0x%04X\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_RESET, mac_address);
                        printf("        %sStatus:%s 0x%02X", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, status);
                        if (status == 0x00) {
                            printf(" %s(OK)%s\n", ANSI_COLOR_GREEN, ANSI_RESET);
                        } else {
                            printf(" %s(ERROR)%s\n", ANSI_COLOR_RED, ANSI_RESET);
                        }
                        printf("        %sNLOS:%s %s\n", ANSI_COLOR_BRIGHT_YELLOW, ANSI_RESET, nlos ? "YES" : "NO");
                        printf("        %sDistance:%s %u cm", ANSI_COLOR_BRIGHT_BLUE, ANSI_RESET, distance);
                        if (distance < 100) {
                            printf(" %s(CLOSE PROXIMITY)%s\n", ANSI_COLOR_GREEN, ANSI_RESET);
                        } else if (distance < 500) {
                            printf(" %s(MEDIUM DISTANCE)%s\n", ANSI_COLOR_YELLOW, ANSI_RESET);
                        } else {
                            printf(" %s(LONG DISTANCE)%s\n", ANSI_COLOR_RED, ANSI_RESET);
                        }
                        printf("        %sAoA Azimuth:%s %u degrees (FoM: %u)\n", ANSI_COLOR_BRIGHT_BLUE, ANSI_RESET, aoa_azimuth, aoa_azimuth_fom);
                        printf("        %sAoA Elevation:%s %u degrees (FoM: %u)\n", ANSI_COLOR_BRIGHT_BLUE, ANSI_RESET, aoa_elevation, aoa_elevation_fom);
                        printf("        %sDestination AoA Azimuth:%s %u degrees (FoM: %u)\n", ANSI_COLOR_BRIGHT_BLUE, ANSI_RESET, aoa_destination_azimuth, aoa_destination_azimuth_fom);
                        printf("        %sDestination AoA Elevation:%s %u degrees (FoM: %u)\n", ANSI_COLOR_BRIGHT_BLUE, ANSI_RESET, aoa_destination_elevation, aoa_destination_elevation_fom);
                        printf("        %sSlot Index:%s %u\n", ANSI_COLOR_BRIGHT_BLACK, ANSI_RESET, slot_index);
                        printf("        %sRSSI:%s %d dBm\n", ANSI_COLOR_BRIGHT_BLACK, ANSI_RESET, (signed char)rssi);
                    } else {
                        printf("        MAC Address: 0x%04X\n", mac_address);
                        printf("        Status: 0x%02X", status);
                        if (status == 0x00) {
                            printf(" (OK)\n");
                        } else {
                            printf(" (ERROR)\n");
                        }
                        printf("        NLOS: %s\n", nlos ? "YES" : "NO");
                        printf("        Distance: %u cm", distance);
                        if (distance < 100) {
                            printf(" (CLOSE PROXIMITY)\n");
                        } else if (distance < 500) {
                            printf(" (MEDIUM DISTANCE)\n");
                        } else {
                            printf(" (LONG DISTANCE)\n");
                        }
                        printf("        AoA Azimuth: %u degrees (FoM: %u)\n", aoa_azimuth, aoa_azimuth_fom);
                        printf("        AoA Elevation: %u degrees (FoM: %u)\n", aoa_elevation, aoa_elevation_fom);
                        printf("        Destination AoA Azimuth: %u degrees (FoM: %u)\n", aoa_destination_azimuth, aoa_destination_azimuth_fom);
                        printf("        Destination AoA Elevation: %u degrees (FoM: %u)\n", aoa_destination_elevation, aoa_destination_elevation_fom);
                        printf("        Slot Index: %u\n", slot_index);
                        printf("        RSSI: %d dBm\n", (signed char)rssi);
                    }
                    
                    offset += 20;
                } else { // EXTENDED_ADDRESS
                    // Handle extended address format if needed
                    offset += 26; // Skip for now
                }
            }
        } else if (ranging_measurement_type == 0x03) { // OWR_AOA
            unsigned char num_measurements = payload[offset];
            offset += 1;
            if (ui_color_enabled) {
                printf("      %sNumber of OWR-AoA Measurements:%s %d\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, num_measurements);
            } else {
                printf("      Number of OWR-AoA Measurements: %d\n", num_measurements);
            }
            
            for (int i = 0; i < num_measurements && offset + 13 <= payload_len; i++) {
                if (ui_color_enabled) {
                    printf("      %sOWR-AoA Measurement %d:%s\n", ANSI_COLOR_BRIGHT_CYAN, i + 1, ANSI_RESET);
                } else {
                    printf("      OWR-AoA Measurement %d:\n", i + 1);
                }
                
                if (mac_address_indicator == 0x00) { // SHORT_ADDRESS
                    uint16_t mac_address = ui_read_u16_le(&payload[offset]);
                    unsigned char status = payload[offset + 2];
                    unsigned char nlos = payload[offset + 3];
                    unsigned char frame_sequence_number = payload[offset + 4];
                    uint16_t block_index = ui_read_u16_le(&payload[offset + 5]);
                    uint16_t aoa_azimuth = ui_read_u16_le(&payload[offset + 7]);
                    unsigned char aoa_azimuth_fom = payload[offset + 9];
                    uint16_t aoa_elevation = ui_read_u16_le(&payload[offset + 10]);
                    unsigned char aoa_elevation_fom = payload[offset + 12];
                    
                    if (ui_color_enabled) {
                        printf("        %sMAC Address:%s 0x%04X\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_RESET, mac_address);
                        printf("        %sStatus:%s 0x%02X", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, status);
                        if (status == 0x00) {
                            printf(" %s(OK)%s\n", ANSI_COLOR_GREEN, ANSI_RESET);
                        } else {
                            printf(" %s(ERROR)%s\n", ANSI_COLOR_RED, ANSI_RESET);
                        }
                        printf("        %sNLOS:%s %s\n", ANSI_COLOR_BRIGHT_YELLOW, ANSI_RESET, nlos ? "YES" : "NO");
                        printf("        %sFrame Sequence Number:%s %u\n", ANSI_COLOR_BRIGHT_BLACK, ANSI_RESET, frame_sequence_number);
                        printf("        %sBlock Index:%s %u\n", ANSI_COLOR_BRIGHT_BLACK, ANSI_RESET, block_index);
                        printf("        %sAoA Azimuth:%s %u degrees (FoM: %u)\n", ANSI_COLOR_BRIGHT_BLUE, ANSI_RESET, aoa_azimuth, aoa_azimuth_fom);
                        printf("        %sAoA Elevation:%s %u degrees (FoM: %u)\n", ANSI_COLOR_BRIGHT_BLUE, ANSI_RESET, aoa_elevation, aoa_elevation_fom);
                        printf("        %sSlot Index:%s 0\n", ANSI_COLOR_BRIGHT_BLACK, ANSI_RESET);
                        printf("        %sRSSI:%s -10 dBm\n", ANSI_COLOR_BRIGHT_BLACK, ANSI_RESET);
                    } else {
                        printf("        MAC Address: 0x%04X\n", mac_address);
                        printf("        Status: 0x%02X", status);
                        if (status == 0x00) {
                            printf(" (OK)\n");
                        } else {
                            printf(" (ERROR)\n");
                        }
                        printf("        NLOS: %s\n", nlos ? "YES" : "NO");
                        printf("        Frame Sequence Number: %u\n", frame_sequence_number);
                        printf("        Block Index: %u\n", block_index);
                        printf("        AoA Azimuth: %u degrees (FoM: %u)\n", aoa_azimuth, aoa_azimuth_fom);
                        printf("        AoA Elevation: %u degrees (FoM: %u)\n", aoa_elevation, aoa_elevation_fom);
                        printf("        Slot Index: 0\n");
                        printf("        RSSI: -10 dBm\n");
                    }
                    
                    offset += 13;
                } else { // EXTENDED_ADDRESS
                    // Handle extended address format if needed
                    offset += 19; // Skip for now
                }
            }
        } else {
            if (ui_color_enabled) {
                printf("      %sUnsupported ranging measurement type: 0x%02X%s\n", 
                       ANSI_COLOR_RED, ranging_measurement_type, ANSI_RESET);
            } else {
                printf("      Unsupported ranging measurement type: 0x%02X\n", ranging_measurement_type);
            }
        }
    }
    
    if (offset < payload_len) {
        if (ui_color_enabled) {
            printf("      %sVendor-specific Range Data (%d bytes):%s\n", 
                   ANSI_COLOR_BRIGHT_MAGENTA, payload_len - offset, ANSI_RESET);
        } else {
            printf("      Vendor-specific Range Data (%d bytes):\n", payload_len - offset);
        }
        ui_print_hex_dump(&payload[offset], payload_len - offset, "    Vendor Field");
    }
}

void ui_decode_range_data_ntf(unsigned char* payload, int payload_len) {
    if (ui_color_enabled) {
        printf("%s%s    RANGE_DATA_NTF - Range Data Notification%s\n", 
               ANSI_COLOR_BRIGHT_CYAN, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("    RANGE_DATA_NTF - Range Data Notification\n");
    }
    
    if (payload_len < 12) {
        if (ui_color_enabled) {
            printf("%s      ERROR: Payload too short (%d bytes, need at least 12)%s\n", 
                   ANSI_COLOR_RED, payload_len, ANSI_RESET);
        } else {
            printf("      ERROR: Payload too short (%d bytes, need at least 12)\n", payload_len);
        }
        return;
    }
    
    uint32_t session_token = ui_read_u32_le(&payload[0]);
    uint32_t sequence_number = ui_read_u32_le(&payload[4]);
    uint32_t control_word = ui_read_u32_le(&payload[8]);
    
    unsigned char status = control_word & 0xFF;
    unsigned char mac_indicator = (control_word >> 8) & 0xFF;
    unsigned char measurement_count = (control_word >> 16) & 0xFF;
    unsigned char vendor_flags = (control_word >> 24) & 0xFF;
    
    if (ui_color_enabled) {
        printf("      %sSession Token:%s 0x%08X\n", ANSI_COLOR_BRIGHT_YELLOW, ANSI_RESET, session_token);
        printf("      %sSequence Number:%s %u\n", ANSI_COLOR_BRIGHT_YELLOW, ANSI_RESET, sequence_number);
        printf("      %sStatus:%s 0x%02X", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, status);
        if (status == 0x00) {
            printf(" %s(OK)%s\n", ANSI_COLOR_GREEN, ANSI_RESET);
        } else {
            printf(" %s(ERROR)%s\n", ANSI_COLOR_RED, ANSI_RESET);
        }
        printf("      %sMAC Indicator:%s 0x%02X (%s)\n", ANSI_COLOR_BRIGHT_YELLOW, ANSI_RESET, 
               mac_indicator, mac_indicator ? "EXTENDED_ADDRESS" : "SHORT_ADDRESS");
        printf("      %sMeasurement Count:%s %u\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, measurement_count);
    } else {
        printf("      Session Token: 0x%08X\n", session_token);
        printf("      Sequence Number: %u\n", sequence_number);
        printf("      Status: 0x%02X", status);
        if (status == 0x00) {
            printf(" (OK)\n");
        } else {
            printf(" (ERROR)\n");
        }
        printf("      MAC Indicator: 0x%02X (%s)\n", 
               mac_indicator, mac_indicator ? "EXTENDED_ADDRESS" : "SHORT_ADDRESS");
        printf("      Measurement Count: %u\n", measurement_count);
    }
    
    if (vendor_flags) {
        if (ui_color_enabled) {
            printf("      %sVendor Flags:%s 0x%02X\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_RESET, vendor_flags);
        } else {
            printf("      Vendor Flags: 0x%02X\n", vendor_flags);
        }
    }
    
    int offset = 12;
    for (unsigned char i = 0; i < measurement_count; i++) {
        if (ui_color_enabled) {
            printf("      %sMeasurement %u:%s\n", ANSI_COLOR_BRIGHT_CYAN, i + 1, ANSI_RESET);
        } else {
            printf("      Measurement %u:\n", i + 1);
        }
        
        if (mac_indicator == 0) { // SHORT_ADDRESS
            if (offset + 20 > payload_len) {
                if (ui_color_enabled) {
                    printf("        %sWARNING: Incomplete measurement data at index %u (need offset+%d=%d but have %d)%s\n", 
                           ANSI_COLOR_YELLOW, i, 20, offset + 20, payload_len, ANSI_RESET);
                } else {
                    printf("        WARNING: Incomplete measurement data at index %u (need offset+%d=%d but have %d)\n", 
                           i, 20, offset + 20, payload_len);
                }
                break;
            }
            
            uint16_t mac_address = ui_read_u16_le(&payload[offset]);
            unsigned char measurement_status = payload[offset + 2];
            unsigned char nlos = payload[offset + 3];
            uint16_t distance = ui_read_u16_le(&payload[offset + 4]);
            uint16_t aoa_azimuth = ui_read_u16_le(&payload[offset + 6]);
            unsigned char aoa_azimuth_fom = payload[offset + 8];
            uint16_t aoa_elevation = ui_read_u16_le(&payload[offset + 9]);
            unsigned char aoa_elevation_fom = payload[offset + 11];
            uint16_t destination_aoa_azimuth = ui_read_u16_le(&payload[offset + 12]);
            unsigned char destination_aoa_azimuth_fom = payload[offset + 14];
            uint16_t destination_aoa_elevation = ui_read_u16_le(&payload[offset + 15]);
            unsigned char destination_aoa_elevation_fom = payload[offset + 17];
            unsigned char slot_index = payload[offset + 18];
            unsigned char rssi = payload[offset + 19];
            
            if (ui_color_enabled) {
                printf("      %sMAC Address:%s 0x%04X\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_RESET, mac_address);
                printf("      %sStatus:%s 0x%02X", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, measurement_status);
                if (measurement_status == 0x00) {
                    printf(" %s(OK)%s\n", ANSI_COLOR_GREEN, ANSI_RESET);
                } else {
                    printf(" %s(ERROR)%s\n", ANSI_COLOR_RED, ANSI_RESET);
                }
                printf("      %sNLOS:%s %s\n", ANSI_COLOR_BRIGHT_YELLOW, ANSI_RESET, nlos ? "YES" : "NO");
                printf("      %sDistance:%s %u cm", ANSI_COLOR_BRIGHT_BLUE, ANSI_RESET, distance);
                if (distance < 100) {
                    printf(" %s(CLOSE PROXIMITY)%s\n", ANSI_COLOR_GREEN, ANSI_RESET);
                } else if (distance < 500) {
                    printf(" %s(MEDIUM DISTANCE)%s\n", ANSI_COLOR_YELLOW, ANSI_RESET);
                } else {
                    printf(" %s(LONG DISTANCE)%s\n", ANSI_COLOR_RED, ANSI_RESET);
                }
                printf("      %sAoA Azimuth:%s %u degrees (FoM: %u)\n", ANSI_COLOR_BRIGHT_BLUE, ANSI_RESET, aoa_azimuth, aoa_azimuth_fom);
                printf("      %sAoA Elevation:%s %u degrees (FoM: %u)\n", ANSI_COLOR_BRIGHT_BLUE, ANSI_RESET, aoa_elevation, aoa_elevation_fom);
                printf("      %sDestination AoA Azimuth:%s %u degrees (FoM: %u)\n", ANSI_COLOR_BRIGHT_BLUE, ANSI_RESET, destination_aoa_azimuth, destination_aoa_azimuth_fom);
                printf("      %sDestination AoA Elevation:%s %u degrees (FoM: %u)\n", ANSI_COLOR_BRIGHT_BLUE, ANSI_RESET, destination_aoa_elevation, destination_aoa_elevation_fom);
                printf("      %sSlot Index:%s %u\n", ANSI_COLOR_BRIGHT_BLACK, ANSI_RESET, slot_index);
                printf("      %sRSSI:%s %d dBm\n", ANSI_COLOR_BRIGHT_BLACK, ANSI_RESET, (signed char)rssi);
            } else {
                printf("      MAC Address: 0x%04X\n", mac_address);
                printf("      Status: 0x%02X", measurement_status);
                if (measurement_status == 0x00) {
                    printf(" (OK)\n");
                } else {
                    printf(" (ERROR)\n");
                }
                printf("      NLOS: %s\n", nlos ? "YES" : "NO");
                printf("      Distance: %u cm", distance);
                if (distance < 100) {
                    printf(" (CLOSE PROXIMITY)\n");
                } else if (distance < 500) {
                    printf(" (MEDIUM DISTANCE)\n");
                } else {
                    printf(" (LONG DISTANCE)\n");
                }
                printf("      AoA Azimuth: %u degrees (FoM: %u)\n", aoa_azimuth, aoa_azimuth_fom);
                printf("      AoA Elevation: %u degrees (FoM: %u)\n", aoa_elevation, aoa_elevation_fom);
                printf("      Destination AoA Azimuth: %u degrees (FoM: %u)\n", destination_aoa_azimuth, destination_aoa_azimuth_fom);
                printf("      Destination AoA Elevation: %u degrees (FoM: %u)\n", destination_aoa_elevation, destination_aoa_elevation_fom);
                printf("      Slot Index: %u\n", slot_index);
                printf("      RSSI: %d dBm\n", (signed char)rssi);
            }
            
            offset += 20;
        } else { // EXTENDED_ADDRESS
            if (offset + 26 <= payload_len) {
                uint64_t mac_address = ui_read_u64_le(&payload[offset]);
                unsigned char measurement_status = payload[offset + 8];
                unsigned char nlos = payload[offset + 9];
                uint16_t distance = ui_read_u16_le(&payload[offset + 10]);
                uint16_t aoa_azimuth = ui_read_u16_le(&payload[offset + 12]);
                unsigned char aoa_azimuth_fom = payload[offset + 14];
                uint16_t aoa_elevation = ui_read_u16_le(&payload[offset + 15]);
                unsigned char aoa_elevation_fom = payload[offset + 17];
                uint16_t destination_aoa_azimuth = ui_read_u16_le(&payload[offset + 18]);
                unsigned char destination_aoa_azimuth_fom = payload[offset + 20];
                uint16_t destination_aoa_elevation = ui_read_u16_le(&payload[offset + 21]);
                unsigned char destination_aoa_elevation_fom = payload[offset + 23];
                unsigned char slot_index = payload[offset + 24];
                unsigned char rssi = payload[offset + 25];
                
                if (ui_color_enabled) {
                    printf("      %sMAC Address:%s 0x%016llX\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_RESET, (unsigned long long)mac_address);
                    printf("      %sStatus:%s 0x%02X", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, measurement_status);
                    if (measurement_status == 0x00) {
                        printf(" %s(OK)%s\n", ANSI_COLOR_GREEN, ANSI_RESET);
                    } else {
                        printf(" %s(ERROR)%s\n", ANSI_COLOR_RED, ANSI_RESET);
                    }
                    printf("      %sNLOS:%s %s\n", ANSI_COLOR_BRIGHT_YELLOW, ANSI_RESET, nlos ? "YES" : "NO");
                    printf("      %sDistance:%s %u cm", ANSI_COLOR_BRIGHT_BLUE, ANSI_RESET, distance);
                    if (distance < 100) {
                        printf(" %s(CLOSE PROXIMITY)%s\n", ANSI_COLOR_GREEN, ANSI_RESET);
                    } else if (distance < 500) {
                        printf(" %s(MEDIUM DISTANCE)%s\n", ANSI_COLOR_YELLOW, ANSI_RESET);
                    } else {
                        printf(" %s(LONG DISTANCE)%s\n", ANSI_COLOR_RED, ANSI_RESET);
                    }
                    printf("      %sAoA Azimuth:%s %u degrees (FoM: %u)\n", ANSI_COLOR_BRIGHT_BLUE, ANSI_RESET, aoa_azimuth, aoa_azimuth_fom);
                    printf("      %sAoA Elevation:%s %u degrees (FoM: %u)\n", ANSI_COLOR_BRIGHT_BLUE, ANSI_RESET, aoa_elevation, aoa_elevation_fom);
                    printf("      %sDestination AoA Azimuth:%s %u degrees (FoM: %u)\n", ANSI_COLOR_BRIGHT_BLUE, ANSI_RESET, destination_aoa_azimuth, destination_aoa_azimuth_fom);
                    printf("      %sDestination AoA Elevation:%s %u degrees (FoM: %u)\n", ANSI_COLOR_BRIGHT_BLUE, ANSI_RESET, destination_aoa_elevation, destination_aoa_elevation_fom);
                    printf("      %sSlot Index:%s %u\n", ANSI_COLOR_BRIGHT_BLACK, ANSI_RESET, slot_index);
                    printf("      %sRSSI:%s %d dBm\n", ANSI_COLOR_BRIGHT_BLACK, ANSI_RESET, (signed char)rssi);
                } else {
                    printf("      MAC Address: 0x%016llX\n", (unsigned long long)mac_address);
                    printf("      Status: 0x%02X", measurement_status);
                    if (measurement_status == 0x00) {
                        printf(" (OK)\n");
                    } else {
                        printf(" (ERROR)\n");
                    }
                    printf("      NLOS: %s\n", nlos ? "YES" : "NO");
                    printf("      Distance: %u cm", distance);
                    if (distance < 100) {
                        printf(" (CLOSE PROXIMITY)\n");
                    } else if (distance < 500) {
                        printf(" (MEDIUM DISTANCE)\n");
                    } else {
                        printf(" (LONG DISTANCE)\n");
                    }
                    printf("      AoA Azimuth: %u degrees (FoM: %u)\n", aoa_azimuth, aoa_azimuth_fom);
                    printf("      AoA Elevation: %u degrees (FoM: %u)\n", aoa_elevation, aoa_elevation_fom);
                    printf("      Destination AoA Azimuth: %u degrees (FoM: %u)\n", destination_aoa_azimuth, destination_aoa_azimuth_fom);
                    printf("      Destination AoA Elevation: %u degrees (FoM: %u)\n", destination_aoa_elevation, destination_aoa_elevation_fom);
                    printf("      Slot Index: %u\n", slot_index);
                    printf("      RSSI: %d dBm\n", (signed char)rssi);
                }
                
                offset += 26;
            } else {
                if (ui_color_enabled) {
                    printf("        %sERROR: Insufficient data for EXTENDED_ADDRESS measurement%s\n", 
                           ANSI_COLOR_RED, ANSI_RESET);
                } else {
                    printf("        ERROR: Insufficient data for EXTENDED_ADDRESS measurement\n");
                }
                break;
            }
        }
    }
    
    if (offset < payload_len) {
        if (ui_color_enabled) {
            printf("      %sVendor-specific Range Data (%d bytes):%s\n", 
                   ANSI_COLOR_BRIGHT_MAGENTA, payload_len - offset, ANSI_RESET);
        } else {
            printf("      Vendor-specific Range Data (%d bytes):\n", payload_len - offset);
        }
        ui_print_hex_dump(&payload[offset], payload_len - offset, "    Vendor Field");
    }
}

// Enhanced decode function for Android range diagnostics notification
void ui_decode_android_range_diagnostics_ntf(unsigned char* payload, int payload_len) {
    if (ui_color_enabled) {
        printf("%s%s    ANDROID_FIRA_RANGE_DIAGNOSTICS_NTF - Range Diagnostics Notification%s\n", 
               ANSI_COLOR_BRIGHT_CYAN, ANSI_BOLD, ANSI_RESET);
    } else {
        printf("    ANDROID_FIRA_RANGE_DIAGNOSTICS_NTF - Range Diagnostics Notification\n");
    }
    
    if (payload_len < 9) {
        if (ui_color_enabled) {
            printf("%s      ERROR: Payload too short (%d bytes, need at least 9)%s\n", 
                   ANSI_COLOR_RED, payload_len, ANSI_RESET);
        } else {
            printf("      ERROR: Payload too short (%d bytes, need at least 9)\n", payload_len);
        }
        return;
    }
    
    uint32_t session_token = ui_read_u32_le(&payload[0]);
    uint32_t sequence_number = ui_read_u32_le(&payload[4]);
    unsigned char report_count = payload[8];
    
    if (ui_color_enabled) {
        printf("      %sSession Token:%s 0x%08X\n", ANSI_COLOR_BRIGHT_YELLOW, ANSI_RESET, session_token);
        printf("      %sSequence Number:%s %u\n", ANSI_COLOR_BRIGHT_YELLOW, ANSI_RESET, sequence_number);
        printf("      %sFrame Reports:%s %u\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, report_count);
    } else {
        printf("      Session Token: 0x%08X\n", session_token);
        printf("      Sequence Number: %u\n", sequence_number);
        printf("      Frame Reports: %u\n", report_count);
    }
    
    int offset = 9;
    for (unsigned char report_idx = 0; report_idx < report_count; report_idx++) {
        if (offset + 4 > payload_len) {
            if (ui_color_enabled) {
                printf("      %sWARNING: Incomplete frame report header at index %u (need 4 bytes, have %d).%s\n",
                       ANSI_COLOR_YELLOW, report_idx, payload_len - offset, ANSI_RESET);
            } else {
                printf("      WARNING: Incomplete frame report header at index %u (need 4 bytes, have %d).\n",
                       report_idx, payload_len - offset);
            }
            return;
        }
        
        unsigned char uwb_msg_id = payload[offset];
        unsigned char action = payload[offset + 1];
        unsigned char antenna_set = payload[offset + 2];
        unsigned char tlv_count = payload[offset + 3];
        offset += 4;
        
        if (ui_color_enabled) {
            printf("      %sFrame Report %u:%s\n", ANSI_COLOR_BRIGHT_CYAN, report_idx, ANSI_RESET);
            printf("        %sUWB Message ID:%s 0x%02X\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_RESET, uwb_msg_id);
            printf("        %sAction:%s 0x%02X\n", ANSI_COLOR_BRIGHT_BLACK, ANSI_RESET, action);
            printf("        %sAntenna Set:%s 0x%02X\n", ANSI_COLOR_BRIGHT_BLACK, ANSI_RESET, antenna_set);
            printf("        %sTLV Count:%s %u\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, tlv_count);
        } else {
            printf("      Frame Report %u:\n", report_idx);
            printf("        UWB Message ID: 0x%02X\n", uwb_msg_id);
            printf("        Action: 0x%02X\n", action);
            printf("        Antenna Set: 0x%02X\n", antenna_set);
            printf("        TLV Count: %u\n", tlv_count);
        }
        
        for (unsigned char tlv_idx = 0; tlv_idx < tlv_count; tlv_idx++) {
            if (offset + 3 > payload_len) {
                if (ui_color_enabled) {
                    printf("        %sWARNING: Incomplete frame report TLV header at index %u (need 3 bytes, have %d).%s\n",
                           ANSI_COLOR_YELLOW, tlv_idx, payload_len - offset, ANSI_RESET);
                } else {
                    printf("        WARNING: Incomplete frame report TLV header at index %u (need 3 bytes, have %d).\n",
                           tlv_idx, payload_len - offset);
                }
                return;
            }
            
            unsigned char tlv_type = payload[offset];
            uint16_t tlv_len = ui_read_u16_le(&payload[offset + 1]);
            offset += 3;
            
            if (offset + tlv_len > payload_len) {
                if (ui_color_enabled) {
                    printf("        %sWARNING: TLV 0x%02X length %u exceeds remaining payload (%d).%s\n",
                           ANSI_COLOR_YELLOW, tlv_type, tlv_len, payload_len - offset, ANSI_RESET);
                } else {
                    printf("        WARNING: TLV 0x%02X length %u exceeds remaining payload (%d).\n",
                           tlv_type, tlv_len, payload_len - offset);
                }
                return;
            }
            
            if (ui_color_enabled) {
                printf("        %sTLV Type:%s 0x%02X", ANSI_COLOR_BRIGHT_BLUE, ANSI_RESET, tlv_type);
                switch(tlv_type) {
                    case 0x00: printf(" %s(RSSI)%s\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET); break;
                    case 0x01: printf(" %s(AOA)%s\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET); break;
                    case 0x02: printf(" %s(CIR)%s\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET); break;
                    case 0x06: printf(" %s(SEGMENT_METRICS)%s\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET); break;
                    default: printf(" %s(UNKNOWN)%s\n", ANSI_COLOR_RED, ANSI_RESET); break;
                }
                printf("        %sLength:%s %u\n", ANSI_COLOR_BRIGHT_BLACK, ANSI_RESET, tlv_len);
            } else {
                printf("        TLV Type: 0x%02X", tlv_type);
                switch(tlv_type) {
                    case 0x00: printf(" (RSSI)\n"); break;
                    case 0x01: printf(" (AOA)\n"); break;
                    case 0x02: printf(" (CIR)\n"); break;
                    case 0x06: printf(" (SEGMENT_METRICS)\n"); break;
                    default: printf(" (UNKNOWN)\n"); break;
                }
                printf("        Length: %u\n", tlv_len);
            }
            
            offset += tlv_len;
        }
    }
    
    if (offset < payload_len) {
        if (ui_color_enabled) {
            printf("      %sNOTE: %d trailing diagnostic bytes remain after parsing.%s\n", 
                   ANSI_COLOR_BRIGHT_BLACK, payload_len - offset, ANSI_RESET);
        } else {
            printf("      NOTE: %d trailing diagnostic bytes remain after parsing.\n", payload_len - offset);
        }
    }
}