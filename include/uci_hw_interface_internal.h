#ifndef UCI_HW_INTERFACE_INTERNAL_H
#define UCI_HW_INTERFACE_INTERNAL_H

#include <stddef.h>

/*
 * Internal transport helpers shared with tests. These are not part of the
 * user-facing hardware API, but they let the regression suite exercise the
 * same fragmentation/reassembly logic used by the hardware path.
 */
void uci_hw_interface_reset_transport_state_for_test(void);
int uci_hw_interface_process_fragment_for_test(const unsigned char* fragment,
                                               size_t fragment_len,
                                               unsigned char* out_buffer,
                                               size_t out_capacity);
int uci_hw_interface_build_fragment_for_test(const unsigned char* packet,
                                             size_t packet_len,
                                             size_t offset,
                                             unsigned char* fragment,
                                             size_t fragment_capacity,
                                             size_t* fragment_size,
                                             size_t* next_offset);

#endif
