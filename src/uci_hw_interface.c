#include "../include/uci_hw_interface.h"
#include "../include/uci_hw_chardev.h"
#include "../include/uci.h"
#include "../include/uci_packet_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Global variables for hardware interface
static uci_hw_chardev_t g_uwb_chardev;
static int g_hw_initialized = 0;
static int g_verbose_mode = 0;
static char g_device_path[256] = "/dev/ttyUSB0";  // Default device path

#define UCI_HAL_MAX_FRAGMENT_PAYLOAD 255
#define UCI_MAX_REASSEMBLED_PAYLOAD 1024

typedef struct {
    int active;
    unsigned char mt;
    unsigned char gid;
    unsigned char opcode;
    unsigned char payload[UCI_MAX_REASSEMBLED_PAYLOAD];
    size_t payload_len;
} uci_fragment_buffer_t;

static uci_fragment_buffer_t g_fragment_buffer = {0};

#define UCI_HW_MAX_QUEUED_PACKETS 4

typedef struct {
    size_t len;
    unsigned char data[sizeof(struct uci_packet_header) + UCI_MAX_REASSEMBLED_PAYLOAD];
} uci_packet_queue_entry_t;

static uci_packet_queue_entry_t g_packet_queue[UCI_HW_MAX_QUEUED_PACKETS];
static size_t g_packet_queue_head = 0;
static size_t g_packet_queue_tail = 0;
static size_t g_packet_queue_count = 0;

static void uci_fragment_reset(void) {
    g_fragment_buffer.active = 0;
    g_fragment_buffer.payload_len = 0;
}

static void packet_queue_reset(void) {
    g_packet_queue_head = 0;
    g_packet_queue_tail = 0;
    g_packet_queue_count = 0;
}

static void uci_hw_interface_reset_state(void) {
    uci_fragment_reset();
    packet_queue_reset();
}

static int packet_queue_enqueue(const unsigned char *packet, size_t len) {
    if (len > sizeof(g_packet_queue[0].data)) {
        if (g_verbose_mode) {
            printf("Packet too large for queue (%zu bytes)\n", len);
        }
        return -1;
    }

    if (g_packet_queue_count == UCI_HW_MAX_QUEUED_PACKETS) {
        if (g_verbose_mode) {
            printf("Packet queue full; dropping oldest entry\n");
        }
        g_packet_queue_head = (g_packet_queue_head + 1) % UCI_HW_MAX_QUEUED_PACKETS;
        g_packet_queue_count--;
    }

    uci_packet_queue_entry_t *slot = &g_packet_queue[g_packet_queue_tail];
    memcpy(slot->data, packet, len);
    slot->len = len;

    g_packet_queue_tail = (g_packet_queue_tail + 1) % UCI_HW_MAX_QUEUED_PACKETS;
    g_packet_queue_count++;
    return 0;
}

static int packet_queue_dequeue(unsigned char *buffer, size_t buffer_size) {
    if (g_packet_queue_count == 0) {
        return 0;
    }

    uci_packet_queue_entry_t *slot = &g_packet_queue[g_packet_queue_head];
    if (slot->len > buffer_size) {
        if (g_verbose_mode) {
            printf("Provided buffer (%zu) smaller than queued packet (%zu)\n",
                   buffer_size, slot->len);
        }
        return -1;
    }

    memcpy(buffer, slot->data, slot->len);
    g_packet_queue_head = (g_packet_queue_head + 1) % UCI_HW_MAX_QUEUED_PACKETS;
    g_packet_queue_count--;
    return (int)slot->len;
}

static int uci_fragment_process(const unsigned char* fragment,
                                size_t fragment_len,
                                unsigned char* out_buffer,
                                size_t out_capacity) {
    if (fragment_len < sizeof(struct uci_packet_header)) {
        if (g_verbose_mode) {
            printf("Received fragment too short (%zu bytes)\n", fragment_len);
        }
        return -1;
    }

    const struct uci_packet_header* header = (const struct uci_packet_header*)fragment;
    uci_header_fields_t header_fields;
    uci_extract_header_fields_safe(header, &header_fields);

    size_t payload_len = header_fields.payload_length;
    size_t expected_len = sizeof(struct uci_packet_header) + payload_len;
    if (fragment_len < expected_len) {
        if (g_verbose_mode) {
            printf("Incomplete fragment: expected %zu bytes, got %zu\n", expected_len, fragment_len);
        }
        return -1;
    }
    if (fragment_len > expected_len && g_verbose_mode) {
        printf("Warning: fragment contains %zu surplus bytes; ignoring trailing data\n",
               fragment_len - expected_len);
    }

    unsigned char mt = header_fields.message_type;
    unsigned char gid = header_fields.group_id;
    unsigned char opcode = header_fields.opcode_id;
    unsigned char pbf = header_fields.packet_boundary;

    const unsigned char* payload_ptr = fragment + sizeof(struct uci_packet_header);

    if (g_verbose_mode) {
            printf("  Fragment header: MT=0x%02X GID=0x%02X OID=0x%02X PBF=%s len=%zu\n",
                   mt, gid, opcode,
                   (pbf == COMPLETE) ? "COMPLETE" : "NOT_COMPLETE",
                   payload_len);
    }

    if (pbf == NOT_COMPLETE) {
        if (!g_fragment_buffer.active) {
            g_fragment_buffer.active = 1;
            g_fragment_buffer.mt = mt;
            g_fragment_buffer.gid = gid;
            g_fragment_buffer.opcode = opcode;
            g_fragment_buffer.payload_len = 0;
        } else if (g_fragment_buffer.mt != mt || g_fragment_buffer.gid != gid ||
                   g_fragment_buffer.opcode != opcode) {
            if (g_verbose_mode) {
                printf("Fragment sequence mismatch; resetting reassembly buffer\n");
            }
            uci_fragment_reset();
            g_fragment_buffer.active = 1;
            g_fragment_buffer.mt = mt;
            g_fragment_buffer.gid = gid;
            g_fragment_buffer.opcode = opcode;
        }

        if (g_fragment_buffer.payload_len + payload_len > sizeof(g_fragment_buffer.payload)) {
            if (g_verbose_mode) {
                printf("Fragment payload overflow (%zu + %zu)\n",
                       g_fragment_buffer.payload_len, payload_len);
            }
            uci_fragment_reset();
            return -1;
        }
        memcpy(g_fragment_buffer.payload + g_fragment_buffer.payload_len, payload_ptr, payload_len);
        g_fragment_buffer.payload_len += payload_len;
        return 0; // waiting for more fragments
    }

    if (g_fragment_buffer.active) {
        if (g_fragment_buffer.mt != mt || g_fragment_buffer.gid != gid ||
            g_fragment_buffer.opcode != opcode) {
            if (g_verbose_mode) {
                printf("Final fragment mismatch; dropping buffered data\n");
            }
            uci_fragment_reset();
            return -1;
        }

        if (g_fragment_buffer.payload_len + payload_len > sizeof(g_fragment_buffer.payload)) {
            if (g_verbose_mode) {
                printf("Reassembled payload exceeds buffer (%zu + %zu)\n",
                       g_fragment_buffer.payload_len, payload_len);
            }
            uci_fragment_reset();
            return -1;
        }

        size_t total_len = g_fragment_buffer.payload_len + payload_len;
        if (total_len > UCI_HAL_MAX_FRAGMENT_PAYLOAD) {
            if (g_verbose_mode) {
                printf("Reassembled payload %zu exceeds HAL limit %d\n",
                       total_len, UCI_HAL_MAX_FRAGMENT_PAYLOAD);
            }
            uci_fragment_reset();
            return -1;
        }

        if (out_capacity < sizeof(struct uci_packet_header) + total_len) {
            if (g_verbose_mode) {
                printf("Output buffer too small for reassembled payload (%zu needed)\n",
                       sizeof(struct uci_packet_header) + total_len);
            }
            uci_fragment_reset();
            return -1;
        }

        struct uci_packet_header final_header;
        if (set_header_values_safe(&final_header, mt, COMPLETE, gid, opcode, (uci_uint16)total_len) != UCI_SUCCESS) {
            uci_fragment_reset();
            return -1;
        }
        memcpy(out_buffer, &final_header, sizeof(final_header));
        memcpy(out_buffer + sizeof(final_header), g_fragment_buffer.payload, g_fragment_buffer.payload_len);
        memcpy(out_buffer + sizeof(final_header) + g_fragment_buffer.payload_len,
               payload_ptr, payload_len);

        if (g_verbose_mode) {
            printf("  Reassembled payload (%zu bytes)\n", total_len);
        }

        uci_fragment_reset();
        return (int)(sizeof(final_header) + total_len);
    }

    // Single-fragment complete packet
    if (out_capacity < expected_len) {
        if (g_verbose_mode) {
            printf("Output buffer too small (%zu needed)\n", expected_len);
        }
        return -1;
    }

    memcpy(out_buffer, fragment, expected_len);
    if (g_verbose_mode) {
        printf("  Single-fragment payload (%zu bytes)\n", payload_len);
    }
    return (int)expected_len;
}

// Enable verbose mode for hardware interface
void uci_hw_interface_set_verbose(int verbose) {
    g_verbose_mode = verbose ? 1 : 0;
    uci_hw_chardev_set_verbose(&g_uwb_chardev, g_verbose_mode);
}

// Initialize hardware interface
int uci_hw_interface_init(const char* device_path) {
    if (g_hw_initialized) {
        return 0; // Already initialized
    }

    uci_hw_interface_reset_state();

    if (g_verbose_mode) {
        printf("Initializing UCI hardware interface with device: %s\n", device_path);
    }
    
    // Store device path
    strncpy(g_device_path, device_path, sizeof(g_device_path) - 1);
    g_device_path[sizeof(g_device_path) - 1] = '\0'; // Ensure null termination
    
    // Initialize the character device interface
    if (uci_hw_chardev_init(&g_uwb_chardev, device_path) == 0) {
        if (uci_hw_chardev_open(&g_uwb_chardev) == 0) {
            if (g_verbose_mode) {
                printf("Character device interface initialized successfully\n");
            }
            uci_fragment_reset();
            g_hw_initialized = 1;
            return 0;
        } else {
            if (g_verbose_mode) {
                printf("Failed to open character device interface\n");
            }
            return -1;
        }
    } else {
        if (g_verbose_mode) {
            printf("Failed to initialize character device interface\n");
        }
        return -1;
    }
}

int uci_hw_interface_send_packet(const unsigned char* packet, size_t packet_len) {
    if (!g_hw_initialized) {
        if (g_verbose_mode) {
            printf("Hardware interface not initialized\n");
        }
        return -1;
    }

    if (!uci_hw_chardev_is_connected(&g_uwb_chardev)) {
        if (g_verbose_mode) {
            printf("Hardware device not connected\n");
        }
        return -1;
    }

    if (!packet || packet_len < sizeof(struct uci_packet_header)) {
        if (g_verbose_mode) {
            printf("Invalid packet parameters\n");
        }
        return -1;
    }

    const struct uci_packet_header* packet_header = (const struct uci_packet_header*)packet;
    uci_header_fields_t packet_fields;
    uci_extract_header_fields_safe(packet_header, &packet_fields);

    if (packet_len < sizeof(struct uci_packet_header) + packet_fields.payload_length) {
        if (g_verbose_mode) {
            printf("Incomplete packet: header expects %u payload bytes, got %zu total bytes\n",
                   packet_fields.payload_length, packet_len);
        }
        return -1;
    }

    const unsigned char* payload = packet + sizeof(struct uci_packet_header);
    size_t payload_len = packet_fields.payload_length;
    size_t remaining = payload_len;
    size_t offset = 0;
    int fragment_index = 0;

    do {
        size_t chunk = remaining;
        if (chunk > UCI_HAL_MAX_FRAGMENT_PAYLOAD) {
            chunk = UCI_HAL_MAX_FRAGMENT_PAYLOAD;
        }

        unsigned char fragment_pbf = (remaining > UCI_HAL_MAX_FRAGMENT_PAYLOAD) ? NOT_COMPLETE : COMPLETE;
        // Honour the original packet PBF only for single-fragment sends.
        if (payload_len <= UCI_HAL_MAX_FRAGMENT_PAYLOAD) {
            fragment_pbf = packet_fields.packet_boundary;
        } else if (fragment_index == 0 && g_verbose_mode && packet_fields.packet_boundary != COMPLETE) {
            printf("Ignoring caller PBF for multi-fragment send; using NOT_COMPLETE/COMPLETE automatically\n");
        }

        size_t fragment_size = sizeof(struct uci_packet_header) + chunk;
        unsigned char fragment[sizeof(struct uci_packet_header) + UCI_HAL_MAX_FRAGMENT_PAYLOAD];
        struct uci_packet_header* header = (struct uci_packet_header*)fragment;
        if (set_header_values_safe(header,
                                   packet_fields.message_type,
                                   fragment_pbf,
                                   packet_fields.group_id,
                                   packet_fields.opcode_id,
                                   (uci_uint16)chunk) != UCI_SUCCESS) {
            return -1;
        }

        if (chunk > 0 && payload) {
            memcpy(fragment + sizeof(struct uci_packet_header), payload + offset, chunk);
        }

        if (g_verbose_mode) {
            printf("Sending UCI fragment to hardware (%s):\n", g_device_path);
            printf("  Header: %02X %02X %02X %02X\n",
                   fragment[0], fragment[1], fragment[2], fragment[3]);
            if (chunk > 0) {
                printf("  Payload: ");
                for (size_t i = 0; i < chunk; i++) {
                    printf("%02X ", fragment[sizeof(struct uci_packet_header) + i]);
                }
                printf("\n");
            }
        }

        int result = uci_hw_chardev_send(&g_uwb_chardev, fragment, fragment_size);
        if (result < 0) {
            if (g_verbose_mode) {
                printf("Failed to send UCI fragment to hardware\n");
            }
            return -1;
        }

        if (remaining > chunk) {
            remaining -= chunk;
            offset += chunk;
            fragment_index++;
        } else {
            remaining = 0;
        }
    } while (remaining > 0);

    return 0;
}

// Send UCI command to hardware device
int uci_hw_interface_send_command(unsigned char mt, unsigned char pbf, unsigned char gid, unsigned char oid,
                                 unsigned char* payload, int payload_len) {
    size_t packet_len = 0;
    unsigned char* packet = create_uci_packet(mt,
                                              pbf,
                                              gid,
                                              oid,
                                              payload,
                                              (payload_len > 0) ? (size_t)payload_len : 0,
                                              &packet_len);
    int rc;

    if (!packet) {
        if (g_verbose_mode) {
            printf("Failed to construct command packet\n");
        }
        return -1;
    }

    rc = uci_hw_interface_send_packet(packet, packet_len);
    free(packet);
    return rc;
}

// Receive UCI response from hardware device with timeout
int uci_hw_interface_receive_response(unsigned char* buffer, size_t buffer_size, int timeout_ms) {
    if (!g_hw_initialized) {
        if (g_verbose_mode) {
            printf("Hardware interface not initialized\n");
        }
        return -1;
    }
    
    if (!uci_hw_chardev_is_connected(&g_uwb_chardev)) {
        if (g_verbose_mode) {
            printf("Hardware device not connected\n");
        }
        return -1;
    }
    
    if (!buffer || buffer_size == 0) {
        if (g_verbose_mode) {
            printf("Invalid buffer parameters\n");
        }
        return -1;
    }

    int queued = packet_queue_dequeue(buffer, buffer_size);
    if (queued > 0) {
        return queued;
    } else if (queued < 0) {
        return -1;
    }

    unsigned char fragment[sizeof(struct uci_packet_header) + UCI_HAL_MAX_FRAGMENT_PAYLOAD];
    unsigned char assembled[sizeof(struct uci_packet_header) + UCI_MAX_REASSEMBLED_PAYLOAD];

    while (1) {
        int bytes_received =
            uci_hw_chardev_receive(&g_uwb_chardev, fragment, sizeof(fragment), timeout_ms);

        if (bytes_received < 0) {
            if (g_verbose_mode) {
                printf("Failed to receive UCI response from hardware\n");
            }
            return -1;
        }

        if (bytes_received == 0) {
            break; // timeout/no data
        }

        if (g_verbose_mode) {
            printf("Received fragment (%d bytes) from UCI hardware (%s)\n",
                   bytes_received, g_device_path);
        }

        int assembled_len =
            uci_fragment_process(fragment, (size_t)bytes_received, assembled, sizeof(assembled));
        if (assembled_len < 0) {
            return -1;
        }

        if (assembled_len == 0) {
            // Waiting for additional fragments
            continue;
        }

        if (g_packet_queue_count == 0 && (size_t)assembled_len <= buffer_size) {
            memcpy(buffer, assembled, (size_t)assembled_len);
            return assembled_len;
        }

        packet_queue_enqueue(assembled, (size_t)assembled_len);
        queued = packet_queue_dequeue(buffer, buffer_size);
        if (queued > 0) {
            return queued;
        } else if (queued < 0) {
            return -1;
        }
    }

    queued = packet_queue_dequeue(buffer, buffer_size);
    if (queued > 0) {
        return queued;
    }
    if (queued < 0) {
        return -1;
    }

    return 0;
}

int uci_hw_interface_exchange_command(unsigned char mt,
                                      unsigned char pbf,
                                      unsigned char gid,
                                      unsigned char oid,
                                      unsigned char* payload,
                                      int payload_len,
                                      unsigned char* response_buffer,
                                      size_t response_buffer_size,
                                      int timeout_ms) {
    if (!response_buffer || response_buffer_size == 0) {
        return -1;
    }

    size_t packet_len = 0;
    unsigned char* packet = create_uci_packet(mt,
                                              pbf,
                                              gid,
                                              oid,
                                              payload,
                                              (payload_len > 0) ? (size_t)payload_len : 0,
                                              &packet_len);
    int rc;

    if (!packet) {
        return UCI_HW_EXCHANGE_SEND_ERROR;
    }

    rc = uci_hw_interface_exchange_packet(packet,
                                          packet_len,
                                          response_buffer,
                                          response_buffer_size,
                                          timeout_ms);
    free(packet);
    return rc;
}

int uci_hw_interface_exchange_packet(const unsigned char* packet,
                                     size_t packet_len,
                                     unsigned char* response_buffer,
                                     size_t response_buffer_size,
                                     int timeout_ms) {
    if (!response_buffer || response_buffer_size == 0) {
        return -1;
    }

    if (!uci_hw_interface_is_connected()) {
        return UCI_HW_EXCHANGE_SEND_ERROR;
    }

    if (uci_hw_interface_send_packet(packet, packet_len) != 0) {
        return UCI_HW_EXCHANGE_SEND_ERROR;
    }

    return uci_hw_interface_receive_response(response_buffer, response_buffer_size, timeout_ms);
}

// Cleanup hardware interface
void uci_hw_interface_cleanup(void) {
    if (g_hw_initialized) {
        if (g_verbose_mode) {
            printf("Cleaning up UCI hardware interface\n");
        }
        uci_hw_chardev_close(&g_uwb_chardev);
        uci_hw_interface_reset_state();
        g_hw_initialized = 0;
    }
}

// Check if hardware is connected
int uci_hw_interface_is_connected(void) {
    return g_hw_initialized && uci_hw_chardev_is_connected(&g_uwb_chardev);
}

// Get device path
const char* uci_hw_interface_get_device_path(void) {
    return g_device_path;
}
