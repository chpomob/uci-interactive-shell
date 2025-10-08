#ifndef UCI_PACKET_ANALYZER_H
#define UCI_PACKET_ANALYZER_H

#include <stddef.h>

/**
 * Core packet analysis function that works with both colored and non-colored output
 * This function performs all packet analysis and uses ui_color_enabled global to decide formatting
 */
void uci_analyze_packet_core(unsigned char* packet, size_t packet_len);

#endif // UCI_PACKET_ANALYZER_H
