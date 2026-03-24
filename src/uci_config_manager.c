#include "../include/uci_config_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>  // For strcasecmp
#include <stdint.h>
#include <errno.h>
#include <ctype.h>

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif

// Application configuration parameter information
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
    {RANGING_TIME_STRUCT, "ranging_time_struct", "Ranging time structure", 0, 3, 0, 1, ""},
    {SLOTS_PER_RR, "slots_per_rr", "Slots per ranging round", 0, 0xFF, 1, 1, ""},
    {TX_ADAPTIVE_PAYLOAD_POWER, "tx_adaptive_payload_power", "Transmit adaptive payload power", 0, 1, 0, 1, ""},
    {RNG_DATA_NTF_AOA_BOUND, "rng_data_ntf_aoa_bound", "Ranging data notification AoA bound", 0, 0xFFFF, 0, 2, "degrees"},
    {RESPONDER_SLOT_INDEX, "responder_slot_index", "Responder slot index", 0, 0xFF, 0, 1, ""},
    {PRF_MODE, "prf_mode", "Pulse repetition frequency mode", 0, 2, 0, 1, ""},
    {CAP_SIZE_RANGE, "cap_size_range", "Capability size range", 0, 0xFFFF, 0, 2, ""},
    {TX_JITTER_WINDOW_SIZE, "tx_jitter_window_size", "Transmit jitter window size", 0, 0xFFFF, 0, 2, "RSTU"},
    {SCHEDULED_MODE, "scheduled_mode", "Scheduled mode", 0, 1, 0, 1, ""},
    {KEY_ROTATION, "key_rotation", "Key rotation", 0, 1, 0, 1, ""},
    {KEY_ROTATION_RATE, "key_rotation_rate", "Key rotation rate", 0, 0xFFFF, 0, 2, ""},
    {SESSION_PRIORITY, "session_priority", "Session priority", 0, 100, 50, 1, ""},
    {MAC_ADDRESS_MODE, "mac_address_mode", "MAC address mode", 0, 1, 0, 1, ""},
    {VENDOR_ID, "vendor_id", "Vendor ID", 0, 0xFFFF, 0, 2, ""},
    {STATIC_STS_IV, "static_sts_iv", "Static STS initialization vector", 0, 0xFFFFFFFFFFFFFFFFULL, 0, 8, ""},
    {NUMBER_OF_STS_SEGMENTS, "number_of_sts_segments", "Number of STS segments", 0, 0xFF, 1, 1, ""},
    {MAX_RR_RETRY, "max_rr_retry", "Maximum ranging round retry", 0, 0xFF, 3, 1, ""},
    {UWB_INITIATION_TIME, "uwb_initiation_time", "UWB initiation time", 0, 0xFFFFFFFF, 0, 4, "ms"},
    {HOPPING_MODE, "hopping_mode", "Hopping mode", 0, 3, 0, 1, ""},
    {BLOCK_STRIDE_LENGTH, "block_stride_length", "Block stride length", 0, 0xFFFF, 0, 1, ""},
    {RESULT_REPORT_CONFIG, "result_report_config", "Result report configuration", 0, 0xFF, 0, 1, ""},
    {IN_BAND_TERMINATION_ATTEMPT_COUNT, "in_band_termination_attempt_count", "In-band termination attempt count", 0, 0xFF, 3, 1, ""},
    {BPRF_PHR_DATA_RATE, "bprf_phr_data_rate", "BPRF PHR data rate", 0, 1, 0, 1, ""},
    {MAX_NUMBER_OF_MEASUREMENTS, "max_number_of_measurements", "Maximum number of measurements", 0, 0xFFFF, 0, 2, ""},
    {UL_TDOA_TX_INTERVAL, "ul_tdoa_tx_interval", "UL-TDoA transmit interval", 0, 0xFFFFFFFF, 0, 4, ""},
    {UL_TDOA_RANDOM_WINDOW, "ul_tdoa_random_window", "UL-TDoA random window", 0, 0xFFFFFFFF, 0, 4, ""},
    {STS_LENGTH, "sts_length", "STS length", 0, 0xFF, 1, 1, ""},
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
    {SCHEDULED_MODE, "continuous", 0x00},
    {SCHEDULED_MODE, "scheduled", 0x01},
};

static const config_param_info_t* find_app_config_info(AppConfigTlvType cfg_id) {
    size_t num_params = sizeof(app_config_params) / sizeof(app_config_params[0]);
    for (size_t i = 0; i < num_params; i++) {
        if (app_config_params[i].cfg_id == cfg_id) {
            return &app_config_params[i];
        }
    }
    return NULL;
}

static const config_param_info_t* find_app_config_info_by_name(const char* name) {
    if (!name) {
        return NULL;
    }

    size_t num_params = sizeof(app_config_params) / sizeof(app_config_params[0]);
    for (size_t i = 0; i < num_params; i++) {
        if (strcasecmp(app_config_params[i].name, name) == 0) {
            return &app_config_params[i];
        }
    }

    return NULL;
}

// Device configuration parameter information
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

static const device_config_param_info_t* find_device_config_info(DeviceConfigId cfg_id) {
    size_t num_params = sizeof(device_config_params) / sizeof(device_config_params[0]);
    for (size_t i = 0; i < num_params; i++) {
        if (device_config_params[i].cfg_id == cfg_id) {
            return &device_config_params[i];
        }
    }
    return NULL;
}

static const device_config_param_info_t* find_device_config_info_by_name(const char* name) {
    if (!name) {
        return NULL;
    }

    size_t num_params = sizeof(device_config_params) / sizeof(device_config_params[0]);
    for (size_t i = 0; i < num_params; i++) {
        if (strcasecmp(device_config_params[i].name, name) == 0) {
            return &device_config_params[i];
        }
    }

    return NULL;
}

static int parse_unsigned_value(const char* value_str, uint64_t* out_value) {
    if (!value_str || !out_value) {
        return -1;
    }

    errno = 0;
    char* endptr = NULL;
    unsigned long long parsed = strtoull(value_str, &endptr, 0);
    if (errno != 0 || !value_str[0] || (endptr && *endptr != '\0')) {
        return -1;
    }

    *out_value = (uint64_t)parsed;
    return 0;
}

static int parse_boolean_value(const char* value_str, unsigned char* out_value) {
    if (!value_str || !out_value) {
        return -1;
    }

    if (strcasecmp(value_str, "on") == 0 ||
        strcasecmp(value_str, "enable") == 0 ||
        strcasecmp(value_str, "enabled") == 0 ||
        strcasecmp(value_str, "true") == 0 ||
        strcasecmp(value_str, "yes") == 0) {
        *out_value = 1;
        return 0;
    }

    if (strcasecmp(value_str, "off") == 0 ||
        strcasecmp(value_str, "disable") == 0 ||
        strcasecmp(value_str, "disabled") == 0 ||
        strcasecmp(value_str, "false") == 0 ||
        strcasecmp(value_str, "no") == 0) {
        *out_value = 0;
        return 0;
    }

    uint64_t numeric = 0;
    if (parse_unsigned_value(value_str, &numeric) == 0 && numeric <= 1) {
        *out_value = (unsigned char)numeric;
        return 0;
    }

    return -1;
}

static void trim_ascii_whitespace(char** start, char** end) {
    while (*start < *end && isspace((unsigned char)**start)) {
        (*start)++;
    }
    while (*end > *start && isspace((unsigned char)*((*end) - 1))) {
        (*end)--;
    }
}

static int parse_dst_mac_address_list(const char* value_str, unsigned char* value, size_t* value_len) {
    char buffer[256];
    char* cursor = NULL;
    char* token = NULL;
    size_t count = 0;

    if (!value_str || !value || !value_len) {
        return -1;
    }
    if (strlen(value_str) >= sizeof(buffer)) {
        return -1;
    }

    strcpy(buffer, value_str);
    cursor = buffer;
    while ((token = strsep(&cursor, ",")) != NULL) {
        char* start = token;
        char* end = token + strlen(token);
        uint64_t numeric = 0;

        trim_ascii_whitespace(&start, &end);
        if (start == end) {
            return -1;
        }

        *end = '\0';
        if (parse_unsigned_value(start, &numeric) != 0 || numeric > 0xFFFFU) {
            return -1;
        }
        if ((count + 1U) * 2U > *value_len) {
            return -1;
        }

        value[count * 2U] = (unsigned char)(numeric & 0xFFU);
        value[count * 2U + 1U] = (unsigned char)((numeric >> 8) & 0xFFU);
        count++;
    }

    if (count == 0U) {
        return -1;
    }

    *value_len = count * 2U;
    return 0;
}


// Configuration storage arrays
static unsigned char app_config_values[256][256];  // Up to 256 config params with up to 256 bytes each
static size_t app_config_lengths[256] = {0};       // Length of each config param
static unsigned char device_config_values[256][256]; // Up to 256 device config params with up to 256 bytes each
static size_t device_config_lengths[256] = {0};     // Length of each device config param

// Verbose mode flag
static int g_verbose_mode = 0;

// Initialize configuration manager
int uci_config_init() {
    // Initialize all configuration values to defaults
    size_t num_app_params = uci_config_get_app_param_count();
    for (size_t i = 0; i < num_app_params; i++) {
        const config_param_info_t* param = uci_config_get_app_param_info_at(i);
        if (!param) {
            continue;
        }
        size_t value_len = param->value_len > 0 ? param->value_len : 1;
        if (value_len > sizeof(app_config_values[param->cfg_id])) {
            value_len = sizeof(app_config_values[param->cfg_id]);
        }
        memset(app_config_values[param->cfg_id], 0, value_len);
        app_config_lengths[param->cfg_id] = value_len;
        for (size_t byte = 0; byte < value_len && byte < sizeof(param->default_value); byte++) {
            app_config_values[param->cfg_id][byte] =
                (unsigned char)((param->default_value >> (8 * byte)) & 0xFF);
        }
    }
    
    // Initialize device configuration values to defaults
    size_t num_device_params = uci_config_get_device_param_count();
    for (size_t i = 0; i < num_device_params; i++) {
        const device_config_param_info_t* param = uci_config_get_device_param_info_at(i);
        if (!param) {
            continue;
        }
        size_t value_len = param->value_len > 0 ? param->value_len : 1;
        if (value_len > sizeof(device_config_values[param->cfg_id])) {
            value_len = sizeof(device_config_values[param->cfg_id]);
        }
        device_config_lengths[param->cfg_id] = value_len;
        for (size_t byte = 0; byte < value_len; byte++) {
            device_config_values[param->cfg_id][byte] =
                (unsigned char)((param->default_value >> (8 * byte)) & 0xFF);
        }
    }
    
    if (g_verbose_mode) {
        printf("UCI configuration manager initialized with %zu app params and %zu device params\n", 
               num_app_params, num_device_params);
    }
    
    return 0;
}

// Cleanup configuration manager
void uci_config_cleanup() {
    // Nothing to do for now
    if (g_verbose_mode) {
        printf("UCI configuration manager cleaned up\n");
    }
}

// Enable/disable verbose mode
void uci_config_set_verbose(int verbose) {
    g_verbose_mode = verbose ? 1 : 0;
}

// Set application configuration parameter
int uci_config_set_app_param(AppConfigTlvType cfg_id, const unsigned char* value, size_t value_len) {
    if (!value || value_len == 0 || value_len > 256) {
        if (g_verbose_mode) {
            printf("Error: Invalid parameters for uci_config_set_app_param\n");
        }
        return -1;
    }
    
    // Store the value
    memcpy(app_config_values[cfg_id], value, value_len);
    app_config_lengths[cfg_id] = value_len;
    
    if (g_verbose_mode) {
        const char* param_name = uci_config_get_app_param_name(cfg_id);
        printf("Stored app config %s (0x%02X) with %zu bytes:", 
               param_name ? param_name : "unknown", cfg_id, value_len);
        for (size_t i = 0; i < value_len; i++) {
            printf(" %02X", value[i]);
        }
        printf("\n");
    }
    
    return 0;
}

// Get application configuration parameter
int uci_config_get_app_param(AppConfigTlvType cfg_id, unsigned char* value, size_t* value_len) {
    if (!value || !value_len || *value_len < app_config_lengths[cfg_id]) {
        if (g_verbose_mode) {
            printf("Error: Invalid parameters for uci_config_get_app_param\n");
        }
        return -1;
    }
    
    // Copy the value
    memcpy(value, app_config_values[cfg_id], app_config_lengths[cfg_id]);
    *value_len = app_config_lengths[cfg_id];
    
    if (g_verbose_mode) {
        const char* param_name = uci_config_get_app_param_name(cfg_id);
        printf("Retrieved app config %s (0x%02X) with %zu bytes:", 
               param_name ? param_name : "unknown", cfg_id, *value_len);
        for (size_t i = 0; i < *value_len; i++) {
            printf(" %02X", value[i]);
        }
        printf("\n");
    }
    
    return 0;
}

// List application configuration parameters
int uci_config_list_app_params() {
    printf("Available Application Configuration Parameters:\n");
    printf("=============================================\n");
    
    size_t count = uci_config_get_app_param_count();
    for (size_t i = 0; i < count; i++) {
        const config_param_info_t* param = uci_config_get_app_param_info_at(i);
        if (!param) {
            continue;
        }
        printf("  %-30s (0x%02X) - %s (default: %lu %s, range: %lu-%lu)\n", 
               param->name, param->cfg_id, param->description, 
               (unsigned long)param->default_value, param->unit, 
               (unsigned long)param->min_value, (unsigned long)param->max_value);
    }
    
    return 0;
}

// Get application configuration parameter name
const char* uci_config_get_app_param_name(AppConfigTlvType cfg_id) {
    const config_param_info_t* info = find_app_config_info(cfg_id);
    return info ? info->name : NULL;
}

// Get application configuration parameter description
const char* uci_config_get_app_param_desc(AppConfigTlvType cfg_id) {
    const config_param_info_t* info = find_app_config_info(cfg_id);
    return info ? info->description : "Unknown configuration parameter";
}

size_t uci_config_get_app_param_count(void) {
    return sizeof(app_config_params) / sizeof(app_config_params[0]);
}

const config_param_info_t* uci_config_get_app_param_info(AppConfigTlvType cfg_id) {
    return find_app_config_info(cfg_id);
}

const config_param_info_t* uci_config_get_app_param_info_at(size_t index) {
    size_t count = uci_config_get_app_param_count();
    return (index < count) ? &app_config_params[index] : NULL;
}

// Get application configuration parameter default value
uint64_t uci_config_get_app_param_default(AppConfigTlvType cfg_id) {
    const config_param_info_t* info = find_app_config_info(cfg_id);
    return info ? info->default_value : 0;
}

// Get application configuration parameter range
int uci_config_get_app_param_range(AppConfigTlvType cfg_id, uint64_t* min_val, uint64_t* max_val) {
    if (!min_val || !max_val) {
        return -1;
    }
    
    const config_param_info_t* info = find_app_config_info(cfg_id);
    if (info) {
        *min_val = info->min_value;
        *max_val = info->max_value;
        return 0;
    }
    
    *min_val = 0;
    *max_val = 0;
    return -1;
}

// Set device configuration parameter
int uci_config_set_device_param(DeviceConfigId cfg_id, const unsigned char* value, size_t value_len) {
    const device_config_param_info_t* info = uci_config_get_device_param_info(cfg_id);
    if (!info) {
        if (g_verbose_mode) {
            printf("Error: Unknown device config ID 0x%02X\n", cfg_id);
        }
        return -1;
    }

    if (!value || value_len == 0 || value_len > 256) {
        if (g_verbose_mode) {
            printf("Error: Invalid parameters for uci_config_set_device_param\n");
        }
        return -1;
    }

    size_t expected_len = info->value_len > 0 ? info->value_len : value_len;
    if (expected_len > 0 && value_len != expected_len) {
        if (g_verbose_mode) {
            printf("Error: Device config %s expects %zu bytes but got %zu\n",
                   info->name, expected_len, value_len);
        }
        return -1;
    }
    
    // Store the value
    memcpy(device_config_values[cfg_id], value, value_len);
    device_config_lengths[cfg_id] = value_len;
    
    if (g_verbose_mode) {
        const char* param_name = uci_config_get_device_param_name(cfg_id);
        printf("Stored device config %s (0x%02X) with %zu bytes:", 
               param_name ? param_name : "unknown", cfg_id, value_len);
        for (size_t i = 0; i < value_len; i++) {
            printf(" %02X", value[i]);
        }
        printf("\n");
    }
    
    return 0;
}

// Get device configuration parameter
int uci_config_get_device_param(DeviceConfigId cfg_id, unsigned char* value, size_t* value_len) {
    const device_config_param_info_t* info = uci_config_get_device_param_info(cfg_id);
    if (!info) {
        if (g_verbose_mode) {
            printf("Error: Unknown device config ID 0x%02X\n", cfg_id);
        }
        return -1;
    }

    if (!value || !value_len || *value_len < device_config_lengths[cfg_id]) {
        if (g_verbose_mode) {
            printf("Error: Invalid parameters for uci_config_get_device_param\n");
        }
        return -1;
    }
    
    // Copy the value
    memcpy(value, device_config_values[cfg_id], device_config_lengths[cfg_id]);
    *value_len = device_config_lengths[cfg_id];
    
    if (g_verbose_mode) {
        const char* param_name = uci_config_get_device_param_name(cfg_id);
        printf("Retrieved device config %s (0x%02X) with %zu bytes:", 
               param_name ? param_name : "unknown", cfg_id, *value_len);
        for (size_t i = 0; i < *value_len; i++) {
            printf(" %02X", value[i]);
        }
        printf("\n");
    }
    
    return 0;
}

// List device configuration parameters
int uci_config_list_device_params() {
    printf("Available Device Configuration Parameters:\n");
    printf("=========================================\n");
    
    size_t count = uci_config_get_device_param_count();
    for (size_t i = 0; i < count; i++) {
        const device_config_param_info_t* param = uci_config_get_device_param_info_at(i);
        if (!param) {
            continue;
        }
        printf("  %-28s (0x%02X) - %s (default: %lu, range: %lu-%lu, size: %zu byte%s)\n", 
               param->name, param->cfg_id, param->description, 
               (unsigned long)param->default_value, 
               (unsigned long)param->min_value, (unsigned long)param->max_value,
               param->value_len,
               param->value_len == 1 ? "" : "s");
    }
    
    return 0;
}

// Get device configuration parameter name
const char* uci_config_get_device_param_name(DeviceConfigId cfg_id) {
    const device_config_param_info_t* info = uci_config_get_device_param_info(cfg_id);
    return info ? info->name : NULL;
}

// Get device configuration parameter description
const char* uci_config_get_device_param_desc(DeviceConfigId cfg_id) {
    const device_config_param_info_t* info = uci_config_get_device_param_info(cfg_id);
    return info ? info->description : "Unknown device configuration parameter";
}

// Get device configuration parameter default value
uint64_t uci_config_get_device_param_default(DeviceConfigId cfg_id) {
    const device_config_param_info_t* info = uci_config_get_device_param_info(cfg_id);
    return info ? info->default_value : 0;
}

// Get device configuration parameter range
int uci_config_get_device_param_range(DeviceConfigId cfg_id, uint64_t* min_val, uint64_t* max_val) {
    if (!min_val || !max_val) {
        return -1;
    }
    
    const device_config_param_info_t* info = uci_config_get_device_param_info(cfg_id);
    if (!info) {
        *min_val = 0;
        *max_val = 0;
        return -1;
    }

    *min_val = info->min_value;
    *max_val = info->max_value;
    return 0;
}

size_t uci_config_get_device_param_length(DeviceConfigId cfg_id) {
    const device_config_param_info_t* info = uci_config_get_device_param_info(cfg_id);
    return info ? info->value_len : 0;
}

const device_config_param_info_t* uci_config_get_device_param_info(DeviceConfigId cfg_id) {
    return find_device_config_info(cfg_id);
}

size_t uci_config_get_device_param_count(void) {
    return sizeof(device_config_params) / sizeof(device_config_params[0]);
}

const device_config_param_info_t* uci_config_get_device_param_info_at(size_t index) {
    size_t count = uci_config_get_device_param_count();
    return (index < count) ? &device_config_params[index] : NULL;
}

int uci_config_lookup_device_param(const char* name, DeviceConfigId* cfg_id,
                                   const device_config_param_info_t** info_out) {
    if (!name || !cfg_id) {
        return -1;
    }

    const device_config_param_info_t* info = find_device_config_info_by_name(name);
    if (info) {
        *cfg_id = info->cfg_id;
        if (info_out) {
            *info_out = info;
        }
        return 0;
    }

    errno = 0;
    char* endptr = NULL;
    unsigned long value = strtoul(name, &endptr, 0);
    if (errno == 0 && endptr && *endptr == '\0' && value <= 0xFF) {
        *cfg_id = (DeviceConfigId)value;
        if (info_out) {
            *info_out = find_device_config_info((DeviceConfigId)value);
        }
        return 0;
    }

    return -1;
}

// Parse application parameter name to ID
int uci_config_parse_app_param_name(const char* name, AppConfigTlvType* cfg_id) {
    if (!name || !cfg_id) {
        return -1;
    }
    
    const config_param_info_t* info = find_app_config_info_by_name(name);
    if (info) {
        *cfg_id = info->cfg_id;
        return 0;
    }
    
    // Try to parse as hex number
    char* endptr;
    unsigned long value = strtoul(name, &endptr, 0);
    if (*endptr == '\0' && value <= 0xFF) {
        *cfg_id = (AppConfigTlvType)value;
        return 0;
    }
    
    return -1;
}

int uci_config_lookup_app_param(const char* name, AppConfigTlvType* cfg_id,
                                const config_param_info_t** info_out) {
    if (!name || !cfg_id) {
        return -1;
    }

    const config_param_info_t* info = find_app_config_info_by_name(name);
    if (!info) {
        for (size_t i = 0; i < ARRAY_SIZE(app_param_aliases); i++) {
            if (strcasecmp(app_param_aliases[i].alias, name) == 0) {
                info = find_app_config_info(app_param_aliases[i].cfg_id);
                break;
            }
        }
    }

    if (!info) {
        errno = 0;
        char* endptr = NULL;
        unsigned long value = strtoul(name, &endptr, 0);
        if (errno == 0 && endptr && *endptr == '\0' && value <= 0xFF) {
            info = find_app_config_info((AppConfigTlvType)value);
        }
    }

    if (!info) {
        return -1;
    }

    *cfg_id = info->cfg_id;
    if (info_out) {
        *info_out = info;
    }
    return 0;
}

// Parse device parameter name to ID
int uci_config_parse_device_param_name(const char* name, DeviceConfigId* cfg_id) {
    if (!name || !cfg_id) {
        return -1;
    }

    return uci_config_lookup_device_param(name, cfg_id, NULL);
}

int uci_config_parse_device_value(DeviceConfigId cfg_id, const char* value_str,
                                  unsigned char* value, size_t* value_len) {
    if (!value_str || !value || !value_len) {
        return -1;
    }

    const device_config_param_info_t* info = uci_config_get_device_param_info(cfg_id);
    if (!info) {
        return -1;
    }

    size_t expected_len = info->value_len > 0 ? info->value_len : 1;
    if (*value_len < expected_len) {
        return -1;
    }

    memset(value, 0, expected_len);

    if (cfg_id == DEVICE_STATE) {
        unsigned char state_value = 0;
        if (strcasecmp(value_str, "ready") == 0) {
            state_value = DEVICE_STATE_READY;
        } else if (strcasecmp(value_str, "active") == 0) {
            state_value = DEVICE_STATE_ACTIVE;
        } else if (strcasecmp(value_str, "error") == 0) {
            state_value = DEVICE_STATE_ERROR;
        } else {
            uint64_t numeric = 0;
            if (parse_unsigned_value(value_str, &numeric) != 0 || numeric > 0xFF) {
                return -1;
            }
            state_value = (unsigned char)numeric;
        }

        value[0] = state_value;
        *value_len = 1;
        return 0;
    }

    if (cfg_id == LOW_POWER_MODE ||
        cfg_id == DEVICE_PAN_COORD ||
        cfg_id == DEVICE_PROMISCUOUS) {
        unsigned char bool_value = 0;
        if (parse_boolean_value(value_str, &bool_value) != 0) {
            return -1;
        }
        value[0] = bool_value;
        *value_len = 1;
        return 0;
    }

    uint64_t numeric_value = 0;
    if (parse_unsigned_value(value_str, &numeric_value) != 0) {
        return -1;
    }

    if (numeric_value < info->min_value || numeric_value > info->max_value) {
        return -1;
    }

    for (size_t i = 0; i < expected_len; i++) {
        value[i] = (unsigned char)((numeric_value >> (8 * i)) & 0xFF);
    }

    *value_len = expected_len;
    return 0;
}

int uci_config_parse_app_value(AppConfigTlvType cfg_id, const char* value_str,
                               unsigned char* value, size_t* value_len) {
    if (!value_str || !value || !value_len) {
        return -1;
    }

    const config_param_info_t* info = uci_config_get_app_param_info(cfg_id);
    if (!info) {
        return -1;
    }

    size_t expected_len = info->value_len;
    if (expected_len == 0) {
        expected_len = 1;
    }

    if (*value_len < expected_len) {
        return -1;
    }

    if (cfg_id == SESSION_KEY || cfg_id == SUBSESSION_KEY) {
        const char* hex_str = value_str;
        size_t parsed_len = *value_len;
        if (strncmp(hex_str, "0x", 2) == 0 || strncmp(hex_str, "0X", 2) == 0) {
            hex_str += 2;
        }
        if (uci_config_parse_hex_value(hex_str, value, &parsed_len) != 0) {
            return -1;
        }
        if (parsed_len != 16 && parsed_len != 32) {
            return -1;
        }
        *value_len = parsed_len;
        return 0;
    }

    if (cfg_id == SESSION_TIME_BASE) {
        const char* hex_str = value_str;
        size_t parsed_len = *value_len;
        if (strncmp(hex_str, "0x", 2) == 0 || strncmp(hex_str, "0X", 2) == 0) {
            hex_str += 2;
        }
        if (uci_config_parse_hex_value(hex_str, value, &parsed_len) != 0) {
            return -1;
        }
        if (parsed_len != 9) {
            return -1;
        }
        *value_len = parsed_len;
        return 0;
    }

    if (cfg_id == DST_MAC_ADDRESS) {
        size_t parsed_len = *value_len;
        if (parse_dst_mac_address_list(value_str, value, &parsed_len) != 0) {
            return -1;
        }
        if (parsed_len == 0 || (parsed_len % 2) != 0 || parsed_len > 16) {
            return -1;
        }
        *value_len = parsed_len;
        return 0;
    }

    uint64_t numeric_value = 0;
    int have_numeric = 0;
    int mapped_value = 0;

    for (size_t i = 0; i < ARRAY_SIZE(app_value_mappings); i++) {
        if (app_value_mappings[i].cfg_id == cfg_id &&
            strcasecmp(app_value_mappings[i].token, value_str) == 0) {
            numeric_value = app_value_mappings[i].value;
            have_numeric = 1;
            mapped_value = 1;
            break;
        }
    }

    if (!have_numeric && expected_len == 1) {
        unsigned char bool_value = 0;
        if (parse_boolean_value(value_str, &bool_value) == 0) {
            numeric_value = bool_value;
            have_numeric = 1;
        }
    }

    if (!have_numeric) {
        if (parse_unsigned_value(value_str, &numeric_value) != 0) {
            return -1;
        }
    }

    if (!mapped_value) {
        if (numeric_value < info->min_value || numeric_value > info->max_value) {
            return -1;
        }
    }

    memset(value, 0, expected_len);
    for (size_t i = 0; i < expected_len; i++) {
        value[i] = (unsigned char)((numeric_value >> (8 * i)) & 0xFF);
    }

    *value_len = expected_len;
    return 0;
}

// Parse hex value string to bytes
int uci_config_parse_hex_value(const char* hex_str, unsigned char* value, size_t* value_len) {
    if (!hex_str || !value || !value_len) {
        return -1;
    }
    
    size_t len = strlen(hex_str);
    if (len % 2 != 0) {
        return -1; // Must be even number of hex digits
    }
    
    size_t byte_count = len / 2;
    if (byte_count > *value_len) {
        return -1; // Buffer too small
    }
    
    for (size_t i = 0; i < byte_count; i++) {
        char hex_byte[3] = {hex_str[i*2], hex_str[i*2+1], '\0'};
        char* endptr;
        unsigned long byte_val = strtoul(hex_byte, &endptr, 16);
        if (*endptr != '\0') {
            return -1; // Invalid hex digit
        }
        value[i] = (unsigned char)byte_val;
    }
    
    *value_len = byte_count;
    return 0;
}

// Format bytes as hex string
int uci_config_format_hex_value(const unsigned char* value, size_t value_len, char* output, size_t output_len) {
    if (!value || !output || output_len < (value_len * 3)) {
        return -1;
    }
    
    for (size_t i = 0; i < value_len; i++) {
        snprintf(output + i * 3, output_len - i * 3, "%02X ", value[i]);
    }
    
    // Remove trailing space
    if (value_len > 0) {
        output[value_len * 3 - 1] = '\0';
    } else {
        output[0] = '\0';
    }
    
    return 0;
}

// Show help for specific application parameter
int uci_config_show_app_param_help(AppConfigTlvType cfg_id) {
    const config_param_info_t* param = find_app_config_info(cfg_id);
    if (param) {
        printf("Application Configuration Parameter: %s (0x%02X)\n", param->name, param->cfg_id);
        printf("  Description: %s\n", param->description);
        printf("  Default Value: %lu %s\n", (unsigned long)param->default_value, param->unit);
        printf("  Valid Range: %lu to %lu\n", (unsigned long)param->min_value, (unsigned long)param->max_value);
        if (strlen(param->unit) > 0) {
            printf("  Unit: %s\n", param->unit);
        }
        return 0;
    }
    
    printf("Unknown Application Configuration Parameter: 0x%02X\n", cfg_id);
    return -1;
}

// Show help for specific device parameter
int uci_config_show_device_param_help(DeviceConfigId cfg_id) {
    const device_config_param_info_t* param = uci_config_get_device_param_info(cfg_id);
    if (!param) {
        printf("Unknown Device Configuration Parameter: 0x%02X\n", cfg_id);
        return -1;
    }

    printf("Device Configuration Parameter: %s (0x%02X)\n", param->name, param->cfg_id);
    printf("  Description: %s\n", param->description);
    printf("  Default Value: %lu\n", (unsigned long)param->default_value);
    printf("  Valid Range: %lu to %lu\n", (unsigned long)param->min_value, (unsigned long)param->max_value);
    printf("  Value Length: %zu byte%s\n", param->value_len, param->value_len == 1 ? "" : "s");
    if (strlen(param->unit) > 0) {
        printf("  Unit: %s\n", param->unit);
    }
    return 0;
}

// Show all application parameters
int uci_config_show_all_app_params() {
    printf("=== All Application Configuration Parameters ===\n");
    size_t count = uci_config_get_app_param_count();
    for (size_t i = 0; i < count; i++) {
        const config_param_info_t* param = uci_config_get_app_param_info_at(i);
        if (!param) {
            continue;
        }
        printf("\n%s (0x%02X):\n", param->name, param->cfg_id);
        printf("  Description: %s\n", param->description);
        printf("  Default Value: %lu %s\n", (unsigned long)param->default_value, param->unit);
        printf("  Valid Range: %lu to %lu\n", (unsigned long)param->min_value, (unsigned long)param->max_value);
        if (strlen(param->unit) > 0) {
            printf("  Unit: %s\n", param->unit);
        }
        
        // Show current value
        unsigned char current_value[256];
        size_t current_len = sizeof(current_value);
        if (uci_config_get_app_param(param->cfg_id, current_value, &current_len) == 0) {
            printf("  Current Value: ");
            for (size_t j = 0; j < current_len; j++) {
                printf("%02X ", current_value[j]);
            }
            printf("(%zu bytes)\n", current_len);
        } else {
            printf("  Current Value: Unable to retrieve\n");
        }
    }
    printf("\nTotal: %zu parameters\n", count);
    return 0;
}

// Show all device parameters
int uci_config_show_all_device_params() {
    printf("=== All Device Configuration Parameters ===\n");
    size_t count = uci_config_get_device_param_count();
    for (size_t i = 0; i < count; i++) {
        const device_config_param_info_t* param = uci_config_get_device_param_info_at(i);
        if (!param) {
            continue;
        }
        printf("\n%s (0x%02X):\n", param->name, param->cfg_id);
        printf("  Description: %s\n", param->description);
        printf("  Default Value: %lu\n", (unsigned long)param->default_value);
        printf("  Valid Range: %lu to %lu\n", (unsigned long)param->min_value, (unsigned long)param->max_value);
        printf("  Value Length: %zu byte%s\n", param->value_len, param->value_len == 1 ? "" : "s");
        if (strlen(param->unit) > 0) {
            printf("  Unit: %s\n", param->unit);
        }
        
        // Show current value
        unsigned char current_value[256];
        size_t current_len = sizeof(current_value);
        if (uci_config_get_device_param(param->cfg_id, current_value, &current_len) == 0) {
            printf("  Current Value: ");
            for (size_t j = 0; j < current_len; j++) {
                printf("%02X ", current_value[j]);
            }
            printf("(%zu bytes)\n", current_len);
        } else {
            printf("  Current Value: Unable to retrieve\n");
        }
    }
    printf("\nTotal: %zu parameters\n", count);
    return 0;
}
