#include "../include/uci_decode_utils.h"
#include "../include/uci_packet_utils.h"
#include "../include/uci_ui.h"
#include <stdio.h>
#include <string.h>

void uci_print_named_u8_line(const char* label, unsigned char value, const char* text) {
    printf("      %s: 0x%02X (%s)\n", label, value, text);
}

void uci_print_status_line(const char* label, unsigned char status) {
    uci_print_named_u8_line(label, status, uci_status_to_string(status));
}

void uci_print_data_transfer_status_line(const char* label, unsigned char status) {
    const uci_lookup_entry_t* entry = uci_lookup_data_transfer_status(status);
    uci_print_named_u8_line(label, status, entry ? entry->label : "UNKNOWN");
}

void uci_print_session_state_line(const char* label, unsigned char session_state) {
    uci_print_named_u8_line(label, session_state, uci_session_state_to_string(session_state));
}

void uci_print_session_reason_line(const char* label, unsigned char reason_code) {
    uci_print_named_u8_line(label, reason_code, uci_session_reason_to_string(reason_code));
}

void uci_print_device_state_line(const char* label, unsigned char device_state) {
    uci_print_named_u8_line(label, device_state, uci_device_state_to_string(device_state));
}

void uci_print_status_analysis(unsigned char status_code, int color_enabled) {
    const char *status_label = uci_status_to_string(status_code);
    const char *status_description = uci_status_description(status_code);

    if (color_enabled) {
        printf("  %s%sStatus Code Analysis:%s\n", ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET);
        printf("    %sCode: 0x%02X%s - ", ANSI_COLOR_BRIGHT_WHITE, status_code, ANSI_RESET);

        if (status_code == UCI_STATUS_OK) {
            printf("%s%s - %s%s\n", ANSI_COLOR_BRIGHT_GREEN, status_label, status_description, ANSI_RESET);
        } else if (strcmp(status_label, "UNKNOWN") == 0) {
            printf("%sUNKNOWN - Status code 0x%02X%s\n", ANSI_COLOR_BRIGHT_BLACK, status_code, ANSI_RESET);
        } else {
            printf("%s%s - %s%s\n", ANSI_COLOR_RED, status_label, status_description, ANSI_RESET);
        }
        return;
    }

    printf("  Status Code Analysis:\n");
    if (strcmp(status_label, "UNKNOWN") == 0) {
        printf("    Code: 0x%02X - UNKNOWN\n", status_code);
    } else {
        printf("    Code: 0x%02X - %s - %s\n", status_code, status_label, status_description);
    }
}
