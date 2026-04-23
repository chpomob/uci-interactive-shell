#include "../include/uci_config_metadata.h"

#include <string.h>
#include <strings.h>

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif

static const config_param_info_t app_config_params[] = {
    {DEVICE_TYPE, "device_type", "Device type (controller/controlee)", 0, 1, 1, 1, ""},
    {RANGING_ROUND_USAGE, "ranging_round_usage", "Ranging round usage", 1, 8, 1, 1, ""},
    {STS_CONFIG, "sts_config", "Secure timestamp configuration", 0, 4, 0, 1, ""},
    {MULTI_NODE_MODE, "multi_node_mode", "Multi-node mode", 0, 2, 1, 1, ""},
    {CHANNEL_NUMBER, "channel_number", "UWB channel number", 0, 15, 9, 1, ""},
    {NO_OF_CONTROLEE, "no_of_controlee", "Number of controlees", 0, 8, 0, 1, ""},
    {DEVICE_MAC_ADDRESS, "device_mac_address", "Device MAC address", 0, 0xFFFF, 0, 2, ""},
    {DST_MAC_ADDRESS, "dst_mac_address", "Destination MAC address list", 0, 0xFFFF, 0, 2, ""},
    {SLOT_DURATION, "slot_duration", "Slot duration in RSTU", 0, 0xFFFF, 2400, 2, "RSTU"},
    {RANGING_DURATION, "ranging_duration", "Ranging duration in milliseconds", 0, 0xFFFFFFFF, 1000, 4, "ms"},
    {STS_INDEX, "sts_index", "STS index", 0, 0xFFFFFFFF, 0, 4, ""},
    {MAC_FCS_TYPE, "mac_fcs_type", "MAC frame check sequence type", 0, 1, 0, 1, ""},
    {RANGING_ROUND_CONTROL, "ranging_round_control", "Ranging round control", 0, 0xFF, 0, 1, ""},
    {AOA_RESULT_REQ, "aoa_result_req", "Angle-of-arrival result request", 0, 3, 0, 1, ""},
    {RNG_DATA_NTF, "rng_data_ntf", "Ranging data notification", 0, 7, 0, 1, ""},
    {RNG_DATA_NTF_PROXIMITY_NEAR, "rng_data_ntf_proximity_near", "Ranging data notification proximity near", 0, 0xFFFF, 0, 2, "cm"},
    {RNG_DATA_NTF_PROXIMITY_FAR, "rng_data_ntf_proximity_far", "Ranging data notification proximity far", 0, 0xFFFF, 0, 2, "cm"},
    {DEVICE_ROLE, "device_role", "Device role", 0, 8, 1, 1, ""},
    {RFRAME_CONFIG, "rframe_config", "Ranging frame configuration", 0, 7, 0, 1, ""},
    {RSSI_REPORTING, "rssi_reporting", "RSSI reporting", 0, 1, 0, 1, ""},
    {PREAMBLE_CODE_INDEX, "preamble_code_index", "Preamble code index", 0, 32, 10, 1, ""},
    {SFD_ID, "sfd_id", "Start of frame delimiter ID", 0, 4, 2, 1, ""},
    {PSDU_DATA_RATE, "psdu_data_rate", "PSDU data rate", 0, 3, 0, 1, ""},
    {PREAMBLE_DURATION, "preamble_duration", "Preamble duration", 0, 1, 1, 1, ""},
    {LINK_LAYER_MODE, "link_layer_mode", "Link layer mode", 0, 1, 0, 1, ""},
    {DATA_REPETITION_COUNT, "data_repetition_count", "Data repetition count", 0, 63, 0, 1, ""},
    {RANGING_TIME_STRUCT, "ranging_time_struct", "Ranging time structure", 0, 1, 0, 1, ""},
    {SLOTS_PER_RR, "slots_per_rr", "Slots per ranging round", 1, 0xFF, 1, 1, ""},
    {TX_ADAPTIVE_PAYLOAD_POWER, "tx_adaptive_payload_power", "Transmit adaptive payload power", 0, 1, 0, 1, ""},
    {RNG_DATA_NTF_AOA_BOUND, "rng_data_ntf_aoa_bound", "Ranging data notification AoA bound", 0, 0xFFFF, 0, 2, "degrees"},
    {RESPONDER_SLOT_INDEX, "responder_slot_index", "Responder slot index", 0, 0xFF, 0, 1, ""},
    {PRF_MODE, "prf_mode", "Pulse repetition frequency mode", 0, 2, 0, 1, ""},
    {CAP_SIZE_RANGE, "cap_size_range", "Capability size range", 0, 0xFFFF, 0, 2, ""},
    {TX_JITTER_WINDOW_SIZE, "tx_jitter_window_size", "Transmit jitter window size", 0, 0xFFFF, 0, 2, "RSTU"},
    {SCHEDULED_MODE, "scheduled_mode", "Scheduled mode", 0, 2, 0, 1, ""},
    {KEY_ROTATION, "key_rotation", "Key rotation", 0, 1, 0, 1, ""},
    {KEY_ROTATION_RATE, "key_rotation_rate", "Key rotation rate", 0, 15, 0, 2, ""},
    {SESSION_PRIORITY, "session_priority", "Session priority", 0, 100, 50, 1, ""},
    {MAC_ADDRESS_MODE, "mac_address_mode", "MAC address mode", 0, 1, 0, 1, ""},
    {VENDOR_ID, "vendor_id", "Vendor ID", 0, 0xFFFF, 0, 2, ""},
    {STATIC_STS_IV, "static_sts_iv", "Static STS initialization vector", 0, 0xFFFFFFFFFFFFFFFFULL, 0, 8, ""},
    {NUMBER_OF_STS_SEGMENTS, "number_of_sts_segments", "Number of STS segments", 0, 4, 1, 1, ""},
    {MAX_RR_RETRY, "max_rr_retry", "Maximum ranging round retry", 0, 0xFFFF, 0, 2, ""},
    {UWB_INITIATION_TIME, "uwb_initiation_time", "UWB initiation time", 0, 0xFFFFFFFF, 0, 4, "ms"},
    {HOPPING_MODE, "hopping_mode", "Hopping mode", 0, 3, 0, 1, ""},
    {BLOCK_STRIDE_LENGTH, "block_stride_length", "Block stride length", 0, 0xFF, 0, 1, ""},
    {RESULT_REPORT_CONFIG, "result_report_config", "Result report configuration", 0, 0xFF, 0, 1, ""},
    {IN_BAND_TERMINATION_ATTEMPT_COUNT, "in_band_termination_attempt_count", "In-band termination attempt count", 0, 0xFF, 3, 1, ""},
    {BPRF_PHR_DATA_RATE, "bprf_phr_data_rate", "BPRF PHR data rate", 0, 1, 0, 1, ""},
    {MAX_NUMBER_OF_MEASUREMENTS, "max_number_of_measurements", "Maximum number of measurements", 0, 0xFFFF, 0, 2, ""},
    {UL_TDOA_TX_INTERVAL, "ul_tdoa_tx_interval", "UL-TDoA transmit interval", 0, 0xFFFFFFFF, 0, 4, ""},
    {UL_TDOA_RANDOM_WINDOW, "ul_tdoa_random_window", "UL-TDoA random window", 0, 0xFFFFFFFF, 0, 4, ""},
    {STS_LENGTH, "sts_length", "STS length", 0, 2, 1, 1, ""},
    {SUSPEND_RANGING_ROUNDS, "suspend_ranging_rounds", "Suspend ranging rounds", 0, 0xFF, 0, 1, ""},
    {UL_TDOA_NTF_REPORT_CONFIG, "ul_tdoa_ntf_report_config", "UL-TDoA notification report configuration", 0, 0xFFFFFF, 0, 3, ""},
    {UL_TDOA_DEVICE_ID, "ul_tdoa_device_id", "UL-TDoA device ID", 0, 0xFF, 0, 1, ""},
    {UL_TDOA_TX_TIMESTAMP, "ul_tdoa_tx_timestamp", "UL-TDoA transmit timestamp", 0, 0xFF, 0, 1, ""},
    {MIN_FRAMES_PER_RR, "min_frames_per_rr", "Minimum frames per ranging round", 0, 0xFF, 1, 1, ""},
    {MTU_SIZE, "mtu_size", "Maximum transfer unit size", 0, 0xFFFF, 512, 2, ""},
    {INTER_FRAME_INTERVAL, "inter_frame_interval", "Inter-frame interval", 0, 0xFF, 1, 1, ""},
    {DL_TDOA_RANGING_METHOD, "dl_tdoa_ranging_method", "DL-TDoA ranging method", 0, 1, 0, 1, ""},
    {DL_TDOA_TX_TIMESTAMP_CONF, "dl_tdoa_tx_timestamp_conf", "DL-TDoA transmit timestamp configuration", 0, 0xFF, 0, 1, ""},
    {DL_TDOA_HOP_COUNT, "dl_tdoa_hop_count", "DL-TDoA hop count", 0, 0xFF, 0, 1, ""},
    {DL_TDOA_ANCHOR_CFO, "dl_tdoa_anchor_cfo", "DL-TDoA anchor CFO", 0, 0xFF, 0, 1, ""},
    {DL_TDOA_ANCHOR_LOCATION, "dl_tdoa_anchor_location", "DL-TDoA anchor location", 0, 0xFF, 0, 1, ""},
    {DL_TDOA_TX_ACTIVE_RANGING_ROUNDS, "dl_tdoa_tx_active_ranging_rounds", "DL-TDoA transmit active ranging rounds", 0, 0xFF, 0, 1, ""},
    {DL_TDOA_BLOCK_STRIDING, "dl_tdoa_block_striding", "DL-TDoA block striding", 0, 0xFF, 0, 1, ""},
    {DL_TDOA_TIME_REFERENCE_ANCHOR, "dl_tdoa_time_reference_anchor", "DL-TDoA time reference anchor", 0, 0xFF, 0, 1, ""},
    {SESSION_KEY, "session_key", "Session key for provisioned STS", 0, 0, 0, 16, ""},
    {SUBSESSION_KEY, "subsession_key", "Sub-session key for provisioned STS", 0, 0, 0, 16, ""},
    {SESSION_DATA_TRANSFER_STATUS_NTF_CONFIG, "session_data_transfer_status_ntf_config", "Session data transfer status notification configuration", 0, 0xFF, 0, 1, ""},
    {SESSION_TIME_BASE, "session_time_base", "Session time base structure", 0, 0, 0, 9, ""},
    {DL_TDOA_RESPONDER_TOF, "dl_tdoa_responder_tof", "DL-TDoA responder ToF", 0, 0xFF, 0, 1, ""},
    {SECURE_RANGING_NEFA_LEVEL, "secure_ranging_nefa_level", "Secure ranging NEFA level", 0, 0xFF, 0, 1, ""},
    {SECURE_RANGING_CSW_LENGTH, "secure_ranging_csw_length", "Secure ranging CSW length", 0, 0xFF, 0, 1, ""},
    {APPLICATION_DATA_ENDPOINT, "application_data_endpoint", "Application data endpoint", 0, 0xFF, 0, 1, ""},
    {OWR_AOA_MEASUREMENT_NTF_PERIOD, "owr_aoa_measurement_ntf_period", "OWR AoA measurement notification period", 0, 0xFF, 0, 1, ""},
    {SUB_SESSION_ID, "sub_session_id", "Sub-session ID", 0, 0xFFFFFFFF, 0, 4, ""},
};

typedef struct {
    AppConfigTlvType cfg_id;
    const char* alias;
} app_param_alias_t;

static const app_param_alias_t app_param_aliases[] = {
    {RANGING_ROUND_USAGE, "ranging_usage"},
    {CHANNEL_NUMBER, "channel"},
    {AOA_RESULT_REQ, "aoa_request"},
    {UWB_INITIATION_TIME, "initiation_time"},
};

typedef struct {
    AppConfigTlvType cfg_id;
    const char* token;
    uint64_t value;
} app_value_mapping_t;

static const app_value_mapping_t app_value_mappings[] = {
    {DEVICE_TYPE, "controlee", 0x00},
    {DEVICE_TYPE, "controller", 0x01},
    {RANGING_ROUND_USAGE, "ss_deferred", 0x01},
    {RANGING_ROUND_USAGE, "ss-deferred", 0x01},
    {RANGING_ROUND_USAGE, "ds_deferred", 0x02},
    {RANGING_ROUND_USAGE, "ds-deferred", 0x02},
    {RANGING_ROUND_USAGE, "ss_non_deferred", 0x03},
    {RANGING_ROUND_USAGE, "ss-non-deferred", 0x03},
    {RANGING_ROUND_USAGE, "ss", 0x03},
    {RANGING_ROUND_USAGE, "ds_non_deferred", 0x04},
    {RANGING_ROUND_USAGE, "ds-non-deferred", 0x04},
    {RANGING_ROUND_USAGE, "ds", 0x04},
    {RANGING_ROUND_USAGE, "owr_dl_tdoa", 0x05},
    {RANGING_ROUND_USAGE, "owr-dl-tdoa", 0x05},
    {RANGING_ROUND_USAGE, "dl_tdoa", 0x05},
    {RANGING_ROUND_USAGE, "owr_aoa", 0x06},
    {RANGING_ROUND_USAGE, "owr-aoa", 0x06},
    {RANGING_ROUND_USAGE, "ess_twr", 0x07},
    {RANGING_ROUND_USAGE, "ess-twr", 0x07},
    {RANGING_ROUND_USAGE, "ads_twr", 0x08},
    {RANGING_ROUND_USAGE, "ads-twr", 0x08},
    {STS_CONFIG, "static", 0x00},
    {STS_CONFIG, "dynamic", 0x01},
    {STS_CONFIG, "dynamic_responder_subsession", 0x02},
    {STS_CONFIG, "dynamic-responder-subsession", 0x02},
    {STS_CONFIG, "dynamic_rsk", 0x02},
    {STS_CONFIG, "provisioned", 0x03},
    {STS_CONFIG, "provisioned_responder_subsession", 0x04},
    {STS_CONFIG, "provisioned-responder-subsession", 0x04},
    {STS_CONFIG, "provisioned_rsk", 0x04},
    {MULTI_NODE_MODE, "unicast", 0x00},
    {MULTI_NODE_MODE, "one_to_many", 0x01},
    {MULTI_NODE_MODE, "one-to-many", 0x01},
    {MULTI_NODE_MODE, "many_to_many", 0x02},
    {MULTI_NODE_MODE, "many-to-many", 0x02},
    {MULTI_NODE_MODE, "anycast", 0x01},
    {MULTI_NODE_MODE, "multicast", 0x02},
    {MAC_ADDRESS_MODE, "short", 0x00},
    {MAC_ADDRESS_MODE, "two_byte", 0x00},
    {MAC_ADDRESS_MODE, "two-byte", 0x00},
    {MAC_ADDRESS_MODE, "extended", 0x01},
    {MAC_ADDRESS_MODE, "eight_byte", 0x01},
    {MAC_ADDRESS_MODE, "eight-byte", 0x01},
    {PRF_MODE, "bprf", 0x00},
    {PRF_MODE, "hprf", 0x01},
    {PRF_MODE, "hprf_124_8", 0x01},
    {PRF_MODE, "hprf-124-8", 0x01},
    {PRF_MODE, "hprf_249_6", 0x02},
    {PRF_MODE, "hprf-249-6", 0x02},
    {DEVICE_ROLE, "responder", 0x00},
    {DEVICE_ROLE, "initiator", 0x01},
    {DEVICE_ROLE, "advertiser", 0x05},
    {DEVICE_ROLE, "observer", 0x06},
    {DEVICE_ROLE, "dt_anchor", 0x07},
    {DEVICE_ROLE, "dt-anchor", 0x07},
    {DEVICE_ROLE, "dt_tag", 0x08},
    {DEVICE_ROLE, "dt-tag", 0x08},
    {AOA_RESULT_REQ, "disable", 0x00},
    {AOA_RESULT_REQ, "off", 0x00},
    {AOA_RESULT_REQ, "enable", 0x01},
    {AOA_RESULT_REQ, "on", 0x01},
    {DL_TDOA_RANGING_METHOD, "ss_twr", 0x00},
    {DL_TDOA_RANGING_METHOD, "ds_twr", 0x01},
    {SCHEDULED_MODE, "cont", 0x00},
    {SCHEDULED_MODE, "contention", 0x00},
    {SCHEDULED_MODE, "contention_based", 0x00},
    {SCHEDULED_MODE, "contention-based", 0x00},
    {SCHEDULED_MODE, "continuous", 0x00},
    {SCHEDULED_MODE, "scheduled", 0x01},
    {SCHEDULED_MODE, "time_scheduled", 0x01},
    {SCHEDULED_MODE, "time-scheduled", 0x01},
    {SCHEDULED_MODE, "hybrid", 0x02},
};

static const device_config_param_info_t device_config_params[] = {
    {DEVICE_STATE, "device_state", "Device state", 0, 3, DEVICE_STATE_READY, 1, ""},
    {LOW_POWER_MODE, "low_power_mode", "Low power mode", 0, 1, 0, 1, ""},
    {DEVICE_CHANNEL, "device_channel", "Default operating channel", 0, 0xFF, 0, 1, ""},
    {DEVICE_PREAMBLE_CODE, "device_preamble_code", "Default preamble code index", 0, 0xFF, 0, 1, ""},
    {DEVICE_PAN_ID, "device_pan_id", "Default PAN identifier", 0, 0xFFFF, 0, 2, ""},
    {DEVICE_SHORT_ADDR, "device_short_addr", "Device short address", 0, 0xFFFF, 0, 2, ""},
    {DEVICE_EXTENDED_ADDR, "device_extended_addr", "Device extended address", 0, 0xFFFFFFFFFFFFFFFFULL, 0, 8, ""},
    {DEVICE_PAN_COORD, "device_pan_coord", "PAN coordinator flag", 0, 1, 0, 1, ""},
    {DEVICE_PROMISCUOUS, "device_promiscuous", "Promiscuous mode flag", 0, 1, 0, 1, ""},
    {DEVICE_FRAME_RETRIES, "device_frame_retries", "Max MAC frame retries", 0, 0xFF, 0, 1, ""},
    {DEVICE_TRACES, "device_traces", "Trace enable bitmask", 0, 0xFFFFFFFF, 0, 4, ""},
    {DEVICE_PM_MIN_INACTIVITY_S4, "device_pm_min_inactivity_s4", "Minimum inactivity before S4", 0, 0xFFFFFFFF, 0, 4, "ms"},
};

const config_param_info_t* uci_config_metadata_find_app_param(AppConfigTlvType cfg_id) {
    for (size_t i = 0; i < ARRAY_SIZE(app_config_params); i++) {
        if (app_config_params[i].cfg_id == cfg_id) {
            return &app_config_params[i];
        }
    }

    return NULL;
}

const config_param_info_t* uci_config_metadata_find_app_param_by_name(const char* name) {
    if (!name) {
        return NULL;
    }

    for (size_t i = 0; i < ARRAY_SIZE(app_config_params); i++) {
        if (strcasecmp(app_config_params[i].name, name) == 0) {
            return &app_config_params[i];
        }
    }

    return NULL;
}

size_t uci_config_metadata_get_app_param_count(void) {
    return ARRAY_SIZE(app_config_params);
}

const config_param_info_t* uci_config_metadata_get_app_param_info_at(size_t index) {
    return index < ARRAY_SIZE(app_config_params) ? &app_config_params[index] : NULL;
}

int uci_config_metadata_lookup_app_param_alias(const char* name, AppConfigTlvType* cfg_id) {
    if (!name || !cfg_id) {
        return -1;
    }

    for (size_t i = 0; i < ARRAY_SIZE(app_param_aliases); i++) {
        if (strcasecmp(app_param_aliases[i].alias, name) == 0) {
            *cfg_id = app_param_aliases[i].cfg_id;
            return 0;
        }
    }

    return -1;
}

int uci_config_metadata_lookup_app_value(AppConfigTlvType cfg_id, const char* token, uint64_t* value) {
    if (!token || !value) {
        return -1;
    }

    for (size_t i = 0; i < ARRAY_SIZE(app_value_mappings); i++) {
        if (app_value_mappings[i].cfg_id == cfg_id &&
            strcasecmp(app_value_mappings[i].token, token) == 0) {
            *value = app_value_mappings[i].value;
            return 0;
        }
    }

    return -1;
}

const device_config_param_info_t* uci_config_metadata_find_device_param(DeviceConfigId cfg_id) {
    for (size_t i = 0; i < ARRAY_SIZE(device_config_params); i++) {
        if (device_config_params[i].cfg_id == cfg_id) {
            return &device_config_params[i];
        }
    }

    return NULL;
}

const device_config_param_info_t* uci_config_metadata_find_device_param_by_name(const char* name) {
    if (!name) {
        return NULL;
    }

    for (size_t i = 0; i < ARRAY_SIZE(device_config_params); i++) {
        if (strcasecmp(device_config_params[i].name, name) == 0) {
            return &device_config_params[i];
        }
    }

    return NULL;
}

size_t uci_config_metadata_get_device_param_count(void) {
    return ARRAY_SIZE(device_config_params);
}

const device_config_param_info_t* uci_config_metadata_get_device_param_info_at(size_t index) {
    return index < ARRAY_SIZE(device_config_params) ? &device_config_params[index] : NULL;
}
