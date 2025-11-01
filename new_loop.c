    // Parse ranging measurements
    int offset = 25; // Starting after the fixed header fields
    for (uint8_t i = 0; i < num_measurements; i++) {
        if (offset >= payload_len) {
            if (ui_color_enabled) {
                printf("      %s%sWARNING:%s No data left while expecting measurement %u\n", 
                       ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, i + 1);
            } else {
                printf("      WARNING: No data left while expecting measurement %u\n", i + 1);
            }
            break;
        }

        if (ui_color_enabled) {
            printf("    %s%sMeasurement %u:%s\n", ANSI_COLOR_BRIGHT_CYAN, ANSI_BOLD, i + 1, ANSI_RESET);
        } else {
            printf("    Measurement %u:\n", i + 1);
        }

        bool parsed = false;
        switch (ranging_measurement_type) {
            case 0x00:
                parsed = decode_range_measurement_owr_ul_tdoa(payload, payload_len, &offset, mac_addressing_mode ? 8 : 2, i + 1);
                break;
            case 0x01:
                parsed = decode_range_measurement_twr(payload, payload_len, &offset, mac_addressing_mode ? 8 : 2, i + 1);
                break;
            case 0x02:
                parsed = decode_range_measurement_owr_dl_tdoa(payload, payload_len, &offset, mac_addressing_mode ? 8 : 2, i + 1);
                break;
            case 0x03:
                parsed = decode_range_measurement_owr_aoa(payload, payload_len, &offset, mac_addressing_mode ? 8 : 2, i + 1);
                break;
            case 0x04:
                parsed = decode_range_measurement_ccc_controller(payload, payload_len, &offset, i + 1);
                break;
            case 0x05:
                parsed = decode_range_measurement_ccc_controlee(payload, payload_len, &offset, i + 1);
                break;
            case 0x06:
                parsed = decode_range_measurement_owr_dl_tdoa_v2(payload, payload_len, &offset, mac_addressing_mode ? 8 : 2, i + 1);
                break;
            default:
                parsed = decode_range_measurement_unknown(i + 1, ranging_measurement_type);
                offset = payload_len;
                break;
        }

        if (!parsed) {
            if (ui_color_enabled) {
                printf("      %s%sWARNING:%s Stopping measurement parsing after entry %u\n", 
                       ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, i + 1);
            } else {
                printf("      WARNING: Stopping measurement parsing after entry %u\n", i + 1);
            }
            break;
        }
    }

    // Check for any remaining vendor-specific data
    if (offset < payload_len) {
        if (ui_color_enabled) {
            printf("    %s%sVendor-specific Data:%s %d bytes\n", ANSI_COLOR_BRIGHT_BLACK, ANSI_BOLD, ANSI_RESET, payload_len - offset);
        } else {
            printf("    Vendor-specific Data: %d bytes\n", payload_len - offset);
        }
        print_hex_data(4,
                       "Trailing Vendor Data",
                       &payload[offset],
                       payload_len - offset,
                       32);
    }