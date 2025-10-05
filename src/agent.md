# Agent Guide: `src/`

## Files of Interest
- `main.c`: Readline-driven CLI loop; handles history, aliases, command routing.
- `uci.c`: Simulation brain—parses/synthesizes UCI packets, maintains session state, dispatches notifications.
- `uci_cli.c` / `uci_cli_completion.c`: History + auto-complete helpers.
- `uci_config_manager.c`: App/device TLV catalog + storage; dependency for config commands.
- `uci_hw*.c`: Hardware transport (legacy FD path vs fragment-aware chardev interface).

## Editing Principles
- Prefer helper functions over lengthy inline logic; `send_uci_command` is already large, so keep new branches compact and reuse shared encoding helpers.
- Always validate payload lengths before reading data; return `UCI_STATUS_INVALID_PARAM` on truncated buffers and emit `CORE_GENERIC_ERROR_NTF` where precedent exists.
- Use the config manager for TLV persistence (device + session configs). Avoid re-implementing storage arrays elsewhere.
- When adding notifications, also update the decoder section (search for `decode_*`) to keep pretty-printing in sync.

## Adding Command Support
1. Define constants in `include/uci_pdl.h` if the opcode/TLV is new.
2. Handle the command in `send_uci_command` (simulation) and optionally `uci_hw_interface` if hardware support is needed.
3. Update decoding/printing helpers so that simulated responses are human-readable.
4. Extend unit tests in `tests/test_uci_functions.c` to cover new behavior.

## Session & Config Helpers
- Use `store_session_config` / `get_session_config` for session TLVs.
- Keep `uci_sessions` consistent: update `session_state`, `is_allocated`, and `session_handle` atomically.
- Device config persistence relies on `uci_config_set_device_param`; ensure new code includes `#include "../include/uci_config_manager.h"` when needed.

## Testing from This Directory
- Rebuild affected objects via `make unit-test` or `make` after edits.
- Run `make clean` to avoid stale objects if you change header dependencies.

## Diagnostics Tips
- Use `uci_hw_interface_set_verbose(1)` (exposed via CLI) to inspect hardware fragments.
- For simulation debugging, temporarily `printf` inside branches but strip them before commit or guard with verbosity flags.

Keep modifications isolated and documented; note any protocol assumptions in `COMMAND_SUPPORT.md`.
