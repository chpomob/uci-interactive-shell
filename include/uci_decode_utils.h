#ifndef UCI_DECODE_UTILS_H
#define UCI_DECODE_UTILS_H

void uci_print_named_u8_line(const char* label, unsigned char value, const char* text);
void uci_print_status_line(const char* label, unsigned char status);
void uci_print_session_state_line(const char* label, unsigned char session_state);
void uci_print_session_reason_line(const char* label, unsigned char reason_code);
void uci_print_device_state_line(const char* label, unsigned char device_state);
void uci_print_status_analysis(unsigned char status_code, int color_enabled);

#endif /* UCI_DECODE_UTILS_H */
