#ifndef UCI_PACKET_ANALYZER_VENDOR_H
#define UCI_PACKET_ANALYZER_VENDOR_H

#include <stdint.h>

void uci_packet_analyzer_handle_qorvo_ext2(uint8_t mt,
                                           uint8_t opcode,
                                           unsigned char* payload_ptr,
                                           unsigned char payload_len_field);

#endif /* UCI_PACKET_ANALYZER_VENDOR_H */
