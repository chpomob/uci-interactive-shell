#ifndef UCI_UI_MAIN_PATCH_H
#define UCI_UI_MAIN_PATCH_H

// This patch adds UI enhancements to the main UCI application
// It includes colorized output, improved formatting, and better user experience

#include "../include/uci_ui.h"

// Function prototypes for UI-enhanced output
void ui_print_welcome_message();
void ui_print_hardware_mode_initialized(const char* device_path);
void ui_print_hardware_mode_not_initialized();
void ui_print_command_not_found(const char* command);
void ui_print_sending_uci_packet(const char* destination);
void ui_print_received_uci_packet();
void ui_print_simulation_started(const char* simulation_name);
void ui_print_simulation_completed(const char* simulation_name);

#endif // UCI_UI_MAIN_PATCH_H