# Hardware Integration Test Suite

This repository includes an opt-in integration binary to validate the UCI stack against a real UWB controller.

## Binary

- Target: `test_hardware_integration`
- Source: `tests/test_hardware_integration.c`
- Build and run: `make hardware-integration-test`

If no hardware path is provided, the binary prints `SKIPPED` and exits `0` so it is safe in CI.

## Environment

- `UCI_HW_DEVICE` (required): character device path such as `/dev/ttyUSB0`.
- `UCI_HW_TIMEOUT_MS` (optional): response timeout in milliseconds, default `1500`.
- `UCI_HW_VERBOSE` (optional): set to `1` for verbose transport logs.
- `UCI_HW_INCLUDE_RESET` (optional): set to `1` to run destructive `CORE_DEVICE_RESET` validation.

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

Implementation details:

- Sends UCI commands via `uci_hw_interface_send_command`.
- Waits for matching `RESPONSE` packets and tolerates interleaved notifications.
- Retries on `UCI_STATUS_COMMAND_RETRY` up to three attempts.
- Verifies header coherence (MT/GID/OID/payload length) and known status codes.

## Recommended Bring-Up Order

1. Confirm permissions (`dialout` group or equivalent).
2. Run non-destructive suite first:
   `UCI_HW_DEVICE=/dev/ttyUSB0 make hardware-integration-test`
3. Enable reset test only after baseline pass:
   `UCI_HW_DEVICE=/dev/ttyUSB0 UCI_HW_INCLUDE_RESET=1 make hardware-integration-test`
