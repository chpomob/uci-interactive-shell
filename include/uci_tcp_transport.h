#ifndef UCI_TCP_TRANSPORT_H
#define UCI_TCP_TRANSPORT_H

#include <stddef.h>
#include <stdint.h>

int uci_tcp_transport_connect(const char* host, uint16_t port);
void uci_tcp_transport_disconnect(void);
int uci_tcp_transport_is_connected(void);
const char* uci_tcp_transport_get_endpoint(void);
int uci_tcp_transport_get_fd(void);
int uci_tcp_transport_send_packet(const unsigned char* packet, size_t packet_len);
int uci_tcp_transport_receive_packet(unsigned char* buffer, size_t buffer_size, int timeout_ms);

#endif
