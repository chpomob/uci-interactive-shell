#!/usr/bin/env bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SIM_ROOT="${ROOT_DIR%/uci_interactive_shell}/uci_device_simulator"
SIM_BIN="${SIM_ROOT}/build/uci-device-sim"
PORT="${UCI_TCP_SIM_TEST_PORT:-$((25000 + ($$ % 1000)))}"
HOST="${UCI_TCP_SIM_TEST_HOST:-127.0.0.1}"
SCENARIO="${UCI_TCP_SIM_SCENARIO:-default}"
RAW_OUTPUT="$(mktemp)"
SANITIZED_OUTPUT="$(mktemp)"
SIM_LOG="$(mktemp)"

cleanup() {
    if [[ -n "${SIM_PID:-}" ]]; then
        kill "${SIM_PID}" >/dev/null 2>&1 || true
        wait "${SIM_PID}" 2>/dev/null || true
    fi
    rm -f "${RAW_OUTPUT}" "${SANITIZED_OUTPUT}" "${SIM_LOG}"
}
trap cleanup EXIT

if [[ ! -x "${SIM_BIN}" ]]; then
    echo "SKIPPED: missing simulator binary at ${SIM_BIN}"
    exit 0
fi

if [[ "${SCENARIO}" == "default" ]]; then
    "${SIM_BIN}" "${HOST}" "${PORT}" >"${SIM_LOG}" 2>&1 &
else
    "${SIM_BIN}" "${HOST}" "${PORT}" "${SCENARIO}" >"${SIM_LOG}" 2>&1 &
fi
SIM_PID=$!
sleep 0.2

if ! kill -0 "${SIM_PID}" >/dev/null 2>&1; then
    if grep -Eq "Operation not permitted|Permission denied" "${SIM_LOG}"; then
        echo "SKIPPED: simulator could not open local TCP sockets in this environment"
        exit 0
    fi
    echo "FAIL: simulator failed to start"
    echo "--- simulator log ---"
    cat "${SIM_LOG}"
    exit 1
fi

cat <<EOF_CMDS | "${ROOT_DIR}/uci-shell" >"${RAW_OUTPUT}"
mode_tcp ${HOST} ${PORT}
mode_info
get_device_info
device_reset
get_caps_info
query_timestamp
set_config device_state active
get_config device_state
set_config low_power_mode on
get_config low_power_mode
set_config device_pan_id 0x1234
get_config device_pan_id
session_init 305419896 fira_ranging
session_query_data_size_in_ranging 305419896
set_app_config 305419896 device_type responder
set_app_config 305419896 ranging_round_usage data
set_app_config 305419896 sts_config dynamic
set_app_config 305419896 multi_node_mode multicast
set_app_config 305419896 device_role controlee
set_app_config 305419896 channel_number 5
set_app_config 305419896 no_of_controlee 3
set_app_config 305419896 device_mac_address 0xABCD
set_app_config 305419896 dst_mac_address 0x5678
set_app_config 305419896 slot_duration 2400
set_app_config 305419896 ranging_duration 2000
set_app_config 305419896 sts_index 5
set_app_config 305419896 mac_fcs_type 1
set_app_config 305419896 ranging_round_control 5
set_app_config 305419896 aoa_result_req 3
set_app_config 305419896 rng_data_ntf 2
set_app_config 305419896 rng_data_ntf_proximity_near 100
set_app_config 305419896 rng_data_ntf_proximity_far 500
set_app_config 305419896 rframe_config 2
set_app_config 305419896 rssi_reporting 1
set_app_config 305419896 preamble_code_index 12
set_app_config 305419896 sfd_id 1
set_app_config 305419896 psdu_data_rate 2
set_app_config 305419896 preamble_duration 2
set_app_config 305419896 link_layer_mode 1
set_app_config 305419896 data_repetition_count 4
set_app_config 305419896 ranging_time_struct 3
set_app_config 305419896 slots_per_rr 6
set_app_config 305419896 tx_adaptive_payload_power 1
set_app_config 305419896 rng_data_ntf_aoa_bound 45
set_app_config 305419896 responder_slot_index 7
set_app_config 305419896 prf_mode 1
set_app_config 305419896 cap_size_range 512
set_app_config 305419896 tx_jitter_window_size 16
set_app_config 305419896 scheduled_mode scheduled
set_app_config 305419896 key_rotation 1
set_app_config 305419896 key_rotation_rate 32
set_app_config 305419896 session_priority 75
set_app_config 305419896 mac_address_mode 1
set_app_config 305419896 hopping_mode 1
set_app_config 305419896 result_report_config 7
set_app_config 305419896 in_band_termination_attempt_count 4
set_app_config 305419896 bprf_phr_data_rate 1
set_app_config 305419896 max_number_of_measurements 16
set_app_config 305419896 ul_tdoa_tx_interval 100
set_app_config 305419896 min_frames_per_rr 2
set_app_config 305419896 mtu_size 1024
set_app_config 305419896 inter_frame_interval 5
set_app_config 305419896 dl_tdoa_ranging_method ds_twr
set_app_config 305419896 dl_tdoa_tx_timestamp_conf 3
set_app_config 305419896 dl_tdoa_hop_count 1
get_app_config 305419896 device_type
get_app_config 305419896 device_type multi_node_mode
get_app_config 305419896 ranging_round_usage
get_app_config 305419896 sts_config
get_app_config 305419896 channel_number
get_app_config 305419896 no_of_controlee
get_app_config 305419896 device_mac_address
get_app_config 305419896 dst_mac_address
get_app_config 305419896 slot_duration
get_app_config 305419896 ranging_duration
get_app_config 305419896 sts_index
get_app_config 305419896 mac_fcs_type
get_app_config 305419896 ranging_round_control
get_app_config 305419896 aoa_result_req
get_app_config 305419896 rng_data_ntf
get_app_config 305419896 rng_data_ntf_proximity_near
get_app_config 305419896 rng_data_ntf_proximity_far
get_app_config 305419896 rframe_config
get_app_config 305419896 rssi_reporting
get_app_config 305419896 preamble_code_index
get_app_config 305419896 sfd_id
get_app_config 305419896 psdu_data_rate
get_app_config 305419896 preamble_duration
get_app_config 305419896 link_layer_mode
get_app_config 305419896 data_repetition_count
get_app_config 305419896 ranging_time_struct
get_app_config 305419896 slots_per_rr
get_app_config 305419896 tx_adaptive_payload_power
get_app_config 305419896 rng_data_ntf_aoa_bound
get_app_config 305419896 responder_slot_index
get_app_config 305419896 prf_mode
get_app_config 305419896 cap_size_range
get_app_config 305419896 tx_jitter_window_size
get_app_config 305419896 scheduled_mode
get_app_config 305419896 key_rotation
get_app_config 305419896 key_rotation_rate
get_app_config 305419896 session_priority
get_app_config 305419896 mac_address_mode
get_app_config 305419896 hopping_mode
get_app_config 305419896 result_report_config
get_app_config 305419896 in_band_termination_attempt_count
get_app_config 305419896 bprf_phr_data_rate
get_app_config 305419896 max_number_of_measurements
get_app_config 305419896 ul_tdoa_tx_interval
get_app_config 305419896 min_frames_per_rr
get_app_config 305419896 mtu_size
get_app_config 305419896 inter_frame_interval
get_app_config 305419896 dl_tdoa_ranging_method
get_app_config 305419896 dl_tdoa_tx_timestamp_conf
get_app_config 305419896 dl_tdoa_hop_count
get_app_config 305419896
session_update_multicast_list 305419896 add 0x1234 0xAABBCCDD
session_update_multicast_list 305419896 remove 0x1234 0xAABBCCDD
session_update_dt_tag_rounds 305419896 010509
session_data_transfer_phase_config 305419896 7 165 3 112233
session_start 305419896
session_send_data 305419896 0x11223344 15 AABB
session_logical_link_create 305419896 0x12 0x77 5
session_logical_link_get_param 305419896 0x12
session_logical_link_close 305419896 0x12
get_session_state 305419896
session_stop 305419896
quit
EOF_CMDS

sed -r 's/\x1B\[[0-9;]*[A-Za-z]//g' "${RAW_OUTPUT}" >"${SANITIZED_OUTPUT}"

require_line() {
    local pattern="$1"
    if ! grep -Fq "${pattern}" "${SANITIZED_OUTPUT}"; then
        echo "FAIL: missing output pattern: ${pattern}"
        echo "--- sanitized output ---"
        cat "${SANITIZED_OUTPUT}"
        exit 1
    fi
}

forbid_line() {
    local pattern="$1"
    if grep -Fq "${pattern}" "${SANITIZED_OUTPUT}"; then
        echo "FAIL: forbidden output pattern present: ${pattern}"
        echo "--- sanitized output ---"
        cat "${SANITIZED_OUTPUT}"
        exit 1
    fi
}

require_line "Current mode: TCP"
require_line "TCP endpoint: ${HOST}:${PORT}"
require_line "CORE_DEVICE_INFO Response:"
require_line "CORE_DEVICE_RESET Response:"
require_line "CORE_DEVICE_STATUS_NTF:"
require_line "CORE_GET_CAPS_INFO Response:"
require_line "CORE_QUERY_UWBS_TIMESTAMP Response:"
require_line "Timestamp:"
require_line "CORE_SET_CONFIG Response:"
require_line "Config[0]: ID=0x00 (device_state), Status=0x00 (OK)"
require_line "CORE_GET_CONFIG Response:"
require_line "Interpreted: ACTIVE (0x02)"
require_line "Config[0]: ID=0x01 (low_power_mode), Status=0x00 (OK)"
require_line "Interpreted: ON (0x01)"
require_line "Config[0]: ID=0xA2 (device_pan_id), Status=0x00 (OK)"
require_line "4660 (0x34 12)"
require_line "SESSION_INIT Response:"
require_line "Session Handle: 0x12345678"
require_line "SESSION_QUERY_DATA_SIZE_IN_RANGING Response:"
require_line "Max Data Size: 512 bytes"
require_line "SESSION_SET_APP_CONFIG Response:"
require_line "Number of Config Status: 0"
require_line "SESSION_GET_APP_CONFIG Response:"
require_line "TLV[0]: Config ID=0x00 (device_type), Length=1 bytes"
require_line "Value: 0x01"
require_line "TLV[1]: Config ID=0x03 (multi_node_mode), Length=1 bytes"
require_line "Interpreted: MULTICAST (0x02)"
require_line "TLV[0]: Config ID=0x01 (ranging_round_usage), Length=1 bytes"
require_line "Interpreted: DATA (0x01)"
require_line "TLV[0]: Config ID=0x02 (sts_config), Length=1 bytes"
require_line "Interpreted: DYNAMIC_STS (0x01)"
require_line "TLV[0]: Config ID=0x04 (channel_number), Length=1 bytes"
require_line "Interpreted: Channel 5 (0x05)"
require_line "TLV[0]: Config ID=0x05 (no_of_controlee), Length=1 bytes"
require_line "Interpreted: 3 controlees (0x03)"
require_line "TLV[0]: Config ID=0x06 (device_mac_address), Length=2 bytes"
require_line "Interpreted: 0xABCD"
require_line "TLV[0]: Config ID=0x07 (dst_mac_address), Length=2 bytes"
require_line "Interpreted: 0x5678"
require_line "TLV[0]: Config ID=0x08 (slot_duration), Length=2 bytes"
require_line "Interpreted: 2400 RSTU (0x0960)"
require_line "TLV[0]: Config ID=0x09 (ranging_duration), Length=4 bytes"
require_line "Interpreted: 2000 ms (0x000007D0)"
require_line "TLV[0]: Config ID=0x0A (sts_index), Length=4 bytes"
require_line "Interpreted: 5 (0x05 00 00 00) [Range: 0-4294967295]"
require_line "TLV[0]: Config ID=0x0B (mac_fcs_type), Length=1 bytes"
require_line "Interpreted: CRC32 (0x01)"
require_line "TLV[0]: Config ID=0x0C (ranging_round_control), Length=1 bytes"
require_line "Interpreted: 5 (0x05) [Range: 0-255]"
require_line "TLV[0]: Config ID=0x0D (aoa_result_req), Length=1 bytes"
require_line "Interpreted: AOA_ELEVATION_AND_AZIMUTH (0x03)"
require_line "TLV[0]: Config ID=0x0E (rng_data_ntf), Length=1 bytes"
require_line "Interpreted: 2 (0x02) [Range: 0-3]"
require_line "TLV[0]: Config ID=0x0F (rng_data_ntf_proximity_near), Length=2 bytes"
require_line "Interpreted: 100 (0x64 00) [Range: 0-65535]"
require_line "TLV[0]: Config ID=0x10 (rng_data_ntf_proximity_far), Length=2 bytes"
require_line "Interpreted: 500 (0xF4 01) [Range: 0-65535]"
require_line "TLV[0]: Config ID=0x12 (rframe_config), Length=1 bytes"
require_line "Interpreted: SP2 (0x02)"
require_line "TLV[0]: Config ID=0x13 (rssi_reporting), Length=1 bytes"
require_line "Interpreted: 1 (0x01) [Range: 0-1]"
require_line "TLV[0]: Config ID=0x14 (preamble_code_index), Length=1 bytes"
require_line "Interpreted: 12 (0x0C) [Range: 0-31]"
require_line "TLV[0]: Config ID=0x15 (sfd_id), Length=1 bytes"
require_line "Interpreted: 1 (0x01) [Range: 0-3]"
require_line "TLV[0]: Config ID=0x16 (psdu_data_rate), Length=1 bytes"
require_line "Interpreted: 2 (0x02) [Range: 0-3]"
require_line "TLV[0]: Config ID=0x17 (preamble_duration), Length=1 bytes"
require_line "Interpreted: 2 (0x02) [Range: 0-3]"
require_line "TLV[0]: Config ID=0x18 (link_layer_mode), Length=1 bytes"
require_line "Interpreted: EXTENDED (0x01)"
require_line "TLV[0]: Config ID=0x19 (data_repetition_count), Length=1 bytes"
require_line "Interpreted: 4 (0x04) [Range: 0-63]"
require_line "TLV[0]: Config ID=0x1A (ranging_time_struct), Length=1 bytes"
require_line "Interpreted: 3 (0x03) [Range: 0-3]"
require_line "TLV[0]: Config ID=0x1B (slots_per_rr), Length=1 bytes"
require_line "Interpreted: 6 (0x06) [Range: 0-255]"
require_line "TLV[0]: Config ID=0x1C (tx_adaptive_payload_power), Length=1 bytes"
require_line "Interpreted: ENABLED (0x01)"
require_line "TLV[0]: Config ID=0x1D (rng_data_ntf_aoa_bound), Length=2 bytes"
require_line "Interpreted: 45 (0x2D 00) [Range: 0-65535]"
require_line "TLV[0]: Config ID=0x1E (responder_slot_index), Length=1 bytes"
require_line "Interpreted: 7 (0x07) [Range: 0-255]"
require_line "TLV[0]: Config ID=0x1F (prf_mode), Length=1 bytes"
require_line "Interpreted: HPRF (0x01)"
require_line "TLV[0]: Config ID=0x20 (cap_size_range), Length=2 bytes"
require_line "Interpreted: 512 (0x00 02) [Range: 0-65535]"
require_line "TLV[0]: Config ID=0x21 (tx_jitter_window_size), Length=2 bytes"
require_line "Interpreted: 16 (0x10 00) [Range: 0-65535]"
require_line "TLV[0]: Config ID=0x22 (scheduled_mode), Length=1 bytes"
require_line "Interpreted: SCHEDULED (0x01)"
require_line "TLV[0]: Config ID=0x23 (key_rotation), Length=1 bytes"
require_line "Interpreted: ENABLED (0x01)"
require_line "TLV[0]: Config ID=0x24 (key_rotation_rate), Length=2 bytes"
require_line "Interpreted: 32 (0x20 00) [Range: 0-65535]"
require_line "TLV[0]: Config ID=0x25 (session_priority), Length=1 bytes"
require_line "Interpreted: 75 (0x4B) [Range: 0-100]"
require_line "TLV[0]: Config ID=0x26 (mac_address_mode), Length=1 bytes"
require_line "Interpreted: 1 (0x01) [Range: 0-1]"
require_line "TLV[0]: Config ID=0x2C (hopping_mode), Length=1 bytes"
require_line "Interpreted: ENABLED (0x01)"
require_line "TLV[0]: Config ID=0x2E (result_report_config), Length=1 bytes"
require_line "Interpreted: 7 (0x07) [Range: 0-255]"
require_line "TLV[0]: Config ID=0x2F (in_band_termination_attempt_count), Length=1 bytes"
require_line "Interpreted: 4 (0x04) [Range: 0-255]"
require_line "TLV[0]: Config ID=0x31 (bprf_phr_data_rate), Length=1 bytes"
require_line "Interpreted: 1 (0x01) [Range: 0-1]"
require_line "TLV[0]: Config ID=0x32 (max_number_of_measurements), Length=2 bytes"
require_line "Interpreted: 16 (0x10 00) [Range: 0-65535]"
require_line "TLV[0]: Config ID=0x33 (ul_tdoa_tx_interval), Length=4 bytes"
require_line "Interpreted: 100 (0x64 00 00 00) [Range: 0-4294967295]"
require_line "TLV[0]: Config ID=0x3A (min_frames_per_rr), Length=1 bytes"
require_line "Interpreted: 2 (0x02) [Range: 0-255]"
require_line "TLV[0]: Config ID=0x3B (mtu_size), Length=2 bytes"
require_line "Interpreted: 1024 (0x00 04) [Range: 0-65535]"
require_line "TLV[0]: Config ID=0x3C (inter_frame_interval), Length=1 bytes"
require_line "Interpreted: 5 (0x05) [Range: 0-255]"
require_line "TLV[0]: Config ID=0x3D (dl_tdoa_ranging_method), Length=1 bytes"
require_line "Interpreted: 1 (0x01) [Range: 0-1]"
require_line "TLV[0]: Config ID=0x3E (dl_tdoa_tx_timestamp_conf), Length=1 bytes"
require_line "Interpreted: 3 (0x03) [Range: 0-255]"
require_line "TLV[0]: Config ID=0x3F (dl_tdoa_hop_count), Length=1 bytes"
require_line "Interpreted: 1 (0x01) [Range: 0-255]"
require_line "Number of TLVs: 51"
require_line "TLV[10]: Config ID=0x0A (sts_index), Length=4 bytes"
require_line "TLV[15]: Config ID=0x0F (rng_data_ntf_proximity_near), Length=2 bytes"
require_line "TLV[16]: Config ID=0x10 (rng_data_ntf_proximity_far), Length=2 bytes"
require_line "TLV[17]: Config ID=0x11 (device_role), Length=1 bytes"
require_line "Interpreted: CONTROLEE (0x01)"
require_line "TLV[18]: Config ID=0x12 (rframe_config), Length=1 bytes"
require_line "TLV[19]: Config ID=0x13 (rssi_reporting), Length=1 bytes"
require_line "TLV[20]: Config ID=0x14 (preamble_code_index), Length=1 bytes"
require_line "TLV[21]: Config ID=0x15 (sfd_id), Length=1 bytes"
require_line "TLV[22]: Config ID=0x16 (psdu_data_rate), Length=1 bytes"
require_line "TLV[23]: Config ID=0x17 (preamble_duration), Length=1 bytes"
require_line "TLV[24]: Config ID=0x18 (link_layer_mode), Length=1 bytes"
require_line "TLV[25]: Config ID=0x19 (data_repetition_count), Length=1 bytes"
require_line "TLV[26]: Config ID=0x1A (ranging_time_struct), Length=1 bytes"
require_line "TLV[27]: Config ID=0x1B (slots_per_rr), Length=1 bytes"
require_line "TLV[28]: Config ID=0x1C (tx_adaptive_payload_power), Length=1 bytes"
require_line "TLV[29]: Config ID=0x1D (rng_data_ntf_aoa_bound), Length=2 bytes"
require_line "TLV[30]: Config ID=0x1E (responder_slot_index), Length=1 bytes"
require_line "TLV[31]: Config ID=0x1F (prf_mode), Length=1 bytes"
require_line "TLV[32]: Config ID=0x20 (cap_size_range), Length=2 bytes"
require_line "TLV[33]: Config ID=0x21 (tx_jitter_window_size), Length=2 bytes"
require_line "TLV[34]: Config ID=0x22 (scheduled_mode), Length=1 bytes"
require_line "TLV[35]: Config ID=0x23 (key_rotation), Length=1 bytes"
require_line "TLV[36]: Config ID=0x24 (key_rotation_rate), Length=2 bytes"
require_line "TLV[37]: Config ID=0x25 (session_priority), Length=1 bytes"
require_line "TLV[38]: Config ID=0x26 (mac_address_mode), Length=1 bytes"
require_line "TLV[39]: Config ID=0x2C (hopping_mode), Length=1 bytes"
require_line "TLV[40]: Config ID=0x2E (result_report_config), Length=1 bytes"
require_line "TLV[41]: Config ID=0x2F (in_band_termination_attempt_count), Length=1 bytes"
require_line "TLV[42]: Config ID=0x31 (bprf_phr_data_rate), Length=1 bytes"
require_line "TLV[43]: Config ID=0x32 (max_number_of_measurements), Length=2 bytes"
require_line "TLV[44]: Config ID=0x33 (ul_tdoa_tx_interval), Length=4 bytes"
require_line "TLV[45]: Config ID=0x3A (min_frames_per_rr), Length=1 bytes"
require_line "TLV[46]: Config ID=0x3B (mtu_size), Length=2 bytes"
require_line "TLV[47]: Config ID=0x3C (inter_frame_interval), Length=1 bytes"
require_line "TLV[48]: Config ID=0x3D (dl_tdoa_ranging_method), Length=1 bytes"
require_line "TLV[49]: Config ID=0x3E (dl_tdoa_tx_timestamp_conf), Length=1 bytes"
require_line "TLV[50]: Config ID=0x3F (dl_tdoa_hop_count), Length=1 bytes"
require_line "SESSION_UPDATE_CONTROLLER_MULTICAST_LIST Response:"
require_line "Entries Processed: 1"
require_line "Entry[0]: Short=0x1234, Subsession=0xAABBCCDD, Status=0x00 (OK)"
require_line "SESSION_UPDATE_ACTIVE_ROUNDS_DT_TAG Response:"
require_line "Stored Round Indices: 3"
require_line "Round Indices: 0x01 0x05 0x09"
require_line "SESSION_DATA_TRANSFER_PHASE_CONFIG Response:"
require_line "Status: 0x00 (OK)"
require_line "SESSION_STATUS_NTF:"
require_line "SESSION_START Response:"
require_line "SESSION_DATA_CREDIT_NTF:"
require_line "Credit Availability: 0x01 (Credits Available)"
require_line "SESSION_DATA_TRANSFER_STATUS_NTF:"
require_line "UCI Sequence Number: 15"
require_line "TX Attempt Count: 1"
require_line "SESSION_LOGICAL_LINK_CREATE Response:"
require_line "Logical Link ID: 0x12"
require_line "Initial Credit: 5"
require_line "SESSION_LOGICAL_LINK_UWBS_CREATE_NTF:"
require_line "Credits: 5"
require_line "SESSION_LOGICAL_LINK_GET_PARAM Response:"
require_line "Mode: 0x77"
require_line "Credits: 5"
require_line "SESSION_LOGICAL_LINK_CLOSE Response:"
require_line "SESSION_LOGICAL_LINK_UWBS_CLOSE_NTF:"
require_line "Reason: 0x00"
require_line "SESSION_GET_STATE Response:"
require_line "SESSION_STOP Response:"

if [[ "${SCENARIO}" == "ranging_stream" ]]; then
    require_line "RANGE_DATA_NTF (SESSION_INFO_NTF):"
    require_line "Sequence Number"
    require_line "Sequence Number: 1"
    require_line "Sequence Number: 2"
    require_line "Measurement Count"
    forbid_line "Sequence Number: 3"
fi

echo "PASS: shell TCP simulator integration"
