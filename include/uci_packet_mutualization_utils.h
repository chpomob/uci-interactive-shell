#ifndef UCI_PACKET_MUTUALIZATION_UTILS_H
#define UCI_PACKET_MUTUALIZATION_UTILS_H

#include "uci_packet_structures.h"
#include "uci_functions.h"  // Include for send_uci_command declaration

// Helper function to create and send a UCI packet using mutualized structures
extern unsigned char* uci_create_packet_from_struct(uint8_t mt, uint8_t pbf, uint8_t gid, uint8_t oid, 
                                                    const void* payload_struct, size_t* packet_len);

// Helper function to decode a UCI packet payload into a mutualized structure
extern uci_error_t uci_decode_packet_to_struct(const unsigned char* packet, size_t packet_len, void* payload_struct, 
                                               uint8_t gid, uint8_t oid);

// Helper functions for creating specific packet structures
extern uci_error_t uci_create_session_init_struct(uci_session_init_payload_t* out_struct, 
                                                  uint32_t session_id, uint8_t session_type);

extern uci_error_t uci_create_session_deinit_struct(uci_session_deinit_payload_t* out_struct, 
                                                    uint32_t session_id);

extern uci_error_t uci_create_session_control_struct(uci_session_control_payload_t* out_struct, 
                                                     uint32_t session_id);

extern uci_error_t uci_create_session_get_ranging_count_struct(uci_session_get_ranging_count_payload_t* out_struct, 
                                                               uint32_t session_id, uint16_t ranging_count);

extern uci_error_t uci_create_session_logical_link_struct(uci_session_logical_link_payload_t* out_struct, 
                                                          uint32_t session_id, uint8_t link_id, uint8_t mode);

// Helper function to print payload information for debugging
extern void uci_print_payload_struct(uint8_t gid, uint8_t oid, const void* payload_struct);

// Function to integrate with the existing send_uci_command function using mutualized structures
extern void uci_send_command_with_struct(uint8_t mt, uint8_t pbf, uint8_t gid, uint8_t oid, 
                                         const void* payload_struct);

#endif // UCI_PACKET_MUTUALIZATION_UTILS_H