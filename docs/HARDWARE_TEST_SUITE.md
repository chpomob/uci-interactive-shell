# Hardware Integration Test Suite

This repository includes an opt-in integration binary to validate the UCI stack against a real UWB controller.

## Binary

- Target: `test_hardware_integration`
- Source: `tests/test_hardware_integration.c`
- Build and run: `make hardware-integration-test`
- Minimal CLI smoke script: `hardware_acceptance_smoke.sh`
- Build and run: `UCI_HW_DEVICE=/dev/ttyUSB0 make hardware-acceptance-smoke`

If no hardware path is provided, the binary prints `SKIPPED` and exits `0` so it is safe in CI.

## Environment

- `UCI_HW_DEVICE` (required): character device path such as `/dev/ttyUSB0`.
- `UCI_HW_TIMEOUT_MS` (optional): response timeout in milliseconds, default `1500`.
- `UCI_HW_VERBOSE` (optional): set to `1` for verbose transport logs.
- `UCI_HW_INCLUDE_RESET` (optional): set to `1` to run destructive `CORE_DEVICE_RESET` validation.
- `UCI_HW_SCRIPT_TIMEOUT_SEC` (optional): CLI smoke timeout in seconds, default `20`.
- `UCI_HW_SESSION_ID` (optional): smoke-script session ID, default `1`.
- `UCI_HW_SESSION_TYPE` (optional): smoke-script session type, default `fira_ranging`.
- `UCI_HW_INCLUDE_SESSION_START` (optional): include `session_start` in the smoke script.

Example:

```bash
UCI_HW_DEVICE=/dev/ttyUSB0 UCI_HW_TIMEOUT_MS=2500 make hardware-integration-test
```

## Coverage

The suite validates:

1. Transport init and link connectivity.
2. `CORE_DEVICE_INFO` response routing and status.
3. `CORE_GET_CAPS_INFO` response routing and status.
4. `CORE_GET_CONFIG` for `DEVICE_STATE` response routing and status.
5. Optional `CORE_DEVICE_RESET` status (gated by `UCI_HW_INCLUDE_RESET=1`).

The smoke script validates the minimal user-facing bring-up path:

1. `mode_hw <device>`
2. `get_device_info`
3. `get_caps_info`
4. `session_init`
5. `get_session_state`
6. `session_deinit`

`session_start` is available as an opt-in step instead of the default because
successful start usually depends on a device/profile-specific app-config
baseline that should be pinned separately from transport bring-up.

Implementation details:

- Sends UCI commands via `uci_hw_interface_send_command`.
- Waits for matching `RESPONSE` packets and tolerates interleaved notifications.
- Retries on `UCI_STATUS_COMMAND_RETRY` up to three attempts.
- Verifies header coherence (MT/GID/OID/payload length) and known status codes.

## Recommended Bring-Up Order

1. Confirm permissions (`dialout` group or equivalent).
2. Run non-destructive suite first:
   `UCI_HW_DEVICE=/dev/ttyUSB0 make hardware-integration-test`
3. Run the CLI smoke flow once transport-level checks pass:
   `UCI_HW_DEVICE=/dev/ttyUSB0 make hardware-acceptance-smoke`
4. Enable `session_start` only when the target profile is configured:
   `UCI_HW_DEVICE=/dev/ttyUSB0 UCI_HW_INCLUDE_SESSION_START=1 make hardware-acceptance-smoke`
5. Enable reset test only after baseline pass:
   `UCI_HW_DEVICE=/dev/ttyUSB0 UCI_HW_INCLUDE_RESET=1 make hardware-integration-test`
