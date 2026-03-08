#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "../include/uci.h"
#include "../include/uci_functions.h"
#include "../include/uci_config_manager.h"
#include "../include/uci_packet_utils.h"

#define MAX_RESPONSE_PAYLOAD_LEN 255

enum {
    UCI_DEVICE_INFO_UCI_VERSION = 0x0100,
    UCI_DEVICE_INFO_MAC_VERSION = 0x0200,
    UCI_DEVICE_INFO_PHY_VERSION = 0x0200,
    UCI_DEVICE_INFO_TEST_VERSION = 0x0100,
    UCI_DEVICE_INFO_VENDOR_SPEC_COUNT = 0x00,
};

// External globals from uci.c
extern unsigned long long g_fake_timestamp;

// Helper functions from other modules
extern int is_valid_device_config_id(unsigned char cfg_id);
extern void enqueue_notification(unsigned char gid, unsigned char oid, unsigned char* payload, int payload_len);

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

static const size_t kDefaultCapabilityTlvsCount =
    sizeof(kDefaultCapabilityTlvs) / sizeof(kDefaultCapabilityTlvs[0]);

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

int build_core_device_info_response(unsigned char* response_payload, size_t max_len) {
    struct uci_payload_builder builder;
    uci_payload_builder_init(&builder, response_payload, max_len);

    if (uci_payload_builder_put_u8(&builder, UCI_STATUS_OK) < 0 ||
        uci_payload_builder_put_u16_le(&builder, UCI_DEVICE_INFO_UCI_VERSION) < 0 ||
        uci_payload_builder_put_u16_le(&builder, UCI_DEVICE_INFO_MAC_VERSION) < 0 ||
        uci_payload_builder_put_u16_le(&builder, UCI_DEVICE_INFO_PHY_VERSION) < 0 ||
        uci_payload_builder_put_u16_le(&builder, UCI_DEVICE_INFO_TEST_VERSION) < 0 ||
        uci_payload_builder_put_u8(&builder, UCI_DEVICE_INFO_VENDOR_SPEC_COUNT) < 0) {
        return 0;
    }

    return (int)uci_payload_builder_length(&builder);
}

int build_core_get_caps_info_response(unsigned char* response_payload, size_t max_len) {
    size_t caps_payload_len = uci_build_core_capabilities_payload(response_payload, max_len);

    if (caps_payload_len == 0) {
        // Fallback to simple status to avoid emitting a malformed packet
        response_payload[0] = UCI_STATUS_FAILED;
        return 1;
    }

    return (int)caps_payload_len;
}

int build_core_query_timestamp_response(unsigned char* response_payload, size_t max_len) {
    struct uci_payload_builder builder;
    uci_payload_builder_init(&builder, response_payload, max_len);

    if (uci_payload_builder_put_u8(&builder, UCI_STATUS_OK) < 0 ||
        uci_payload_builder_put_u64_le(&builder, g_fake_timestamp++) < 0) {
        return 0;
    }

    return (int)uci_payload_builder_length(&builder);
}

int build_core_get_config_response(unsigned char* response_payload, size_t max_len,
                                   const unsigned char* payload, int payload_len) {
    struct uci_payload_builder builder;
    uci_payload_builder_init(&builder, response_payload, max_len);

    if (uci_payload_builder_put_u8(&builder, UCI_STATUS_OK) < 0 ||
        uci_payload_builder_put_u8(&builder, 0x00) < 0) {
        return 0;
    }

    unsigned char cfg_count = 0;
    unsigned char status = UCI_STATUS_OK;

    if (!payload || payload_len < 1) {
        response_payload[0] = UCI_STATUS_INVALID_PARAM;
        response_payload[1] = 0;
        return 2;
    }

    unsigned char requested = payload[0];
    unsigned int available_ids = payload_len > 1 ? (payload_len - 1) : 0;
    if (requested > available_ids) {
        requested = (unsigned char)available_ids;
    }

    for (unsigned char i = 0; i < requested; i++) {
        unsigned char cfg_id = payload[1 + i];
        if (!is_valid_device_config_id(cfg_id)) {
            status = UCI_STATUS_INVALID_PARAM;
            continue;
        }

        unsigned char value_buffer[256] = {0};
        size_t value_len = sizeof(value_buffer);
        if (uci_config_get_device_param((DeviceConfigId)cfg_id, value_buffer, &value_len) != 0 ||
            value_len == 0 || value_len > 255) {
            status = UCI_STATUS_INVALID_PARAM;
            continue;
        }

        if (uci_payload_builder_put_u8(&builder, cfg_id) < 0 ||
            uci_payload_builder_put_u8(&builder, (unsigned char)value_len) < 0 ||
            uci_payload_builder_put_mem(&builder, value_buffer, value_len) < 0) {
            status = UCI_STATUS_INVALID_MSG_SIZE;
            break;
        }
        cfg_count++;
    }

    if (cfg_count == 0 && status == UCI_STATUS_OK) {
        status = UCI_STATUS_INVALID_PARAM;
    }

    response_payload[0] = (cfg_count > 0) ? status : UCI_STATUS_INVALID_PARAM;
    response_payload[1] = cfg_count;

    if (uci_payload_builder_length(&builder) == 2) {
        unsigned char err = response_payload[0];
        enqueue_notification(CORE, CORE_GENERIC_ERROR_NTF, &err, 1);
    }

    return (int)uci_payload_builder_length(&builder);
}

int build_core_set_config_response(unsigned char* response_payload, size_t max_len,
                                   const unsigned char* payload, int payload_len) {
    unsigned char cfg_ids[MAX_RESPONSE_PAYLOAD_LEN] = {0};
    unsigned char cfg_status[MAX_RESPONSE_PAYLOAD_LEN] = {0};
    unsigned char processed_tlvs = 0;
    unsigned char status = UCI_STATUS_OK;

    if (!payload || payload_len < 1) {
        response_payload[0] = UCI_STATUS_INVALID_PARAM;
        response_payload[1] = 0;
        return 2;
    }

    unsigned char declared_tlvs = payload[0];
    const unsigned char *tlv_buffer = (payload_len > 1) ? &payload[1] : NULL;
    size_t tlv_length = (payload_len > 1) ? (size_t)(payload_len - 1) : 0;
    struct uci_tlv_reader reader;
    uci_tlv_reader_init(&reader, tlv_buffer, tlv_length);

    for (unsigned char i = 0; i < declared_tlvs; i++) {
        unsigned char cfg_id = 0;
        unsigned char len = 0;
        const unsigned char *value_ptr = NULL;

        int res = uci_tlv_reader_next(&reader, &cfg_id, &value_ptr, &len);
        if (res <= 0) {
            cfg_status[i] = UCI_STATUS_INVALID_MSG_SIZE;
            status = UCI_STATUS_INVALID_MSG_SIZE;
            break;
        }

        cfg_ids[i] = cfg_id;

        if (!is_valid_device_config_id(cfg_id)) {
            cfg_status[i] = UCI_STATUS_INVALID_PARAM;
            status = UCI_STATUS_INVALID_PARAM;
            processed_tlvs++;
            continue;
        }

        if (uci_config_set_device_param((DeviceConfigId)cfg_id, value_ptr, len) != 0) {
            cfg_status[i] = UCI_STATUS_FAILED;
            status = UCI_STATUS_FAILED;
        } else {
            cfg_status[i] = UCI_STATUS_OK;
        }

        processed_tlvs++;
    }

    if (status == UCI_STATUS_OK) {
        unsigned char tmp_type;
        unsigned char tmp_len;
        if (uci_tlv_reader_next(&reader, &tmp_type, NULL, &tmp_len) > 0) {
            status = UCI_STATUS_INVALID_PARAM;
        }
    }

    struct uci_payload_builder builder;
    uci_payload_builder_init(&builder, response_payload, max_len);

    if (uci_payload_builder_put_u8(&builder, status) < 0 ||
        uci_payload_builder_put_u8(&builder, processed_tlvs) < 0) {
        return 0;
    }

    for (unsigned char i = 0; i < processed_tlvs; i++) {
        if (uci_payload_builder_put_u8(&builder, cfg_ids[i]) < 0 ||
            uci_payload_builder_put_u8(&builder, cfg_status[i]) < 0) {
            return 0;
        }
    }

    response_payload[0] = status;
    response_payload[1] = processed_tlvs;

    return (int)uci_payload_builder_length(&builder);
}

int build_core_device_reset_response(unsigned char* response_payload, size_t max_len) {
    struct uci_payload_builder builder;
    uci_payload_builder_init(&builder, response_payload, max_len);
    if (uci_payload_builder_put_u8(&builder, UCI_STATUS_OK) < 0) {
        return 0;
    }
    return (int)uci_payload_builder_length(&builder);
}

int build_core_device_suspend_response(unsigned char* response_payload, size_t max_len) {
    struct uci_payload_builder builder;
    uci_payload_builder_init(&builder, response_payload, max_len);
    if (uci_payload_builder_put_u8(&builder, UCI_STATUS_OK) < 0) {
        return 0;
    }
    return (int)uci_payload_builder_length(&builder);
}
