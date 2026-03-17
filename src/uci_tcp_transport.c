#include "../include/uci_tcp_transport.h"

#include "../include/uci.h"

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

static int g_tcp_fd = -1;
static char g_tcp_endpoint[128] = "";

static int wait_for_tcp_data(int timeout_ms) {
    fd_set read_fds;
    struct timeval timeout;

    if (g_tcp_fd < 0) {
        return -1;
    }

    FD_ZERO(&read_fds);
    FD_SET(g_tcp_fd, &read_fds);

    timeout.tv_sec = timeout_ms / 1000;
    timeout.tv_usec = (timeout_ms % 1000) * 1000;

    return select(g_tcp_fd + 1, &read_fds, NULL, NULL, timeout_ms >= 0 ? &timeout : NULL);
}

static int recv_full(unsigned char* buffer, size_t length, int timeout_ms) {
    size_t total = 0;

    while (total < length) {
        int ready = wait_for_tcp_data(timeout_ms);
        ssize_t rc;

        if (ready <= 0) {
            return (total > 0) ? -1 : ready;
        }

        rc = recv(g_tcp_fd, buffer + total, length - total, 0);
        if (rc <= 0) {
            return -1;
        }
        total += (size_t)rc;
    }

    return (int)total;
}

int uci_tcp_transport_connect(const char* host, uint16_t port) {
    struct sockaddr_in addr;
    int fd;

    if (!host || host[0] == '\0') {
        return -1;
    }

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        return -1;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (inet_pton(AF_INET, host, &addr.sin_addr) != 1) {
        close(fd);
        return -1;
    }

    if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) != 0) {
        close(fd);
        return -1;
    }

    uci_tcp_transport_disconnect();
    g_tcp_fd = fd;
    snprintf(g_tcp_endpoint, sizeof(g_tcp_endpoint), "%s:%u", host, (unsigned)port);
    return 0;
}

void uci_tcp_transport_disconnect(void) {
    if (g_tcp_fd >= 0) {
        close(g_tcp_fd);
        g_tcp_fd = -1;
    }
    g_tcp_endpoint[0] = '\0';
}

int uci_tcp_transport_is_connected(void) {
    return g_tcp_fd >= 0;
}

const char* uci_tcp_transport_get_endpoint(void) {
    return g_tcp_endpoint;
}

int uci_tcp_transport_get_fd(void) {
    return g_tcp_fd;
}

int uci_tcp_transport_send_packet(const unsigned char* packet, size_t packet_len) {
    size_t total = 0;

    if (g_tcp_fd < 0 || !packet || packet_len == 0) {
        return -1;
    }

    while (total < packet_len) {
        ssize_t rc = send(g_tcp_fd, packet + total, packet_len - total, 0);
        if (rc <= 0) {
            return -1;
        }
        total += (size_t)rc;
    }

    return (int)total;
}

int uci_tcp_transport_receive_packet(unsigned char* buffer, size_t buffer_size, int timeout_ms) {
    unsigned char header_bytes[sizeof(struct uci_packet_header)];
    struct uci_packet_header* header = (struct uci_packet_header*)header_bytes;
    uci_header_fields_t fields;
    size_t payload_len;

    if (!buffer || buffer_size < sizeof(struct uci_packet_header) || g_tcp_fd < 0) {
        return -1;
    }

    {
        int header_rc = recv_full(header_bytes, sizeof(header_bytes), timeout_ms);
        if (header_rc <= 0) {
            return header_rc;
        }
    }

    if (uci_extract_header_fields_safe(header, &fields) != UCI_SUCCESS) {
        return -1;
    }

    payload_len = fields.payload_length;
    if (buffer_size < sizeof(struct uci_packet_header) + payload_len) {
        return -1;
    }

    memcpy(buffer, header_bytes, sizeof(header_bytes));
    if (payload_len > 0) {
        int payload_rc = recv_full(buffer + sizeof(struct uci_packet_header), payload_len, timeout_ms);
        if (payload_rc <= 0) {
            return -1;
        }
    }

    return (int)(sizeof(struct uci_packet_header) + payload_len);
}
