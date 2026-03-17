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

cleanup() {
    if [[ -n "${SIM_PID:-}" ]]; then
        kill "${SIM_PID}" >/dev/null 2>&1 || true
        wait "${SIM_PID}" 2>/dev/null || true
    fi
    rm -f "${RAW_OUTPUT}" "${SANITIZED_OUTPUT}"
}
trap cleanup EXIT

if [[ ! -x "${SIM_BIN}" ]]; then
    echo "SKIPPED: missing simulator binary at ${SIM_BIN}"
    exit 0
fi

if [[ "${SCENARIO}" == "default" ]]; then
    "${SIM_BIN}" "${HOST}" "${PORT}" >/dev/null 2>&1 &
else
    "${SIM_BIN}" "${HOST}" "${PORT}" "${SCENARIO}" >/dev/null 2>&1 &
fi
SIM_PID=$!
sleep 0.2

cat <<EOF_CMDS | "${ROOT_DIR}/uci-shell" >"${RAW_OUTPUT}"
mode_tcp ${HOST} ${PORT}
mode_info
get_device_info
get_caps_info
set_config device_state active
get_config device_state
set_config low_power_mode on
get_config low_power_mode
set_config device_pan_id 0x1234
get_config device_pan_id
session_init 305419896 fira_ranging
session_query_data_size_in_ranging 305419896
set_app_config 305419896 device_type responder
get_app_config 305419896 device_type
session_start 305419896
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

require_line "Current mode: TCP"
require_line "TCP endpoint: ${HOST}:${PORT}"
require_line "CORE_DEVICE_INFO Response:"
require_line "CORE_GET_CAPS_INFO Response:"
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
require_line "Config[0]: ID=0x00 (device_type), Status=0x00 (OK)"
require_line "SESSION_GET_APP_CONFIG Response:"
require_line "TLV[0]: Config ID=0x00 (device_type), Length=1 bytes"
require_line "Value: 0x01"
require_line "SESSION_STATUS_NTF:"
require_line "SESSION_START Response:"
require_line "SESSION_GET_STATE Response:"
require_line "SESSION_STOP Response:"

if [[ "${SCENARIO}" == "ranging_stream" ]]; then
    require_line "RANGE_DATA_NTF (SESSION_INFO_NTF):"
    require_line "Measurement Count"
fi

echo "PASS: shell TCP simulator integration"
