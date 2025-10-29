#ifndef UCI_FUNCTIONS_H
#define UCI_FUNCTIONS_H

#include "uci.h"

#include <stddef.h>

// Session management helper function declarations
void init_uci_sessions();
int find_free_session_slot();
int find_session_by_id(uci_uint32 session_id);
int find_session_by_handle(uci_uint32 session_handle);
int find_session_by_token_or_id(uci_uint32 identifier);
int get_allocated_session_count();
void store_session_config(int session_idx, uci_uint8 cfg_id, uci_uint8* value, uci_uint8 len);
int get_session_config(int session_idx, uci_uint8 cfg_id, uci_uint8* value, uci_uint8* len);
void increment_session_ranging_count(int session_idx);

// Notification handler function declarations
void handle_session_info_ntf(uci_uint8* payload, int payload_len);
void uci_process_pending_notifications();

size_t uci_build_core_capabilities_payload(uci_uint8* buffer, size_t max_len);
typedef int (*uci_command_capture_hook_t)(uci_uint8 mt,
                                          uci_uint8 pbf,
                                          uci_uint8 gid,
                                          uci_uint8 oid,
                                          const uci_uint8* payload,
                                          int payload_len);

typedef int (*uci_data_message_hook_t)(const unsigned char* payload, size_t payload_len);

typedef void (*uci_notification_callback_t)(const struct uci_packet_header* header,
                                            const uci_header_fields_t* fields,
                                            const unsigned char* payload,
                                            size_t payload_len);

void uci_set_command_capture_hook(uci_command_capture_hook_t hook);
void uci_set_data_message_hook(uci_data_message_hook_t hook);
void uci_set_notification_callback(uci_notification_callback_t callback);

void send_uci_command(uci_uint8 mt, uci_uint8 pbf, uci_uint8 gid, uci_uint8 oid, uci_uint8* payload, int payload_len);
void parse_uci_packet(uci_uint8* packet, size_t packet_len);

#endif // UCI_FUNCTIONS_H
