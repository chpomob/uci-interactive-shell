#include <stdio.h>
#include <string.h>
#include "../include/uci_ui.h"
#include "../include/uci_ui_main_patch.h"

void ui_print_welcome_message() {
    if (ui_color_enabled) {
        printf("%s%s%s", ANSI_BG_BLUE, ANSI_COLOR_BRIGHT_WHITE, ANSI_BOLD);
        printf("==========================================\n");
        printf("  UCI Interactive Shell - Enhanced UI     \n");
        printf("==========================================%s\n", ANSI_RESET);
    } else {
        printf("==========================================\n");
        printf("  UCI Interactive Shell                  \n");
        printf("==========================================\n");
    }
    
    if (ui_color_enabled) {
        printf("%s%s", ANSI_COLOR_BRIGHT_CYAN, ANSI_BOLD);
        printf("Welcome to the UCI (Ultra-Wideband Control Interface) Shell\n");
        printf("Type 'help' for available commands or 'quit' to exit%s\n\n", ANSI_RESET);
    } else {
        printf("Welcome to the UCI (Ultra-Wideband Control Interface) Shell\n");
        printf("Type 'help' for available commands or 'quit' to exit\n\n");
    }
}