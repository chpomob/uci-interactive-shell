#ifndef UCI_FUNCTIONS_H
#define UCI_FUNCTIONS_H

#include "uci.h"

#include <stddef.h>

// Session management helper function declarations
void init_uci_sessions();
int find_free_session_slot();
int find_session_by_id(unsigned int session_id);
void store_session_config(int session_idx, unsigned char cfg_id, unsigned char* value, unsigned char len);
int get_session_config(int session_idx, unsigned char cfg_id, unsigned char* value, unsigned char* len);

// Notification handler function declarations
void handle_session_info_ntf(unsigned char* payload, int payload_len);

void send_uci_command(unsigned char mt, unsigned char pbf, unsigned char gid, unsigned char oid, unsigned char* payload, int payload_len);
void parse_uci_packet(unsigned char* packet, size_t packet_len);

#endif // UCI_FUNCTIONS_H
