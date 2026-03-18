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
set_app_config 305419896 multi_node_mode multicast
set_app_config 305419896 device_role controlee
get_app_config 305419896 device_type
get_app_config 305419896 device_type multi_node_mode
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
require_line "TLV[2]: Config ID=0x11 (device_role), Length=1 bytes"
require_line "Interpreted: CONTROLEE (0x01)"
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
