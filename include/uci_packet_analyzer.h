#ifndef UCI_PACKET_ANALYZER_H
#define UCI_PACKET_ANALYZER_H

#include <stddef.h>
#include "uci.h"

/**
 * Core packet analysis function that works with both colored and non-colored output
 * This function performs all packet analysis and uses ui_color_enabled global to decide formatting
 */
void uci_analyze_packet_core(unsigned char* packet, size_t packet_len);

/*
 * Analyze a packet view that has already been decoded into header fields plus
 * payload bytes. This allows segmented-message reassembly to share the same
 * analyzer path without fabricating a fake on-wire header.
 */
void uci_analyze_packet_fields(const uci_header_fields_t* header_fields,
                               const unsigned char* header_bytes,
                               const unsigned char* payload,
                               size_t payload_len,
                               int segmented_reassembled);

/**
 * Enhanced error analysis function for UCI status codes based on QM SDK patterns
 * Provides detailed interpretation of UCI status codes with context
 */
void enhanced_error_analysis(unsigned char status_code);

#endif // UCI_PACKET_ANALYZER_H
