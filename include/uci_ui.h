#ifndef UCI_UI_H
#define UCI_UI_H

// ANSI Color Codes for terminal output
#define ANSI_COLOR_BLACK   "\x1b[30m"
#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_WHITE   "\x1b[37m"
#define ANSI_COLOR_RESET   "\x1b[0m"
#define ANSI_RESET         "\x1b[0m"

// Bright Colors
#define ANSI_COLOR_BRIGHT_BLACK   "\x1b[90m"
#define ANSI_COLOR_BRIGHT_RED     "\x1b[91m"
#define ANSI_COLOR_BRIGHT_GREEN   "\x1b[92m"
#define ANSI_COLOR_BRIGHT_YELLOW  "\x1b[93m"
#define ANSI_COLOR_BRIGHT_BLUE    "\x1b[94m"
#define ANSI_COLOR_BRIGHT_MAGENTA "\x1b[95m"
#define ANSI_COLOR_BRIGHT_CYAN    "\x1b[96m"
#define ANSI_COLOR_BRIGHT_WHITE   "\x1b[97m"

// Background Colors
#define ANSI_BG_BLACK   "\x1b[40m"
#define ANSI_BG_RED     "\x1b[41m"
#define ANSI_BG_GREEN   "\x1b[42m"
#define ANSI_BG_YELLOW  "\x1b[43m"
#define ANSI_BG_BLUE    "\x1b[44m"
#define ANSI_BG_MAGENTA "\x1b[45m"
#define ANSI_BG_CYAN    "\x1b[46m"
#define ANSI_BG_WHITE   "\x1b[47m"

// Formatting
#define ANSI_BOLD       "\x1b[1m"
#define ANSI_UNDERLINE  "\x1b[4m"
#define ANSI_BLINK      "\x1b[5m"
#define ANSI_REVERSE    "\x1b[7m"
#define ANSI_HIDDEN     "\x1b[8m"

// Reset formatting
#define ANSI_FORMATTING_RESET "\x1b[21;24;25;27;28m"

// UI Enhancement Functions
void ui_print_header(const char* text);
void ui_print_subheader(const char* text);
void ui_print_success(const char* text);
void ui_print_error(const char* text);
void ui_print_warning(const char* text);
void ui_print_info(const char* text);
void ui_print_debug(const char* text);
void ui_print_packet_analysis(unsigned char* packet, size_t packet_len);
void ui_print_hex_dump(unsigned char* data, size_t len, const char* prefix);
void ui_print_command(const char* command);
void ui_print_response(const char* response);
void ui_print_notification(const char* notification);

// Additional UI enhancement functions for specific use cases
void ui_print_sending_uci_packet(const char* destination);
void ui_print_received_uci_packet();
void ui_print_simulation_started(const char* simulation_name);
void ui_print_simulation_completed(const char* simulation_name);
void ui_print_hardware_mode_initialized(const char* device_path);
void ui_print_hardware_mode_not_initialized();
void ui_print_command_not_found(const char* command);

// Conditional color output (can be disabled)
extern int ui_color_enabled;

// Enable/disable color output
void ui_enable_color(int enable);

// Welcome message
void ui_print_welcome_message(void);

#endif // UCI_UI_H