#ifndef UCI_SESSION_STORE_H
#define UCI_SESSION_STORE_H

#include "uci.h"

void uci_session_store_reset_all(void);
void uci_session_clear_logical_links(struct uci_session* session);
uci_logical_link_entry* uci_session_find_logical_link(struct uci_session* session,
                                                      unsigned char link_id);
uci_logical_link_entry* uci_session_allocate_logical_link(struct uci_session* session,
                                                          unsigned char requested_id,
                                                          unsigned char* assigned_id);
unsigned char uci_session_add_multicast_entry(struct uci_session* session,
                                              unsigned short short_address,
                                              unsigned int subsession_id,
                                              const unsigned char* key,
                                              unsigned char key_len);
unsigned char uci_session_remove_multicast_entry(struct uci_session* session,
                                                 unsigned short short_address,
                                                 unsigned int subsession_id);

#endif // UCI_SESSION_STORE_H
