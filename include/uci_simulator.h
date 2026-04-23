#ifndef UCI_SIMULATOR_H
#define UCI_SIMULATOR_H

#include <stddef.h>
#include <stdint.h>

void uci_simulator_reset_runtime(void);
void uci_simulator_dispatch_command(uint8_t gid,
                                    uint8_t oid,
                                    const unsigned char *payload,
                                    size_t payload_len);
void uci_simulator_handle_data_message_send(const unsigned char *payload,
                                            size_t payload_len);
void enqueue_notification(unsigned char gid,
                          unsigned char oid,
                          const unsigned char *payload,
                          size_t payload_len);

#endif
