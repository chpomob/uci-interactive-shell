#!/bin/bash

set -euo pipefail

DEVICE_PATH="${1:-${UCI_HW_DEVICE:-}}"
TIMEOUT_SECONDS="${UCI_HW_SCRIPT_TIMEOUT_SEC:-20}"
SESSION_ID="${UCI_HW_SESSION_ID:-1}"
SESSION_TYPE="${UCI_HW_SESSION_TYPE:-fira_ranging}"
INCLUDE_SESSION_START="${UCI_HW_INCLUDE_SESSION_START:-0}"

if [[ -z "${DEVICE_PATH}" ]]; then
    echo "=== Hardware acceptance smoke ==="
    echo "SKIPPED: set UCI_HW_DEVICE=/dev/ttyUSB0 or pass the device path as the first argument."
    exit 0
fi

make uci-shell

RAW_LOG="$(mktemp)"
CLEAN_LOG="$(mktemp)"

cleanup() {
    rm -f "${RAW_LOG}" "${CLEAN_LOG}"
}
trap cleanup EXIT

COMMANDS=(
    "mode_hw ${DEVICE_PATH}"
    "get_device_info"
    "get_caps_info"
    "session_init ${SESSION_ID} ${SESSION_TYPE}"
    "get_session_state ${SESSION_ID}"
)

if [[ "${INCLUDE_SESSION_START}" == "1" ]]; then
    COMMANDS+=(
        "session_start ${SESSION_ID}"
        "get_session_state ${SESSION_ID}"
    )
fi

COMMANDS+=(
    "session_deinit ${SESSION_ID}"
    "quit"
)

printf "=== Hardware acceptance smoke ===\n"
printf "Device: %s\n" "${DEVICE_PATH}"
printf "Session ID: %s\n" "${SESSION_ID}"
printf "Session type: %s\n" "${SESSION_TYPE}"
printf "Include session_start: %s\n" "${INCLUDE_SESSION_START}"

{
    for command in "${COMMANDS[@]}"; do
        printf '%s\n' "${command}"
    done
} | timeout "${TIMEOUT_SECONDS}s" ./uci-shell >"${RAW_LOG}" 2>&1

sed -r 's/\x1B\[[0-9;]*[[:alpha:]]//g' "${RAW_LOG}" >"${CLEAN_LOG}"

for forbidden in \
    "No response received from hardware" \
    "Failed to initialize hardware mode" \
    "Failed to send command to hardware" \
    "Error receiving response from hardware"; do
    if grep -Fq "${forbidden}" "${CLEAN_LOG}"; then
        echo "FAILED: found '${forbidden}'"
        cat "${CLEAN_LOG}"
        exit 1
    fi
done

for required in \
    "Hardware mode enabled with device: ${DEVICE_PATH}" \
    "CORE_DEVICE_INFO Response:" \
    "CORE_GET_CAPS_INFO Response:" \
    "SESSION_INIT Response:" \
    "SESSION_GET_STATE Response:" \
    "SESSION_DEINIT Response:"; do
    if ! grep -Fq "${required}" "${CLEAN_LOG}"; then
        echo "FAILED: missing '${required}'"
        cat "${CLEAN_LOG}"
        exit 1
    fi
done

if [[ "${INCLUDE_SESSION_START}" == "1" ]]; then
    if ! grep -Fq "SESSION_START Response:" "${CLEAN_LOG}"; then
        echo "FAILED: missing 'SESSION_START Response:'"
        cat "${CLEAN_LOG}"
        exit 1
    fi
fi

echo "RESULT: HARDWARE ACCEPTANCE SMOKE PASSED"
