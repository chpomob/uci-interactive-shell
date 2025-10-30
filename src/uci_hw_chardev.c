#include "../include/uci_hw_chardev.h"
#include "../include/uci.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <poll.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>

// Initialize character device interface
int uci_hw_chardev_init(uci_hw_chardev_t* hw, const char* device_path) {
    if (!hw || !device_path) {
        fprintf(stderr, "Error: Invalid parameters for uci_hw_chardev_init\n");
        return -1;
    }
    
    // Initialize structure
    memset(hw, 0, sizeof(uci_hw_chardev_t));
    hw->fd = -1;
    hw->is_open = 0;
    hw->verbose = 0;
    
    // Copy device path
    strncpy(hw->device_path, device_path, sizeof(hw->device_path) - 1);
    hw->device_path[sizeof(hw->device_path) - 1] = '\0';
    
    if (hw->verbose) {
        printf("Initialized UCI chardev interface with device: %s\n", hw->device_path);
    }
    
    return 0;
}

// Open character device
int uci_hw_chardev_open(uci_hw_chardev_t* hw) {
    if (!hw) {
        fprintf(stderr, "Error: Invalid parameters for uci_hw_chardev_open\n");
        return -1;
    }
    
    if (hw->is_open) {
        if (hw->verbose) {
            printf("Device already open: %s\n", hw->device_path);
        }
        return 0; // Already open
    }
    
    if (hw->verbose) {
        printf("Opening UCI character device: %s\n", hw->device_path);
    }
    
    // Open the device file for read/write, non-blocking
    hw->fd = open(hw->device_path, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (hw->fd < 0) {
        if (hw->verbose) {
            perror("Failed to open UCI character device");
        }
        return -1;
    }
    
    // Configure the device for raw binary communication if it's a terminal device
    struct termios tty;
    if (tcgetattr(hw->fd, &tty) == 0) {
        if (hw->verbose) {
            printf("Configuring terminal device settings for raw binary communication\n");
        }
        
        // Set raw mode (no processing of input/output)
        cfmakeraw(&tty);
        
        // Set speed to 115200 baud (typical for UWB devices)
        cfsetispeed(&tty, B115200);
        cfsetospeed(&tty, B115200);
        
        // Set 8N1 (8 data bits, no parity, 1 stop bit)
        tty.c_cflag &= ~PARENB;  // No parity
        tty.c_cflag &= ~CSTOPB;  // 1 stop bit
        tty.c_cflag &= ~CSIZE;   // Clear data bits
        tty.c_cflag |= CS8;      // 8 data bits
        
        // No flow control
#ifdef CRTSCTS
        tty.c_cflag &= ~CRTSCTS;
#endif
        
        // Apply settings
        if (tcsetattr(hw->fd, TCSANOW, &tty) != 0) {
            if (hw->verbose) {
                perror("tcsetattr failed");
            }
            // Continue anyway
        }
    } else {
        if (hw->verbose) {
            printf("Device is not a terminal, skipping terminal configuration\n");
        }
    }
    
    hw->is_open = 1;
    
    if (hw->verbose) {
        printf("Successfully opened UCI character device (fd=%d)\n", hw->fd);
    }
    
    return 0;
}

// Close character device
int uci_hw_chardev_close(uci_hw_chardev_t* hw) {
    if (!hw) {
        fprintf(stderr, "Error: Invalid parameters for uci_hw_chardev_close\n");
        return -1;
    }
    
    if (hw->verbose) {
        printf("Closing UCI character device (fd=%d)\n", hw->fd);
    }
    
    if (hw->fd >= 0) {
        close(hw->fd);
        hw->fd = -1;
    }
    
    hw->is_open = 0;
    
    return 0;
}

static int wait_for_fd(int fd, short events, int timeout_ms) {
    struct pollfd pfd = {
        .fd = fd,
        .events = events,
    };

    int ret;
    do {
        ret = poll(&pfd, 1, timeout_ms);
    } while (ret < 0 && errno == EINTR);

    return ret;
}

static int read_exact(uci_hw_chardev_t* hw, unsigned char* buffer, size_t length, int timeout_ms) {
    size_t offset = 0;

    while (offset < length) {
        int wait_ret = wait_for_fd(hw->fd, POLLIN, timeout_ms);
        if (wait_ret == 0) {
            // Timeout
            return (offset == 0) ? 0 : -1;
        } else if (wait_ret < 0) {
            return -1;
        }

        ssize_t bytes_read = read(hw->fd, buffer + offset, length - offset);
        if (bytes_read < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                continue;
            }
            return -1;
        }

        if (bytes_read == 0) {
            // Device closed
            return -1;
        }

        offset += (size_t)bytes_read;
    }

    return (int)offset;
}

static int write_exact(uci_hw_chardev_t* hw, const unsigned char* data, size_t length, int timeout_ms) {
    size_t offset = 0;

    while (offset < length) {
        int wait_ret = wait_for_fd(hw->fd, POLLOUT, timeout_ms);
        if (wait_ret <= 0) {
            return (wait_ret == 0 && offset == 0) ? 0 : -1;
        }

        ssize_t written = write(hw->fd, data + offset, length - offset);
        if (written < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                continue;
            }
            return -1;
        }

        if (written == 0) {
            return -1;
        }

        offset += (size_t)written;
    }

    return (int)offset;
}

// Send UCI packet to character device
int uci_hw_chardev_send(uci_hw_chardev_t* hw, const unsigned char* data, size_t length) {
    if (!hw || !data || length == 0) {
        fprintf(stderr, "Error: Invalid parameters for uci_hw_chardev_send\n");
        return -1;
    }
    
    if (!hw->is_open || hw->fd < 0) {
        fprintf(stderr, "Error: Device not open for sending\n");
        return -1;
    }
    
    if (hw->verbose) {
        printf("Sending %zu bytes to UCI device (%s):\n  ", length, hw->device_path);
        for (size_t i = 0; i < length; i++) {
            printf("%02X ", data[i]);
        }
        printf("\n");
    }
    
    int written = write_exact(hw, data, length, 100);
    if (written < 0) {
        if (hw->verbose) {
            perror("Failed to write to UCI device");
        }
        return -1;
    }

    // Flush output to ensure data is sent when dealing with TTYs
    if (tcdrain(hw->fd) != 0 && hw->verbose) {
        perror("tcdrain failed");
    }

    return written;
}

// Receive UCI packet from character device with timeout
int uci_hw_chardev_receive(uci_hw_chardev_t* hw, unsigned char* buffer, size_t buffer_size, int timeout_ms) {
    if (!hw || !buffer || buffer_size == 0) {
        fprintf(stderr, "Error: Invalid parameters for uci_hw_chardev_receive\n");
        return -1;
    }
    
    if (!hw->is_open || hw->fd < 0) {
        fprintf(stderr, "Error: Device not open for receiving\n");
        return -1;
    }
    
    if (buffer_size < sizeof(struct uci_packet_header)) {
        errno = EMSGSIZE;
        return -1;
    }

    int header_bytes = read_exact(hw, buffer, sizeof(struct uci_packet_header), timeout_ms);
    if (header_bytes <= 0) {
        return header_bytes;
    }

    if ((size_t)header_bytes != sizeof(struct uci_packet_header)) {
        if (hw->verbose) {
            printf("Short read while fetching UCI header (%d bytes)\n", header_bytes);
        }
        return -1;
    }

    const struct uci_packet_header* header = (const struct uci_packet_header*)buffer;
    unsigned char payload_len = header->payload_len;

    if (sizeof(struct uci_packet_header) + payload_len > buffer_size) {
        if (hw->verbose) {
            printf("Payload length %u exceeds buffer capacity %zu\n",
                   payload_len, buffer_size);
        }
        return -1;
    }

    if (payload_len > 0) {
        int payload_bytes = read_exact(hw, buffer + sizeof(struct uci_packet_header), payload_len, timeout_ms);
        if (payload_bytes <= 0) {
            return -1;
        }

        if ((unsigned int)payload_bytes != payload_len) {
            if (hw->verbose) {
                printf("Short read while fetching payload (%d/%u bytes)\n",
                       payload_bytes, payload_len);
            }
            return -1;
        }
    }

    if (hw->verbose) {
        printf("Received %zu bytes from UCI device (%s):\n  ",
               sizeof(struct uci_packet_header) + payload_len, hw->device_path);
        for (size_t i = 0; i < sizeof(struct uci_packet_header) + payload_len; i++) {
            printf("%02X ", buffer[i]);
        }
        printf("\n");
    }

    return (int)(sizeof(struct uci_packet_header) + payload_len);
}

// Set verbose mode
void uci_hw_chardev_set_verbose(uci_hw_chardev_t* hw, int verbose) {
    if (hw) {
        hw->verbose = verbose ? 1 : 0;
    }
}

// Check if device is connected
int uci_hw_chardev_is_connected(uci_hw_chardev_t* hw) {
    if (!hw) {
        return 0;
    }
    return hw->is_open && hw->fd >= 0;
}

// Get device path
const char* uci_hw_chardev_get_device_path(uci_hw_chardev_t* hw) {
    if (!hw) {
        return NULL;
    }
    return hw->device_path;
}
