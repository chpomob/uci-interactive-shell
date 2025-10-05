#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include "../include/uci.h"
#include "../include/uci_functions.h"

// Helper functions for Little-Endian conversion
static inline uint16_t read_u16_le(const unsigned char* buffer) {
    return (uint16_t)buffer[0] | ((uint16_t)buffer[1] << 8);
}

static inline uint32_t read_u32_le(const unsigned char* buffer) {
    return (uint32_t)buffer[0] | ((uint32_t)buffer[1] << 8) |
           ((uint32_t)buffer[2] << 16) | ((uint32_t)buffer[3] << 24);
}

static inline uint64_t read_u64_le(const unsigned char* buffer) {
    return (uint64_t)buffer[0] | ((uint64_t)buffer[1] << 8) |
           ((uint64_t)buffer[2] << 16) | ((uint64_t)buffer[3] << 24) |
           ((uint64_t)buffer[4] << 32) | ((uint64_t)buffer[5] << 40) |
           ((uint64_t)buffer[6] << 48) | ((uint64_t)buffer[7] << 56);
}

static inline void write_u16_le(unsigned char* buffer, uint16_t value) {
    buffer[0] = value & 0xFF;
    buffer[1] = (value >> 8) & 0xFF;
}

static inline void write_u32_le(unsigned char* buffer, uint32_t value) {
    buffer[0] = value & 0xFF;
    buffer[1] = (value >> 8) & 0xFF;
    buffer[2] = (value >> 16) & 0xFF;
    buffer[3] = (value >> 24) & 0xFF;
}

static inline void write_u64_le(unsigned char* buffer, uint64_t value) {
    for (int i = 0; i < 8; i++) {
        buffer[i] = (value >> (i * 8)) & 0xFF;
    }
}

#define MAX_RESPONSE_PAYLOAD_LEN 255
#define ANDROID_GET_POWER_STATS 0x00
#define ANDROID_SET_COUNTRY_CODE 0x01
#define ANDROID_FIRA_RANGE_DIAGNOSTICS 0x02
#define ANDROID_RADAR_SET_APP_CONFIG 0x11
#define ANDROID_RADAR_GET_APP_CONFIG 0x12
#define TEST_RF_SET_CONFIG 0x00
#define TEST_RF_PERIODIC_TX 0x02
#define TEST_RF_PER_RX 0x03
#define TEST_RF_RX 0x05
#define TEST_RF_STOP 0x07

// Global flag for hardware mode
static int g_hardware_mode = 0;
static char g_hardware_device_path[256] = "/dev/ttyUSB0";  // Default device path
static unsigned long long g_fake_timestamp = 0;
static unsigned int g_session_handle_counter = 1;

#define MAX_PENDING_NOTIFICATIONS 16
typedef struct {
    unsigned char data[sizeof(struct uci_packet_header) + MAX_RESPONSE_PAYLOAD_LEN];
    size_t length;
} notification_item_t;

static notification_item_t g_notification_queue[MAX_PENDING_NOTIFICATIONS];
static size_t g_notification_head = 0;
static size_t g_notification_tail = 0;

static void enqueue_notification(unsigned char gid, unsigned char oid, const unsigned char* payload, size_t payload_len);

static void enqueue_session_status_notification(const struct uci_session* session, unsigned char new_state, unsigned char reason_code) {
    unsigned char payload[6];
    write_u32_le(payload, session->session_handle);
    payload[4] = new_state;
    payload[5] = reason_code;
    enqueue_notification(SESSION_CONFIG, SESSION_STATUS_NTF, payload, sizeof(payload));
}

static int notification_queue_empty() {
    return g_notification_head == g_notification_tail;
}

static int notification_queue_full() {
    return ((g_notification_tail + 1) % MAX_PENDING_NOTIFICATIONS) == g_notification_head;
}

static void enqueue_notification(unsigned char gid, unsigned char oid, const unsigned char* payload, size_t payload_len) {
    if (payload_len > MAX_RESPONSE_PAYLOAD_LEN) {
        payload_len = MAX_RESPONSE_PAYLOAD_LEN;
    }

    if (notification_queue_full()) {
        printf("Notification queue full, dropping notification (GID=0x%02X OID=0x%02X)\n", gid, oid);
        return;
    }

    notification_item_t* item = &g_notification_queue[g_notification_tail];
    struct uci_packet_header* header = (struct uci_packet_header*)item->data;
    set_header_values(header, NOTIFICATION, COMPLETE, gid, oid, (unsigned char)payload_len);
    if (payload_len > 0 && payload) {
        memcpy(item->data + sizeof(struct uci_packet_header), payload, payload_len);
    }
    item->length = sizeof(struct uci_packet_header) + payload_len;
    g_notification_tail = (g_notification_tail + 1) % MAX_PENDING_NOTIFICATIONS;
}

void uci_process_pending_notifications() {
    while (!notification_queue_empty()) {
        notification_item_t* item = &g_notification_queue[g_notification_head];
        parse_uci_packet(item->data, item->length);
        g_notification_head = (g_notification_head + 1) % MAX_PENDING_NOTIFICATIONS;
    }
}

void decode_session_data_credit_ntf(unsigned char* payload, int payload_len);
void decode_session_data_transfer_status_ntf(unsigned char* payload, int payload_len);


void decode_session_data_credit_ntf(unsigned char* payload, int payload_len) {
    printf("    SESSION_DATA_CREDIT_NTF - Data Credit Notification\n");
    
    if (payload_len < 5) {
        printf("      ERROR: Payload too short (%d bytes, need at least 5)\n", payload_len);
        return;
    }
    
    unsigned int session_token = read_u32_le(payload);
    unsigned char credit_availability = payload[4];
    
    printf("      Session Token: 0x%08X\n", session_token);
    printf("      Credit Availability: 0x%02X", credit_availability);
    if (credit_availability == 0x00) {
        printf(" (NOT_AVAILABLE)\n");
    } else {
        printf(" (AVAILABLE)\n");
    }
}

void decode_session_data_transfer_status_ntf(unsigned char* payload, int payload_len) {
    printf("    SESSION_DATA_TRANSFER_STATUS_NTF - Data Transfer Status Notification\n");
    
    if (payload_len < 6) {
        printf("      ERROR: Payload too short (%d bytes, need at least 6)\n", payload_len);
        return;
    }
    
    unsigned int session_token = read_u32_le(payload);
    unsigned short uci_sequence_number = read_u16_le(&payload[4]);
    
    printf("      Session Token: 0x%08X\n", session_token);
    printf("      UCI Sequence Number: %u\n", uci_sequence_number);
    
    if (payload_len >= 8) {
        unsigned char status = payload[6];
        unsigned char tx_count = payload[7];
        
        printf("      Status: 0x%02X", status);
        switch(status) {
            case UCI_DATA_TRANSFER_STATUS_REPETITION_OK: printf(" (REPETITION_OK)\n"); break;
            case UCI_DATA_TRANSFER_STATUS_OK: printf(" (OK)\n"); break;
            case UCI_DATA_TRANSFER_STATUS_ERROR_DATA_TRANSFER: printf(" (ERROR_DATA_TRANSFER)\n"); break;
            case UCI_DATA_TRANSFER_STATUS_ERROR_NO_CREDIT_AVAILABLE: printf(" (ERROR_NO_CREDIT_AVAILABLE)\n"); break;
            case UCI_DATA_TRANSFER_STATUS_ERROR_REJECTED: printf(" (ERROR_REJECTED)\n"); break;
            case UCI_DATA_TRANSFER_STATUS_SESSION_TYPE_NOT_SUPPORTED: printf(" (SESSION_TYPE_NOT_SUPPORTED)\n"); break;
            case UCI_DATA_TRANSFER_STATUS_ERROR_DATA_TRANSFER_IS_ONGOING: printf(" (ERROR_DATA_TRANSFER_IS_ONGOING)\n"); break;
            case UCI_DATA_TRANSFER_STATUS_INVALID_FORMAT: printf(" (INVALID_FORMAT)\n"); break;
            default: printf(" (UNKNOWN)\n"); break;
        }
        
        printf("      TX Count: %d\n", tx_count);
    }
    
    if (payload_len > 8) {
        printf("      Additional Data (%d bytes): ", payload_len - 8);
        for (int i = 8; i < payload_len; i++) {
            printf("%02X ", payload[i]);
        }
        printf("\n");
    }
}

// Function to analyze and display a UCI packet in human-readable format
void analyze_uci_packet(unsigned char* packet, size_t packet_len) {
    if (packet_len < sizeof(struct uci_packet_header)) {
        printf("Error: UCI packet too short to contain a header (need at least %zu bytes, got %zu)\n", 
               sizeof(struct uci_packet_header), packet_len);
        return;
    }

    struct uci_packet_header* header = (struct uci_packet_header*)packet;

    printf("=== UCI Packet Analysis ===\n");
    printf("Total Packet Length: %zu bytes\n", packet_len);
    printf("Header Bytes: %02X %02X %02X %02X\n", 
           packet[0], packet[1], packet[2], packet[3]);

    // Extract header fields
    unsigned char gid = get_gid(header);
    unsigned char pbf = get_pbf(header);
    unsigned char mt = get_mt(header);
    unsigned char opcode = get_opcode(header);
    unsigned char opcode_reserved_bits = get_reserved_opcode_bits(header);
    unsigned char payload_len = header->payload_len;

    printf("  Message Type (MT): 0x%01X", mt);
    switch(mt) {
        case DATA: printf(" (DATA)"); break;
        case COMMAND: printf(" (COMMAND)"); break;
        case RESPONSE: printf(" (RESPONSE)"); break;
        case NOTIFICATION: printf(" (NOTIFICATION)"); break;
        default: printf(" (UNKNOWN)"); break;
    }
    printf("\n");

    printf("  Packet Boundary Flag (PBF): 0x%01X", pbf);
    switch(pbf) {
        case COMPLETE: printf(" (COMPLETE)"); break;
        case NOT_COMPLETE: printf(" (NOT_COMPLETE)"); break;
        default: printf(" (UNKNOWN)"); break;
    }
    printf("\n");

    printf("  Group ID (GID): 0x%01X", gid);
    switch(gid) {
        case CORE: printf(" (CORE)"); break;
        case SESSION_CONFIG: printf(" (SESSION_CONFIG)"); break;
        case SESSION_CONTROL: printf(" (SESSION_CONTROL)"); break;
        case DATA_CONTROL: printf(" (DATA_CONTROL)"); break;
        case TEST: printf(" (TEST)"); break;
        case VENDOR_ANDROID: printf(" (VENDOR_ANDROID)"); break;
        default: printf(" (UNKNOWN)"); break;
    }
    printf("\n");

    printf("  Opcode: 0x%02X", opcode);
    // Show opcode meaning based on GID
    if (gid == CORE) {
        switch(opcode) {
            case CORE_DEVICE_INFO: printf(" (CORE_DEVICE_INFO)"); break;
            case CORE_DEVICE_RESET: printf(" (CORE_DEVICE_RESET)"); break;
            case CORE_GET_CAPS_INFO: printf(" (CORE_GET_CAPS_INFO)"); break;
            case CORE_SET_CONFIG: printf(" (CORE_SET_CONFIG)"); break;
            case CORE_GET_CONFIG: printf(" (CORE_GET_CONFIG)"); break;
            case CORE_DEVICE_SUSPEND: printf(" (CORE_DEVICE_SUSPEND)"); break;
            case CORE_DEVICE_STATUS_NTF: printf(" (CORE_DEVICE_STATUS_NTF)"); break;
            case CORE_GENERIC_ERROR_NTF: printf(" (CORE_GENERIC_ERROR_NTF)"); break;
            default: printf(" (CORE_UNKNOWN)"); break;
        }
    } else if (gid == SESSION_CONFIG) {
        switch(opcode) {
            case SESSION_INIT: printf(" (SESSION_INIT)"); break;
            case SESSION_DEINIT: printf(" (SESSION_DEINIT)"); break;
            case SESSION_STATUS_NTF: printf(" (SESSION_STATUS_NTF)"); break;
            case SESSION_SET_APP_CONFIG: printf(" (SESSION_SET_APP_CONFIG)"); break;
            case SESSION_GET_APP_CONFIG: printf(" (SESSION_GET_APP_CONFIG)"); break;
            case SESSION_GET_COUNT: printf(" (SESSION_GET_COUNT)"); break;
            case SESSION_GET_STATE: printf(" (SESSION_GET_STATE)"); break;
            default: printf(" (SESSION_CONFIG_UNKNOWN)"); break;
        }
    } else if (gid == SESSION_CONTROL) {
        switch(opcode) {
            case SESSION_START: printf(" (SESSION_START/SESSION_INFO_NTF)"); break;
            case SESSION_STOP: printf(" (SESSION_STOP)"); break;
            case SESSION_GET_RANGING_COUNT: printf(" (SESSION_GET_RANGING_COUNT)"); break;
            case SESSION_DATA_CREDIT_NTF: printf(" (SESSION_DATA_CREDIT_NTF)"); break;
            case SESSION_DATA_TRANSFER_STATUS_NTF: printf(" (SESSION_DATA_TRANSFER_STATUS_NTF)"); break;
            default: printf(" (SESSION_CONTROL_UNKNOWN)"); break;
        }
    } else if (gid == TEST) {
        printf(" (TEST_CMD_0x%02X)", opcode);
    } else if (gid == VENDOR_ANDROID) {
        printf(" (VENDOR_ANDROID_CMD_0x%02X)", opcode);
    } else {
        printf(" (UNKNOWN_OPCODE)");
    }
    if (opcode_reserved_bits) {
        printf(" (reserved bits=0x%01X)", opcode_reserved_bits);
    }
    printf("\n");

    printf("  Reserved Byte 2: 0x%02X\n", header->reserved2);
    printf("  Payload Length: %u\n", payload_len);

    // Check if payload length matches actual available data
    if (payload_len > packet_len - sizeof(struct uci_packet_header)) {
        printf("  ERROR: Header payload length (%u) exceeds available data (%zu bytes after header)\n", 
               payload_len, packet_len - sizeof(struct uci_packet_header));
        printf("  Actual available payload: %zu bytes\n", packet_len - sizeof(struct uci_packet_header));
        payload_len = (unsigned char)(packet_len - sizeof(struct uci_packet_header));
    }

    if (payload_len > 0) {
        printf("  Payload (%u bytes): ", payload_len);
        for (unsigned char i = 0; i < payload_len; i++) {
            printf("%02X ", packet[sizeof(struct uci_packet_header) + i]);
        }
        printf("\n");
        
        // Try to interpret payload based on message type and opcode
        printf("  Payload Interpretation:\n");
        if (mt == RESPONSE && gid == CORE && opcode == CORE_DEVICE_INFO) {
            if (payload_len >= 9) {
                unsigned char status = packet[sizeof(struct uci_packet_header) + 0];
                unsigned short uci_version = read_u16_le(&packet[sizeof(struct uci_packet_header) + 1]);
                unsigned short mac_version = read_u16_le(&packet[sizeof(struct uci_packet_header) + 3]);
                unsigned short phy_version = read_u16_le(&packet[sizeof(struct uci_packet_header) + 5]);
                unsigned short uci_test_version = read_u16_le(&packet[sizeof(struct uci_packet_header) + 7]);
                
                printf("    Status: 0x%02X\n", status);
                printf("    UCI Version: 0x%04X\n", uci_version);
                printf("    MAC Version: 0x%04X\n", mac_version);
                printf("    PHY Version: 0x%04X\n", phy_version);
                printf("    UCI Test Version: 0x%04X\n", uci_test_version);
                if (payload_len > 9) {
                    printf("    Additional vendor-specific data: %u bytes\n", payload_len - 9);
                }
            } else {
                printf("    ERROR: CORE_DEVICE_INFO response too short (%u bytes, need at least 9)\n", payload_len);
            }
        } else if (mt == NOTIFICATION && gid == CORE && opcode == CORE_DEVICE_STATUS_NTF) {
            if (payload_len >= 1) {
                unsigned char device_state = packet[sizeof(struct uci_packet_header)];
                printf("    Device State: 0x%02X", device_state);
                switch(device_state) {
                    case DEVICE_STATE_READY: printf(" (READY)"); break;
                    case DEVICE_STATE_ACTIVE: printf(" (ACTIVE)"); break;
                    case DEVICE_STATE_ERROR: printf(" (ERROR)"); break;
                    default: printf(" (UNKNOWN)"); break;
                }
                printf("\n");
            } else {
                printf("    ERROR: CORE_DEVICE_STATUS_NTF requires 1 byte payload, got %u\n", payload_len);
            }
        } else if (mt == RESPONSE && gid == CORE && opcode == CORE_SET_CONFIG) {
            if (payload_len >= 2) {
                unsigned char status = packet[sizeof(struct uci_packet_header)];
                unsigned char num_configs = packet[sizeof(struct uci_packet_header) + 1];
                printf("    Status: 0x%02X\n", status);
                printf("    Number of Config Status: %u\n", num_configs);
            } else {
                printf("    ERROR: CORE_SET_CONFIG response requires at least 2 bytes, got %u\n", payload_len);
            }
        } else if (mt == RESPONSE && gid == CORE && opcode == CORE_GET_CONFIG) {
            if (payload_len >= 2) {
                unsigned char status = packet[sizeof(struct uci_packet_header)];
                unsigned char num_tlvs = packet[sizeof(struct uci_packet_header) + 1];
                printf("    Status: 0x%02X\n", status);
                printf("    Number of TLVs: %u\n", num_tlvs);
            } else {
                printf("    ERROR: CORE_GET_CONFIG response requires at least 2 bytes, got %u\n", payload_len);
            }
        } else if (mt == NOTIFICATION && gid == SESSION_CONFIG && opcode == SESSION_STATUS_NTF) {
            if (payload_len >= 6) {
                unsigned int token = read_u32_le(&packet[sizeof(struct uci_packet_header)]);
                unsigned char state = packet[sizeof(struct uci_packet_header) + 4];
                unsigned char reason = packet[sizeof(struct uci_packet_header) + 5];
                
                printf("    Session Token: 0x%08X\n", token);
                printf("    Session State: 0x%02X", state);
                switch(state) {
                    case SESSION_STATE_INIT: printf(" (INIT)"); break;
                    case SESSION_STATE_DEINIT: printf(" (DEINIT)"); break;
                    case SESSION_STATE_ACTIVE: printf(" (ACTIVE)"); break;
                    case SESSION_STATE_IDLE: printf(" (IDLE)"); break;
                    default: printf(" (UNKNOWN)"); break;
                }
                printf("\n");
                
                printf("    Reason Code: 0x%02X", reason);
                switch(reason) {
                    case STATE_CHANGE_WITH_SESSION_MANAGEMENT_COMMANDS: printf(" (MGMT_CMD)"); break;
                    case MAX_RANGING_ROUND_RETRY_COUNT_REACHED: printf(" (MAX_RETRY)"); break;
                    case MAX_NUMBER_OF_MEASUREMENTS_REACHED: printf(" (MAX_MEASUREMENTS)"); break;
                    default: printf(" (UNKNOWN)"); break;
                }
                printf("\n");
            } else {
                printf("    ERROR: SESSION_STATUS_NTF requires at least 6 bytes, got %u\n", payload_len);
            }
        }
    } else {
        printf("  No payload data\n");
    }
    
    // Detailed payload decoding based on packet type
    if (payload_len > 0) {
        printf("  Detailed Payload Decoding:\n");
        
        // Define payload pointer for easier access
        unsigned char* payload_ptr = packet + sizeof(struct uci_packet_header);
        
        // Decode payload based on MT, GID, and Opcode
        int decoded = 0;
        if (mt == RESPONSE && gid == CORE) {
            switch(opcode) {
                case CORE_DEVICE_INFO:
                    decode_core_device_info_rsp(payload_ptr, (int)payload_len);
                    decoded = 1;
                    break;
                case CORE_GET_CAPS_INFO:
                    decode_core_get_caps_info_rsp(payload_ptr, (int)payload_len);
                    decoded = 1;
                    break;
                case CORE_SET_CONFIG:
                    decode_core_set_config_rsp(payload_ptr, (int)payload_len);
                    decoded = 1;
                    break;
                case CORE_GET_CONFIG:
                    decode_core_get_config_rsp(payload_ptr, (int)payload_len);
                    decoded = 1;
                    break;
                case CORE_DEVICE_RESET:
                    decode_core_device_reset_rsp(payload_ptr, (int)payload_len);
                    decoded = 1;
                    break;
                case CORE_DEVICE_SUSPEND:
                    decode_core_device_suspend_rsp(payload_ptr, (int)payload_len);
                    decoded = 1;
                    break;
                default:
                    printf("    No specific decoder for CORE RESPONSE opcode 0x%02X\n", opcode);
                    break;
            }
        } else if (mt == RESPONSE && gid == SESSION_CONFIG) {
            switch(opcode) {
                case SESSION_INIT:
                    decode_session_init_rsp(payload_ptr, (int)payload_len);
                    decoded = 1;
                    break;
                case SESSION_DEINIT:
                    decode_session_deinit_rsp(payload_ptr, (int)payload_len);
                    decoded = 1;
                    break;
                case SESSION_SET_APP_CONFIG:
                    decode_session_set_app_config_rsp(payload_ptr, (int)payload_len);
                    decoded = 1;
                    break;
                case SESSION_GET_APP_CONFIG:
                    decode_session_get_app_config_rsp(payload_ptr, (int)payload_len);
                    decoded = 1;
                    break;
                case SESSION_GET_COUNT:
                    decode_session_get_count_rsp(payload_ptr, (int)payload_len);
                    decoded = 1;
                    break;
                case SESSION_GET_STATE:
                    decode_session_get_state_rsp(payload_ptr, (int)payload_len);
                    decoded = 1;
                    break;
                default:
                    printf("    No specific decoder for SESSION_CONFIG RESPONSE opcode 0x%02X\n", opcode);
                    break;
            }
        } else if (mt == RESPONSE && gid == SESSION_CONTROL) {
            switch(opcode) {
                case SESSION_START:
                    decode_session_start_rsp(payload_ptr, (int)payload_len);
                    decoded = 1;
                    break;
                case SESSION_STOP:
                    decode_session_stop_rsp(payload_ptr, (int)payload_len);
                    decoded = 1;
                    break;
                case SESSION_GET_RANGING_COUNT:
                    decode_session_get_ranging_count_rsp(payload_ptr, (int)payload_len);
                    decoded = 1;
                    break;
                default:
                    printf("    No specific decoder for SESSION_CONTROL RESPONSE opcode 0x%02X\n", opcode);
                    break;
            }
        } else if (mt == NOTIFICATION && gid == CORE) {
            switch(opcode) {
                case CORE_DEVICE_STATUS_NTF:
                    decode_core_device_status_ntf(payload_ptr, (int)payload_len);
                    decoded = 1;
                    break;
                case CORE_GENERIC_ERROR_NTF:
                    decode_core_generic_error_ntf(payload_ptr, (int)payload_len);
                    decoded = 1;
                    break;
                default:
                    printf("    No specific decoder for CORE NOTIFICATION opcode 0x%02X\n", opcode);
                    break;
            }
        } else if (mt == NOTIFICATION && gid == SESSION_CONFIG) {
            switch(opcode) {
                case SESSION_STATUS_NTF:
                    decode_session_status_ntf(payload_ptr, (int)payload_len);
                    decoded = 1;
                    break;
                default:
                    // Heuristic: Check if this might be a SESSION_STATUS_NTF with wrong opcode
                    if (payload_len == 6) {
                        // SESSION_STATUS_NTF has 6 bytes: session_token(4) + session_state(1) + reason_code(1)
                        printf("    WARNING: Opcode 0x%02X is not SESSION_STATUS_NTF, but payload structure suggests it might be.\n", opcode);
                        printf("    Interpreting as SESSION_STATUS_NTF:\n");
                        decode_session_status_ntf(payload_ptr, (int)payload_len);
                        decoded = 1;
                        printf("    NOTE: This suggests possible firmware bug or packet corruption.\n");
                    } else {
                        printf("    No specific decoder for SESSION_CONFIG NOTIFICATION opcode 0x%02X\n", opcode);
                    }
                    break;
            }
        } else if (mt == NOTIFICATION && gid == SESSION_CONTROL) {
            switch(opcode) {
                case SESSION_INFO_NTF:
                    decode_session_info_ntf(payload_ptr, (int)payload_len);
                    decoded = 1;
                    break;
                case SESSION_DATA_CREDIT_NTF:
                    decode_session_data_credit_ntf(payload_ptr, (int)payload_len);
                    decoded = 1;
                    break;
                case SESSION_DATA_TRANSFER_STATUS_NTF:
                    decode_session_data_transfer_status_ntf(payload_ptr, (int)payload_len);
                    decoded = 1;
                    break;
                default:
                    printf("    No specific decoder for SESSION_CONTROL NOTIFICATION opcode 0x%02X\n", opcode);
                    break;
            }
        } else if (gid == TEST || gid == VENDOR_ANDROID) {
            printf("    Decoder not implemented for TEST/VENDOR_ANDROID packets\n");
            decoded = 1;
        } else {
            printf("    No specific decoder for MT=%d, GID=%d, OP=0x%02X\n", mt, gid, opcode);
        }
        
        // If no decoder matched, provide generic information
        if (!decoded && payload_len > 0) {
            printf("    Unrecognized packet type - providing generic analysis:\n");
            printf("    Payload bytes: ");
            for (size_t i = 0; i < payload_len && i < 32; i++) {
                printf("%02X ", payload_ptr[i]);
            }
            if (payload_len > 32) {
                printf("... (and %u more bytes)", (unsigned int)(payload_len - 32));
            }
            printf("\n");
        }
    }
    
    printf("========================\n");
}

// Function to enable hardware mode
void uci_enable_hardware_mode(const char* device_path) {
    g_hardware_mode = 1;
    if (device_path && strlen(device_path) < sizeof(g_hardware_device_path)) {
        strcpy(g_hardware_device_path, device_path);
    }
    printf("Hardware mode enabled with device: %s\n", g_hardware_device_path);
}

// Function to disable hardware mode
void uci_disable_hardware_mode() {
    g_hardware_mode = 0;
    printf("Hardware mode disabled\n");
}

// Function to check if hardware mode is enabled
int uci_is_hardware_mode_enabled() {
    return g_hardware_mode;
}

// Global session storage
struct uci_session uci_sessions[MAX_SESSIONS];

void send_uci_command(unsigned char mt, unsigned char pbf, unsigned char gid, unsigned char oid, unsigned char* payload, int payload_len) {
    // Initialize session storage on first call
    static int initialized = 0;
    if (!initialized) {
        init_uci_sessions();
        initialized = 1;
    }
    
    // Check if we're in hardware mode
    if (g_hardware_mode) {
        printf("[HARDWARE MODE] ");
        
        // Create the complete UCI packet
        int total_packet_size = sizeof(struct uci_packet_header) + payload_len;
        unsigned char* packet = malloc(total_packet_size);
        if (!packet) {
            printf("Error: Failed to allocate memory for UCI packet.\n");
            return;
        }
        
        // Set up header
        struct uci_packet_header* header = (struct uci_packet_header*)packet;
        set_header_values(header, mt, pbf, gid, oid, payload_len);
        
        // Copy payload if present
        if (payload && payload_len > 0) {
            memcpy(packet + sizeof(struct uci_packet_header), payload, payload_len);
        }
        
        printf("Sending UCI packet to hardware (%s):\n", g_hardware_device_path);
        // Print the raw bytes as they would appear on the wire
        unsigned char* header_bytes = (unsigned char*)header;
        printf("  Header: %02X %02X %02X %02X\n", header_bytes[0], header_bytes[1], header_bytes[2], header_bytes[3]);
        if (payload_len > 0) {
            printf("  Payload: ");
            for (int i = 0; i < payload_len; i++) {
                printf("%02X ", payload[i]);
            }
            printf("\n");
        }
        
        // In a real implementation, we would send the packet to the hardware device
        // For now, we'll just print that we would send it to the hardware
        printf("  -> Would send to hardware device %s\n", g_hardware_device_path);
        
        // Then we would receive a response from the hardware
        // For now, we'll simulate receiving a minimal response
        printf("  <- Simulating response from hardware device\n");
        
        // Clean up
        free(packet);
        
        // Create a minimal response for now
        unsigned char response_packet[sizeof(struct uci_packet_header) + 10];
        struct uci_packet_header* response_header = (struct uci_packet_header*)response_packet;
        set_header_values(response_header, RESPONSE, COMPLETE, gid, oid, 1);
        response_packet[sizeof(struct uci_packet_header)] = UCI_STATUS_OK;
        response_header->payload_len = 1;
        
        parse_uci_packet(response_packet, sizeof(struct uci_packet_header) + response_header->payload_len);
        return;
    }

    struct uci_packet_header header;
    set_header_values(&header, mt, pbf, gid, oid, payload_len);

    printf("Sending UCI packet:\n");
    // Print the raw bytes as they would appear on the wire
    unsigned char* header_bytes = (unsigned char*)&header;
    printf("  Header: %02X %02X %02X %02X\n", header_bytes[0], header_bytes[1], header_bytes[2], header_bytes[3]);
    printf("  Payload: ");
    for (int i = 0; i < payload_len; i++) {
        printf("%02X ", payload[i]);
    }
    printf("\n");

    // Simulate receiving a response
    printf("Simulating UCI response...\n");
    unsigned char response_packet[sizeof(struct uci_packet_header) + MAX_RESPONSE_PAYLOAD_LEN];
    struct uci_packet_header* response_header = (struct uci_packet_header*)response_packet;
    set_header_values(response_header, RESPONSE, COMPLETE, gid, oid, 0); // Initialize with 0, will be updated below
    
    if (gid == CORE && oid == CORE_DEVICE_INFO) {
        // Simulate a CORE_DEVICE_INFO_RSP according to Android UWB specification
        unsigned char device_info_rsp_payload[9];
        device_info_rsp_payload[0] = UCI_STATUS_OK;
        write_u16_le(&device_info_rsp_payload[1], 0x0100);
        write_u16_le(&device_info_rsp_payload[3], 0x0200);
        write_u16_le(&device_info_rsp_payload[5], 0x0200);
        write_u16_le(&device_info_rsp_payload[7], 0x0100);
        memcpy(response_packet + sizeof(struct uci_packet_header), device_info_rsp_payload, sizeof(device_info_rsp_payload));
        response_header->payload_len = sizeof(device_info_rsp_payload);
    } else if (gid == CORE && oid == CORE_GET_CAPS_INFO) {
        // Simulate a CORE_GET_CAPS_INFO_RSP
        unsigned char caps_rsp_payload[] = {UCI_STATUS_OK, 0x01, SUPPORTED_V1_FIRA_PHY_VERSION_RANGE_V2_MAX_MESSAGE_SIZE, 0x02, 0x01, 0x00};
        memcpy(response_packet + sizeof(struct uci_packet_header), caps_rsp_payload, sizeof(caps_rsp_payload));
        response_header->payload_len = sizeof(caps_rsp_payload);
    } else if (gid == CORE && oid == CORE_QUERY_UWBS_TIMESTAMP) {
        unsigned char timestamp_rsp[9];
        timestamp_rsp[0] = UCI_STATUS_OK;
        unsigned long long timestamp = g_fake_timestamp++;
        write_u64_le(&timestamp_rsp[1], timestamp);
        memcpy(response_packet + sizeof(struct uci_packet_header), timestamp_rsp, sizeof(timestamp_rsp));
        response_header->payload_len = sizeof(timestamp_rsp);
    } else if (gid == CORE && oid == CORE_GET_CONFIG) {
        unsigned char cfg_count = 0;
        unsigned char cfg_ids_local[MAX_RESPONSE_PAYLOAD_LEN] = {0};
        if (payload && payload_len >= 1) {
            unsigned char requested = payload[0];
            unsigned int available = payload_len - 1;
            unsigned int max_entries = (MAX_RESPONSE_PAYLOAD_LEN - 2) / 3;
            if (requested > available) {
                requested = (unsigned char)available;
            }
            if (requested > max_entries) {
                requested = (unsigned char)max_entries;
            }
            cfg_count = requested;
            for (unsigned char i = 0; i < cfg_count; i++) {
                cfg_ids_local[i] = payload[1 + i];
            }
        }

        unsigned char get_config_rsp_payload[MAX_RESPONSE_PAYLOAD_LEN] = {0};
        unsigned char response_len = (unsigned char)(2 + (cfg_count * 3));
        get_config_rsp_payload[0] = (cfg_count > 0) ? UCI_STATUS_OK : UCI_STATUS_INVALID_PARAM;
        get_config_rsp_payload[1] = cfg_count;

        if (cfg_count == 0) {
            unsigned char err = UCI_STATUS_INVALID_PARAM;
            enqueue_notification(CORE, CORE_GENERIC_ERROR_NTF, &err, 1);
        }

        int response_offset = 2;
        for (unsigned char i = 0; i < cfg_count; i++) {
            unsigned char cfg_id = cfg_ids_local[i];
            get_config_rsp_payload[response_offset] = cfg_id;
            get_config_rsp_payload[response_offset + 1] = 1;
            if (cfg_id == DEVICE_STATE) {
                get_config_rsp_payload[response_offset + 2] = DEVICE_STATE_ACTIVE;
            } else if (cfg_id == LOW_POWER_MODE) {
                get_config_rsp_payload[response_offset + 2] = 0;
            } else {
                get_config_rsp_payload[response_offset + 2] = 0;
            }
            response_offset += 3;
        }

        memcpy(response_packet + sizeof(struct uci_packet_header), get_config_rsp_payload, response_len);
        response_header->payload_len = response_len;
    } else if (gid == CORE && oid == CORE_SET_CONFIG) {
        unsigned char cfg_ids[MAX_RESPONSE_PAYLOAD_LEN] = {0};
        unsigned char processed_tlvs = 0;
        unsigned char status = UCI_STATUS_OK;

        if (payload && payload_len > 0) {
            unsigned char declared_tlvs = payload[0];
            int offset = 1;
            for (unsigned char i = 0; i < declared_tlvs; i++) {
                if (offset + 2 > payload_len) {
                    status = UCI_STATUS_INVALID_PARAM;
                    break; // Not enough bytes for cfg_id + length
                }

                unsigned char cfg_id = payload[offset];
                unsigned char cfg_len = payload[offset + 1];
                offset += 2;

                if (offset + cfg_len > payload_len) {
                    status = UCI_STATUS_INVALID_PARAM;
                    break; // Truncated value
                }

                if (processed_tlvs < (MAX_RESPONSE_PAYLOAD_LEN - 2) / 2) {
                    cfg_ids[processed_tlvs] = cfg_id;
                    processed_tlvs++;
                }

                offset += cfg_len;
            }
            if (processed_tlvs != declared_tlvs) {
                status = UCI_STATUS_INVALID_PARAM;
            }
        } else {
            status = UCI_STATUS_INVALID_PARAM;
        }

        unsigned char response_len = (unsigned char)(2 + (processed_tlvs * 2));
        unsigned char set_config_rsp_payload[MAX_RESPONSE_PAYLOAD_LEN] = {0};
        set_config_rsp_payload[0] = status;
        set_config_rsp_payload[1] = processed_tlvs;
        for (unsigned char i = 0; i < processed_tlvs; i++) {
            set_config_rsp_payload[2 + (i * 2)] = cfg_ids[i];
            set_config_rsp_payload[2 + (i * 2) + 1] = UCI_STATUS_OK;
        }

        if (status != UCI_STATUS_OK) {
            enqueue_notification(CORE, CORE_GENERIC_ERROR_NTF, &status, 1);
        }

        memcpy(response_packet + sizeof(struct uci_packet_header), set_config_rsp_payload, response_len);
        response_header->payload_len = response_len;
    } else if (gid == CORE && oid == CORE_GET_CONFIG) {
        unsigned char cfg_count = 0;
        if (payload && payload_len >= 1) {
            unsigned char declared = payload[0];
            unsigned int available = payload_len - 1;
            unsigned int max_entries = (MAX_RESPONSE_PAYLOAD_LEN - 2) / 3;
            if (declared > available) {
                declared = (unsigned char)available;
            }
            if (declared > max_entries) {
                declared = (unsigned char)max_entries;
            }
            cfg_count = declared;
        }

        unsigned char get_config_rsp_payload[MAX_RESPONSE_PAYLOAD_LEN] = {0};
        unsigned char response_len = (unsigned char)(2 + (cfg_count * 3));
        get_config_rsp_payload[0] = (cfg_count > 0) ? UCI_STATUS_OK : UCI_STATUS_INVALID_PARAM;
        get_config_rsp_payload[1] = cfg_count;

        int response_offset = 2;
        for (unsigned char i = 0; i < cfg_count; i++) {
            DeviceConfigId cfg_id = (DeviceConfigId)payload[1 + i];
            get_config_rsp_payload[response_offset] = cfg_id;
            get_config_rsp_payload[response_offset + 1] = 1;

            if (cfg_id == DEVICE_STATE) {
                get_config_rsp_payload[response_offset + 2] = DEVICE_STATE_ACTIVE;
            } else if (cfg_id == LOW_POWER_MODE) {
                get_config_rsp_payload[response_offset + 2] = 0;
            } else {
                get_config_rsp_payload[response_offset + 2] = 0;
            }

            response_offset += 3;
        }

        memcpy(response_packet + sizeof(struct uci_packet_header), get_config_rsp_payload, response_len);
        response_header->payload_len = response_len;
    } else if (gid == CORE && oid == CORE_DEVICE_RESET) {
        // Simulate a CORE_DEVICE_RESET_RSP
        unsigned char reset_rsp_payload[] = {UCI_STATUS_OK};
        memcpy(response_packet + sizeof(struct uci_packet_header), reset_rsp_payload, sizeof(reset_rsp_payload));
        response_header->payload_len = sizeof(reset_rsp_payload);
        
        // Reset all sessions when device is reset
        init_uci_sessions();
        unsigned char device_state_payload = DEVICE_STATE_READY;
        enqueue_notification(CORE, CORE_DEVICE_STATUS_NTF, &device_state_payload, 1);
    } else if (gid == CORE && oid == CORE_DEVICE_SUSPEND) {
        unsigned char suspend_rsp_payload[] = {UCI_STATUS_OK};
        memcpy(response_packet + sizeof(struct uci_packet_header), suspend_rsp_payload, sizeof(suspend_rsp_payload));
        response_header->payload_len = sizeof(suspend_rsp_payload);
    } else if (gid == SESSION_CONFIG && oid == SESSION_INIT) {
        if (!payload || payload_len < 5) {
            unsigned char session_init_error_rsp[] = {UCI_STATUS_INVALID_PARAM};
            memcpy(response_packet + sizeof(struct uci_packet_header), session_init_error_rsp, sizeof(session_init_error_rsp));
            response_header->payload_len = sizeof(session_init_error_rsp);
            parse_uci_packet(response_packet, sizeof(struct uci_packet_header) + response_header->payload_len);
            return;
        }

        unsigned int session_id = read_u32_le(payload);
        SessionType session_type = (SessionType)payload[4];

        int session_idx = find_free_session_slot();
        if (session_idx < 0) {
            unsigned char session_init_error_rsp[] = {UCI_STATUS_MAX_SESSIONS_EXCEEDED};
            memcpy(response_packet + sizeof(struct uci_packet_header), session_init_error_rsp, sizeof(session_init_error_rsp));
            response_header->payload_len = sizeof(session_init_error_rsp);
        } else {
            unsigned int session_handle = g_session_handle_counter++;
            if (session_handle == 0) {
                session_handle = g_session_handle_counter++;
            }

            uci_sessions[session_idx].session_id = session_id;
            uci_sessions[session_idx].session_type = session_type;
            uci_sessions[session_idx].session_state = SESSION_STATE_INIT;
            uci_sessions[session_idx].is_allocated = 1;
            uci_sessions[session_idx].num_configs = 0;
            memset(uci_sessions[session_idx].configs, 0, sizeof(uci_sessions[session_idx].configs));
            uci_sessions[session_idx].session_handle = session_handle;
            uci_sessions[session_idx].ranging_count = 0;

            unsigned char session_init_rsp_payload[5];
            session_init_rsp_payload[0] = UCI_STATUS_OK;
            write_u32_le(&session_init_rsp_payload[1], session_handle);
            memcpy(response_packet + sizeof(struct uci_packet_header), session_init_rsp_payload, sizeof(session_init_rsp_payload));
            response_header->payload_len = sizeof(session_init_rsp_payload);

            enqueue_session_status_notification(&uci_sessions[session_idx], SESSION_STATE_INIT, STATE_CHANGE_WITH_SESSION_MANAGEMENT_COMMANDS);
        }
    } else if (gid == SESSION_CONFIG && oid == SESSION_DEINIT) {
        if (!payload || payload_len < 4) {
            unsigned char session_deinit_error_rsp[] = {UCI_STATUS_INVALID_PARAM};
            memcpy(response_packet + sizeof(struct uci_packet_header), session_deinit_error_rsp, sizeof(session_deinit_error_rsp));
            response_header->payload_len = sizeof(session_deinit_error_rsp);
            parse_uci_packet(response_packet, sizeof(struct uci_packet_header) + response_header->payload_len);
            return;
        }

        unsigned int identifier = read_u32_le(payload);

        int session_idx = find_session_by_token_or_id(identifier);
        if (session_idx >= 0) {
            struct uci_session* session = &uci_sessions[session_idx];
            enqueue_session_status_notification(session, SESSION_STATE_DEINIT, STATE_CHANGE_WITH_SESSION_MANAGEMENT_COMMANDS);
            session->session_state = SESSION_STATE_DEINIT;
            session->is_allocated = 0;
            session->session_id = 0;
            session->session_type = 0;
            session->session_handle = 0;
            session->ranging_count = 0;
            session->num_configs = 0;
            memset(session->configs, 0, sizeof(session->configs));
        } else {
            unsigned char err = UCI_STATUS_INVALID_PARAM;
            enqueue_notification(CORE, CORE_GENERIC_ERROR_NTF, &err, 1);
        }

        unsigned char session_deinit_rsp_payload[] = {UCI_STATUS_OK};
        memcpy(response_packet + sizeof(struct uci_packet_header), session_deinit_rsp_payload, sizeof(session_deinit_rsp_payload));
        response_header->payload_len = sizeof(session_deinit_rsp_payload);
    } else if (gid == SESSION_CONTROL && oid == SESSION_START) {
        if (!payload || payload_len < 4) {
            unsigned char session_start_error_rsp[] = {UCI_STATUS_INVALID_PARAM};
            memcpy(response_packet + sizeof(struct uci_packet_header), session_start_error_rsp, sizeof(session_start_error_rsp));
            response_header->payload_len = sizeof(session_start_error_rsp);
            parse_uci_packet(response_packet, sizeof(struct uci_packet_header) + response_header->payload_len);
            return;
        }

        unsigned int identifier = read_u32_le(payload);

        int session_idx = find_session_by_token_or_id(identifier);
        if (session_idx >= 0) {
            struct uci_session* session = &uci_sessions[session_idx];
            session->session_state = SESSION_STATE_ACTIVE;
            session->ranging_count = 0;
            enqueue_session_status_notification(session, SESSION_STATE_ACTIVE, STATE_CHANGE_WITH_SESSION_MANAGEMENT_COMMANDS);
        } else {
            unsigned char err = UCI_STATUS_INVALID_PARAM;
            enqueue_notification(CORE, CORE_GENERIC_ERROR_NTF, &err, 1);
        }

        unsigned char session_start_rsp_payload[] = {UCI_STATUS_OK};
        memcpy(response_packet + sizeof(struct uci_packet_header), session_start_rsp_payload, sizeof(session_start_rsp_payload));
        response_header->payload_len = sizeof(session_start_rsp_payload);
    } else if (gid == SESSION_CONTROL && oid == SESSION_STOP) {
        if (!payload || payload_len < 4) {
            unsigned char session_stop_error_rsp[] = {UCI_STATUS_INVALID_PARAM};
            memcpy(response_packet + sizeof(struct uci_packet_header), session_stop_error_rsp, sizeof(session_stop_error_rsp));
            response_header->payload_len = sizeof(session_stop_error_rsp);
            parse_uci_packet(response_packet, sizeof(struct uci_packet_header) + response_header->payload_len);
            return;
        }

        unsigned int identifier = read_u32_le(payload);

        int session_idx = find_session_by_token_or_id(identifier);
        if (session_idx >= 0) {
            struct uci_session* session = &uci_sessions[session_idx];
            session->session_state = SESSION_STATE_IDLE;
            increment_session_ranging_count(session_idx);
            enqueue_session_status_notification(session, SESSION_STATE_IDLE, STATE_CHANGE_WITH_SESSION_MANAGEMENT_COMMANDS);
        } else {
            unsigned char err = UCI_STATUS_INVALID_PARAM;
            enqueue_notification(CORE, CORE_GENERIC_ERROR_NTF, &err, 1);
        }

        unsigned char session_stop_rsp_payload[] = {UCI_STATUS_OK};
        memcpy(response_packet + sizeof(struct uci_packet_header), session_stop_rsp_payload, sizeof(session_stop_rsp_payload));
        response_header->payload_len = sizeof(session_stop_rsp_payload);
    } else if (gid == SESSION_CONFIG && oid == SESSION_GET_STATE) {
        if (!payload || payload_len < 4) {
            unsigned char get_state_error_rsp[] = {UCI_STATUS_INVALID_PARAM};
            memcpy(response_packet + sizeof(struct uci_packet_header), get_state_error_rsp, sizeof(get_state_error_rsp));
            response_header->payload_len = sizeof(get_state_error_rsp);
            parse_uci_packet(response_packet, sizeof(struct uci_packet_header) + response_header->payload_len);
            return;
        }

        unsigned int identifier = read_u32_le(payload);

        int session_idx = find_session_by_token_or_id(identifier);
        unsigned char session_state;
        if (session_idx >= 0) {
            session_state = uci_sessions[session_idx].session_state;
        } else {
            session_state = SESSION_STATE_DEINIT; // Session not found
        }
        
        // Return response: status + session_state
        unsigned char get_state_rsp_payload[] = {UCI_STATUS_OK, session_state};
        memcpy(response_packet + sizeof(struct uci_packet_header), get_state_rsp_payload, sizeof(get_state_rsp_payload));
        response_header->payload_len = sizeof(get_state_rsp_payload);
    } else if (gid == SESSION_CONFIG && oid == SESSION_GET_COUNT) {
        unsigned char session_count = (unsigned char)get_allocated_session_count();
        unsigned char get_count_rsp_payload[] = {UCI_STATUS_OK, session_count};
        memcpy(response_packet + sizeof(struct uci_packet_header), get_count_rsp_payload, sizeof(get_count_rsp_payload));
        response_header->payload_len = sizeof(get_count_rsp_payload);
    } else if (gid == SESSION_CONTROL && oid == SESSION_GET_RANGING_COUNT) {
        if (!payload || payload_len < 4) {
            unsigned char get_ranging_error_rsp[] = {UCI_STATUS_INVALID_PARAM};
            memcpy(response_packet + sizeof(struct uci_packet_header), get_ranging_error_rsp, sizeof(get_ranging_error_rsp));
            response_header->payload_len = sizeof(get_ranging_error_rsp);
            parse_uci_packet(response_packet, sizeof(struct uci_packet_header) + response_header->payload_len);
            return;
        }

        unsigned int identifier = read_u32_le(payload);
        int session_idx = find_session_by_token_or_id(identifier);
        unsigned short ranging_count = 0;
        unsigned char status = UCI_STATUS_OK;
        if (session_idx >= 0) {
            ranging_count = uci_sessions[session_idx].ranging_count;
        } else {
            status = UCI_STATUS_INVALID_PARAM;
        }

        unsigned char ranging_rsp_payload[3];
        ranging_rsp_payload[0] = status;
        write_u16_le(&ranging_rsp_payload[1], ranging_count);
        memcpy(response_packet + sizeof(struct uci_packet_header), ranging_rsp_payload, sizeof(ranging_rsp_payload));
        response_header->payload_len = sizeof(ranging_rsp_payload);
    } else if (gid == SESSION_CONFIG && oid == SESSION_QUERY_DATA_SIZE_IN_RANGING) {
        if (!payload || payload_len < 4) {
            unsigned char query_data_error_rsp[] = {UCI_STATUS_INVALID_PARAM};
            memcpy(response_packet + sizeof(struct uci_packet_header), query_data_error_rsp, sizeof(query_data_error_rsp));
            response_header->payload_len = sizeof(query_data_error_rsp);
            parse_uci_packet(response_packet, sizeof(struct uci_packet_header) + response_header->payload_len);
            return;
        }

        unsigned int identifier = read_u32_le(payload);
        int session_idx = find_session_by_token_or_id(identifier);
        unsigned short max_data_size = 0x0200;
        unsigned char status = (session_idx >= 0) ? UCI_STATUS_OK : UCI_STATUS_INVALID_PARAM;

        unsigned char query_rsp_payload[3];
        query_rsp_payload[0] = status;
        write_u16_le(&query_rsp_payload[1], max_data_size);
        memcpy(response_packet + sizeof(struct uci_packet_header), query_rsp_payload, sizeof(query_rsp_payload));
        response_header->payload_len = sizeof(query_rsp_payload);
        if (status != UCI_STATUS_OK) {
            enqueue_notification(CORE, CORE_GENERIC_ERROR_NTF, &status, 1);
        }
    } else if (gid == SESSION_CONFIG && oid == SESSION_UPDATE_CONTROLLER_MULTICAST_LIST) {
        unsigned char status = (payload && payload_len >= 5) ? UCI_STATUS_OK : UCI_STATUS_INVALID_PARAM;
        unsigned char multicast_rsp_payload[] = {status};
        memcpy(response_packet + sizeof(struct uci_packet_header), multicast_rsp_payload, sizeof(multicast_rsp_payload));
        response_header->payload_len = sizeof(multicast_rsp_payload);
    } else if (gid == SESSION_CONFIG && oid == SESSION_UPDATE_ACTIVE_ROUNDS_DT_TAG) {
        unsigned char update_dt_tag_rsp[] = {UCI_STATUS_OK};
        memcpy(response_packet + sizeof(struct uci_packet_header), update_dt_tag_rsp, sizeof(update_dt_tag_rsp));
        response_header->payload_len = sizeof(update_dt_tag_rsp);
    } else if (gid == SESSION_CONFIG && oid == SESSION_SET_APP_CONFIG) {
        unsigned char cfg_ids[MAX_RESPONSE_PAYLOAD_LEN] = {0};
        unsigned char processed_tlvs = 0;
        unsigned char declared_tlvs = 0;
        unsigned int identifier = 0;
        int session_idx = -1;

        if (payload && payload_len >= 5) {
            identifier = read_u32_le(payload);
            declared_tlvs = payload[4];
            session_idx = find_session_by_token_or_id(identifier);

            int offset = 5;
            for (unsigned char i = 0; i < declared_tlvs; i++) {
                if (offset + 2 > payload_len) {
                    break;
                }

                unsigned char cfg_id = payload[offset];
                unsigned char cfg_len = payload[offset + 1];
                offset += 2;

                if (offset + cfg_len > payload_len) {
                    break;
                }

                if (session_idx >= 0) {
                    store_session_config(session_idx, cfg_id, &payload[offset], cfg_len);
                }

                if (processed_tlvs < (MAX_RESPONSE_PAYLOAD_LEN - 2) / 2) {
                    cfg_ids[processed_tlvs] = cfg_id;
                    processed_tlvs++;
                }

                offset += cfg_len;
            }
        }

        unsigned char status = UCI_STATUS_OK;
        if (!payload || payload_len < 5 || processed_tlvs != declared_tlvs || session_idx < 0) {
            status = UCI_STATUS_INVALID_PARAM;
            if (session_idx < 0) {
                processed_tlvs = 0;
            }
        }

        unsigned char response_len = (unsigned char)(2 + (processed_tlvs * 2));
        unsigned char set_app_cfg_rsp[MAX_RESPONSE_PAYLOAD_LEN] = {0};

        set_app_cfg_rsp[0] = status;
        set_app_cfg_rsp[1] = processed_tlvs;
        for (unsigned char i = 0; i < processed_tlvs; i++) {
            set_app_cfg_rsp[2 + (i * 2)] = cfg_ids[i];
            set_app_cfg_rsp[2 + (i * 2) + 1] = UCI_STATUS_OK;
        }

        if (status != UCI_STATUS_OK) {
            enqueue_notification(CORE, CORE_GENERIC_ERROR_NTF, &status, 1);
        }

        memcpy(response_packet + sizeof(struct uci_packet_header), set_app_cfg_rsp, response_len);
        response_header->payload_len = response_len;
    } else if (gid == SESSION_CONFIG && oid == SESSION_GET_APP_CONFIG) {
        unsigned int identifier = 0;
        unsigned char declared_cfgs = 0;
        int session_idx = -1;
        unsigned char cfg_count = 0;

        if (payload && payload_len >= 5) {
            identifier = read_u32_le(payload);
            declared_cfgs = payload[4];
            session_idx = find_session_by_token_or_id(identifier);

            unsigned int available_ids = payload_len - 5;
            unsigned int max_cfg_entries = (MAX_RESPONSE_PAYLOAD_LEN - 2) / 3;
            if (declared_cfgs > available_ids) {
                declared_cfgs = (unsigned char)available_ids;
            }
            if (declared_cfgs > max_cfg_entries) {
                declared_cfgs = (unsigned char)max_cfg_entries;
            }
            cfg_count = declared_cfgs;
        }

        if (session_idx < 0) {
            cfg_count = 0;
        }

        unsigned char get_app_cfg_rsp[MAX_RESPONSE_PAYLOAD_LEN] = {0};
        unsigned char status = (session_idx >= 0) ? UCI_STATUS_OK : UCI_STATUS_INVALID_PARAM;
        unsigned char returned_cfgs = 0;
        size_t response_len = 2;

        if (status != UCI_STATUS_OK) {
            unsigned char err = UCI_STATUS_INVALID_PARAM;
            enqueue_notification(CORE, CORE_GENERIC_ERROR_NTF, &err, 1);
        }

        for (unsigned char i = 0; i < cfg_count; i++) {
            unsigned char cfg_id = payload[5 + i];
            unsigned char value_buf[MAX_SESSION_CONFIG_VALUE_SIZE];
            unsigned char value_len = MAX_SESSION_CONFIG_VALUE_SIZE;
            unsigned char copy_len = 0;

            if (session_idx >= 0 &&
                get_session_config(session_idx, cfg_id, value_buf, &value_len) && value_len > 0) {
                copy_len = value_len;
            }

            size_t required = 2 + copy_len;
            if (response_len + required > MAX_RESPONSE_PAYLOAD_LEN) {
                status = UCI_STATUS_INVALID_PARAM;
                unsigned char err = UCI_STATUS_INVALID_PARAM;
                enqueue_notification(CORE, CORE_GENERIC_ERROR_NTF, &err, 1);
                break;
            }

            get_app_cfg_rsp[response_len] = cfg_id;
            get_app_cfg_rsp[response_len + 1] = copy_len;
            if (copy_len > 0) {
                memcpy(&get_app_cfg_rsp[response_len + 2], value_buf, copy_len);
            }
            response_len += required;
            returned_cfgs++;
        }

        get_app_cfg_rsp[0] = status;
        get_app_cfg_rsp[1] = returned_cfgs;

        memcpy(response_packet + sizeof(struct uci_packet_header), get_app_cfg_rsp, response_len);
        response_header->payload_len = (unsigned char)response_len;
    } else if (gid == TEST) {
        if (oid == TEST_RF_SET_CONFIG) {
            unsigned char rf_set_rsp[] = {UCI_STATUS_OK, 0x00};
            memcpy(response_packet + sizeof(struct uci_packet_header), rf_set_rsp, sizeof(rf_set_rsp));
            response_header->payload_len = sizeof(rf_set_rsp);
        } else if (oid == TEST_RF_PERIODIC_TX || oid == TEST_RF_PER_RX || oid == TEST_RF_RX || oid == TEST_RF_STOP) {
            unsigned char status = UCI_STATUS_OK;
            memcpy(response_packet + sizeof(struct uci_packet_header), &status, 1);
            response_header->payload_len = 1;
        } else {
            unsigned char status = UCI_STATUS_OK;
            memcpy(response_packet + sizeof(struct uci_packet_header), &status, 1);
            response_header->payload_len = 1;
        }
    } else if (gid == VENDOR_ANDROID) {
        if (oid == ANDROID_GET_POWER_STATS) {
            unsigned char power_stats_rsp[17] = {0};
            power_stats_rsp[0] = UCI_STATUS_OK;
            memcpy(response_packet + sizeof(struct uci_packet_header), power_stats_rsp, sizeof(power_stats_rsp));
            response_header->payload_len = sizeof(power_stats_rsp);
        } else if (oid == ANDROID_SET_COUNTRY_CODE) {
            unsigned char set_country_rsp[] = {UCI_STATUS_OK};
            memcpy(response_packet + sizeof(struct uci_packet_header), set_country_rsp, sizeof(set_country_rsp));
            response_header->payload_len = sizeof(set_country_rsp);
        } else if (oid == ANDROID_RADAR_SET_APP_CONFIG) {
            unsigned char set_radar_rsp[] = {UCI_STATUS_OK, 0x00};
            memcpy(response_packet + sizeof(struct uci_packet_header), set_radar_rsp, sizeof(set_radar_rsp));
            response_header->payload_len = sizeof(set_radar_rsp);
        } else if (oid == ANDROID_RADAR_GET_APP_CONFIG) {
            unsigned char get_radar_rsp[] = {UCI_STATUS_OK, 0x00};
            memcpy(response_packet + sizeof(struct uci_packet_header), get_radar_rsp, sizeof(get_radar_rsp));
            response_header->payload_len = sizeof(get_radar_rsp);
        } else {
            unsigned char status = UCI_STATUS_OK;
            memcpy(response_packet + sizeof(struct uci_packet_header), &status, 1);
            response_header->payload_len = 1;
        }
    } else {
        unsigned char status = UCI_STATUS_OK;
        memcpy(response_packet + sizeof(struct uci_packet_header), &status, 1);
        response_header->payload_len = 1;
    }
    
    parse_uci_packet(response_packet, sizeof(struct uci_packet_header) + response_header->payload_len);
}

void handle_core_device_suspend_rsp(unsigned char* payload, int payload_len) {
    if (payload_len < 1) {
        printf("Error: CORE_DEVICE_SUSPEND_RSP payload too short.\n");
        return;
    }
    unsigned char status = payload[0];
    printf("  Status: 0x%02X (%s)\n", status, status == UCI_STATUS_OK ? "OK" : "ERROR");
    if (status == UCI_STATUS_OK) {
        printf("  Device suspended successfully.\n");
    } else {
        printf("  Device suspend failed.\n");
    }
}

void handle_core_device_info_rsp(unsigned char* payload, int payload_len) {
    if (payload_len < 9) { // status (1) + uci_version (2) + mac_version (2) + phy_version (2) + uci_test_version (2)
        printf("Error: CORE_DEVICE_INFO_RSP payload too short.\n");
        return;
    }

    unsigned char status = payload[0];
    unsigned short uci_version = read_u16_le(&payload[1]);
    unsigned short mac_version = read_u16_le(&payload[3]);
    unsigned short phy_version = read_u16_le(&payload[5]);
    unsigned short uci_test_version = read_u16_le(&payload[7]);
    // Remaining bytes are vendor_spec_info (may be 0 or more bytes)

    printf("  Status: 0x%02X\n", status);
    printf("  UCI Version: 0x%04X\n", uci_version);
    printf("  MAC Version: 0x%04X\n", mac_version);
    printf("  PHY Version: 0x%04X\n", phy_version);
    printf("  UCI Test Version: 0x%04X\n", uci_test_version);
    
    if (payload_len > 9) {
        printf("  Vendor Specific Info: ");
        for (int i = 9; i < payload_len; i++) {
            printf("%02X ", payload[i]);
        }
        printf("\n");
    }
}

void handle_core_get_caps_info_rsp(unsigned char* payload, int payload_len) {
    if (payload_len < 2) { // status (1) + num_tlvs (1)
        printf("Error: CORE_GET_CAPS_INFO_RSP payload too short.\n");
        return;
    }

    unsigned char status = payload[0];
    unsigned char num_tlvs = payload[1];
    printf("  Status: 0x%02X\n", status);
    printf("  Number of TLVs: %d\n", num_tlvs);

    int offset = 2;
    for (int i = 0; i < num_tlvs; i++) {
        if (offset + 2 > payload_len) {
            printf("Error: Incomplete TLV in CORE_GET_CAPS_INFO_RSP payload.\n");
            return;
        }
        CapTlvType tlv_type = (CapTlvType)payload[offset];
        unsigned char tlv_len = payload[offset + 1];
        printf("    TLV Type: 0x%02X, Length: %d, Value: ", tlv_type, tlv_len);
        offset += 2;
        if (offset + tlv_len > payload_len) {
            printf("Error: Incomplete TLV value in CORE_GET_CAPS_INFO_RSP payload.\n");
            return;
        }
        for (int j = 0; j < tlv_len; j++) {
            printf("%02X ", payload[offset + j]);
        }
        printf("\n");
        offset += tlv_len;
    }
}

void handle_core_set_config_rsp(unsigned char* payload, int payload_len) {
    if (payload_len < 2) { // status (1) + num_cfg_status (1)
        printf("Error: CORE_SET_CONFIG_RSP payload too short.\n");
        return;
    }

    unsigned char status = payload[0];
    unsigned char num_cfg_status = payload[1];
    printf("  Status: 0x%02X\n", status);
    printf("  Number of Config Status: %d\n", num_cfg_status);

    int offset = 2;
    for (int i = 0; i < num_cfg_status; i++) {
        if (offset + 2 > payload_len) {
            printf("Error: Incomplete Config Status in CORE_SET_CONFIG_RSP payload.\n");
            return;
        }
        DeviceConfigId cfg_id = (DeviceConfigId)payload[offset];
        unsigned char cfg_status = payload[offset + 1];
        printf("    Config ID: 0x%02X (%s), Status: 0x%02X", cfg_id, 
               cfg_id == DEVICE_STATE ? "DEVICE_STATE" : 
               cfg_id == LOW_POWER_MODE ? "LOW_POWER_MODE" : "UNKNOWN", 
               cfg_status);
        if (cfg_status == UCI_STATUS_OK) {
            printf(" (OK)");
        } else if (cfg_status == UCI_STATUS_INVALID_PARAM) {
            printf(" (Invalid Parameter)");
        } else if (cfg_status == UCI_STATUS_REJECTED) {
            printf(" (Rejected)");
        }
        printf("\n");
        offset += 2;
    }
}

// Helper function to print device config ID name
const char* get_device_config_name(DeviceConfigId cfg_id) {
    switch(cfg_id) {
        case DEVICE_STATE: return "DEVICE_STATE";
        case LOW_POWER_MODE: return "LOW_POWER_MODE";
        default: return "UNKNOWN";
    }
}

// Helper function to interpret and print device state value
void print_device_state_value(unsigned char value) {
    switch(value) {
        case DEVICE_STATE_READY: printf("(READY)"); break;
        case DEVICE_STATE_ACTIVE: printf("(ACTIVE)"); break;
        case DEVICE_STATE_ERROR: printf("(ERROR)"); break;
        default: printf("(UNKNOWN: 0x%02X)", value); break;
    }
}


void handle_core_device_reset_rsp(unsigned char* payload, int payload_len) {
    if (payload_len < 1) {
        printf("Error: CORE_DEVICE_RESET_RSP payload too short.\n");
        return;
    }
    unsigned char status = payload[0];
    printf("  Status: 0x%02X (%s)\n", status, status == UCI_STATUS_OK ? "OK" : "ERROR");
    if (status == UCI_STATUS_OK) {
        printf("  Device reset successfully.\n");
    } else {
        printf("  Device reset failed.\n");
    }
}

void handle_core_get_config_rsp(unsigned char* payload, int payload_len) {
    if (payload_len < 2) { // status (1) + num_tlvs (1)
        printf("Error: CORE_GET_CONFIG_RSP payload too short.\n");
        return;
    }

    unsigned char status = payload[0];
    unsigned char num_tlvs = payload[1];
    printf("  Status: 0x%02X\n", status);
    printf("  Number of TLVs: %d\n", num_tlvs);

    int offset = 2;
    for (int i = 0; i < num_tlvs; i++) {
        if (offset + 2 > payload_len) {
            printf("Error: Incomplete TLV in CORE_GET_CONFIG_RSP payload.\n");
            return;
        }
        DeviceConfigId cfg_id = (DeviceConfigId)payload[offset];
        unsigned char tlv_len = payload[offset + 1];
        offset += 2;
        if (offset + tlv_len > payload_len) {
            printf("Error: Incomplete TLV value in CORE_GET_CONFIG_RSP payload.\n");
            return;
        }
        
        printf("    Config ID: 0x%02X (%s), Length: %d, Value: ", cfg_id, 
               get_device_config_name(cfg_id), tlv_len);
        
        for (int j = 0; j < tlv_len; j++) {
            printf("%02X ", payload[offset + j]);
        }
        
        // Interpret the value based on config ID
        if (cfg_id == DEVICE_STATE && tlv_len == 1) {
            print_device_state_value(payload[offset]);
        } else if (cfg_id == LOW_POWER_MODE && tlv_len == 1) {
            unsigned char lpm_state = payload[offset];
            printf("(LPM: %s)", lpm_state ? "ON" : "OFF");
        }
        
        printf("\n");
        offset += tlv_len;
    }
}

// Helper function to initialize session storage
void init_uci_sessions() {
    g_session_handle_counter = 1;
    g_notification_head = 0;
    g_notification_tail = 0;
    for (int i = 0; i < MAX_SESSIONS; i++) {
        uci_sessions[i].session_id = 0;
        uci_sessions[i].session_type = 0;
        uci_sessions[i].is_allocated = 0;
        uci_sessions[i].session_state = SESSION_STATE_DEINIT;
        uci_sessions[i].num_configs = 0;
        uci_sessions[i].session_handle = 0;
        uci_sessions[i].ranging_count = 0;
        memset(uci_sessions[i].configs, 0, sizeof(uci_sessions[i].configs));
    }
}

// Helper function to find an available session slot
int find_free_session_slot() {
    for (int i = 0; i < MAX_SESSIONS; i++) {
        if (!uci_sessions[i].is_allocated) {
            return i;
        }
    }
    return -1; // No free slots
}

// Helper function to find a session by ID
int find_session_by_id(unsigned int session_id) {
    for (int i = 0; i < MAX_SESSIONS; i++) {
        if (uci_sessions[i].is_allocated && uci_sessions[i].session_id == session_id) {
            return i;
        }
    }
    return -1; // Session not found
}

int find_session_by_handle(unsigned int session_handle) {
    for (int i = 0; i < MAX_SESSIONS; i++) {
        if (uci_sessions[i].is_allocated && uci_sessions[i].session_handle == session_handle) {
            return i;
        }
    }
    return -1;
}

int find_session_by_token_or_id(unsigned int identifier) {
    int idx = find_session_by_handle(identifier);
    if (idx >= 0) {
        return idx;
    }
    return find_session_by_id(identifier);
}

int get_allocated_session_count() {
    int count = 0;
    for (int i = 0; i < MAX_SESSIONS; i++) {
        if (uci_sessions[i].is_allocated) {
            count++;
        }
    }
    return count;
}

void increment_session_ranging_count(int session_idx) {
    if (session_idx < 0 || session_idx >= MAX_SESSIONS) {
        return;
    }
    if (!uci_sessions[session_idx].is_allocated) {
        return;
    }
    if (uci_sessions[session_idx].ranging_count < 0xFFFF) {
        uci_sessions[session_idx].ranging_count++;
    }
}

// Helper function to store configuration value in session
void store_session_config(int session_idx, unsigned char cfg_id, unsigned char* value, unsigned char len) {
    if (session_idx < 0 || session_idx >= MAX_SESSIONS) {
        printf("Error: Invalid session index %d in store_session_config\n", session_idx);
        return;
    }
    if (!value) {
        printf("Error: Null value pointer in store_session_config\n");
        return;
    }
    if (len == 0) {
        printf("Error: Zero length configuration value in store_session_config\n");
        return;
    }

    struct uci_session* session = &uci_sessions[session_idx];

    for (int i = 0; i < MAX_SESSION_CONFIGS; i++) {
        uci_session_config_entry* entry = &session->configs[i];
        if (entry->in_use && entry->cfg_id == cfg_id) {
            entry->length = len;
            memcpy(entry->value, value, len);
            return;
        }
    }

    for (int i = 0; i < MAX_SESSION_CONFIGS; i++) {
        uci_session_config_entry* entry = &session->configs[i];
        if (!entry->in_use) {
            entry->in_use = 1;
            entry->cfg_id = cfg_id;
            entry->length = len;
            memcpy(entry->value, value, len);
            if (session->num_configs < MAX_SESSION_CONFIGS) {
                session->num_configs++;
            }
            return;
        }
    }

    printf("Warning: No space to store session config 0x%02X\n", cfg_id);
}

// Helper function to get configuration value from session
int get_session_config(int session_idx, unsigned char cfg_id, unsigned char* value, unsigned char* len) {
    if (session_idx < 0 || session_idx >= MAX_SESSIONS) {
        printf("Error: Invalid session index %d in get_session_config\n", session_idx);
        return 0;
    }
    if (!len) {
        printf("Error: Null length pointer in get_session_config\n");
        return 0;
    }

    struct uci_session* session = &uci_sessions[session_idx];
    for (int i = 0; i < MAX_SESSION_CONFIGS; i++) {
        uci_session_config_entry* entry = &session->configs[i];
        if (entry->in_use && entry->cfg_id == cfg_id) {
            unsigned char available = *len;
            *len = entry->length;
            if (value && available > 0) {
                size_t copy_len = entry->length;
                if (copy_len > available) {
                    copy_len = available;
                    printf("Warning: Buffer too small for session config 0x%02X (need %u bytes)\n",
                           cfg_id, entry->length);
                }
                memcpy(value, entry->value, copy_len);
            }
            return 1;
        }
    }

    *len = 0;
    return 0;
}

// Notification handlers
void handle_core_device_status_ntf(unsigned char* payload, int payload_len) {
    if (payload_len < 1) {
        printf("Error: CORE_DEVICE_STATUS_NTF payload too short.\n");
        return;
    }

    unsigned char device_state = payload[0];
    printf("  Device State: 0x%02X", device_state);
    switch(device_state) {
        case DEVICE_STATE_READY: printf(" (READY)"); break;
        case DEVICE_STATE_ACTIVE: printf(" (ACTIVE)"); break;
        case 0xFF: printf(" (ERROR)"); break;
        default: printf(" (UNKNOWN)"); break;
    }
    printf("\n");
}

void handle_core_generic_error_ntf(unsigned char* payload, int payload_len) {
    if (payload_len < 1) {
        printf("Error: CORE_GENERIC_ERROR_NTF payload too short.\n");
        return;
    }
    
    unsigned char status = payload[0];
    printf("  Generic Error Status: 0x%02X", status);
    switch(status) {
        case UCI_STATUS_OK: printf(" (OK)"); break;
        case UCI_STATUS_REJECTED: printf(" (REJECTED)"); break;
        case UCI_STATUS_FAILED: printf(" (FAILED)"); break;
        case UCI_STATUS_SYNTAX_ERROR: printf(" (SYNTAX_ERROR)"); break;
        case UCI_STATUS_INVALID_PARAM: printf(" (INVALID_PARAM)"); break;
        default: printf(" (UNKNOWN)"); break;
    }
    printf("\n");
}

void handle_generic_notification(unsigned char gid, unsigned char opcode, unsigned char* payload, int payload_len) {
    printf("  [Generic Notification - GID: 0x%02X, OID: 0x%02X]\n", gid, opcode);
    printf("  Payload: ");
    for (int i = 0; i < payload_len; i++) {
        printf("%02X ", payload[i]);
    }
    printf("\n");
}

// Session Configuration Notifications
void handle_session_config_ntf(unsigned char opcode, unsigned char* payload, int payload_len) {
    if (opcode == SESSION_STATUS_NTF) {
        if (payload_len < 6) { // session_id(4) + session_state(1) + reason_code(1)
            printf("  Error: SESSION_STATUS_NTF payload too short.\n");
            return;
        }
        
        unsigned int session_token = read_u32_le(payload);
        unsigned char session_state = payload[4];
        unsigned char reason_code = payload[5];
        
        printf("  Session Token: 0x%08X\n", session_token);
        printf("  Session State: 0x%02X", session_state);
        switch(session_state) {
            case SESSION_STATE_INIT: printf(" (INIT)"); break;
            case SESSION_STATE_DEINIT: printf(" (DEINIT)"); break;
            case SESSION_STATE_ACTIVE: printf(" (ACTIVE)"); break;
            case SESSION_STATE_IDLE: printf(" (IDLE)"); break;
            default: printf(" (UNKNOWN)"); break;
        }
        printf("\n");
        printf("  Reason Code: 0x%02X", reason_code);
        switch(reason_code) {
            case STATE_CHANGE_WITH_SESSION_MANAGEMENT_COMMANDS: printf(" (SESSION_MANAGEMENT_COMMAND)"); break;
            case MAX_RANGING_ROUND_RETRY_COUNT_REACHED: printf(" (MAX_RETRY_COUNT_REACHED)"); break;
            case MAX_NUMBER_OF_MEASUREMENTS_REACHED: printf(" (MAX_MEASUREMENTS_REACHED)"); break;
            case SESSION_SUSPENDED_DUE_TO_INBAND_SIGNAL: printf(" (SUSPENDED_INBAND_SIGNAL)"); break;
            case SESSION_RESUMED_DUE_TO_INBAND_SIGNAL: printf(" (RESUMED_INBAND_SIGNAL)"); break;
            case SESSION_STOPPED_DUE_TO_INBAND_SIGNAL: printf(" (STOPPED_INBAND_SIGNAL)"); break;
            default: printf(" (UNKNOWN_REASON)"); break;
        }
        printf("\n");
    } else {
        handle_generic_notification(SESSION_CONFIG, opcode, payload, payload_len);
    }
}

// Session Control Notifications
void handle_session_control_ntf(unsigned char opcode, unsigned char* payload, int payload_len) {
    if (opcode == SESSION_DATA_CREDIT_NTF) {
        if (payload_len < 5) { // session_token(4) + credit_availability(1)
            printf("  Error: SESSION_DATA_CREDIT_NTF payload too short.\n");
            return;
        }
        
        unsigned int session_token = read_u32_le(payload);
        unsigned char credit_availability = payload[4];
        
        printf("  Session Token: 0x%08X\n", session_token);
        printf("  Credit Availability: %s\n", credit_availability ? "AVAILABLE" : "NOT_AVAILABLE");
    } else if (opcode == SESSION_DATA_TRANSFER_STATUS_NTF) {
        if (payload_len < 6) { // session_token(4) + uci_seq_num(2) + status(1) + tx_count(1)
            printf("  Error: SESSION_DATA_TRANSFER_STATUS_NTF payload too short.\n");
            return;
        }
        
        unsigned int session_token = read_u32_le(payload);
        unsigned short uci_sequence_number = read_u16_le(&payload[4]);
        unsigned char status = payload[6];
        unsigned char tx_count = payload[7];
        
        printf("  Session Token: 0x%08X\n", session_token);
        printf("  UCI Sequence Number: %d\n", uci_sequence_number);
        printf("  Status: 0x%02X", status);
        switch(status) {
            case UCI_DATA_TRANSFER_STATUS_REPETITION_OK: printf(" (REPETITION_OK)"); break;
            case UCI_DATA_TRANSFER_STATUS_OK: printf(" (OK)"); break;
            case UCI_DATA_TRANSFER_STATUS_ERROR_DATA_TRANSFER: printf(" (ERROR_DATA_TRANSFER)"); break;
            case UCI_DATA_TRANSFER_STATUS_ERROR_NO_CREDIT_AVAILABLE: printf(" (ERROR_NO_CREDIT_AVAILABLE)"); break;
            case UCI_DATA_TRANSFER_STATUS_ERROR_REJECTED: printf(" (ERROR_REJECTED)"); break;
            case UCI_DATA_TRANSFER_STATUS_SESSION_TYPE_NOT_SUPPORTED: printf(" (SESSION_TYPE_NOT_SUPPORTED)"); break;
            case UCI_DATA_TRANSFER_STATUS_ERROR_DATA_TRANSFER_IS_ONGOING: printf(" (ERROR_DATA_TRANSFER_IS_ONGOING)"); break;
            case UCI_DATA_TRANSFER_STATUS_INVALID_FORMAT: printf(" (INVALID_FORMAT)"); break;
            default: printf(" (UNKNOWN)"); break;
        }
        printf("\n");
        printf("  TX Count: %d\n", tx_count);
    } else if (opcode == SESSION_INFO_NTF) {
        // Handle ranging/session info notifications - the core UWB functionality
        handle_session_info_ntf(payload, payload_len);
    } else {
        handle_generic_notification(SESSION_CONTROL, opcode, payload, payload_len);
    }
}

// Ranging/Session Info Notification Handler
void handle_session_info_ntf(unsigned char* payload, int payload_len) {
    if (!payload || payload_len < 20) {
        printf("  Error: SESSION_INFO_NTF payload too short. Need at least 20 bytes, got %d.\n", payload_len);
        return;
    }
    
    // Parse header fields
    unsigned int sequence_number = read_u32_le(&payload[0]);
    unsigned int session_token = read_u32_le(&payload[4]);
    unsigned char rcr_indicator = payload[8];
    unsigned int current_ranging_interval = read_u32_le(&payload[9]);
    unsigned char ranging_measurement_type = payload[13];
    unsigned char mac_address_indicator = payload[15]; // Skip reserved byte at position 14
    unsigned int hus_primary_session_id = read_u32_le(&payload[16]);
    
    printf("  Sequence Number: %u\n", sequence_number);
    printf("  Session Token: 0x%08X\n", session_token);
    printf("  RCR Indicator: 0x%02X\n", rcr_indicator);
    printf("  Current Ranging Interval: %u ms\n", current_ranging_interval);
    printf("  Ranging Measurement Type: 0x%02X", ranging_measurement_type);
    switch(ranging_measurement_type) {
        case 0x00: printf(" (ONE_WAY)"); break;
        case 0x01: printf(" (TWO_WAY)"); break;
        case 0x02: printf(" (DL_TDOA)"); break;
        case 0x03: printf(" (OWR_AOA)"); break;
        default: printf(" (UNKNOWN)"); break;
    }
    printf("\n");
    printf("  MAC Address Indicator: %s\n", mac_address_indicator ? "EXTENDED_ADDRESS" : "SHORT_ADDRESS");
    printf("  HUS Primary Session ID: 0x%08X\n", hus_primary_session_id);
    
    // Parse ranging measurements
    int offset = 20; // Header size (20 bytes)
    if (offset >= payload_len) {
        printf("  No ranging measurements in notification (offset=%d, payload_len=%d).\n", offset, payload_len);
        return;
    }
    
    printf("  Ranging Measurements (offset=%d, payload_len=%d):\n", offset, payload_len);
    
    if (ranging_measurement_type == 0x01) { // TWO_WAY
        unsigned char num_measurements = payload[offset];
        offset += 1;
        printf("    Number of Two-Way Measurements: %d\n", num_measurements);
        
        for (int i = 0; i < num_measurements && offset < payload_len; i++) {
            if (offset + 20 > payload_len) { // Size for SHORT_ADDRESS two-way measurement = 20 bytes
                printf("    Warning: Incomplete measurement data at index %d (need offset+%d=%d but have %d)\n", 
                       i, 20, offset + 20, payload_len);
                break;
            }
            
            printf("    Measurement %d:\n", i+1);
            
            if (mac_address_indicator == 0) { // SHORT_ADDRESS
                if (offset + 20 > payload_len) {
                    printf("      Error: Insufficient data for SHORT_ADDRESS measurement\n");
                    break;
                }
                
                unsigned short mac_address = read_u16_le(&payload[offset]);
                unsigned char status = payload[offset + 2];
                unsigned char nlos = payload[offset + 3];
                unsigned short distance = read_u16_le(&payload[offset + 4]);
                unsigned short aoa_azimuth = read_u16_le(&payload[offset + 6]);
                unsigned char aoa_azimuth_fom = payload[offset + 8];
                unsigned short aoa_elevation = read_u16_le(&payload[offset + 9]);
                unsigned char aoa_elevation_fom = payload[offset + 11];
                unsigned short aoa_destination_azimuth = read_u16_le(&payload[offset + 12]);
                unsigned char aoa_destination_azimuth_fom = payload[offset + 14];
                unsigned short aoa_destination_elevation = read_u16_le(&payload[offset + 15]);
                unsigned char aoa_destination_elevation_fom = payload[offset + 17];
                unsigned char slot_index = payload[offset + 18];
                unsigned char rssi = payload[offset + 19];
                
                printf("      MAC Address: 0x%04X\n", mac_address);
                printf("      Status: 0x%02X", status);
                if (status == 0x00) printf(" (OK)");
                printf("\n");
                printf("      NLOS: %s\n", nlos ? "YES" : "NO");
                printf("      Distance: %u cm\n", distance);
                printf("      AoA Azimuth: %u degrees (FoM: %u)\n", aoa_azimuth, aoa_azimuth_fom);
                printf("      AoA Elevation: %u degrees (FoM: %u)\n", aoa_elevation, aoa_elevation_fom);
                printf("      Destination AoA Azimuth: %u degrees (FoM: %u)\n", aoa_destination_azimuth, aoa_destination_azimuth_fom);
                printf("      Destination AoA Elevation: %u degrees (FoM: %u)\n", aoa_destination_elevation, aoa_destination_elevation_fom);
                printf("      Slot Index: %u\n", slot_index);
                printf("      RSSI: %d dBm\n", (signed char)rssi);
                
                offset += 20; // Move to next measurement
            } else { // EXTENDED_ADDRESS
                if (offset + 26 > payload_len) {
                    printf("      Error: Insufficient data for EXTENDED_ADDRESS measurement\n");
                    break;
                }
                
                unsigned long long mac_address = read_u64_le(&payload[offset]);
                unsigned char status = payload[offset + 8];
                unsigned char nlos = payload[offset + 9];
                unsigned short distance = read_u16_le(&payload[offset + 10]);
                unsigned short aoa_azimuth = read_u16_le(&payload[offset + 12]);
                unsigned char aoa_azimuth_fom = payload[offset + 14];
                unsigned short aoa_elevation = read_u16_le(&payload[offset + 15]);
                unsigned char aoa_elevation_fom = payload[offset + 17];
                unsigned short aoa_destination_azimuth = read_u16_le(&payload[offset + 18]);
                unsigned char aoa_destination_azimuth_fom = payload[offset + 20];
                unsigned short aoa_destination_elevation = read_u16_le(&payload[offset + 21]);
                unsigned char aoa_destination_elevation_fom = payload[offset + 23];
                unsigned char slot_index = payload[offset + 24];
                unsigned char rssi = payload[offset + 25];
                
                printf("      MAC Address: 0x%016llX\n", mac_address);
                printf("      Status: 0x%02X", status);
                if (status == 0x00) printf(" (OK)");
                printf("\n");
                printf("      NLOS: %s\n", nlos ? "YES" : "NO");
                printf("      Distance: %u cm\n", distance);
                printf("      AoA Azimuth: %u degrees (FoM: %u)\n", aoa_azimuth, aoa_azimuth_fom);
                printf("      AoA Elevation: %u degrees (FoM: %u)\n", aoa_elevation, aoa_elevation_fom);
                printf("      Destination AoA Azimuth: %u degrees (FoM: %u)\n", aoa_destination_azimuth, aoa_destination_azimuth_fom);
                printf("      Destination AoA Elevation: %u degrees (FoM: %u)\n", aoa_destination_elevation, aoa_destination_elevation_fom);
                printf("      Slot Index: %u\n", slot_index);
                printf("      RSSI: %d dBm\n", (signed char)rssi);
                
                offset += 26; // Move to next measurement
            }
        }
    } else if (ranging_measurement_type == 0x03) { // OWR_AOA
        unsigned char num_measurements = payload[offset];
        offset += 1;
        printf("    Number of OWR-AoA Measurements: %d\n", num_measurements);
        
        for (int i = 0; i < num_measurements && offset + 13 <= payload_len; i++) {
            printf("    OWR-AoA Measurement %d:\n", i + 1);
            
            if (mac_address_indicator == 0x00) { // SHORT_ADDRESS
                unsigned short mac_address = read_u16_le(&payload[offset]);
                unsigned char status = payload[offset + 2];
                unsigned char nlos = payload[offset + 3];
                unsigned char frame_sequence_number = payload[offset + 4];
                unsigned short block_index = read_u16_le(&payload[offset + 5]);
                unsigned short aoa_azimuth = read_u16_le(&payload[offset + 7]);
                unsigned char aoa_azimuth_fom = payload[offset + 9];
                unsigned short aoa_elevation = read_u16_le(&payload[offset + 10]);
                unsigned char aoa_elevation_fom = payload[offset + 12];
                
                printf("      MAC Address: 0x%04X\n", mac_address);
                printf("      Status: 0x%02X", status);
                if (status == 0x00) printf(" (OK)");
                printf("\n");
                printf("      NLOS: %s\n", nlos ? "YES" : "NO");
                printf("      Frame Sequence Number: %u\n", frame_sequence_number);
                printf("      Block Index: %u\n", block_index);
                printf("      AoA Azimuth: %u degrees (FoM: %u)\n", aoa_azimuth, aoa_azimuth_fom);
                printf("      AoA Elevation: %u degrees (FoM: %u)\n", aoa_elevation, aoa_elevation_fom);
                
                offset += 13;
            } else { // EXTENDED_ADDRESS
                if (offset + 19 <= payload_len) {
                    unsigned long long mac_address = read_u64_le(&payload[offset]);
                    unsigned char status = payload[offset + 8];
                    unsigned char nlos = payload[offset + 9];
                    unsigned char frame_sequence_number = payload[offset + 10];
                    unsigned short block_index = read_u16_le(&payload[offset + 11]);
                    unsigned short aoa_azimuth = read_u16_le(&payload[offset + 13]);
                    unsigned char aoa_azimuth_fom = payload[offset + 15];
                    unsigned short aoa_elevation = read_u16_le(&payload[offset + 16]);
                    unsigned char aoa_elevation_fom = payload[offset + 18];
                    
                    printf("      MAC Address: 0x%016llX\n", mac_address);
                    printf("      Status: 0x%02X", status);
                    if (status == 0x00) printf(" (OK)");
                    printf("\n");
                    printf("      NLOS: %s\n", nlos ? "YES" : "NO");
                    printf("      Frame Sequence Number: %u\n", frame_sequence_number);
                    printf("      Block Index: %u\n", block_index);
                    printf("      AoA Azimuth: %u degrees (FoM: %u)\n", aoa_azimuth, aoa_azimuth_fom);
                    printf("      AoA Elevation: %u degrees (FoM: %u)\n", aoa_elevation, aoa_elevation_fom);
                    
                    offset += 19;
                } else {
                    printf("        ERROR: Insufficient data for EXTENDED_ADDRESS OWR-AoA measurement\n");
                    break;
                }
            }
        }
    } else {
        printf("    Unsupported ranging measurement type: 0x%02X\n", ranging_measurement_type);
    }
}

void parse_uci_packet(unsigned char* packet, size_t packet_len) {
    if (packet_len < sizeof(struct uci_packet_header)) {
        printf("Error: UCI packet too short to contain a header.\n");
        return;
    }

    struct uci_packet_header* header = (struct uci_packet_header*)packet;

    size_t available_payload = packet_len - sizeof(struct uci_packet_header);
    size_t payload_len = header->payload_len;
    if (payload_len > available_payload) {
        printf("  Warning: Header payload length %zu exceeds available data %zu. Clamping.\n",
               payload_len, available_payload);
        payload_len = available_payload;
    }

    printf("Received UCI packet:\n");
    printf("  MT: 0x%01X\n", get_mt(header));
    printf("  PBF: 0x%01X\n", get_pbf(header));
    printf("  GID: 0x%01X\n", get_gid(header));
    printf("  Opcode: 0x%02X\n", get_opcode(header));
    printf("  Payload Length: %zu\n", payload_len);

    if (payload_len > 0) {
        printf("  Payload: ");
        for (size_t i = 0; i < payload_len; i++) {
            printf("%02X ", packet[sizeof(struct uci_packet_header) + i]);
        }
        printf("\n");
    }

    unsigned char* payload_ptr = packet + sizeof(struct uci_packet_header);
    int payload_len_int = (int)payload_len;

    if (get_mt(header) == RESPONSE && get_gid(header) == CORE && get_opcode(header) == CORE_DEVICE_INFO) {
        handle_core_device_info_rsp(payload_ptr, payload_len_int);
    } else if (get_mt(header) == RESPONSE && get_gid(header) == CORE && get_opcode(header) == CORE_DEVICE_SUSPEND) {
        handle_core_device_suspend_rsp(payload_ptr, payload_len_int);
    } else if (get_mt(header) == NOTIFICATION && get_gid(header) == CORE) {
        if (get_opcode(header) == CORE_DEVICE_STATUS_NTF) {
            handle_core_device_status_ntf(payload_ptr, payload_len_int);
        } else if (get_opcode(header) == CORE_GENERIC_ERROR_NTF) {
            handle_core_generic_error_ntf(payload_ptr, payload_len_int);
        } else {
            handle_generic_notification(get_gid(header), get_opcode(header), payload_ptr, payload_len_int);
        }
    } else if (get_mt(header) == RESPONSE && get_gid(header) == CORE && get_opcode(header) == CORE_GET_CAPS_INFO) {
        handle_core_get_caps_info_rsp(payload_ptr, payload_len_int);
    } else if (get_mt(header) == RESPONSE && get_gid(header) == CORE && get_opcode(header) == CORE_SET_CONFIG) {
        handle_core_set_config_rsp(payload_ptr, payload_len_int);
    } else if (get_mt(header) == RESPONSE && get_gid(header) == CORE && get_opcode(header) == CORE_DEVICE_RESET) {
        handle_core_device_reset_rsp(payload_ptr, payload_len_int);
    } else if (get_mt(header) == RESPONSE && get_gid(header) == CORE && get_opcode(header) == CORE_GET_CONFIG) {
        handle_core_get_config_rsp(payload_ptr, payload_len_int);
    } else if (get_mt(header) == NOTIFICATION && get_gid(header) == SESSION_CONFIG) {
        handle_session_config_ntf(get_opcode(header), payload_ptr, payload_len_int);
    } else if (get_mt(header) == NOTIFICATION && get_gid(header) == SESSION_CONTROL) {
        handle_session_control_ntf(get_opcode(header), payload_ptr, payload_len_int);
    } else if (get_mt(header) == NOTIFICATION) {
        // Handle other notification types generically if not specifically handled
        handle_generic_notification(get_gid(header), get_opcode(header), payload_ptr, payload_len_int);
    }
}

// === Payload Decoding Functions ===

// CORE_GROUP Payload Decoders
void decode_core_device_info_rsp(unsigned char* payload, int payload_len) {
    printf("    CORE_DEVICE_INFO_RSP - Device Information Response\n");
    
    if (payload_len < 9) {
        printf("      ERROR: Payload too short (%d bytes, need at least 9)\n", payload_len);
        return;
    }
    
    unsigned char status = payload[0];
    unsigned short uci_version = read_u16_le(&payload[1]);
    unsigned short mac_version = read_u16_le(&payload[3]);
    unsigned short phy_version = read_u16_le(&payload[5]);
    unsigned short uci_test_version = read_u16_le(&payload[7]);
    
    printf("      Status: 0x%02X", status);
    switch(status) {
        case UCI_STATUS_OK: printf(" (OK)\n"); break;
        case UCI_STATUS_REJECTED: printf(" (REJECTED)\n"); break;
        case UCI_STATUS_FAILED: printf(" (FAILED)\n"); break;
        case UCI_STATUS_SYNTAX_ERROR: printf(" (SYNTAX_ERROR)\n"); break;
        case UCI_STATUS_INVALID_PARAM: printf(" (INVALID_PARAM)\n"); break;
        case UCI_STATUS_INVALID_RANGE: printf(" (INVALID_RANGE)\n"); break;
        case UCI_STATUS_INVALID_MSG_SIZE: printf(" (INVALID_MSG_SIZE)\n"); break;
        default: printf(" (UNKNOWN)\n"); break;
    }
    
    printf("      UCI Version: 0x%04X\n", uci_version);
    printf("      MAC Version: 0x%04X\n", mac_version);
    printf("      PHY Version: 0x%04X\n", phy_version);
    printf("      UCI Test Version: 0x%04X\n", uci_test_version);
    
    if (payload_len > 9) {
        printf("      Vendor Specific Info (%d bytes): ", payload_len - 9);
        for (int i = 9; i < payload_len; i++) {
            printf("%02X ", payload[i]);
        }
        printf("\n");
    }
}

void decode_core_get_caps_info_rsp(unsigned char* payload, int payload_len) {
    printf("    CORE_GET_CAPS_INFO_RSP - Get Capabilities Information Response\n");
    
    if (payload_len < 2) {
        printf("      ERROR: Payload too short (%d bytes, need at least 2)\n", payload_len);
        return;
    }
    
    unsigned char status = payload[0];
    unsigned char num_tlvs = payload[1];
    
    printf("      Status: 0x%02X", status);
    switch(status) {
        case UCI_STATUS_OK: printf(" (OK)\n"); break;
        case UCI_STATUS_REJECTED: printf(" (REJECTED)\n"); break;
        case UCI_STATUS_FAILED: printf(" (FAILED)\n"); break;
        case UCI_STATUS_INVALID_PARAM: printf(" (INVALID_PARAM)\n"); break;
        default: printf(" (UNKNOWN)\n"); break;
    }
    
    printf("      Number of TLVs: %d\n", num_tlvs);
    
    if (num_tlvs > 0 && payload_len >= 2) {
        int offset = 2;
        for (int i = 0; i < num_tlvs && offset + 3 <= payload_len; i++) {
            CapTlvType tlv_type = (CapTlvType)payload[offset];
            unsigned char tlv_len = payload[offset + 1];
            offset += 2;
            
            printf("      TLV %d:\n", i);
            printf("        Type: 0x%02X", tlv_type);
            switch(tlv_type) {
                case SUPPORTED_V1_FIRA_PHY_VERSION_RANGE_V2_MAX_MESSAGE_SIZE: printf(" (PHY_VERSION_RANGE)\n"); break;
                case SUPPORTED_V1_FIRA_MAC_VERSION_RANGE_V2_MAX_DATA_PAYLOAD_SIZE: printf(" (MAC_VERSION_RANGE)\n"); break;
                case SUPPORTED_V1_DEVICE_ROLES_V2_FIRA_PHY_VERSION_RANGE: printf(" (DEVICE_ROLES)\n"); break;
                case SUPPORTED_V1_RANGING_METHOD_V2_FIRA_MAC_VERSION_RANGE: printf(" (RANGING_METHOD)\n"); break;
                case SUPPORTED_V1_STS_CONFIG_V2_DEVICE_TYPE: printf(" (STS_CONFIG)\n"); break;
                case SUPPORTED_V1_MULTI_NODE_MODES_V2_DEVICE_ROLES: printf(" (MULTI_NODE_MODES)\n"); break;
                case SUPPORTED_V1_RANGING_TIME_STRUCT_V2_RANGING_METHOD: printf(" (RANGING_TIME_STRUCT)\n"); break;
                case SUPPORTED_V1_SCHEDULED_MODE_V2_STS_CONFIG: printf(" (SCHEDULED_MODE)\n"); break;
                case SUPPORTED_V1_HOPPING_MODE_V2_MULTI_NODE_MODE: printf(" (HOPPING_MODE)\n"); break;
                case SUPPORTED_V1_BLOCK_STRIDING_V2_RANGING_TIME_STRUCT: printf(" (BLOCK_STRIDING)\n"); break;
                case SUPPORTED_V1_UWB_INITIATION_TIME_V2_SCHEDULE_MODE: printf(" (UWB_INITIATION_TIME)\n"); break;
                case SUPPORTED_V1_CHANNELS_V2_HOPPING_MODE: printf(" (CHANNELS)\n"); break;
                case SUPPORTED_V1_RFRAME_CONFIG_V2_BLOCK_STRIDING: printf(" (RFRAME_CONFIG)\n"); break;
                case SUPPORTED_V1_CC_CONSTRAINT_LENGTH_V2_UWB_INITIATION_TIME: printf(" (CC_CONSTRAINT_LENGTH)\n"); break;
                case SUPPORTED_V1_BPRF_PARAMETER_SETS_V2_CHANNELS: printf(" (BPRF_PARAMETER_SETS)\n"); break;
                case SUPPORTED_V1_HPRF_PARAMETER_SETS_V2_RFRAME_CONFIG: printf(" (HPRF_PARAMETER_SETS)\n"); break;
                case SUPPORTED_V1_AOA_V2_AOA_SUPPORT: printf(" (AOA)\n"); break;
                case SUPPORTED_V1_EXTENDED_MAC_ADDRESS_V2_EXTENDED_MAC_ADDRESS: printf(" (EXTENDED_MAC_ADDRESS)\n"); break;
                case SUPPORTED_V1_MAX_MESSAGE_SIZE_V2_ASSIGNED: printf(" (MAX_MESSAGE_SIZE)\n"); break;
                case SUPPORTED_V1_MAX_DATA_PACKET_PAYLOAD_SIZE_V2_SESSION_KEY_LENGTH: printf(" (MAX_DATA_PACKET_PAYLOAD_SIZE)\n"); break;
                case SUPPORTED_V2_EXTENDED_MAC_ADDRESS: printf(" (V2_EXTENDED_MAC_ADDRESS)\n"); break;
                case SUPPORTED_V2_ASSIGNED: printf(" (V2_ASSIGNED)\n"); break;
                case SUPPORTED_V2_SESSION_KEY_LENGTH: printf(" (V2_SESSION_KEY_LENGTH)\n"); break;
                case SUPPORTED_V2_DT_ANCHOR_MAX_ACTIVE_RR: printf(" (DT_ANCHOR_MAX_ACTIVE_RR)\n"); break;
                case SUPPORTED_V2_DT_TAG_MAX_ACTIVE_RR: printf(" (DT_TAG_MAX_ACTIVE_RR)\n"); break;
                case SUPPORTED_V2_DT_TAG_BLOCK_SHIPPING: printf(" (DT_TAG_BLOCK_SHIPPING)\n"); break;
                case SUPPORTED_V2_PSDU_LENGTH_SUPPORT: printf(" (PSDU_LENGTH_SUPPORT)\n"); break;
                default: printf(" (UNKNOWN)\n"); break;
            }
            
            printf("        Length: %d\n", tlv_len);
            
            if (offset + tlv_len <= payload_len) {
                printf("        Value: ");
                for (int j = 0; j < tlv_len; j++) {
                    printf("%02X ", payload[offset + j]);
                }
                printf("\n");
            } else {
                printf("        Value: TRUNCATED (expected %d bytes, only %d available)\n", tlv_len, payload_len - offset);
            }
            
            offset += tlv_len;
        }
    }
}

void decode_core_set_config_rsp(unsigned char* payload, int payload_len) {
    printf("    CORE_SET_CONFIG_RSP - Set Configuration Response\n");
    
    if (payload_len < 2) {
        printf("      ERROR: Payload too short (%d bytes, need at least 2)\n", payload_len);
        return;
    }
    
    unsigned char status = payload[0];
    unsigned char num_configs = payload[1];
    
    printf("      Status: 0x%02X", status);
    switch(status) {
        case UCI_STATUS_OK: printf(" (OK)\n"); break;
        case UCI_STATUS_REJECTED: printf(" (REJECTED)\n"); break;
        case UCI_STATUS_FAILED: printf(" (FAILED)\n"); break;
        case UCI_STATUS_INVALID_PARAM: printf(" (INVALID_PARAM)\n"); break;
        default: printf(" (UNKNOWN)\n"); break;
    }
    
    printf("      Number of Config Status: %d\n", num_configs);
    
    if (num_configs > 0 && payload_len >= 2) {
        int offset = 2;
        for (int i = 0; i < num_configs && offset + 2 <= payload_len; i++) {
            DeviceConfigId cfg_id = (DeviceConfigId)payload[offset];
            unsigned char cfg_status = payload[offset + 1];
            offset += 2;
            
            printf("      Config %d:\n", i);
            printf("        Config ID: 0x%02X", cfg_id);
            switch(cfg_id) {
                case DEVICE_STATE: printf(" (DEVICE_STATE)\n"); break;
                case LOW_POWER_MODE: printf(" (LOW_POWER_MODE)\n"); break;
                default: printf(" (UNKNOWN)\n"); break;
            }
            
            printf("        Status: 0x%02X", cfg_status);
            switch(cfg_status) {
                case UCI_STATUS_OK: printf(" (OK)\n"); break;
                case UCI_STATUS_REJECTED: printf(" (REJECTED)\n"); break;
                case UCI_STATUS_FAILED: printf(" (FAILED)\n"); break;
                case UCI_STATUS_INVALID_PARAM: printf(" (INVALID_PARAM)\n"); break;
                default: printf(" (UNKNOWN)\n"); break;
            }
        }
    }
}

void decode_core_get_config_rsp(unsigned char* payload, int payload_len) {
    printf("    CORE_GET_CONFIG_RSP - Get Configuration Response\n");
    
    if (payload_len < 2) {
        printf("      ERROR: Payload too short (%d bytes, need at least 2)\n", payload_len);
        return;
    }
    
    unsigned char status = payload[0];
    unsigned char num_tlvs = payload[1];
    
    printf("      Status: 0x%02X", status);
    switch(status) {
        case UCI_STATUS_OK: printf(" (OK)\n"); break;
        case UCI_STATUS_REJECTED: printf(" (REJECTED)\n"); break;
        case UCI_STATUS_FAILED: printf(" (FAILED)\n"); break;
        case UCI_STATUS_INVALID_PARAM: printf(" (INVALID_PARAM)\n"); break;
        default: printf(" (UNKNOWN)\n"); break;
    }
    
    printf("      Number of TLVs: %d\n", num_tlvs);
    
    if (num_tlvs > 0 && payload_len >= 2) {
        int offset = 2;
        for (int i = 0; i < num_tlvs && offset + 3 <= payload_len; i++) {
            DeviceConfigId cfg_id = (DeviceConfigId)payload[offset];
            unsigned char cfg_len = payload[offset + 1];
            offset += 2;
            
            printf("      TLV %d:\n", i);
            printf("        Config ID: 0x%02X", cfg_id);
            switch(cfg_id) {
                case DEVICE_STATE: printf(" (DEVICE_STATE)\n"); break;
                case LOW_POWER_MODE: printf(" (LOW_POWER_MODE)\n"); break;
                default: printf(" (UNKNOWN)\n"); break;
            }
            
            printf("        Length: %d\n", cfg_len);
            
            if (offset + cfg_len <= payload_len) {
                printf("        Value: ");
                for (int j = 0; j < cfg_len; j++) {
                    printf("%02X ", payload[offset + j]);
                }
                printf("\n");
                
                // Interpret common values
                if (cfg_id == DEVICE_STATE && cfg_len == 1) {
                    unsigned char state = payload[offset];
                    printf("          Interpreted as Device State: ");
                    switch(state) {
                        case DEVICE_STATE_READY: printf("READY (0x%02X)\n", state); break;
                        case DEVICE_STATE_ACTIVE: printf("ACTIVE (0x%02X)\n", state); break;
                        case DEVICE_STATE_ERROR: printf("ERROR (0x%02X)\n", state); break;
                        default: printf("UNKNOWN (0x%02X)\n", state); break;
                    }
                } else if (cfg_id == LOW_POWER_MODE && cfg_len == 1) {
                    unsigned char lpm = payload[offset];
                    printf("          Interpreted as Low Power Mode: %s (0x%02X)\n", 
                           lpm ? "ON" : "OFF", lpm);
                }
            } else {
                printf("        Value: TRUNCATED (expected %d bytes, only %d available)\n", cfg_len, payload_len - offset);
            }
            
            offset += cfg_len;
        }
    }
}

void decode_core_device_reset_rsp(unsigned char* payload, int payload_len) {
    printf("    CORE_DEVICE_RESET_RSP - Device Reset Response\n");
    
    if (payload_len < 1) {
        printf("      ERROR: Payload too short (%d bytes, need at least 1)\n", payload_len);
        return;
    }
    
    unsigned char status = payload[0];
    
    printf("      Status: 0x%02X", status);
    switch(status) {
        case UCI_STATUS_OK: printf(" (OK)\n"); break;
        case UCI_STATUS_REJECTED: printf(" (REJECTED)\n"); break;
        case UCI_STATUS_FAILED: printf(" (FAILED)\n"); break;
        default: printf(" (UNKNOWN)\n"); break;
    }
}

void decode_core_device_suspend_rsp(unsigned char* payload, int payload_len) {
    printf("    CORE_DEVICE_SUSPEND_RSP - Device Suspend Response\n");
    
    if (payload_len < 1) {
        printf("      ERROR: Payload too short (%d bytes, need at least 1)\n", payload_len);
        return;
    }
    
    unsigned char status = payload[0];
    
    printf("      Status: 0x%02X", status);
    switch(status) {
        case UCI_STATUS_OK: printf(" (OK)\n"); break;
        case UCI_STATUS_REJECTED: printf(" (REJECTED)\n"); break;
        case UCI_STATUS_FAILED: printf(" (FAILED)\n"); break;
        default: printf(" (UNKNOWN)\n"); break;
    }
}

// SESSION_CONFIG Group Payload Decoders
void decode_session_init_rsp(unsigned char* payload, int payload_len) {
    printf("    SESSION_INIT_RSP - Session Initialization Response\n");
    
    if (payload_len < 5) {
        printf("      ERROR: Payload too short (%d bytes, need at least 5)\n", payload_len);
        return;
    }
    
    unsigned char status = payload[0];
    unsigned int session_handle = read_u32_le(&payload[1]);
    
    printf("      Status: 0x%02X", status);
    switch(status) {
        case UCI_STATUS_OK: printf(" (OK)\n"); break;
        case UCI_STATUS_REJECTED: printf(" (REJECTED)\n"); break;
        case UCI_STATUS_FAILED: printf(" (FAILED)\n"); break;
        case UCI_STATUS_INVALID_PARAM: printf(" (INVALID_PARAM)\n"); break;
        case UCI_STATUS_SESSION_DUPLICATE: printf(" (SESSION_DUPLICATE)\n"); break;
        default: printf(" (UNKNOWN)\n"); break;
    }
    
    printf("      Session Handle: 0x%08X\n", session_handle);
}

void decode_session_deinit_rsp(unsigned char* payload, int payload_len) {
    printf("    SESSION_DEINIT_RSP - Session Deinitialization Response\n");
    
    if (payload_len < 1) {
        printf("      ERROR: Payload too short (%d bytes, need at least 1)\n", payload_len);
        return;
    }
    
    unsigned char status = payload[0];
    
    printf("      Status: 0x%02X", status);
    switch(status) {
        case UCI_STATUS_OK: printf(" (OK)\n"); break;
        case UCI_STATUS_REJECTED: printf(" (REJECTED)\n"); break;
        case UCI_STATUS_FAILED: printf(" (FAILED)\n"); break;
        case UCI_STATUS_INVALID_PARAM: printf(" (INVALID_PARAM)\n"); break;
        case UCI_STATUS_SESSION_NOT_EXIST: printf(" (SESSION_NOT_EXIST)\n"); break;
        default: printf(" (UNKNOWN)\n"); break;
    }
}

void decode_session_set_app_config_rsp(unsigned char* payload, int payload_len) {
    printf("    SESSION_SET_APP_CONFIG_RSP - Set Application Configuration Response\n");
    
    if (payload_len < 2) {
        printf("      ERROR: Payload too short (%d bytes, need at least 2)\n", payload_len);
        return;
    }
    
    unsigned char status = payload[0];
    unsigned char num_configs = payload[1];
    
    printf("      Status: 0x%02X", status);
    switch(status) {
        case UCI_STATUS_OK: printf(" (OK)\n"); break;
        case UCI_STATUS_REJECTED: printf(" (REJECTED)\n"); break;
        case UCI_STATUS_FAILED: printf(" (FAILED)\n"); break;
        case UCI_STATUS_INVALID_PARAM: printf(" (INVALID_PARAM)\n"); break;
        case UCI_STATUS_SESSION_NOT_EXIST: printf(" (SESSION_NOT_EXIST)\n"); break;
        default: printf(" (UNKNOWN)\n"); break;
    }
    
    printf("      Number of Config Status: %d\n", num_configs);
    
    if (num_configs > 0 && payload_len >= 2) {
        int offset = 2;
        for (int i = 0; i < num_configs && offset + 2 <= payload_len; i++) {
            AppConfigTlvType cfg_id = (AppConfigTlvType)payload[offset];
            unsigned char cfg_status = payload[offset + 1];
            offset += 2;
            
            printf("      Config %d:\n", i);
            printf("        Config ID: 0x%02X", cfg_id);
            // Add common config ID interpretations
            switch(cfg_id) {
                case DEVICE_TYPE: printf(" (DEVICE_TYPE)\n"); break;
                case RANGING_ROUND_USAGE: printf(" (RANGING_ROUND_USAGE)\n"); break;
                case STS_CONFIG: printf(" (STS_CONFIG)\n"); break;
                case MULTI_NODE_MODE: printf(" (MULTI_NODE_MODE)\n"); break;
                case CHANNEL_NUMBER: printf(" (CHANNEL_NUMBER)\n"); break;
                case NO_OF_CONTROLEE: printf(" (NO_OF_CONTROLEE)\n"); break;
                case DEVICE_MAC_ADDRESS: printf(" (DEVICE_MAC_ADDRESS)\n"); break;
                case DST_MAC_ADDRESS: printf(" (DST_MAC_ADDRESS)\n"); break;
                case SLOT_DURATION: printf(" (SLOT_DURATION)\n"); break;
                case RANGING_DURATION: printf(" (RANGING_DURATION)\n"); break;
                case STS_INDEX: printf(" (STS_INDEX)\n"); break;
                case MAC_FCS_TYPE: printf(" (MAC_FCS_TYPE)\n"); break;
                case RANGING_ROUND_CONTROL: printf(" (RANGING_ROUND_CONTROL)\n"); break;
                case AOA_RESULT_REQ: printf(" (AOA_RESULT_REQ)\n"); break;
                case RNG_DATA_NTF: printf(" (RNG_DATA_NTF)\n"); break;
                case RNG_DATA_NTF_PROXIMITY_NEAR: printf(" (RNG_DATA_NTF_PROXIMITY_NEAR)\n"); break;
                case RNG_DATA_NTF_PROXIMITY_FAR: printf(" (RNG_DATA_NTF_PROXIMITY_FAR)\n"); break;
                case DEVICE_ROLE: printf(" (DEVICE_ROLE)\n"); break;
                case RFRAME_CONFIG: printf(" (RFRAME_CONFIG)\n"); break;
                case RSSI_REPORTING: printf(" (RSSI_REPORTING)\n"); break;
                case PREAMBLE_CODE_INDEX: printf(" (PREAMBLE_CODE_INDEX)\n"); break;
                case SFD_ID: printf(" (SFD_ID)\n"); break;
                case PSDU_DATA_RATE: printf(" (PSDU_DATA_RATE)\n"); break;
                case PREAMBLE_DURATION: printf(" (PREAMBLE_DURATION)\n"); break;
                case LINK_LAYER_MODE: printf(" (LINK_LAYER_MODE)\n"); break;
                case DATA_REPETITION_COUNT: printf(" (DATA_REPETITION_COUNT)\n"); break;
                case RANGING_TIME_STRUCT: printf(" (RANGING_TIME_STRUCT)\n"); break;
                case SLOTS_PER_RR: printf(" (SLOTS_PER_RR)\n"); break;
                case TX_ADAPTIVE_PAYLOAD_POWER: printf(" (TX_ADAPTIVE_PAYLOAD_POWER)\n"); break;
                case RNG_DATA_NTF_AOA_BOUND: printf(" (RNG_DATA_NTF_AOA_BOUND)\n"); break;
                case RESPONDER_SLOT_INDEX: printf(" (RESPONDER_SLOT_INDEX)\n"); break;
                case PRF_MODE: printf(" (PRF_MODE)\n"); break;
                case CAP_SIZE_RANGE: printf(" (CAP_SIZE_RANGE)\n"); break;
                case TX_JITTER_WINDOW_SIZE: printf(" (TX_JITTER_WINDOW_SIZE)\n"); break;
                case SCHEDULED_MODE: printf(" (SCHEDULED_MODE)\n"); break;
                case KEY_ROTATION: printf(" (KEY_ROTATION)\n"); break;
                case KEY_ROTATION_RATE: printf(" (KEY_ROTATION_RATE)\n"); break;
                case SESSION_PRIORITY: printf(" (SESSION_PRIORITY)\n"); break;
                case MAC_ADDRESS_MODE: printf(" (MAC_ADDRESS_MODE)\n"); break;
                case VENDOR_ID: printf(" (VENDOR_ID)\n"); break;
                case STATIC_STS_IV: printf(" (STATIC_STS_IV)\n"); break;
                case NUMBER_OF_STS_SEGMENTS: printf(" (NUMBER_OF_STS_SEGMENTS)\n"); break;
                case MAX_RR_RETRY: printf(" (MAX_RR_RETRY)\n"); break;
                case UWB_INITIATION_TIME: printf(" (UWB_INITIATION_TIME)\n"); break;
                case HOPPING_MODE: printf(" (HOPPING_MODE)\n"); break;
                case BLOCK_STRIDE_LENGTH: printf(" (BLOCK_STRIDE_LENGTH)\n"); break;
                case RESULT_REPORT_CONFIG: printf(" (RESULT_REPORT_CONFIG)\n"); break;
                case IN_BAND_TERMINATION_ATTEMPT_COUNT: printf(" (IN_BAND_TERMINATION_ATTEMPT_COUNT)\n"); break;
                case SUB_SESSION_ID: printf(" (SUB_SESSION_ID)\n"); break;
                default: printf(" (UNKNOWN)\n"); break;
            }
            
            printf("        Status: 0x%02X", cfg_status);
            switch(cfg_status) {
                case UCI_STATUS_OK: printf(" (OK)\n"); break;
                case UCI_STATUS_REJECTED: printf(" (REJECTED)\n"); break;
                case UCI_STATUS_FAILED: printf(" (FAILED)\n"); break;
                case UCI_STATUS_INVALID_PARAM: printf(" (INVALID_PARAM)\n"); break;
                case UCI_STATUS_READ_ONLY: printf(" (READ_ONLY)\n"); break;
                default: printf(" (UNKNOWN)\n"); break;
            }
        }
    }
}

void decode_session_get_app_config_rsp(unsigned char* payload, int payload_len) {
    printf("    SESSION_GET_APP_CONFIG_RSP - Get Application Configuration Response\n");
    
    if (payload_len < 2) {
        printf("      ERROR: Payload too short (%d bytes, need at least 2)\n", payload_len);
        return;
    }
    
    unsigned char status = payload[0];
    unsigned char num_tlvs = payload[1];
    
    printf("      Status: 0x%02X", status);
    switch(status) {
        case UCI_STATUS_OK: printf(" (OK)\n"); break;
        case UCI_STATUS_REJECTED: printf(" (REJECTED)\n"); break;
        case UCI_STATUS_FAILED: printf(" (FAILED)\n"); break;
        case UCI_STATUS_INVALID_PARAM: printf(" (INVALID_PARAM)\n"); break;
        case UCI_STATUS_SESSION_NOT_EXIST: printf(" (SESSION_NOT_EXIST)\n"); break;
        default: printf(" (UNKNOWN)\n"); break;
    }
    
    printf("      Number of TLVs: %d\n", num_tlvs);
    
    if (num_tlvs > 0 && payload_len >= 2) {
        int offset = 2;
        for (int i = 0; i < num_tlvs && offset + 3 <= payload_len; i++) {
            AppConfigTlvType cfg_id = (AppConfigTlvType)payload[offset];
            unsigned char cfg_len = payload[offset + 1];
            offset += 2;
            
            printf("      TLV %d:\n", i);
            printf("        Config ID: 0x%02X", cfg_id);
            switch(cfg_id) {
                case DEVICE_TYPE: printf(" (DEVICE_TYPE)\n"); break;
                case RANGING_ROUND_USAGE: printf(" (RANGING_ROUND_USAGE)\n"); break;
                case STS_CONFIG: printf(" (STS_CONFIG)\n"); break;
                case MULTI_NODE_MODE: printf(" (MULTI_NODE_MODE)\n"); break;
                case CHANNEL_NUMBER: printf(" (CHANNEL_NUMBER)\n"); break;
                case NO_OF_CONTROLEE: printf(" (NO_OF_CONTROLEE)\n"); break;
                case DEVICE_MAC_ADDRESS: printf(" (DEVICE_MAC_ADDRESS)\n"); break;
                case DST_MAC_ADDRESS: printf(" (DST_MAC_ADDRESS)\n"); break;
                case SLOT_DURATION: printf(" (SLOT_DURATION)\n"); break;
                case RANGING_DURATION: printf(" (RANGING_DURATION)\n"); break;
                case STS_INDEX: printf(" (STS_INDEX)\n"); break;
                case MAC_FCS_TYPE: printf(" (MAC_FCS_TYPE)\n"); break;
                case RANGING_ROUND_CONTROL: printf(" (RANGING_ROUND_CONTROL)\n"); break;
                case AOA_RESULT_REQ: printf(" (AOA_RESULT_REQ)\n"); break;
                case RNG_DATA_NTF: printf(" (RNG_DATA_NTF)\n"); break;
                case RNG_DATA_NTF_PROXIMITY_NEAR: printf(" (RNG_DATA_NTF_PROXIMITY_NEAR)\n"); break;
                case RNG_DATA_NTF_PROXIMITY_FAR: printf(" (RNG_DATA_NTF_PROXIMITY_FAR)\n"); break;
                case DEVICE_ROLE: printf(" (DEVICE_ROLE)\n"); break;
                case RFRAME_CONFIG: printf(" (RFRAME_CONFIG)\n"); break;
                case RSSI_REPORTING: printf(" (RSSI_REPORTING)\n"); break;
                case PREAMBLE_CODE_INDEX: printf(" (PREAMBLE_CODE_INDEX)\n"); break;
                case SFD_ID: printf(" (SFD_ID)\n"); break;
                case PSDU_DATA_RATE: printf(" (PSDU_DATA_RATE)\n"); break;
                case PREAMBLE_DURATION: printf(" (PREAMBLE_DURATION)\n"); break;
                case LINK_LAYER_MODE: printf(" (LINK_LAYER_MODE)\n"); break;
                case DATA_REPETITION_COUNT: printf(" (DATA_REPETITION_COUNT)\n"); break;
                case RANGING_TIME_STRUCT: printf(" (RANGING_TIME_STRUCT)\n"); break;
                case SLOTS_PER_RR: printf(" (SLOTS_PER_RR)\n"); break;
                case TX_ADAPTIVE_PAYLOAD_POWER: printf(" (TX_ADAPTIVE_PAYLOAD_POWER)\n"); break;
                case RNG_DATA_NTF_AOA_BOUND: printf(" (RNG_DATA_NTF_AOA_BOUND)\n"); break;
                case RESPONDER_SLOT_INDEX: printf(" (RESPONDER_SLOT_INDEX)\n"); break;
                case PRF_MODE: printf(" (PRF_MODE)\n"); break;
                case CAP_SIZE_RANGE: printf(" (CAP_SIZE_RANGE)\n"); break;
                case TX_JITTER_WINDOW_SIZE: printf(" (TX_JITTER_WINDOW_SIZE)\n"); break;
                case SCHEDULED_MODE: printf(" (SCHEDULED_MODE)\n"); break;
                case KEY_ROTATION: printf(" (KEY_ROTATION)\n"); break;
                case KEY_ROTATION_RATE: printf(" (KEY_ROTATION_RATE)\n"); break;
                case SESSION_PRIORITY: printf(" (SESSION_PRIORITY)\n"); break;
                case MAC_ADDRESS_MODE: printf(" (MAC_ADDRESS_MODE)\n"); break;
                case VENDOR_ID: printf(" (VENDOR_ID)\n"); break;
                case STATIC_STS_IV: printf(" (STATIC_STS_IV)\n"); break;
                case NUMBER_OF_STS_SEGMENTS: printf(" (NUMBER_OF_STS_SEGMENTS)\n"); break;
                case MAX_RR_RETRY: printf(" (MAX_RR_RETRY)\n"); break;
                case UWB_INITIATION_TIME: printf(" (UWB_INITIATION_TIME)\n"); break;
                case HOPPING_MODE: printf(" (HOPPING_MODE)\n"); break;
                case BLOCK_STRIDE_LENGTH: printf(" (BLOCK_STRIDE_LENGTH)\n"); break;
                case RESULT_REPORT_CONFIG: printf(" (RESULT_REPORT_CONFIG)\n"); break;
                case IN_BAND_TERMINATION_ATTEMPT_COUNT: printf(" (IN_BAND_TERMINATION_ATTEMPT_COUNT)\n"); break;
                case SUB_SESSION_ID: printf(" (SUB_SESSION_ID)\n"); break;
                default: printf(" (UNKNOWN)\n"); break;
            }
            
            printf("        Length: %d\n", cfg_len);
            
            if (offset + cfg_len <= payload_len) {
                printf("        Value: ");
                for (int j = 0; j < cfg_len; j++) {
                    printf("%02X ", payload[offset + j]);
                }
                printf("\n");
            } else {
                printf("        Value: TRUNCATED (expected %d bytes, only %d available)\n", cfg_len, payload_len - offset);
            }
            
            offset += cfg_len;
        }
    }
}

void decode_session_get_count_rsp(unsigned char* payload, int payload_len) {
    printf("    SESSION_GET_COUNT_RSP - Get Session Count Response\n");
    
    if (payload_len < 2) {
        printf("      ERROR: Payload too short (%d bytes, need at least 2)\n", payload_len);
        return;
    }
    
    unsigned char status = payload[0];
    unsigned char count = payload[1];
    
    printf("      Status: 0x%02X", status);
    switch(status) {
        case UCI_STATUS_OK: printf(" (OK)\n"); break;
        case UCI_STATUS_REJECTED: printf(" (REJECTED)\n"); break;
        case UCI_STATUS_FAILED: printf(" (FAILED)\n"); break;
        default: printf(" (UNKNOWN)\n"); break;
    }
    
    printf("      Session Count: %d\n", count);
}

void decode_session_get_state_rsp(unsigned char* payload, int payload_len) {
    printf("    SESSION_GET_STATE_RSP - Get Session State Response\n");
    
    if (payload_len < 2) {
        printf("      ERROR: Payload too short (%d bytes, need at least 2)\n", payload_len);
        return;
    }
    
    unsigned char status = payload[0];
    unsigned char state = payload[1];
    
    printf("      Status: 0x%02X", status);
    switch(status) {
        case UCI_STATUS_OK: printf(" (OK)\n"); break;
        case UCI_STATUS_REJECTED: printf(" (REJECTED)\n"); break;
        case UCI_STATUS_FAILED: printf(" (FAILED)\n"); break;
        case UCI_STATUS_SESSION_NOT_EXIST: printf(" (SESSION_NOT_EXIST)\n"); break;
        default: printf(" (UNKNOWN)\n"); break;
    }
    
    printf("      Session State: 0x%02X", state);
    switch(state) {
        case SESSION_STATE_INIT: printf(" (INIT)\n"); break;
        case SESSION_STATE_DEINIT: printf(" (DEINIT)\n"); break;
        case SESSION_STATE_ACTIVE: printf(" (ACTIVE)\n"); break;
        case SESSION_STATE_IDLE: printf(" (IDLE)\n"); break;
        default: printf(" (UNKNOWN)\n"); break;
    }
}

// SESSION_CONTROL Group Payload Decoders
void decode_session_start_rsp(unsigned char* payload, int payload_len) {
    printf("    SESSION_START_RSP - Session Start Response\n");
    
    if (payload_len < 1) {
        printf("      ERROR: Payload too short (%d bytes, need at least 1)\n", payload_len);
        return;
    }
    
    unsigned char status = payload[0];
    
    printf("      Status: 0x%02X", status);
    switch(status) {
        case UCI_STATUS_OK: printf(" (OK)\n"); break;
        case UCI_STATUS_REJECTED: printf(" (REJECTED)\n"); break;
        case UCI_STATUS_FAILED: printf(" (FAILED)\n"); break;
        case UCI_STATUS_INVALID_PARAM: printf(" (INVALID_PARAM)\n"); break;
        case UCI_STATUS_SESSION_NOT_EXIST: printf(" (SESSION_NOT_EXIST)\n"); break;
        case UCI_STATUS_SESSION_ACTIVE: printf(" (SESSION_ACTIVE)\n"); break;
        default: printf(" (UNKNOWN)\n"); break;
    }
}

void decode_session_stop_rsp(unsigned char* payload, int payload_len) {
    printf("    SESSION_STOP_RSP - Session Stop Response\n");
    
    if (payload_len < 1) {
        printf("      ERROR: Payload too short (%d bytes, need at least 1)\n", payload_len);
        return;
    }
    
    unsigned char status = payload[0];
    
    printf("      Status: 0x%02X", status);
    switch(status) {
        case UCI_STATUS_OK: printf(" (OK)\n"); break;
        case UCI_STATUS_REJECTED: printf(" (REJECTED)\n"); break;
        case UCI_STATUS_FAILED: printf(" (FAILED)\n"); break;
        case UCI_STATUS_INVALID_PARAM: printf(" (INVALID_PARAM)\n"); break;
        case UCI_STATUS_SESSION_NOT_EXIST: printf(" (SESSION_NOT_EXIST)\n"); break;
        default: printf(" (UNKNOWN)\n"); break;
    }
}

void decode_session_get_ranging_count_rsp(unsigned char* payload, int payload_len) {
    printf("    SESSION_GET_RANGING_COUNT_RSP - Get Ranging Count Response\n");
    
    if (payload_len < 3) {
        printf("      ERROR: Payload too short (%d bytes, need at least 3)\n", payload_len);
        return;
    }
    
    unsigned char status = payload[0];
    unsigned short count = read_u16_le(&payload[1]);
    
    printf("      Status: 0x%02X", status);
    switch(status) {
        case UCI_STATUS_OK: printf(" (OK)\n"); break;
        case UCI_STATUS_REJECTED: printf(" (REJECTED)\n"); break;
        case UCI_STATUS_FAILED: printf(" (FAILED)\n"); break;
        case UCI_STATUS_INVALID_PARAM: printf(" (INVALID_PARAM)\n"); break;
        case UCI_STATUS_SESSION_NOT_EXIST: printf(" (SESSION_NOT_EXIST)\n"); break;
        default: printf(" (UNKNOWN)\n"); break;
    }
    
    printf("      Ranging Count: %d\n", count);
}

// CORE Notification Payload Decoders
void decode_core_device_status_ntf(unsigned char* payload, int payload_len) {
    printf("    CORE_DEVICE_STATUS_NTF - Device Status Notification\n");
    
    if (payload_len < 1) {
        printf("      ERROR: Payload too short (%d bytes, need at least 1)\n", payload_len);
        return;
    }
    
    unsigned char device_state = payload[0];
    
    printf("      Device State: 0x%02X", device_state);
    switch(device_state) {
        case DEVICE_STATE_READY: printf(" (READY)\n"); break;
        case DEVICE_STATE_ACTIVE: printf(" (ACTIVE)\n"); break;
        case DEVICE_STATE_ERROR: printf(" (ERROR)\n"); break;
        default: printf(" (UNKNOWN)\n"); break;
    }
}

void decode_core_generic_error_ntf(unsigned char* payload, int payload_len) {
    printf("    CORE_GENERIC_ERROR_NTF - Generic Error Notification\n");
    
    if (payload_len < 1) {
        printf("      ERROR: Payload too short (%d bytes, need at least 1)\n", payload_len);
        return;
    }
    
    unsigned char status = payload[0];
    
    printf("      Error Status: 0x%02X", status);
    switch(status) {
        case UCI_STATUS_OK: printf(" (OK)\n"); break;
        case UCI_STATUS_REJECTED: printf(" (REJECTED)\n"); break;
        case UCI_STATUS_FAILED: printf(" (FAILED)\n"); break;
        case UCI_STATUS_SYNTAX_ERROR: printf(" (SYNTAX_ERROR)\n"); break;
        case UCI_STATUS_INVALID_PARAM: printf(" (INVALID_PARAM)\n"); break;
        case UCI_STATUS_INVALID_RANGE: printf(" (INVALID_RANGE)\n"); break;
        case UCI_STATUS_INVALID_MSG_SIZE: printf(" (INVALID_MSG_SIZE)\n"); break;
        case UCI_STATUS_UNKNOWN_GID: printf(" (UNKNOWN_GID)\n"); break;
        case UCI_STATUS_UNKNOWN_OID: printf(" (UNKNOWN_OID)\n"); break;
        case UCI_STATUS_READ_ONLY: printf(" (READ_ONLY)\n"); break;
        case UCI_STATUS_COMMAND_RETRY: printf(" (COMMAND_RETRY)\n"); break;
        default: printf(" (UNKNOWN)\n"); break;
    }
}

// SESSION_CONFIG Notification Payload Decoders
void decode_session_status_ntf(unsigned char* payload, int payload_len) {
    printf("    SESSION_STATUS_NTF - Session Status Notification\n");
    
    if (payload_len < 6) {
        printf("      ERROR: Payload too short (%d bytes, need at least 6)\n", payload_len);
        return;
    }
    
    unsigned int session_token = read_u32_le(payload);
    unsigned char session_state = payload[4];
    unsigned char reason_code = payload[5];
    
    printf("      Session Token: 0x%08X\n", session_token);
    printf("      Session State: 0x%02X", session_state);
    switch(session_state) {
        case SESSION_STATE_INIT: printf(" (INIT)\n"); break;
        case SESSION_STATE_DEINIT: printf(" (DEINIT)\n"); break;
        case SESSION_STATE_ACTIVE: printf(" (ACTIVE)\n"); break;
        case SESSION_STATE_IDLE: printf(" (IDLE)\n"); break;
        default: printf(" (UNKNOWN)\n"); break;
    }
    
    printf("      Reason Code: 0x%02X", reason_code);
    switch(reason_code) {
        case STATE_CHANGE_WITH_SESSION_MANAGEMENT_COMMANDS: printf(" (STATE_CHANGE_WITH_SESSION_MANAGEMENT_COMMANDS)\n"); break;
        case MAX_RANGING_ROUND_RETRY_COUNT_REACHED: printf(" (MAX_RANGING_ROUND_RETRY_COUNT_REACHED)\n"); break;
        case MAX_NUMBER_OF_MEASUREMENTS_REACHED: printf(" (MAX_NUMBER_OF_MEASUREMENTS_REACHED)\n"); break;
        case SESSION_SUSPENDED_DUE_TO_INBAND_SIGNAL: printf(" (SESSION_SUSPENDED_DUE_TO_INBAND_SIGNAL)\n"); break;
        case SESSION_RESUMED_DUE_TO_INBAND_SIGNAL: printf(" (SESSION_RESUMED_DUE_TO_INBAND_SIGNAL)\n"); break;
        case SESSION_STOPPED_DUE_TO_INBAND_SIGNAL: printf(" (SESSION_STOPPED_DUE_TO_INBAND_SIGNAL)\n"); break;
        default: printf(" (UNKNOWN)\n"); break;
    }
}

// SESSION_CONTROL Notification Payload Decoders
void decode_session_info_ntf(unsigned char* payload, int payload_len) {
    printf("    SESSION_INFO_NTF - Session Information/Ranging Notification\n");
    
    if (payload_len < 20) {
        printf("      ERROR: Payload too short (%d bytes, need at least 20 for header)\n", payload_len);
        return;
    }
    
    // Parse header fields
    unsigned int sequence_number = read_u32_le(&payload[0]);
    unsigned int session_token = read_u32_le(&payload[4]);
    unsigned char rcr_indicator = payload[8];
    unsigned int current_ranging_interval = read_u32_le(&payload[9]);
    unsigned char ranging_measurement_type = payload[13];
    unsigned char reserved = payload[14];
    unsigned char mac_address_indicator = payload[15];
    unsigned int hus_primary_session_id = read_u32_le(&payload[16]);
    
    printf("      Sequence Number: %u\n", sequence_number);
    printf("      Session Token: 0x%08X\n", session_token);
    printf("      RCR Indicator: 0x%02X\n", rcr_indicator);
    printf("      Current Ranging Interval: %u ms\n", current_ranging_interval);
    printf("      Ranging Measurement Type: 0x%02X", ranging_measurement_type);
    switch(ranging_measurement_type) {
        case 0x00: printf(" (ONE_WAY)\n"); break;
        case 0x01: printf(" (TWO_WAY)\n"); break;
        case 0x02: printf(" (DL_TDOA)\n"); break;
        case 0x03: printf(" (OWR_AOA)\n"); break;
        default: printf(" (UNKNOWN)\n"); break;
    }
    
    printf("      Reserved: 0x%02X\n", reserved);
    printf("      MAC Address Indicator: 0x%02X", mac_address_indicator);
    if (mac_address_indicator == 0x00) {
        printf(" (SHORT_ADDRESS)\n");
    } else {
        printf(" (EXTENDED_ADDRESS)\n");
    }
    
    printf("      HUS Primary Session ID: 0x%08X\n", hus_primary_session_id);
    
    // Parse ranging measurements
    int offset = 20;
    if (offset < payload_len) {
        if (ranging_measurement_type == 0x01) { // TWO_WAY
            unsigned char num_measurements = payload[offset];
            offset += 1;
            printf("      Number of Two-Way Measurements: %d\n", num_measurements);
            
            for (int i = 0; i < num_measurements && offset + 20 <= payload_len; i++) {
                printf("      Measurement %d:\n", i + 1);
                
                if (mac_address_indicator == 0x00) { // SHORT_ADDRESS
                    unsigned short mac_address = read_u16_le(&payload[offset]);
                    unsigned char status = payload[offset + 2];
                    unsigned char nlos = payload[offset + 3];
                    unsigned short distance = read_u16_le(&payload[offset + 4]);
                    unsigned short aoa_azimuth = read_u16_le(&payload[offset + 6]);
                    unsigned char aoa_azimuth_fom = payload[offset + 8];
                    unsigned short aoa_elevation = read_u16_le(&payload[offset + 9]);
                    unsigned char aoa_elevation_fom = payload[offset + 11];
                    unsigned short aoa_destination_azimuth = read_u16_le(&payload[offset + 12]);
                    unsigned char aoa_destination_azimuth_fom = payload[offset + 14];
                    unsigned short aoa_destination_elevation = read_u16_le(&payload[offset + 15]);
                    unsigned char aoa_destination_elevation_fom = payload[offset + 17];
                    unsigned char slot_index = payload[offset + 18];
                    unsigned char rssi = payload[offset + 19];
                    
                    printf("        MAC Address: 0x%04X\n", mac_address);
                    printf("        Status: 0x%02X", status);
                    if (status == 0x00) printf(" (OK)");
                    printf("\n");
                    printf("        NLOS: %s\n", nlos ? "YES" : "NO");
                    printf("        Distance: %u cm\n", distance);
                    printf("        AoA Azimuth: %u degrees (FoM: %u)\n", aoa_azimuth, aoa_azimuth_fom);
                    printf("        AoA Elevation: %u degrees (FoM: %u)\n", aoa_elevation, aoa_elevation_fom);
                    printf("        Destination AoA Azimuth: %u degrees (FoM: %u)\n", aoa_destination_azimuth, aoa_destination_azimuth_fom);
                    printf("        Destination AoA Elevation: %u degrees (FoM: %u)\n", aoa_destination_elevation, aoa_destination_elevation_fom);
                    printf("        Slot Index: %u\n", slot_index);
                    printf("        RSSI: %d dBm\n", (signed char)rssi);
                    
                    offset += 20;
                } else { // EXTENDED_ADDRESS
                    if (offset + 26 <= payload_len) {
                        unsigned long long mac_address = read_u64_le(&payload[offset]);
                        unsigned char status = payload[offset + 8];
                        unsigned char nlos = payload[offset + 9];
                        unsigned short distance = read_u16_le(&payload[offset + 10]);
                        unsigned short aoa_azimuth = read_u16_le(&payload[offset + 12]);
                        unsigned char aoa_azimuth_fom = payload[offset + 14];
                        unsigned short aoa_elevation = read_u16_le(&payload[offset + 15]);
                        unsigned char aoa_elevation_fom = payload[offset + 17];
                        unsigned short aoa_destination_azimuth = read_u16_le(&payload[offset + 18]);
                        unsigned char aoa_destination_azimuth_fom = payload[offset + 20];
                        unsigned short aoa_destination_elevation = read_u16_le(&payload[offset + 21]);
                        unsigned char aoa_destination_elevation_fom = payload[offset + 23];
                        unsigned char slot_index = payload[offset + 24];
                        unsigned char rssi = payload[offset + 25];
                        
                        printf("        MAC Address: 0x%016llX\n", mac_address);
                        printf("        Status: 0x%02X", status);
                        if (status == 0x00) printf(" (OK)");
                        printf("\n");
                        printf("        NLOS: %s\n", nlos ? "YES" : "NO");
                        printf("        Distance: %u cm\n", distance);
                        printf("        AoA Azimuth: %u degrees (FoM: %u)\n", aoa_azimuth, aoa_azimuth_fom);
                        printf("        AoA Elevation: %u degrees (FoM: %u)\n", aoa_elevation, aoa_elevation_fom);
                        printf("        Destination AoA Azimuth: %u degrees (FoM: %u)\n", aoa_destination_azimuth, aoa_destination_azimuth_fom);
                        printf("        Destination AoA Elevation: %u degrees (FoM: %u)\n", aoa_destination_elevation, aoa_destination_elevation_fom);
                        printf("        Slot Index: %u\n", slot_index);
                        printf("        RSSI: %d dBm\n", (signed char)rssi);
                        
                        offset += 26; // Move to next measurement
                    } else {
                        printf("        ERROR: Insufficient data for EXTENDED_ADDRESS measurement\n");
                        break;
                    }
                }
            }
        } else if (ranging_measurement_type == 0x03) { // OWR_AOA
            unsigned char num_measurements = payload[offset];
            offset += 1;
            printf("      Number of OWR-AoA Measurements: %d\n", num_measurements);
            
            for (int i = 0; i < num_measurements && offset + 13 <= payload_len; i++) {
                printf("      OWR-AoA Measurement %d:\n", i + 1);
                
                if (mac_address_indicator == 0x00) { // SHORT_ADDRESS
                    unsigned short mac_address = read_u16_le(&payload[offset]);
                    unsigned char status = payload[offset + 2];
                    unsigned char nlos = payload[offset + 3];
                    unsigned char frame_sequence_number = payload[offset + 4];
                    unsigned short block_index = read_u16_le(&payload[offset + 5]);
                    unsigned short aoa_azimuth = read_u16_le(&payload[offset + 7]);
                    unsigned char aoa_azimuth_fom = payload[offset + 9];
                    unsigned short aoa_elevation = read_u16_le(&payload[offset + 10]);
                    unsigned char aoa_elevation_fom = payload[offset + 12];
                    
                    printf("        MAC Address: 0x%04X\n", mac_address);
                    printf("        Status: 0x%02X", status);
                    if (status == 0x00) printf(" (OK)");
                    printf("\n");
                    printf("        NLOS: %s\n", nlos ? "YES" : "NO");
                    printf("        Frame Sequence Number: %u\n", frame_sequence_number);
                    printf("        Block Index: %u\n", block_index);
                    printf("        AoA Azimuth: %u degrees (FoM: %u)\n", aoa_azimuth, aoa_azimuth_fom);
                    printf("        AoA Elevation: %u degrees (FoM: %u)\n", aoa_elevation, aoa_elevation_fom);
                    
                    offset += 13;
                } else { // EXTENDED_ADDRESS
                    if (offset + 19 <= payload_len) {
                        unsigned long long mac_address = read_u64_le(&payload[offset]);
                        unsigned char status = payload[offset + 8];
                        unsigned char nlos = payload[offset + 9];
                        unsigned char frame_sequence_number = payload[offset + 10];
                        unsigned short block_index = read_u16_le(&payload[offset + 11]);
                        unsigned short aoa_azimuth = read_u16_le(&payload[offset + 13]);
                        unsigned char aoa_azimuth_fom = payload[offset + 15];
                        unsigned short aoa_elevation = read_u16_le(&payload[offset + 16]);
                        unsigned char aoa_elevation_fom = payload[offset + 18];
                        
                        printf("        MAC Address: 0x%016llX\n", mac_address);
                        printf("        Status: 0x%02X", status);
                        if (status == 0x00) printf(" (OK)");
                        printf("\n");
                        printf("        NLOS: %s\n", nlos ? "YES" : "NO");
                        printf("        Frame Sequence Number: %u\n", frame_sequence_number);
                        printf("        Block Index: %u\n", block_index);
                        printf("        AoA Azimuth: %u degrees (FoM: %u)\n", aoa_azimuth, aoa_azimuth_fom);
                        printf("        AoA Elevation: %u degrees (FoM: %u)\n", aoa_elevation, aoa_elevation_fom);
                        
                        offset += 19;
                    } else {
                        printf("        ERROR: Insufficient data for EXTENDED_ADDRESS OWR-AoA measurement\n");
                        break;
                    }
                }
            }
        } else {
            printf("      Unsupported ranging measurement type: 0x%02X\n", ranging_measurement_type);
        }
    }
}
