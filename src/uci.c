#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include "../include/uci.h"
#include "../include/uci_functions.h"
#include "../include/uci_config_manager.h"
#include "../include/uci_hw.h"
#include "../include/uci_hw_interface.h"
#include "../include/uci_hw_chardev.h"
#include "../include/uci_ui.h"
#include "../include/uci_ui_main_patch.h"
#include "../include/uci_response_core.h"

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

static double q8_8_to_double(int16_t raw) {
    return (double)raw / 256.0;
}

static double q6_9_to_double(int16_t raw) {
    return (double)raw / 512.0;
}

static inline uint32_t read_u32_be(const unsigned char* buffer) {
    return ((uint32_t)buffer[0] << 24) |
           ((uint32_t)buffer[1] << 16) |
           ((uint32_t)buffer[2] << 8) |
           ((uint32_t)buffer[3]);
}

static void print_short_address_measurement(const unsigned char* data) {
    unsigned short mac_address = read_u16_le(&data[0]);
    unsigned char status = data[2];
    unsigned char nlos = data[3];
    unsigned short distance = read_u16_le(&data[4]);
    unsigned short aoa_azimuth = read_u16_le(&data[6]);
    unsigned char aoa_azimuth_fom = data[8];
    unsigned short aoa_elevation = read_u16_le(&data[9]);
    unsigned char aoa_elevation_fom = data[11];
    unsigned short aoa_destination_azimuth = read_u16_le(&data[12]);
    unsigned char aoa_destination_azimuth_fom = data[14];
    unsigned short aoa_destination_elevation = read_u16_le(&data[15]);
    unsigned char aoa_destination_elevation_fom = data[17];
    unsigned char slot_index = data[18];
    unsigned char rssi = data[19];

    printf("      MAC Address: 0x%04X\n", mac_address);
    printf("      Status: 0x%02X", status);
    if (status == UCI_STATUS_OK) {
        printf(" (OK)\n");
    } else {
        printf(" (UNKNOWN)\n");
    }
    printf("      NLOS: %s\n", nlos ? "YES" : "NO");
    printf("      Distance: %u cm\n", distance);
    printf("      AoA Azimuth: %u degrees (FoM: %u)\n", aoa_azimuth, aoa_azimuth_fom);
    printf("      AoA Elevation: %u degrees (FoM: %u)\n", aoa_elevation, aoa_elevation_fom);
    printf("      Destination AoA Azimuth: %u degrees (FoM: %u)\n", aoa_destination_azimuth, aoa_destination_azimuth_fom);
    printf("      Destination AoA Elevation: %u degrees (FoM: %u)\n", aoa_destination_elevation, aoa_destination_elevation_fom);
    printf("      Slot Index: %u\n", slot_index);
    printf("      RSSI: %d dBm\n", (int8_t)rssi);
}

static void print_extended_address_measurement(const unsigned char* data) {
    uint64_t mac_address = read_u64_le(&data[0]);
    unsigned char status = data[8];
    unsigned char nlos = data[9];
    unsigned short distance = read_u16_le(&data[10]);
    unsigned short aoa_azimuth = read_u16_le(&data[12]);
    unsigned char aoa_azimuth_fom = data[14];
    unsigned short aoa_elevation = read_u16_le(&data[15]);
    unsigned char aoa_elevation_fom = data[17];
    unsigned short aoa_destination_azimuth = read_u16_le(&data[18]);
    unsigned char aoa_destination_azimuth_fom = data[20];
    unsigned short aoa_destination_elevation = read_u16_le(&data[21]);
    unsigned char aoa_destination_elevation_fom = data[23];
    unsigned char slot_index = data[24];
    unsigned char rssi = data[25];

    printf("      MAC Address: 0x%016llX\n", (unsigned long long)mac_address);
    printf("      Status: 0x%02X", status);
    if (status == UCI_STATUS_OK) {
        printf(" (OK)\n");
    } else {
        printf(" (UNKNOWN)\n");
    }
    printf("      NLOS: %s\n", nlos ? "YES" : "NO");
    printf("      Distance: %u cm\n", distance);
    printf("      AoA Azimuth: %u degrees (FoM: %u)\n", aoa_azimuth, aoa_azimuth_fom);
    printf("      AoA Elevation: %u degrees (FoM: %u)\n", aoa_elevation, aoa_elevation_fom);
    printf("      Destination AoA Azimuth: %u degrees (FoM: %u)\n", aoa_destination_azimuth, aoa_destination_azimuth_fom);
    printf("      Destination AoA Elevation: %u degrees (FoM: %u)\n", aoa_destination_elevation, aoa_destination_elevation_fom);
    printf("      Slot Index: %u\n", slot_index);
    printf("      RSSI: %d dBm\n", (int8_t)rssi);
}

static void decode_range_vendor_data(const unsigned char* data, int length) {
    if (!data || length <= 0) {
        return;
    }

    int offset = 0;
    while (offset + 6 <= length) {
        unsigned char field_id = data[offset++];
        unsigned char sub_id = data[offset++];
        unsigned int value = read_u32_be(&data[offset]);
        offset += 4;

        printf("    Vendor Field 0x%02X/0x%02X: 0x%08X", field_id, sub_id, value);
        if (field_id == 0x01 && sub_id == 0x01) {
            printf(" (Distance: %u cm)", value);
        }
        printf("\n");

        /* Remaining vendor bytes after the first known TLV are often zeroed out. */
        int remaining = length - offset;
        int all_zero = 1;
        for (int i = 0; i < remaining; i++) {
            if (data[offset + i] != 0x00) {
                all_zero = 0;
                break;
            }
        }
        if (all_zero) {
            return;
        }
    }

    if (offset < length) {
        printf("    Unparsed Vendor Data (%d bytes): ", length - offset);
        for (int i = offset; i < length; i++) {
            printf("%02X ", data[i]);
        }
        printf("\n");
    }
}

static inline void write_u64_le(unsigned char* buffer, uint64_t value) {
    for (int i = 0; i < 8; i++) {
        buffer[i] = (value >> (i * 8)) & 0xFF;
    }
}

int is_valid_device_config_id(unsigned char cfg_id) {
    return cfg_id == DEVICE_STATE || cfg_id == LOW_POWER_MODE;
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
unsigned long long g_fake_timestamp = 0;
static unsigned int g_session_handle_counter = 1;

static unsigned char session_add_multicast_entry(struct uci_session* session,
                                                 unsigned short short_address,
                                                 unsigned int subsession_id,
                                                 const unsigned char* key,
                                                 unsigned char key_len);
static unsigned char session_remove_multicast_entry(struct uci_session* session,
                                                    unsigned short short_address,
                                                    unsigned int subsession_id);
static int session_find_multicast_entry_idx(const struct uci_session* session,
                                            unsigned short short_address,
                                            unsigned int subsession_id);

typedef struct {
    uint8_t type;
    uint8_t length;
    const uint8_t* value;
} capability_tlv_entry_t;

#define CAP_ENTRY(type_id, array_literal) \
    { (uint8_t)(type_id), (uint8_t)sizeof(array_literal), (array_literal) }

// Capability payload values derived from Android's default UWBS capabilities.
static const uint8_t kCapPhyVersionRange[] = {0x01, 0x00, 0x02, 0x00};
static const uint8_t kCapMacVersionRange[] = {0x01, 0x00, 0x02, 0x00};
static const uint8_t kCapDeviceRoles[] = {0x03};
static const uint8_t kCapRangingMethods[] = {0x07};
static const uint8_t kCapStsConfig[] = {0x07};
static const uint8_t kCapMultiNodeModes[] = {0x07};
static const uint8_t kCapRangingTimeStruct[] = {0x03};
static const uint8_t kCapScheduledMode[] = {0x03};
static const uint8_t kCapHoppingMode[] = {0x03};
static const uint8_t kCapBlockStriding[] = {0x01};
static const uint8_t kCapUwbInitiationTime[] = {0x32, 0x00};
static const uint8_t kCapChannels[] = {0x20, 0x08, 0x00};
static const uint8_t kCapRframeConfig[] = {0x07};
static const uint8_t kCapCcConstraintLength[] = {0x02};
static const uint8_t kCapBprfParameterSets[] = {0x1F};
static const uint8_t kCapHprfParameterSets[] = {0x03};
static const uint8_t kCapAoaSupport[] = {0x07};
static const uint8_t kCapExtendedMacAddress[] = {0x01};
static const uint8_t kCapMaxMessageSize[] = {0x00, 0x04};
static const uint8_t kCapMaxDataPayloadSize[] = {0x10, 0x01};
static const uint8_t kCapV2ExtendedMacAddress[] = {0x01};
static const uint8_t kCapV2Assigned[] = {0x00};
static const uint8_t kCapV2SessionKeyLength[] = {0x10, 0x20};
static const uint8_t kCapV2DtAnchorMaxActiveRr[] = {0x04};
static const uint8_t kCapV2DtTagMaxActiveRr[] = {0x04};
static const uint8_t kCapV2DtTagBlockShipping[] = {0x01};
static const uint8_t kCapV2PsduLengthSupport[] = {0xFF, 0x0F};
static const uint8_t kCapCccChapsPerSlot[] = {0x04};
static const uint8_t kCapCccSyncCodes[] = {0xFF, 0x00};
static const uint8_t kCapCccHoppingModes[] = {0x03};
static const uint8_t kCapCccChannels[] = {0x20, 0x08, 0x00};
static const uint8_t kCapCccVersions[] = {0x01};
static const uint8_t kCapCccUwbConfigs[] = {0x01};
static const uint8_t kCapCccPulseShapeCombos[] = {0x03};
static const uint8_t kCapCccRanMultiplier[] = {0x01};
static const uint8_t kCapCccMaxRangingSessions[] = {0x08};
static const uint8_t kCapCccMinUwbInitiationTimeMs[] = {0x14, 0x00};
static const uint8_t kCapCccPrioritizedChannelList[] = {0x09, 0x0D};
static const uint8_t kCapCccUwbsMaxPpm[] = {0x19};
static const uint8_t kCapAliroMacModes[] = {0x07};
static const uint8_t kCapRadarSupport[] = {0x01};
static const uint8_t kCapPowerStats[] = {0x01};
static const uint8_t kCapAoaResultReqInterleaving[] = {0x01};
static const uint8_t kCapMinRangingIntervalMs[] = {0x32, 0x00};
static const uint8_t kCapRangeDataNtfConfig[] = {0x03};
static const uint8_t kCapRssiReporting[] = {0x01};
static const uint8_t kCapDiagnostics[] = {0x01};
static const uint8_t kCapMinSlotDurationRstu[] = {0x10};
static const uint8_t kCapMaxRangingSessionNumber[] = {0x08};

static const capability_tlv_entry_t kDefaultCapabilityTlvs[] = {
    CAP_ENTRY(SUPPORTED_V1_FIRA_PHY_VERSION_RANGE_V2_MAX_MESSAGE_SIZE, kCapPhyVersionRange),
    CAP_ENTRY(SUPPORTED_V1_FIRA_MAC_VERSION_RANGE_V2_MAX_DATA_PAYLOAD_SIZE, kCapMacVersionRange),
    CAP_ENTRY(SUPPORTED_V1_DEVICE_ROLES_V2_FIRA_PHY_VERSION_RANGE, kCapDeviceRoles),
    CAP_ENTRY(SUPPORTED_V1_RANGING_METHOD_V2_FIRA_MAC_VERSION_RANGE, kCapRangingMethods),
    CAP_ENTRY(SUPPORTED_V1_STS_CONFIG_V2_DEVICE_TYPE, kCapStsConfig),
    CAP_ENTRY(SUPPORTED_V1_MULTI_NODE_MODES_V2_DEVICE_ROLES, kCapMultiNodeModes),
    CAP_ENTRY(SUPPORTED_V1_RANGING_TIME_STRUCT_V2_RANGING_METHOD, kCapRangingTimeStruct),
    CAP_ENTRY(SUPPORTED_V1_SCHEDULED_MODE_V2_STS_CONFIG, kCapScheduledMode),
    CAP_ENTRY(SUPPORTED_V1_HOPPING_MODE_V2_MULTI_NODE_MODE, kCapHoppingMode),
    CAP_ENTRY(SUPPORTED_V1_BLOCK_STRIDING_V2_RANGING_TIME_STRUCT, kCapBlockStriding),
    CAP_ENTRY(SUPPORTED_V1_UWB_INITIATION_TIME_V2_SCHEDULE_MODE, kCapUwbInitiationTime),
    CAP_ENTRY(SUPPORTED_V1_CHANNELS_V2_HOPPING_MODE, kCapChannels),
    CAP_ENTRY(SUPPORTED_V1_RFRAME_CONFIG_V2_BLOCK_STRIDING, kCapRframeConfig),
    CAP_ENTRY(SUPPORTED_V1_CC_CONSTRAINT_LENGTH_V2_UWB_INITIATION_TIME, kCapCcConstraintLength),
    CAP_ENTRY(SUPPORTED_V1_BPRF_PARAMETER_SETS_V2_CHANNELS, kCapBprfParameterSets),
    CAP_ENTRY(SUPPORTED_V1_HPRF_PARAMETER_SETS_V2_RFRAME_CONFIG, kCapHprfParameterSets),
    CAP_ENTRY(SUPPORTED_V1_AOA_V2_AOA_SUPPORT, kCapAoaSupport),
    CAP_ENTRY(SUPPORTED_V1_EXTENDED_MAC_ADDRESS_V2_EXTENDED_MAC_ADDRESS, kCapExtendedMacAddress),
    CAP_ENTRY(SUPPORTED_V1_MAX_MESSAGE_SIZE_V2_ASSIGNED, kCapMaxMessageSize),
    CAP_ENTRY(SUPPORTED_V1_MAX_DATA_PACKET_PAYLOAD_SIZE_V2_SESSION_KEY_LENGTH, kCapMaxDataPayloadSize),
    CAP_ENTRY(SUPPORTED_V2_EXTENDED_MAC_ADDRESS, kCapV2ExtendedMacAddress),
    CAP_ENTRY(SUPPORTED_V2_ASSIGNED, kCapV2Assigned),
    CAP_ENTRY(SUPPORTED_V2_SESSION_KEY_LENGTH, kCapV2SessionKeyLength),
    CAP_ENTRY(SUPPORTED_V2_DT_ANCHOR_MAX_ACTIVE_RR, kCapV2DtAnchorMaxActiveRr),
    CAP_ENTRY(SUPPORTED_V2_DT_TAG_MAX_ACTIVE_RR, kCapV2DtTagMaxActiveRr),
    CAP_ENTRY(SUPPORTED_V2_DT_TAG_BLOCK_SHIPPING, kCapV2DtTagBlockShipping),
    CAP_ENTRY(SUPPORTED_V2_PSDU_LENGTH_SUPPORT, kCapV2PsduLengthSupport),
    CAP_ENTRY(CCC_SUPPORTED_CHAPS_PER_SLOT, kCapCccChapsPerSlot),
    CAP_ENTRY(CCC_SUPPORTED_SYNC_CODES, kCapCccSyncCodes),
    CAP_ENTRY(CCC_SUPPORTED_HOPPING_CONFIG_MODES_AND_SEQUENCES, kCapCccHoppingModes),
    CAP_ENTRY(CCC_SUPPORTED_CHANNELS, kCapCccChannels),
    CAP_ENTRY(CCC_SUPPORTED_VERSIONS, kCapCccVersions),
    CAP_ENTRY(CCC_SUPPORTED_UWB_CONFIGS, kCapCccUwbConfigs),
    CAP_ENTRY(CCC_SUPPORTED_PULSE_SHAPE_COMBOS, kCapCccPulseShapeCombos),
    CAP_ENTRY(CCC_SUPPORTED_RAN_MULTIPLIER, kCapCccRanMultiplier),
    CAP_ENTRY(CCC_SUPPORTED_MAX_RANGING_SESSION_NUMBER, kCapCccMaxRangingSessions),
    CAP_ENTRY(CCC_SUPPORTED_MIN_UWB_INITIATION_TIME_MS, kCapCccMinUwbInitiationTimeMs),
    CAP_ENTRY(CCC_PRIORITIZED_CHANNEL_LIST, kCapCccPrioritizedChannelList),
    CAP_ENTRY(CCC_SUPPORTED_UWBS_MAX_PPM, kCapCccUwbsMaxPpm),
    CAP_ENTRY(ALIRO_SUPPORTED_MAC_MODES, kCapAliroMacModes),
    CAP_ENTRY(RADAR_SUPPORT, kCapRadarSupport),
    CAP_ENTRY(SUPPORTED_POWER_STATS, kCapPowerStats),
    CAP_ENTRY(SUPPORTED_AOA_RESULT_REQ_ANTENNA_INTERLEAVING, kCapAoaResultReqInterleaving),
    CAP_ENTRY(SUPPORTED_MIN_RANGING_INTERVAL_MS, kCapMinRangingIntervalMs),
    CAP_ENTRY(SUPPORTED_RANGE_DATA_NTF_CONFIG, kCapRangeDataNtfConfig),
    CAP_ENTRY(SUPPORTED_RSSI_REPORTING, kCapRssiReporting),
    CAP_ENTRY(SUPPORTED_DIAGNOSTICS, kCapDiagnostics),
    CAP_ENTRY(SUPPORTED_MIN_SLOT_DURATION_RSTU, kCapMinSlotDurationRstu),
    CAP_ENTRY(SUPPORTED_MAX_RANGING_SESSION_NUMBER, kCapMaxRangingSessionNumber),
};

static const size_t kDefaultCapabilityTlvsCount = sizeof(kDefaultCapabilityTlvs) / sizeof(kDefaultCapabilityTlvs[0]);

#define MAX_PENDING_NOTIFICATIONS 16
typedef struct {
    unsigned char data[sizeof(struct uci_packet_header) + MAX_RESPONSE_PAYLOAD_LEN];
    size_t length;
} notification_item_t;

static notification_item_t g_notification_queue[MAX_PENDING_NOTIFICATIONS];
static size_t g_notification_head = 0;
static size_t g_notification_tail = 0;

void enqueue_notification(unsigned char gid, unsigned char oid, const unsigned char* payload, size_t payload_len);

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

size_t uci_build_core_capabilities_payload(unsigned char* buffer, size_t max_len) {
    if (buffer == NULL || max_len < 2) {
        return 0;
    }

    size_t required = 2; // status + TLV count
    for (size_t i = 0; i < kDefaultCapabilityTlvsCount; i++) {
        required += 2 + kDefaultCapabilityTlvs[i].length;
    }

    if (required > max_len) {
        return 0;
    }

    size_t offset = 0;
    buffer[offset++] = UCI_STATUS_OK;
    buffer[offset++] = (uint8_t)kDefaultCapabilityTlvsCount;

    for (size_t i = 0; i < kDefaultCapabilityTlvsCount; i++) {
        const capability_tlv_entry_t* entry = &kDefaultCapabilityTlvs[i];
        buffer[offset++] = entry->type;
        buffer[offset++] = entry->length;
        memcpy(buffer + offset, entry->value, entry->length);
        offset += entry->length;
    }

    return offset;
}

void enqueue_notification(unsigned char gid, unsigned char oid, const unsigned char* payload, size_t payload_len) {
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
    uci_header_fields_t header_fields;
    uci_extract_header_fields(header, &header_fields);

    printf("=== UCI Packet Analysis ===\n");
    printf("Total Packet Length: %zu bytes\n", packet_len);
    printf("Header Bytes: %02X %02X %02X %02X\n", 
           packet[0], packet[1], packet[2], packet[3]);

    // Extract header fields
    unsigned char gid = header_fields.group_id;
    unsigned char pbf = header_fields.packet_boundary;
    unsigned char mt = header_fields.message_type;
    unsigned char opcode = header_fields.opcode_id;
    unsigned char opcode_reserved_bits = header_fields.reserved_opcode_bits;
    unsigned char payload_len = header_fields.payload_length;

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
        if (mt == COMMAND && gid == SESSION_CONFIG) {
            switch(opcode) {
                case SESSION_INIT:
                    decode_session_init_cmd(payload_ptr, (int)payload_len);
                    decoded = 1;
                    break;
                case SESSION_DEINIT:
                    printf("    SESSION_DEINIT_CMD - Session Deinitialization Command\n");
                    if (payload_len >= 4) {
                        unsigned int session_token = read_u32_le(payload_ptr);
                        printf("      Session Token: 0x%08X\n", session_token);
                    }
                    decoded = 1;
                    break;
                case SESSION_SET_APP_CONFIG:
                    printf("    SESSION_SET_APP_CONFIG_CMD - Set Application Config Command\n");
                    if (payload_len >= 5) {
                        unsigned int session_token = read_u32_le(payload_ptr);
                        unsigned char num_tlvs = payload_ptr[4];
                        printf("      Session Token: 0x%08X\n", session_token);
                        printf("      Number of TLVs: %u\n", num_tlvs);
                    }
                    decoded = 1;
                    break;
                default:
                    break;
            }
        } else if (mt == COMMAND && gid == SESSION_CONTROL) {
            switch(opcode) {
                case SESSION_START:
                    printf("    SESSION_START_CMD - Start Session Command\n");
                    if (payload_len >= 4) {
                        unsigned int session_token = read_u32_le(payload_ptr);
                        printf("      Session Token: 0x%08X\n", session_token);
                    }
                    decoded = 1;
                    break;
                case SESSION_STOP:
                    printf("    SESSION_STOP_CMD - Stop Session Command\n");
                    if (payload_len >= 4) {
                        unsigned int session_token = read_u32_le(payload_ptr);
                        printf("      Session Token: 0x%08X\n", session_token);
                    }
                    decoded = 1;
                    break;
                default:
                    break;
            }
        } else if (mt == RESPONSE && gid == CORE) {
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
                case CORE_QUERY_UWBS_TIMESTAMP:
                    decode_core_query_uwbs_timestamp_rsp(payload_ptr, (int)payload_len);
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
        } else if (mt == NOTIFICATION && gid == VENDOR_ANDROID) {
            switch(opcode) {
                case ANDROID_FIRA_RANGE_DIAGNOSTICS:
                    decode_android_range_diagnostics_ntf(payload_ptr, (int)payload_len);
                    decoded = 1;
                    break;
                default:
                    printf("    No specific decoder for VENDOR_ANDROID NOTIFICATION opcode 0x%02X\n", opcode);
                    break;
            }
        } else if (mt == NOTIFICATION && gid == RANGING_DATA) {
            if (opcode == RANGE_DATA_NTF_OPCODE) {
                decode_range_data_ntf(payload_ptr, (int)payload_len);
                decoded = 1;
            } else {
                printf("    No specific decoder for RANGING_DATA NOTIFICATION opcode 0x%02X\n", opcode);
            }
        } else if (gid == TEST) {
            printf("    Decoder not implemented for TEST packets\n");
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

// Function to get the current hardware device path
const char* uci_get_hardware_device_path() {
    return g_hardware_device_path;
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
        
        ui_print_sending_uci_packet(g_hardware_device_path);
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

    ui_print_sending_uci_packet("simulator");
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
        int len = build_core_device_info_response(response_packet + sizeof(struct uci_packet_header), MAX_RESPONSE_PAYLOAD_LEN);
        response_header->payload_len = len;
    } else if (gid == CORE && oid == CORE_GET_CAPS_INFO) {
        int len = build_core_get_caps_info_response(response_packet + sizeof(struct uci_packet_header), MAX_RESPONSE_PAYLOAD_LEN);
        response_header->payload_len = len;
    } else if (gid == CORE && oid == CORE_QUERY_UWBS_TIMESTAMP) {
        int len = build_core_query_timestamp_response(response_packet + sizeof(struct uci_packet_header), MAX_RESPONSE_PAYLOAD_LEN);
        response_header->payload_len = len;
    } else if (gid == CORE && oid == CORE_GET_CONFIG) {
        int len = build_core_get_config_response(response_packet + sizeof(struct uci_packet_header), MAX_RESPONSE_PAYLOAD_LEN, payload, payload_len);
        response_header->payload_len = len;
    } else if (gid == CORE && oid == CORE_SET_CONFIG) {
        int len = build_core_set_config_response(response_packet + sizeof(struct uci_packet_header), MAX_RESPONSE_PAYLOAD_LEN, payload, payload_len);
        response_header->payload_len = len;
    } else if (gid == CORE && oid == CORE_DEVICE_RESET) {
        int len = build_core_device_reset_response(response_packet + sizeof(struct uci_packet_header), MAX_RESPONSE_PAYLOAD_LEN);
        response_header->payload_len = len;
        // Reset all sessions when device is reset
        init_uci_sessions();
        unsigned char device_state_payload = DEVICE_STATE_READY;
        enqueue_notification(CORE, CORE_DEVICE_STATUS_NTF, &device_state_payload, 1);
    } else if (gid == CORE && oid == CORE_DEVICE_SUSPEND) {
        int len = build_core_device_suspend_response(response_packet + sizeof(struct uci_packet_header), MAX_RESPONSE_PAYLOAD_LEN);
        response_header->payload_len = len;
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
            uci_sessions[session_idx].multicast_count = 0;
            memset(uci_sessions[session_idx].multicast_entries, 0,
                   sizeof(uci_sessions[session_idx].multicast_entries));
            uci_sessions[session_idx].dt_tag_round_count = 0;
            memset(uci_sessions[session_idx].dt_tag_round_indexes, 0,
                   sizeof(uci_sessions[session_idx].dt_tag_round_indexes));
            uci_sessions[session_idx].dtp_repetition = 0;
            uci_sessions[session_idx].dtp_control = 0;
            uci_sessions[session_idx].dtp_size = 0;
            uci_sessions[session_idx].dtp_payload_len = 0;
            memset(uci_sessions[session_idx].dtp_payload, 0,
                   sizeof(uci_sessions[session_idx].dtp_payload));

            unsigned char session_init_rsp_payload[5];
            session_init_rsp_payload[0] = UCI_STATUS_OK;
            write_u32_le(&session_init_rsp_payload[1], session_handle);
            memcpy(response_packet + sizeof(struct uci_packet_header), session_init_rsp_payload, sizeof(session_init_rsp_payload));
            response_header->payload_len = sizeof(session_init_rsp_payload);

            enqueue_session_status_notification(&uci_sessions[session_idx], SESSION_STATE_INIT, STATE_CHANGE_WITH_SESSION_MANAGEMENT_COMMANDS);
        }
        
        // Add the missing parse_uci_packet call for successful response
        parse_uci_packet(response_packet, sizeof(struct uci_packet_header) + response_header->payload_len);
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
            session->multicast_count = 0;
            memset(session->multicast_entries, 0, sizeof(session->multicast_entries));
            session->dt_tag_round_count = 0;
            memset(session->dt_tag_round_indexes, 0, sizeof(session->dt_tag_round_indexes));
            session->dtp_repetition = 0;
            session->dtp_control = 0;
            session->dtp_size = 0;
            session->dtp_payload_len = 0;
            memset(session->dtp_payload, 0, sizeof(session->dtp_payload));
        } else {
            unsigned char err = UCI_STATUS_INVALID_PARAM;
            enqueue_notification(CORE, CORE_GENERIC_ERROR_NTF, &err, 1);
        }

        unsigned char session_deinit_rsp_payload[] = {UCI_STATUS_OK};
        memcpy(response_packet + sizeof(struct uci_packet_header), session_deinit_rsp_payload, sizeof(session_deinit_rsp_payload));
        response_header->payload_len = sizeof(session_deinit_rsp_payload);
        
        // Add missing parse_uci_packet call for successful response
        parse_uci_packet(response_packet, sizeof(struct uci_packet_header) + response_header->payload_len);
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
        
        // Add missing parse_uci_packet call for successful response
        parse_uci_packet(response_packet, sizeof(struct uci_packet_header) + response_header->payload_len);
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
        
        // Add missing parse_uci_packet call for successful response
        parse_uci_packet(response_packet, sizeof(struct uci_packet_header) + response_header->payload_len);
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
        
        // Add missing parse_uci_packet call for successful response
        parse_uci_packet(response_packet, sizeof(struct uci_packet_header) + response_header->payload_len);
    } else if (gid == SESSION_CONFIG && oid == SESSION_GET_COUNT) {
        unsigned char session_count = (unsigned char)get_allocated_session_count();
        unsigned char get_count_rsp_payload[] = {UCI_STATUS_OK, session_count};
        memcpy(response_packet + sizeof(struct uci_packet_header), get_count_rsp_payload, sizeof(get_count_rsp_payload));
        response_header->payload_len = sizeof(get_count_rsp_payload);
        
        // Add missing parse_uci_packet call for successful response
        parse_uci_packet(response_packet, sizeof(struct uci_packet_header) + response_header->payload_len);
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
        
        // Add missing parse_uci_packet call for successful response
        parse_uci_packet(response_packet, sizeof(struct uci_packet_header) + response_header->payload_len);
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
        
        // Add missing parse_uci_packet call for successful response
        parse_uci_packet(response_packet, sizeof(struct uci_packet_header) + response_header->payload_len);
    } else if (gid == SESSION_CONFIG && oid == SESSION_UPDATE_CONTROLLER_MULTICAST_LIST) {
        typedef struct {
            unsigned short short_address;
            unsigned int subsession_id;
            unsigned char status;
        } multicast_result_t;

        multicast_result_t results[MAX_MULTICAST_CONTROLEES];
        memset(results, 0, sizeof(results));

        unsigned char overall_status = UCI_STATUS_OK;
        unsigned char processed = 0;
        unsigned char action = 0;
        unsigned char declared = 0;
        unsigned int identifier = 0;
        int session_idx = -1;
        int offset = 6;

        if (!payload || payload_len < 6) {
            overall_status = UCI_STATUS_INVALID_PARAM;
        } else {
            identifier = read_u32_le(payload);
            action = payload[4];
            declared = payload[5];
            session_idx = find_session_by_token_or_id(identifier);
            if (session_idx < 0) {
                overall_status = UCI_STATUS_INVALID_PARAM;
            }
        }

        if (overall_status == UCI_STATUS_OK) {
            struct uci_session* session = &uci_sessions[session_idx];
            for (unsigned char i = 0; i < declared; i++) {
                if (offset + 2 > payload_len) {
                    overall_status = UCI_STATUS_INVALID_PARAM;
                    break;
                }

                unsigned short short_address = read_u16_le(&payload[offset]);
                offset += 2;

                unsigned int subsession_id = 0;
                unsigned char key_len_expected = 0;
                unsigned char key_buffer[32] = {0};

                int additional_bytes = 0;
                switch (action) {
                    case MULTICAST_ACTION_ADD:
                        additional_bytes = 4;
                        break;
                    case MULTICAST_ACTION_REMOVE:
                        additional_bytes = 4;
                        break;
                    case MULTICAST_ACTION_ADD_SHORT_KEY:
                        additional_bytes = 4 + 16;
                        key_len_expected = 16;
                        break;
                    case MULTICAST_ACTION_ADD_LONG_KEY:
                        additional_bytes = 4 + 32;
                        key_len_expected = 32;
                        break;
                    default:
                        overall_status = UCI_STATUS_INVALID_PARAM;
                        break;
                }

                if (overall_status != UCI_STATUS_OK) {
                    break;
                }

                if (offset + additional_bytes > payload_len) {
                    overall_status = UCI_STATUS_INVALID_PARAM;
                    break;
                }

                subsession_id = read_u32_le(&payload[offset]);
                offset += 4;

                if (key_len_expected > 0) {
                    memcpy(key_buffer, &payload[offset], key_len_expected);
                    offset += key_len_expected;
                }

                unsigned char per_status;
                if (action == MULTICAST_ACTION_REMOVE) {
                    per_status = session_remove_multicast_entry(session, short_address, subsession_id);
                } else {
                    per_status = session_add_multicast_entry(session, short_address, subsession_id,
                                                             key_len_expected ? key_buffer : NULL,
                                                             key_len_expected);
                }

                if (processed < MAX_MULTICAST_CONTROLEES) {
                    results[processed].short_address = short_address;
                    results[processed].subsession_id = subsession_id;
                    results[processed].status = per_status;
                    processed++;
                }

                if (per_status != UCI_STATUS_OK && overall_status == UCI_STATUS_OK) {
                    overall_status = per_status;
                }
            }
        }

        unsigned char multicast_rsp_payload[MAX_RESPONSE_PAYLOAD_LEN] = {0};
        multicast_rsp_payload[0] = overall_status;
        multicast_rsp_payload[1] = processed;
        size_t rsp_offset = 2;

        for (unsigned char i = 0; i < processed; i++) {
            if (rsp_offset + 7 > sizeof(multicast_rsp_payload)) {
                multicast_rsp_payload[0] = UCI_STATUS_INVALID_MSG_SIZE;
                multicast_rsp_payload[1] = 0;
                rsp_offset = 2;
                break;
            }
            write_u16_le(&multicast_rsp_payload[rsp_offset], results[i].short_address);
            rsp_offset += 2;
            write_u32_le(&multicast_rsp_payload[rsp_offset], results[i].subsession_id);
            rsp_offset += 4;
            multicast_rsp_payload[rsp_offset++] = results[i].status;
        }

        if (multicast_rsp_payload[0] != UCI_STATUS_OK) {
            unsigned char err = multicast_rsp_payload[0];
            enqueue_notification(CORE, CORE_GENERIC_ERROR_NTF, &err, 1);
        }

        memcpy(response_packet + sizeof(struct uci_packet_header), multicast_rsp_payload, rsp_offset);
        response_header->payload_len = (unsigned char)rsp_offset;
    } else if (gid == SESSION_CONFIG && oid == SESSION_UPDATE_ACTIVE_ROUNDS_DT_TAG) {
        unsigned char status = UCI_STATUS_OK;
        unsigned char stored_count = 0;
        struct uci_session* session = NULL;

        if (!payload || payload_len < 5) {
            status = UCI_STATUS_INVALID_PARAM;
        } else {
            unsigned int identifier = read_u32_le(payload);
            unsigned char declared = payload[4];
            if (payload_len < 5 + declared) {
                status = UCI_STATUS_INVALID_PARAM;
            } else {
                int session_idx = find_session_by_token_or_id(identifier);
                if (session_idx < 0) {
                    status = UCI_STATUS_INVALID_PARAM;
                } else if (declared > MAX_DT_TAG_ROUNDS) {
                    status = UCI_STATUS_INVALID_PARAM;
                } else {
                    session = &uci_sessions[session_idx];
                    session->dt_tag_round_count = declared;
                    memcpy(session->dt_tag_round_indexes, &payload[5], declared);
                    stored_count = declared;
                }
            }
        }

        unsigned char dt_rsp_payload[MAX_RESPONSE_PAYLOAD_LEN] = {0};
        size_t response_len = 2;
        dt_rsp_payload[0] = status;
        dt_rsp_payload[1] = (status == UCI_STATUS_OK) ? stored_count : 0;
        if (status == UCI_STATUS_OK && stored_count > 0) {
            memcpy(&dt_rsp_payload[2], session->dt_tag_round_indexes, stored_count);
            response_len += stored_count;
        }

        if (status != UCI_STATUS_OK) {
            unsigned char err = status;
            enqueue_notification(CORE, CORE_GENERIC_ERROR_NTF, &err, 1);
        }

        memcpy(response_packet + sizeof(struct uci_packet_header), dt_rsp_payload, response_len);
        response_header->payload_len = (unsigned char)response_len;
    } else if (gid == SESSION_CONFIG && oid == SESSION_DATA_TRANSFER_PHASE_CONFIG) {
        unsigned char status = UCI_STATUS_OK;
        struct uci_session* session = NULL;
        unsigned char dtp_repetition = 0;
        unsigned char dtp_control = 0;
        unsigned char dtp_size = 0;
        unsigned char extra_len = 0;
        unsigned char extra_payload[64] = {0};

        if (!payload || payload_len < 7) {
            status = UCI_STATUS_INVALID_PARAM;
        } else {
            unsigned int identifier = read_u32_le(payload);
            dtp_repetition = payload[4];
            dtp_control = payload[5];
            dtp_size = payload[6];
            extra_len = (unsigned char)((payload_len > 7) ? (payload_len - 7) : 0);
            if (extra_len > sizeof(extra_payload)) {
                status = UCI_STATUS_INVALID_MSG_SIZE;
            } else {
                if (extra_len > 0) {
                    memcpy(extra_payload, &payload[7], extra_len);
                }

                int session_idx = find_session_by_token_or_id(identifier);
                if (session_idx < 0) {
                    status = UCI_STATUS_INVALID_PARAM;
                } else {
                    session = &uci_sessions[session_idx];
                }
            }
        }

        if (status == UCI_STATUS_OK && session) {
            session->dtp_repetition = dtp_repetition;
            session->dtp_control = dtp_control;
            session->dtp_size = dtp_size;
            session->dtp_payload_len = extra_len;
            if (extra_len > 0) {
                memcpy(session->dtp_payload, extra_payload, extra_len);
            } else {
                memset(session->dtp_payload, 0, sizeof(session->dtp_payload));
            }
        }

        if (status != UCI_STATUS_OK) {
            unsigned char err = status;
            enqueue_notification(CORE, CORE_GENERIC_ERROR_NTF, &err, 1);
        }

        unsigned char dtp_rsp[] = {status};
        memcpy(response_packet + sizeof(struct uci_packet_header), dtp_rsp, sizeof(dtp_rsp));
        response_header->payload_len = sizeof(dtp_rsp);
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

void handle_core_query_uwbs_timestamp_rsp(unsigned char* payload, int payload_len) {
    if (payload_len < 9) { // status (1) + timestamp (8)
        printf("Error: CORE_QUERY_UWBS_TIMESTAMP_RSP payload too short. Expected at least 9 bytes, got %d.\n", payload_len);
        return;
    }
    unsigned char status = payload[0];
    unsigned long long timestamp = read_u64_le(&payload[1]);
    
    printf("  Status: 0x%02X", status);
    switch(status) {
        case UCI_STATUS_OK: 
            printf(" (OK)\n");
            printf("  Timestamp: %llu\n", timestamp);
            break;
        case UCI_STATUS_REJECTED: 
            printf(" (REJECTED)\n");
            break;
        case UCI_STATUS_FAILED: 
            printf(" (FAILED)\n");
            break;
        default: 
            printf(" (UNKNOWN)\n");
            break;
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
    printf("  Status: 0x%02X", status);
    switch(status) {
        case UCI_STATUS_OK: printf(" (OK)\n"); break;
        case UCI_STATUS_REJECTED: printf(" (REJECTED)\n"); break;
        case UCI_STATUS_FAILED: printf(" (FAILED)\n"); break;
        case UCI_STATUS_INVALID_PARAM: printf(" (INVALID_PARAM)\n"); break;
        default: printf(" (UNKNOWN)\n"); break;
    }
    printf("  Number of TLVs: %d\n", num_tlvs);

    int offset = 2;
    for (int i = 0; i < num_tlvs; i++) {
        if (offset + 2 > payload_len) {
            printf("Error: Incomplete TLV in CORE_GET_CAPS_INFO_RSP payload.\n");
            return;
        }
        CapTlvType tlv_type = (CapTlvType)payload[offset];
        unsigned char tlv_len = payload[offset + 1];
        offset += 2;
        
        // Print TLV type with descriptive name
        printf("    TLV %d:\n", i);
        printf("      Type: 0x%02X", tlv_type);
        switch(tlv_type) {
            case SUPPORTED_V1_FIRA_PHY_VERSION_RANGE_V2_MAX_MESSAGE_SIZE: printf(" (FIRA_PHY_VERSION_RANGE)\n"); break;
            case SUPPORTED_V1_FIRA_MAC_VERSION_RANGE_V2_MAX_DATA_PAYLOAD_SIZE: printf(" (FIRA_MAC_VERSION_RANGE)\n"); break;
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
            case SUPPORTED_V1_AOA_V2_AOA_SUPPORT: printf(" (AOA_SUPPORT)\n"); break;
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
            case CCC_SUPPORTED_CHAPS_PER_SLOT: printf(" (CCC_CHAPS_PER_SLOT)\n"); break;
            case CCC_SUPPORTED_SYNC_CODES: printf(" (CCC_SYNC_CODES)\n"); break;
            case CCC_SUPPORTED_HOPPING_CONFIG_MODES_AND_SEQUENCES: printf(" (CCC_HOPPING_CONFIG_MODES)\n"); break;
            case CCC_SUPPORTED_CHANNELS: printf(" (CCC_CHANNELS)\n"); break;
            case CCC_SUPPORTED_VERSIONS: printf(" (CCC_VERSIONS)\n"); break;
            case CCC_SUPPORTED_UWB_CONFIGS: printf(" (CCC_UWB_CONFIGS)\n"); break;
            case CCC_SUPPORTED_PULSE_SHAPE_COMBOS: printf(" (CCC_PULSE_SHAPE_COMBOS)\n"); break;
            case CCC_SUPPORTED_RAN_MULTIPLIER: printf(" (CCC_RAN_MULTIPLIER)\n"); break;
            case CCC_SUPPORTED_MAX_RANGING_SESSION_NUMBER: printf(" (CCC_MAX_RANGING_SESSION_NUMBER)\n"); break;
            case CCC_SUPPORTED_MIN_UWB_INITIATION_TIME_MS: printf(" (CCC_MIN_UWB_INITIATION_TIME_MS)\n"); break;
            case CCC_PRIORITIZED_CHANNEL_LIST: printf(" (CCC_PRIORITIZED_CHANNEL_LIST)\n"); break;
            case CCC_SUPPORTED_UWBS_MAX_PPM: printf(" (CCC_UWBS_MAX_PPM)\n"); break;
            case ALIRO_SUPPORTED_MAC_MODES: printf(" (ALIRO_MAC_MODES)\n"); break;
            case RADAR_SUPPORT: printf(" (RADAR_SUPPORT)\n"); break;
            case SUPPORTED_POWER_STATS: printf(" (POWER_STATS)\n"); break;
            case SUPPORTED_AOA_RESULT_REQ_ANTENNA_INTERLEAVING: printf(" (AOA_RESULT_REQ_ANTENNA_INTERLEAVING)\n"); break;
            case SUPPORTED_MIN_RANGING_INTERVAL_MS: printf(" (MIN_RANGING_INTERVAL_MS)\n"); break;
            case SUPPORTED_RANGE_DATA_NTF_CONFIG: printf(" (RANGE_DATA_NTF_CONFIG)\n"); break;
            case SUPPORTED_RSSI_REPORTING: printf(" (RSSI_REPORTING)\n"); break;
            case SUPPORTED_DIAGNOSTICS: printf(" (DIAGNOSTICS)\n"); break;
            case SUPPORTED_MIN_SLOT_DURATION_RSTU: printf(" (MIN_SLOT_DURATION_RSTU)\n"); break;
            case SUPPORTED_MAX_RANGING_SESSION_NUMBER: printf(" (MAX_RANGING_SESSION_NUMBER)\n"); break;
            default: printf(" (UNKNOWN)\n"); break;
        }
        printf("      Length: %d bytes\n", tlv_len);
        
        if (offset + tlv_len > payload_len) {
            printf("Error: Incomplete TLV value in CORE_GET_CAPS_INFO_RSP payload.\n");
            return;
        }
        
        // Print value with interpretation based on TLV type
        printf("      Value: ");
        for (int j = 0; j < tlv_len; j++) {
            printf("%02X ", payload[offset + j]);
        }
        printf("\n");
        
        // Interpret value based on TLV type and length
        if (tlv_len > 0) {
            printf("      Interpreted Value: ");
            if (tlv_len == 1) {
                unsigned char val = payload[offset];
                printf("%u", val);
                // Special interpretations for boolean-like values
                if (tlv_type == SUPPORTED_V1_AOA_V2_AOA_SUPPORT ||
                    tlv_type == SUPPORTED_V2_DT_TAG_BLOCK_SHIPPING ||
                    tlv_type == RADAR_SUPPORT) {
                    printf(" (%s)", val ? "SUPPORTED" : "NOT_SUPPORTED");
                }
            } else if (tlv_len == 2) {
                unsigned short val = payload[offset] | (payload[offset + 1] << 8); // Little-endian
                printf("%u", val);
            } else if (tlv_len == 4) {
                unsigned int val = payload[offset] | 
                                  (payload[offset + 1] << 8) | 
                                  (payload[offset + 2] << 16) | 
                                  (payload[offset + 3] << 24); // Little-endian
                printf("%u", val);
            } else {
                // For longer values, show as hex dump with interpretation
                printf("(complex value)");
            }
            printf("\n");
        }
        
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

// Helper function to map application config TLV IDs to human-readable names
static const char* get_app_config_name(AppConfigTlvType cfg_id) {
    switch (cfg_id) {
        case DEVICE_TYPE: return "DEVICE_TYPE";
        case RANGING_ROUND_USAGE: return "RANGING_ROUND_USAGE";
        case STS_CONFIG: return "STS_CONFIG";
        case MULTI_NODE_MODE: return "MULTI_NODE_MODE";
        case CHANNEL_NUMBER: return "CHANNEL_NUMBER";
        case NO_OF_CONTROLEE: return "NO_OF_CONTROLEE";
        case DEVICE_MAC_ADDRESS: return "DEVICE_MAC_ADDRESS";
        case DST_MAC_ADDRESS: return "DST_MAC_ADDRESS";
        case SLOT_DURATION: return "SLOT_DURATION";
        case RANGING_DURATION: return "RANGING_DURATION";
        case STS_INDEX: return "STS_INDEX";
        case MAC_FCS_TYPE: return "MAC_FCS_TYPE";
        case RANGING_ROUND_CONTROL: return "RANGING_ROUND_CONTROL";
        case AOA_RESULT_REQ: return "AOA_RESULT_REQ";
        case RNG_DATA_NTF: return "RNG_DATA_NTF";
        case RNG_DATA_NTF_PROXIMITY_NEAR: return "RNG_DATA_NTF_PROXIMITY_NEAR";
        case RNG_DATA_NTF_PROXIMITY_FAR: return "RNG_DATA_NTF_PROXIMITY_FAR";
        case DEVICE_ROLE: return "DEVICE_ROLE";
        case RFRAME_CONFIG: return "RFRAME_CONFIG";
        case RSSI_REPORTING: return "RSSI_REPORTING";
        case PREAMBLE_CODE_INDEX: return "PREAMBLE_CODE_INDEX";
        case SFD_ID: return "SFD_ID";
        case PSDU_DATA_RATE: return "PSDU_DATA_RATE";
        case PREAMBLE_DURATION: return "PREAMBLE_DURATION";
        case LINK_LAYER_MODE: return "LINK_LAYER_MODE";
        case DATA_REPETITION_COUNT: return "DATA_REPETITION_COUNT";
        case RANGING_TIME_STRUCT: return "RANGING_TIME_STRUCT";
        case SLOTS_PER_RR: return "SLOTS_PER_RR";
        case TX_ADAPTIVE_PAYLOAD_POWER: return "TX_ADAPTIVE_PAYLOAD_POWER";
        case RNG_DATA_NTF_AOA_BOUND: return "RNG_DATA_NTF_AOA_BOUND";
        case RESPONDER_SLOT_INDEX: return "RESPONDER_SLOT_INDEX";
        case PRF_MODE: return "PRF_MODE";
        case CAP_SIZE_RANGE: return "CAP_SIZE_RANGE";
        case TX_JITTER_WINDOW_SIZE: return "TX_JITTER_WINDOW_SIZE";
        case SCHEDULED_MODE: return "SCHEDULED_MODE";
        case KEY_ROTATION: return "KEY_ROTATION";
        case KEY_ROTATION_RATE: return "KEY_ROTATION_RATE";
        case SESSION_PRIORITY: return "SESSION_PRIORITY";
        case MAC_ADDRESS_MODE: return "MAC_ADDRESS_MODE";
        case VENDOR_ID: return "VENDOR_ID";
        case STATIC_STS_IV: return "STATIC_STS_IV";
        case NUMBER_OF_STS_SEGMENTS: return "NUMBER_OF_STS_SEGMENTS";
        case MAX_RR_RETRY: return "MAX_RR_RETRY";
        case UWB_INITIATION_TIME: return "UWB_INITIATION_TIME";
        case HOPPING_MODE: return "HOPPING_MODE";
        case BLOCK_STRIDE_LENGTH: return "BLOCK_STRIDE_LENGTH";
        case RESULT_REPORT_CONFIG: return "RESULT_REPORT_CONFIG";
        case IN_BAND_TERMINATION_ATTEMPT_COUNT: return "IN_BAND_TERMINATION_ATTEMPT_COUNT";
        case SUB_SESSION_ID: return "SUB_SESSION_ID";
        case BPRF_PHR_DATA_RATE: return "BPRF_PHR_DATA_RATE";
        case MAX_NUMBER_OF_MEASUREMENTS: return "MAX_NUMBER_OF_MEASUREMENTS";
        case UL_TDOA_TX_INTERVAL: return "UL_TDOA_TX_INTERVAL";
        case UL_TDOA_RANDOM_WINDOW: return "UL_TDOA_RANDOM_WINDOW";
        case STS_LENGTH: return "STS_LENGTH";
        case SUSPEND_RANGING_ROUNDS: return "SUSPEND_RANGING_ROUNDS";
        case UL_TDOA_NTF_REPORT_CONFIG: return "UL_TDOA_NTF_REPORT_CONFIG";
        case UL_TDOA_DEVICE_ID: return "UL_TDOA_DEVICE_ID";
        case UL_TDOA_TX_TIMESTAMP: return "UL_TDOA_TX_TIMESTAMP";
        case MIN_FRAMES_PER_RR: return "MIN_FRAMES_PER_RR";
        case MTU_SIZE: return "MTU_SIZE";
        case INTER_FRAME_INTERVAL: return "INTER_FRAME_INTERVAL";
        case DL_TDOA_RANGING_METHOD: return "DL_TDOA_RANGING_METHOD";
        case DL_TDOA_TX_TIMESTAMP_CONF: return "DL_TDOA_TX_TIMESTAMP_CONF";
        case DL_TDOA_HOP_COUNT: return "DL_TDOA_HOP_COUNT";
        case DL_TDOA_ANCHOR_CFO: return "DL_TDOA_ANCHOR_CFO";
        case DL_TDOA_ANCHOR_LOCATION: return "DL_TDOA_ANCHOR_LOCATION";
        case DL_TDOA_TX_ACTIVE_RANGING_ROUNDS: return "DL_TDOA_TX_ACTIVE_RANGING_ROUNDS";
        case DL_TDOA_BLOCK_STRIDING: return "DL_TDOA_BLOCK_STRIDING";
        case DL_TDOA_TIME_REFERENCE_ANCHOR: return "DL_TDOA_TIME_REFERENCE_ANCHOR";
        case SESSION_KEY: return "SESSION_KEY";
        case SUBSESSION_KEY: return "SUBSESSION_KEY";
        case SESSION_DATA_TRANSFER_STATUS_NTF_CONFIG: return "SESSION_DATA_TRANSFER_STATUS_NTF_CONFIG";
        case SESSION_TIME_BASE: return "SESSION_TIME_BASE";
        case DL_TDOA_RESPONDER_TOF: return "DL_TDOA_RESPONDER_TOF";
        case SECURE_RANGING_NEFA_LEVEL: return "SECURE_RANGING_NEFA_LEVEL";
        case SECURE_RANGING_CSW_LENGTH: return "SECURE_RANGING_CSW_LENGTH";
        case APPLICATION_DATA_ENDPOINT: return "APPLICATION_DATA_ENDPOINT";
        case OWR_AOA_MEASUREMENT_NTF_PERIOD: return "OWR_AOA_MEASUREMENT_NTF_PERIOD";
        case CCC_HOP_MODE_KEY: return "CCC_HOP_MODE_KEY";
        case CCC_UWB_TIME0: return "CCC_UWB_TIME0";
        case CCC_RANGING_PROTOCOL_VER: return "CCC_RANGING_PROTOCOL_VER";
        case CCC_UWB_CONFIG_ID: return "CCC_UWB_CONFIG_ID";
        case CCC_PULSESHAPE_COMBO: return "CCC_PULSESHAPE_COMBO";
        case CCC_URSK_TTL: return "CCC_URSK_TTL";
        case CCC_LAST_INDEX_USED: return "CCC_LAST_INDEX_USED";
        case ALIRO_MAC_MODE: return "ALIRO_MAC_MODE";
        case NB_OF_RANGE_MEASUREMENTS: return "NB_OF_RANGE_MEASUREMENTS";
        case NB_OF_AZIMUTH_MEASUREMENTS: return "NB_OF_AZIMUTH_MEASUREMENTS";
        case NB_OF_ELEVATION_MEASUREMENTS: return "NB_OF_ELEVATION_MEASUREMENTS";
        case ENABLE_DIAGNOSTICS: return "ENABLE_DIAGNOSTICS";
        case DIAGRAMS_FRAME_REPORTS_FIELDS: return "DIAGRAMS_FRAME_REPORTS_FIELDS";
        case ANTENNA_MODE: return "ANTENNA_MODE";
        default: return NULL;
    }
}

static const char* get_frame_report_tlv_name(FrameReportTlvType tlv_type) {
    switch (tlv_type) {
        case FRAME_REPORT_TLV_RSSI: return "RSSI";
        case FRAME_REPORT_TLV_AOA: return "AOA";
        case FRAME_REPORT_TLV_CIR: return "CIR";
        case FRAME_REPORT_TLV_SEGMENT_METRICS: return "SEGMENT_METRICS";
        default: return NULL;
    }
}

static const char* get_segment_id_name(unsigned char segment_id) {
    switch (segment_id) {
        case 0: return "IPATOV";
        case 1: return "STS0";
        case 2: return "STS1";
        case 3: return "STS2";
        case 4: return "STS3";
        default: return "UNKNOWN";
    }
}

static void decode_frame_report_tlv(FrameReportTlvType tlv_type,
                                    const unsigned char* value,
                                    unsigned short length) {
    const char* name = get_frame_report_tlv_name(tlv_type);
    printf("        TLV Type: 0x%02X", tlv_type);
    if (name) {
        printf(" (%s)", name);
    } else {
        printf(" (UNKNOWN)");
    }
    printf(", Length: %u\n", length);

    if (!value || length == 0) {
        printf("          No data.\n");
        return;
    }

    switch (tlv_type) {
        case FRAME_REPORT_TLV_RSSI: {
            printf("          RSSI Samples (dBm): ");
            for (unsigned short i = 0; i < length; i++) {
                printf("%d ", (int8_t)value[i]);
            }
            printf("\n");
            break;
        }
        case FRAME_REPORT_TLV_AOA: {
            if (length % 8 != 0) {
                printf("          WARNING: AOA payload length %u is not a multiple of 8 bytes.\n", length);
            }
            unsigned short offset = 0;
            unsigned int measurement_idx = 0;
            while (offset + 8 <= length) {
                int16_t tdoa = (int16_t)read_u16_le(&value[offset]);
                int16_t pdoa = (int16_t)read_u16_le(&value[offset + 2]);
                int16_t aoa = (int16_t)read_u16_le(&value[offset + 4]);
                unsigned char fom = value[offset + 6];
                unsigned char meas_type = value[offset + 7];
                printf("          Measurement %u: TDOA=%d, PDOA=%d, AoA=%d, FoM=%u, Type=0x%02X\n",
                       measurement_idx++, tdoa, pdoa, aoa, fom, meas_type);
                offset += 8;
            }
            if (offset < length) {
                printf("          %u trailing bytes remain unparsed.\n", (unsigned int)(length - offset));
            }
            break;
        }
        case FRAME_REPORT_TLV_CIR: {
            unsigned short offset = 0;
            if (length < 1) {
                printf("          WARNING: CIR payload too short.\n");
                break;
            }
            unsigned char cir_entries = value[offset++];
            printf("          CIR Entries: %u\n", cir_entries);
            for (unsigned char entry = 0; entry < cir_entries; entry++) {
                if (offset + 16 > length) {
                    printf("          WARNING: CIR entry %u truncated (need at least 16 bytes, have %u).\n",
                           entry, (unsigned int)(length - offset));
                    break;
                }
                unsigned short first_path_index = read_u16_le(&value[offset]);
                unsigned short first_path_snr = read_u16_le(&value[offset + 2]);
                unsigned short first_path_ns = read_u16_le(&value[offset + 4]);
                unsigned short peak_path_index = read_u16_le(&value[offset + 6]);
                unsigned short peak_path_snr = read_u16_le(&value[offset + 8]);
                unsigned short peak_path_ns = read_u16_le(&value[offset + 10]);
                unsigned char first_path_sample_offset = value[offset + 12];
                unsigned char samples_number = value[offset + 13];
                unsigned short sample_window_len = read_u16_le(&value[offset + 14]);
                offset += 16;

                printf("          CIR %u: first_path_index=%u, first_path_snr=%u, first_path_ns=%u\n",
                       entry, first_path_index, first_path_snr, first_path_ns);
                printf("                   peak_path_index=%u, peak_path_snr=%u, peak_path_ns=%u\n",
                       peak_path_index, peak_path_snr, peak_path_ns);
                printf("                   first_path_sample_offset=%u, samples_number=%u, sample_window_len=%u\n",
                       first_path_sample_offset, samples_number, sample_window_len);

                if (offset + sample_window_len > length) {
                    printf("          WARNING: CIR sample window truncated (expected %u bytes, have %u).\n",
                           sample_window_len, (unsigned int)(length - offset));
                    break;
                }

                unsigned short preview = sample_window_len < 16 ? sample_window_len : 16;
                printf("                   Sample window preview: ");
                for (unsigned short i = 0; i < preview; i++) {
                    printf("%02X ", value[offset + i]);
                }
                if (sample_window_len > preview) {
                    printf("... (+%u bytes)", sample_window_len - preview);
                }
                printf("\n");

                offset += sample_window_len;
            }
            if (offset < length) {
                printf("          %u trailing bytes remain after CIR parsing.\n", (unsigned int)(length - offset));
            }
            break;
        }
        case FRAME_REPORT_TLV_SEGMENT_METRICS: {
            unsigned short offset = 0;
            unsigned int metric_index = 0;
            const unsigned short entry_size = 17; // Derived from SegmentMetricsValue layout

            while (offset + entry_size <= length) {
                unsigned char ras = value[offset++];
                unsigned char segment_id = ras & 0x07;
                unsigned char receiver_is_controller = (ras >> 3) & 0x01;
                unsigned char receiver_id = (ras >> 4) & 0x0F;

                int16_t rf_noise_floor_raw = (int16_t)read_u16_le(&value[offset]);
                offset += 2;
                int16_t segment_rsl_raw = (int16_t)read_u16_le(&value[offset]);
                offset += 2;

                uint16_t first_path_index = read_u16_le(&value[offset]);
                offset += 2;
                int16_t first_path_rsl_raw = (int16_t)read_u16_le(&value[offset]);
                offset += 2;
                int16_t first_path_time_raw = (int16_t)read_u16_le(&value[offset]);
                offset += 2;

                uint16_t peak_path_index = read_u16_le(&value[offset]);
                offset += 2;
                int16_t peak_path_rsl_raw = (int16_t)read_u16_le(&value[offset]);
                offset += 2;
                int16_t peak_path_time_raw = (int16_t)read_u16_le(&value[offset]);
                offset += 2;

                printf("          Segment Metrics %u:\n", metric_index++);
                printf("            Segment: %s (ID=%u)\n", get_segment_id_name(segment_id), segment_id);
                printf("            Receiver: %s (ID=%u)\n",
                       receiver_is_controller ? "Controller" : "Controlee",
                       receiver_id);
                printf("            RF Noise Floor: %.2f dBm (raw 0x%04X)\n",
                       q8_8_to_double(rf_noise_floor_raw), (unsigned int)(uint16_t)rf_noise_floor_raw);
                printf("            Segment RSL: %.2f dBm (raw 0x%04X)\n",
                       q8_8_to_double(segment_rsl_raw), (unsigned int)(uint16_t)segment_rsl_raw);
                printf("            First Path -> index=%u, RSL=%.2f dBm, time=%.3f ns\n",
                       first_path_index,
                       q8_8_to_double(first_path_rsl_raw),
                       q6_9_to_double(first_path_time_raw));
                printf("            Peak  Path -> index=%u, RSL=%.2f dBm, time=%.3f ns\n",
                       peak_path_index,
                       q8_8_to_double(peak_path_rsl_raw),
                       q6_9_to_double(peak_path_time_raw));
            }

            if (offset < length) {
                printf("          WARNING: %u trailing bytes remain after parsing segment metrics.\n",
                       (unsigned int)(length - offset));
            }
            break;
        }
        default: {
            printf("          Raw bytes: ");
            unsigned short preview = length < 32 ? length : 32;
            for (unsigned short i = 0; i < preview; i++) {
                printf("%02X ", value[i]);
            }
            if (length > preview) {
                printf("... (+%u bytes)", length - preview);
            }
            printf("\n");
            break;
        }
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
        uci_sessions[i].multicast_count = 0;
        memset(uci_sessions[i].multicast_entries, 0, sizeof(uci_sessions[i].multicast_entries));
        uci_sessions[i].dt_tag_round_count = 0;
        memset(uci_sessions[i].dt_tag_round_indexes, 0, sizeof(uci_sessions[i].dt_tag_round_indexes));
        uci_sessions[i].dtp_repetition = 0;
        uci_sessions[i].dtp_control = 0;
        uci_sessions[i].dtp_size = 0;
        uci_sessions[i].dtp_payload_len = 0;
        memset(uci_sessions[i].dtp_payload, 0, sizeof(uci_sessions[i].dtp_payload));
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

static int session_find_multicast_entry_idx(const struct uci_session* session,
                                            unsigned short short_address,
                                            unsigned int subsession_id) {
    if (!session) {
        return -1;
    }
    for (int i = 0; i < session->multicast_count; i++) {
        const uci_multicast_entry* entry = &session->multicast_entries[i];
        if (entry->short_address == short_address && entry->subsession_id == subsession_id) {
            return i;
        }
    }
    return -1;
}

static unsigned char session_add_multicast_entry(struct uci_session* session,
                                                 unsigned short short_address,
                                                 unsigned int subsession_id,
                                                 const unsigned char* key,
                                                 unsigned char key_len) {
    if (!session) {
        return UCI_STATUS_INVALID_PARAM;
    }

    if (key_len > sizeof(session->multicast_entries[0].key)) {
        key_len = sizeof(session->multicast_entries[0].key);
    }

    int existing_idx = session_find_multicast_entry_idx(session, short_address, subsession_id);
    if (existing_idx >= 0) {
        uci_multicast_entry* entry = &session->multicast_entries[existing_idx];
        entry->key_len = key_len;
        if (key_len > 0 && key) {
            memcpy(entry->key, key, key_len);
        } else {
            memset(entry->key, 0, sizeof(entry->key));
        }
        return UCI_STATUS_OK;
    }

    if (session->multicast_count >= MAX_MULTICAST_CONTROLEES) {
        return UCI_STATUS_MULTICAST_LIST_FULL;
    }

    uci_multicast_entry* entry = &session->multicast_entries[session->multicast_count++];
    entry->short_address = short_address;
    entry->subsession_id = subsession_id;
    entry->key_len = key_len;
    if (key_len > 0 && key) {
        memcpy(entry->key, key, key_len);
    } else {
        memset(entry->key, 0, sizeof(entry->key));
    }
    return UCI_STATUS_OK;
}

static unsigned char session_remove_multicast_entry(struct uci_session* session,
                                                    unsigned short short_address,
                                                    unsigned int subsession_id) {
    if (!session) {
        return UCI_STATUS_INVALID_PARAM;
    }

    int idx = session_find_multicast_entry_idx(session, short_address, subsession_id);
    if (idx < 0) {
        return UCI_STATUS_ADDRESS_NOT_FOUND;
    }

    int last = session->multicast_count - 1;
    if (idx != last) {
        session->multicast_entries[idx] = session->multicast_entries[last];
    }
    memset(&session->multicast_entries[last], 0, sizeof(session->multicast_entries[last]));
    session->multicast_count--;
    return UCI_STATUS_OK;
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
    if (!payload || payload_len < 24) {
        printf("  Error: SESSION_INFO_NTF payload too short. Need at least 24 bytes, got %d.\n", payload_len);
        return;
    }

    // Parse header fields according to Android UCI spec
    unsigned int sequence_number = read_u32_le(&payload[0]);
    unsigned int session_token = read_u32_le(&payload[4]);
    unsigned char rcr_indicator = payload[8];
    unsigned int current_ranging_interval = read_u32_le(&payload[9]);
    unsigned char ranging_measurement_type = payload[13];
    // unsigned char reserved1 = payload[14]; // Skip reserved byte at position 14
    unsigned char mac_address_indicator = payload[15];
    unsigned int hus_primary_session_id = read_u32_le(&payload[16]);
    // unsigned int reserved2 = read_u32_le(&payload[20]); // Skip reserved 4 bytes at position 20-23

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
    int offset = 24; // Header size (24 bytes): 4+4+1+4+1+1+1+4+4
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
            
            printf("    Measurement %d:\n", i + 1);

            if (mac_address_indicator == 0) { // SHORT_ADDRESS
                if (offset + 20 > payload_len) {
                    printf("      Error: Insufficient data for SHORT_ADDRESS measurement\n");
                    break;
                }
                print_short_address_measurement(&payload[offset]);
                offset += 20;
            } else { // EXTENDED_ADDRESS
                if (offset + 26 > payload_len) {
                    printf("      Error: Insufficient data for EXTENDED_ADDRESS measurement\n");
                    break;
                }
                print_extended_address_measurement(&payload[offset]);
                offset += 26;
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

    if (offset < payload_len) {
        printf("  Vendor-specific Range Data (%d bytes):\n", payload_len - offset);
        decode_range_vendor_data(&payload[offset], payload_len - offset);
    }
}

void parse_uci_packet(unsigned char* packet, size_t packet_len) {
    if (packet_len < sizeof(struct uci_packet_header)) {
        printf("Error: UCI packet too short to contain a header.\n");
        return;
    }

    struct uci_packet_header* header = (struct uci_packet_header*)packet;
    uci_header_fields_t header_fields;
    uci_extract_header_fields(header, &header_fields);

    size_t available_payload = packet_len - sizeof(struct uci_packet_header);
    size_t payload_len = header_fields.payload_length;
    if (payload_len > available_payload) {
        printf("  Warning: Header payload length %zu exceeds available data %zu. Clamping.\n",
               payload_len, available_payload);
        payload_len = available_payload;
    }

    printf("Received UCI packet:\n");
    printf("  MT: 0x%01X\n", header_fields.message_type);
    printf("  PBF: 0x%01X\n", header_fields.packet_boundary);
    printf("  GID: 0x%01X\n", header_fields.group_id);
    printf("  Opcode: 0x%02X\n", header_fields.opcode_id);
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

    unsigned char mt = header_fields.message_type;
    unsigned char gid = header_fields.group_id;
    unsigned char opcode = header_fields.opcode_id;

    if (mt == RESPONSE && gid == CORE && opcode == CORE_DEVICE_INFO) {
        handle_core_device_info_rsp(payload_ptr, payload_len_int);
    } else if (mt == RESPONSE && gid == CORE && opcode == CORE_DEVICE_SUSPEND) {
        handle_core_device_suspend_rsp(payload_ptr, payload_len_int);
    } else if (mt == NOTIFICATION && gid == CORE) {
        if (opcode == CORE_DEVICE_STATUS_NTF) {
            handle_core_device_status_ntf(payload_ptr, payload_len_int);
        } else if (opcode == CORE_GENERIC_ERROR_NTF) {
            handle_core_generic_error_ntf(payload_ptr, payload_len_int);
        } else {
            handle_generic_notification(gid, opcode, payload_ptr, payload_len_int);
        }
    } else if (mt == RESPONSE && gid == CORE && opcode == CORE_GET_CAPS_INFO) {
        handle_core_get_caps_info_rsp(payload_ptr, payload_len_int);
    } else if (mt == RESPONSE && gid == CORE && opcode == CORE_SET_CONFIG) {
        handle_core_set_config_rsp(payload_ptr, payload_len_int);
    } else if (mt == RESPONSE && gid == CORE && opcode == CORE_DEVICE_RESET) {
        handle_core_device_reset_rsp(payload_ptr, payload_len_int);
    } else if (mt == RESPONSE && gid == CORE && opcode == CORE_GET_CONFIG) {
        handle_core_get_config_rsp(payload_ptr, payload_len_int);
    } else if (mt == RESPONSE && gid == CORE && opcode == CORE_QUERY_UWBS_TIMESTAMP) {
        handle_core_query_uwbs_timestamp_rsp(payload_ptr, payload_len_int);
    } else if (mt == NOTIFICATION && gid == SESSION_CONFIG) {
        handle_session_config_ntf(opcode, payload_ptr, payload_len_int);
    } else if (mt == NOTIFICATION && gid == SESSION_CONTROL) {
        handle_session_control_ntf(opcode, payload_ptr, payload_len_int);
    } else if (mt == NOTIFICATION && gid == RANGING_DATA) {
        if (opcode == RANGE_DATA_NTF_OPCODE) {
            decode_range_data_ntf(payload_ptr, payload_len_int);
        } else {
            handle_generic_notification(gid, opcode, payload_ptr, payload_len_int);
        }
    } else if (mt == NOTIFICATION) {
        // Handle other notification types generically if not specifically handled
        handle_generic_notification(gid, opcode, payload_ptr, payload_len_int);
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
                case CCC_SUPPORTED_CHAPS_PER_SLOT: printf(" (CCC_CHAPS_PER_SLOT)\n"); break;
                case CCC_SUPPORTED_SYNC_CODES: printf(" (CCC_SYNC_CODES)\n"); break;
                case CCC_SUPPORTED_HOPPING_CONFIG_MODES_AND_SEQUENCES: printf(" (CCC_HOPPING_CONFIGS)\n"); break;
                case CCC_SUPPORTED_CHANNELS: printf(" (CCC_CHANNELS)\n"); break;
                case CCC_SUPPORTED_VERSIONS: printf(" (CCC_VERSIONS)\n"); break;
                case CCC_SUPPORTED_UWB_CONFIGS: printf(" (CCC_UWB_CONFIGS)\n"); break;
                case CCC_SUPPORTED_PULSE_SHAPE_COMBOS: printf(" (CCC_PULSE_SHAPES)\n"); break;
                case CCC_SUPPORTED_RAN_MULTIPLIER: printf(" (CCC_RAN_MULTIPLIER)\n"); break;
                case CCC_SUPPORTED_MAX_RANGING_SESSION_NUMBER: printf(" (CCC_MAX_RANGING_SESSIONS)\n"); break;
                case CCC_SUPPORTED_MIN_UWB_INITIATION_TIME_MS: printf(" (CCC_MIN_INIT_TIME_MS)\n"); break;
                case CCC_PRIORITIZED_CHANNEL_LIST: printf(" (CCC_PRIORITY_CHANNELS)\n"); break;
                case CCC_SUPPORTED_UWBS_MAX_PPM: printf(" (CCC_MAX_PPM)\n"); break;
                case ALIRO_SUPPORTED_MAC_MODES: printf(" (ALIRO_MAC_MODES)\n"); break;
                case RADAR_SUPPORT: printf(" (RADAR_SUPPORT)\n"); break;
                case SUPPORTED_POWER_STATS: printf(" (POWER_STATS)\n"); break;
                case SUPPORTED_AOA_RESULT_REQ_ANTENNA_INTERLEAVING: printf(" (AOA_REQ_ANTENNA_INTERLEAVE)\n"); break;
                case SUPPORTED_MIN_RANGING_INTERVAL_MS: printf(" (MIN_RANGING_INTERVAL_MS)\n"); break;
                case SUPPORTED_RANGE_DATA_NTF_CONFIG: printf(" (RANGE_DATA_NTF_CONFIG)\n"); break;
                case SUPPORTED_RSSI_REPORTING: printf(" (RSSI_REPORTING)\n"); break;
                case SUPPORTED_DIAGNOSTICS: printf(" (DIAGNOSTICS_SUPPORT)\n"); break;
                case SUPPORTED_MIN_SLOT_DURATION_RSTU: printf(" (MIN_SLOT_DURATION_RSTU)\n"); break;
                case SUPPORTED_MAX_RANGING_SESSION_NUMBER: printf(" (MAX_RANGING_SESSIONS)\n"); break;
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

void decode_core_query_uwbs_timestamp_rsp(unsigned char* payload, int payload_len) {
    printf("    CORE_QUERY_UWBS_TIMESTAMP_RSP - Query UWBS Timestamp Response\n");
    
    if (payload_len < 9) { // status (1) + timestamp (8)
        printf("      ERROR: Payload too short (%d bytes, need at least 9)\\n", payload_len);
        return;
    }
    
    unsigned char status = payload[0];
    unsigned long long timestamp = read_u64_le(&payload[1]);
    
    printf("      Status: 0x%02X", status);
    switch(status) {
        case UCI_STATUS_OK: 
            printf(" (OK)\\n");
            printf("      Timestamp: %llu\\n", timestamp);
            break;
        case UCI_STATUS_REJECTED: 
            printf(" (REJECTED)\\n");
            break;
        case UCI_STATUS_FAILED: 
            printf(" (FAILED)\\n");
            break;
        default: 
            printf(" (UNKNOWN)\\n");
            break;
    }
}

// SESSION_CONFIG Group Payload Decoders
void decode_session_init_cmd(unsigned char* payload, int payload_len) {
    printf("    SESSION_INIT_CMD - Session Initialization Command\n");

    if (payload_len < 5) {
        printf("      ERROR: Payload too short (%d bytes, need at least 5)\n", payload_len);
        return;
    }

    unsigned int session_id = read_u32_le(&payload[0]);
    unsigned char session_type = payload[4];

    printf("      Session ID: 0x%08X\n", session_id);
    printf("      Session Type: 0x%02X", session_type);
    switch(session_type) {
        case 0x00: printf(" (FIRA_RANGING_SESSION)\n"); break;
        case 0x01: printf(" (FIRA_RANGING_AND_IN_BAND_DATA_SESSION)\n"); break;
        case 0x02: printf(" (FIRA_DATA_TRANSFER_SESSION)\n"); break;
        case 0x03: printf(" (FIRA_RANGING_ONLY_PHASE)\n"); break;
        case 0x04: printf(" (FIRA_IN_BAND_DATA_PHASE)\n"); break;
        case 0x05: printf(" (FIRA_RANGING_WITH_DATA_PHASE)\n"); break;
        case 0xA0: printf(" (CCC_RANGING_SESSION)\n"); break;
        case 0xD0: printf(" (DEVICE_TEST_MODE)\n"); break;
        default: printf(" (UNKNOWN)\n"); break;
    }
}

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
            const char* cfg_name = get_app_config_name(cfg_id);
            printf("        Config ID: 0x%02X", cfg_id);
            if (cfg_name) {
                printf(" (%s)", cfg_name);
            } else {
                printf(" (UNKNOWN)");
            }
            printf("\n");
            
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
            const char* cfg_name = get_app_config_name(cfg_id);
            printf("        Config ID: 0x%02X", cfg_id);
            if (cfg_name) {
                printf(" (%s)", cfg_name);
            } else {
                printf(" (UNKNOWN)");
            }
            printf("\n");
            
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

void decode_range_data_ntf(unsigned char* payload, int payload_len) {
    printf("    RANGE_DATA_NTF - Range Data Notification\n");

    if (payload_len < 12) {
        printf("      ERROR: Payload too short (%d bytes, need at least 12)\n", payload_len);
        return;
    }

    unsigned int session_token = read_u32_le(&payload[0]);
    unsigned int sequence_number = read_u32_le(&payload[4]);
    unsigned int control_word = read_u32_le(&payload[8]);

    unsigned char status = control_word & 0xFF;
    unsigned char mac_indicator = (control_word >> 8) & 0xFF;
    unsigned char measurement_count = (control_word >> 16) & 0xFF;
    unsigned char vendor_flags = (control_word >> 24) & 0xFF;

    printf("      Session Token: 0x%08X\n", session_token);
    printf("      Sequence Number: %u\n", sequence_number);
    printf("      Status: 0x%02X", status);
    if (status == UCI_STATUS_OK) {
        printf(" (OK)");
    }
    printf("\n");
    printf("      MAC Indicator: 0x%02X (%s)\n",
           mac_indicator,
           mac_indicator ? "EXTENDED_ADDRESS" : "SHORT_ADDRESS");
    printf("      Measurement Count: %u\n", measurement_count);
    if (vendor_flags) {
        printf("      Vendor Flags: 0x%02X\n", vendor_flags);
    }

    int offset = 12;
    for (unsigned int i = 0; i < measurement_count; i++) {
        printf("      Measurement %u:\n", i + 1);
        if (mac_indicator == 0) {
            if (offset + 20 > payload_len) {
                printf("        WARNING: Incomplete short-address measurement (need 20 bytes, have %d).\n",
                       payload_len - offset);
                return;
            }
            print_short_address_measurement(&payload[offset]);
            offset += 20;
        } else {
            if (offset + 26 > payload_len) {
                printf("        WARNING: Incomplete extended-address measurement (need 26 bytes, have %d).\n",
                       payload_len - offset);
                return;
            }
            print_extended_address_measurement(&payload[offset]);
            offset += 26;
        }
    }

    if (offset < payload_len) {
        printf("      Vendor-specific Range Data (%d bytes):\n", payload_len - offset);
        decode_range_vendor_data(&payload[offset], payload_len - offset);
    }
}

void decode_android_range_diagnostics_ntf(unsigned char* payload, int payload_len) {
    printf("    ANDROID_FIRA_RANGE_DIAGNOSTICS_NTF - Range Diagnostics Notification\n");

    if (payload_len < 9) {
        printf("      ERROR: Payload too short (%d bytes, need at least 9)\n", payload_len);
        return;
    }

    uint32_t session_token = read_u32_le(&payload[0]);
    uint32_t sequence_number = read_u32_le(&payload[4]);
    unsigned char report_count = payload[8];

    printf("      Session Token: 0x%08X\n", session_token);
    printf("      Sequence Number: %u\n", sequence_number);
    printf("      Frame Reports: %u\n", report_count);

    int offset = 9;
    for (unsigned char report_idx = 0; report_idx < report_count; report_idx++) {
        if (offset + 4 > payload_len) {
            printf("      WARNING: Incomplete frame report header at index %u (need 4 bytes, have %d).\n",
                   report_idx, payload_len - offset);
            return;
        }

        unsigned char uwb_msg_id = payload[offset];
        unsigned char action = payload[offset + 1];
        unsigned char antenna_set = payload[offset + 2];
        unsigned char tlv_count = payload[offset + 3];
        offset += 4;

        printf("      Frame Report %u:\n", report_idx);
        printf("        UWB Message ID: 0x%02X\n", uwb_msg_id);
        printf("        Action: 0x%02X\n", action);
        printf("        Antenna Set: 0x%02X\n", antenna_set);
        printf("        TLV Count: %u\n", tlv_count);

        for (unsigned char tlv_idx = 0; tlv_idx < tlv_count; tlv_idx++) {
            if (offset + 3 > payload_len) {
                printf("        WARNING: Incomplete frame report TLV header at index %u (need 3 bytes, have %d).\n",
                       tlv_idx, payload_len - offset);
                return;
            }

            FrameReportTlvType tlv_type = (FrameReportTlvType)payload[offset];
            unsigned short tlv_len = read_u16_le(&payload[offset + 1]);
            offset += 3;

            if (offset + tlv_len > payload_len) {
                printf("        WARNING: TLV 0x%02X length %u exceeds remaining payload (%d).\n",
                       tlv_type, tlv_len, payload_len - offset);
                return;
            }

            decode_frame_report_tlv(tlv_type, &payload[offset], tlv_len);
            offset += tlv_len;
        }
    }

    if (offset < payload_len) {
        printf("      NOTE: %d trailing diagnostic bytes remain after parsing.\n", payload_len - offset);
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

    if (payload_len < 24) {
        printf("      ERROR: Payload too short (%d bytes, need at least 24 for header)\n", payload_len);
        return;
    }

    // Parse header fields according to Android UCI spec
    unsigned int sequence_number = read_u32_le(&payload[0]);
    unsigned int session_token = read_u32_le(&payload[4]);
    unsigned char rcr_indicator = payload[8];
    unsigned int current_ranging_interval = read_u32_le(&payload[9]);
    unsigned char ranging_measurement_type = payload[13];
    unsigned char reserved1 = payload[14];
    unsigned char mac_address_indicator = payload[15];
    unsigned int hus_primary_session_id = read_u32_le(&payload[16]);
    unsigned int reserved2 = read_u32_le(&payload[20]);

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

    if (reserved1 || reserved2) {
        printf("      Reserved: 0x%02X 0x%08X\n", reserved1, reserved2);
    }
    printf("      MAC Address Indicator: 0x%02X", mac_address_indicator);
    if (mac_address_indicator == 0x00) {
        printf(" (SHORT_ADDRESS)\n");
    } else {
        printf(" (EXTENDED_ADDRESS)\n");
    }

    printf("      HUS Primary Session ID: 0x%08X\n", hus_primary_session_id);

    // Parse ranging measurements
    int offset = 24; // Header size (24 bytes): 4+4+1+4+1+1+1+4+4
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
