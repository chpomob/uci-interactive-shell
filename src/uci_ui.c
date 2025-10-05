#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/uci_ui.h"

// Global flag to enable/disable color output
int ui_color_enabled = 1;

void ui_enable_color(int enable) {
    ui_color_enabled = enable;
}

void ui_print_header(const char* text) {
    if (ui_color_enabled) {
        printf("%s%s%s%s%s\n", 
               ANSI_BOLD, ANSI_COLOR_BRIGHT_CYAN, ANSI_BG_BLUE, text, ANSI_RESET);
    } else {
        printf("%s\n", text);
    }
}

void ui_print_subheader(const char* text) {
    if (ui_color_enabled) {
        printf("%s%s%s\n", ANSI_BOLD, ANSI_COLOR_BRIGHT_BLUE, text);
    } else {
        printf("%s\n", text);
    }
}

void ui_print_success(const char* text) {
    if (ui_color_enabled) {
        printf("%s%s✓ %s%s\n", ANSI_COLOR_GREEN, ANSI_BOLD, ANSI_RESET, text);
    } else {
        printf("✓ %s\n", text);
    }
}

void ui_print_error(const char* text) {
    if (ui_color_enabled) {
        printf("%s%s✗ Error: %s%s\n", ANSI_COLOR_RED, ANSI_BOLD, ANSI_RESET, text);
    } else {
        printf("✗ Error: %s\n", text);
    }
}

void ui_print_warning(const char* text) {
    if (ui_color_enabled) {
        printf("%s%s⚠ Warning: %s%s\n", ANSI_COLOR_YELLOW, ANSI_BOLD, ANSI_RESET, text);
    } else {
        printf("⚠ Warning: %s\n", text);
    }
}

void ui_print_info(const char* text) {
    if (ui_color_enabled) {
        printf("%sℹ %s%s\n", ANSI_COLOR_BRIGHT_CYAN, ANSI_RESET, text);
    } else {
        printf("ℹ %s\n", text);
    }
}

void ui_print_debug(const char* text) {
    if (ui_color_enabled) {
        printf("%s%sDEBUG: %s%s\n", ANSI_COLOR_BRIGHT_BLACK, ANSI_BOLD, ANSI_RESET, text);
    } else {
        printf("DEBUG: %s\n", text);
    }
}

void ui_print_command(const char* command) {
    if (ui_color_enabled) {
        printf("%s%sCommand:%s %s\n", ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET, command);
    } else {
        printf("Command: %s\n", command);
    }
}

void ui_print_response(const char* response) {
    if (ui_color_enabled) {
        printf("%s%sResponse:%s %s\n", ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_RESET, response);
    } else {
        printf("Response: %s\n", response);
    }
}

void ui_print_notification(const char* notification) {
    if (ui_color_enabled) {
        printf("%s%sNotification:%s %s\n", ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, notification);
    } else {
        printf("Notification: %s\n", notification);
    }
}

void ui_print_packet_analysis(unsigned char* packet, size_t packet_len) {
    if (!packet || packet_len == 0) {
        ui_print_error("Empty packet for analysis");
        return;
    }
    
    if (ui_color_enabled) {
        printf("%s%s=== Packet Analysis ===%s\n", 
               ANSI_BOLD, ANSI_COLOR_BRIGHT_WHITE, ANSI_RESET);
    } else {
        printf("=== Packet Analysis ===\n");
    }
    
    // Print packet in hex format with color coding
    printf("Packet (%zu bytes): ", packet_len);
    for (size_t i = 0; i < packet_len; i++) {
        if (ui_color_enabled) {
            // Color code based on position/meaning
            if (i < 4) {
                // Header bytes
                printf("%s%02X%s ", ANSI_COLOR_BRIGHT_CYAN, packet[i], ANSI_RESET);
            } else if (i == 4) {
                // Status byte
                if (packet[i] == 0x00) {
                    printf("%s%02X%s ", ANSI_COLOR_GREEN, packet[i], ANSI_RESET);
                } else {
                    printf("%s%02X%s ", ANSI_COLOR_RED, packet[i], ANSI_RESET);
                }
            } else {
                // Payload bytes
                printf("%s%02X%s ", ANSI_COLOR_WHITE, packet[i], ANSI_RESET);
            }
        } else {
            printf("%02X ", packet[i]);
        }
    }
    printf("\n");
}

void ui_print_hex_dump(unsigned char* data, size_t len, const char* prefix) {
    if (!data || len == 0) {
        return;
    }
    
    size_t bytes_per_line = 16;
    
    for (size_t offset = 0; offset < len; offset += bytes_per_line) {
        // Print prefix and offset
        if (prefix) {
            if (ui_color_enabled) {
                printf("%s%s%s ", ANSI_COLOR_BRIGHT_BLACK, prefix, ANSI_RESET);
            } else {
                printf("%s ", prefix);
            }
        }
        
        if (ui_color_enabled) {
            printf("%s%08zx%s  ", ANSI_COLOR_BRIGHT_BLACK, offset, ANSI_RESET);
        } else {
            printf("%08zx  ", offset);
        }
        
        // Print hex bytes
        size_t line_end = (offset + bytes_per_line > len) ? len : offset + bytes_per_line;
        
        for (size_t i = offset; i < line_end; i++) {
            if (ui_color_enabled) {
                // Color code based on byte value
                if (data[i] == 0x00) {
                    printf("%s%02x%s ", ANSI_COLOR_BRIGHT_BLACK, data[i], ANSI_RESET);
                } else if (data[i] < 0x20 || data[i] >= 0x7F) {
                    printf("%s%02x%s ", ANSI_COLOR_BRIGHT_RED, data[i], ANSI_RESET);
                } else {
                    printf("%s%02x%s ", ANSI_COLOR_BRIGHT_GREEN, data[i], ANSI_RESET);
                }
            } else {
                printf("%02x ", data[i]);
            }
        }
        
        // Pad with spaces if line is shorter
        for (size_t i = line_end; i < offset + bytes_per_line; i++) {
            printf("   ");
        }
        
        // Print ASCII representation
        printf(" |");
        for (size_t i = offset; i < line_end; i++) {
            if (ui_color_enabled) {
                if (data[i] >= 0x20 && data[i] < 0x7F) {
                    printf("%c", data[i]);
                } else {
                    printf("%s.%s", ANSI_COLOR_BRIGHT_RED, ANSI_RESET);
                }
            } else {
                if (data[i] >= 0x20 && data[i] < 0x7F) {
                    printf("%c", data[i]);
                } else {
                    printf(".");
                }
            }
        }
        printf("|\n");
    }
}