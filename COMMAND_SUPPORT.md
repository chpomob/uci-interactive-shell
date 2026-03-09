# UCI Command Support Overview

**Hardware-first focus:** This project centers on robust UCI control of real hardware; the simulator only supports protocol validation and debugging.

This CLI now simulates a subset of the Android UCI command surface to unblock
host tooling. The following control flows are explicitly handled:

- **CORE**: device info, capability query, set/get config, device reset,
  suspend, and UWBS timestamp query.
- **SESSION CONFIG**: session init/deinit, set/get app config, get count/state,
  update controller multicast list, DT-Tag active rounds, data transfer phase
  configuration, hybrid controller/controlee configuration, and data-size
  queries.
- **SESSION CONTROL**: start/stop sessions, ranging-count queries, logical link
  creation/closure/get-param flow, and simulated UWBS create/close
  notifications.
- **VENDOR ANDROID**: country code, power stats, and radar config commands.
- **TEST**: RF test configuration, periodic TX, PER RX, generic RX, and stop
  commands.

Key limitations to keep in mind:

- Data packets are not transported; data/control notifications are mocked only
  when explicitly emitted inside `send_uci_command`.
- Device configuration TLVs preserve the byte lengths supplied by the host;
  validation still focuses on known IDs rather than full schema checks.
- Session handles are synthesized locally and do not survive resets.
- Unsupported commands fall back to `UCI_STATUS_OK` without side effects.

When adding new features, ensure the corresponding command path returns a
well-formed status payload and, where practical, validates incoming buffers to
match the patterns documented in the Android UCI manager.

## Declarative Command Framework

The CLI is now driven by declarative command definitions stored in
`src/uci_cmd_framework_{bridge,device,session,simulation}.c`. Each entry
describes the command name, aliases, help text, parameter metadata, and hardware
requirements. `uci_cli.c` and `uci_cli_completion.c` read those tables through
`uci_cmd_framework_bridge.c`, while `uci_command_framework.c` enforces the
parameter validation rules before handing control to the command handlers in
`src/uci_cmd_*`.

## Protocol Definition Policy

- `include/uci_pdl.h` is the authoritative source for standard UCI constants in
  this repository. Those values are kept aligned with Android UWB definitions.
- `include/uci_opcode_constants.h` is the authoritative source for Qorvo and
  Android vendor opcode values. Qorvo values follow the QM SDK.
- Shared semantic helpers in `uci_packet_utils` are the preferred way to decode
  protocol enums in CLI/analyzer code, so repeated status, device-state,
  session-state, and session-reason switch blocks do not drift apart.
- Shared plain-text decode helpers in `uci_decode_utils` now own the
  human-readable status/state printing used by `uci.c` and
  `uci_packet_analyzer.c`.
- `send_uci_command()` now constructs one canonical control packet and feeds
  that packet into simulation or hardware transport, instead of rebuilding
  command headers separately in each path.
- New decoding or command logic must use those named constants instead of local
  literals. `tests/test_protocol_definitions.c` and
  `tests/test_protocol_fixtures.c` exist to catch drift.
- `tests/test_transport_parity.c` verifies that representative handlers emit
  byte-identical control packets in simulation mode and hardware mode before
  any device response is processed.

### Migration Status
- Help output, readline completion, and the `help` command itself all read from
  the shared definitions. Updating a command name/alias instantly propagates to
  discovery, help text, and validation.
- Device/session/hardware commands now rely on typed handlers in
  `src/uci_cmd_core_typed.c`, `src/uci_cmd_hardware_typed.c`, and
  `src/uci_cmd_handlers_session.c`. The legacy `src/uci_cmd_handlers.c`
  bridge—previously responsible for argc/argv parsing—has been deleted.
- General utilities such as `analyze_packet` and `show_*_configs` use the typed
  framework, enabling richer option parsing (e.g., `filter=...`, `id=...`,
  `detail=summary|full`) while retaining backward-compatible `--filter`/`--id`
  flags.
- Core typed handlers now consume validated framework parameters for
  `set_power`, `get_config`, `set_config`, and `validate_arguments` instead of
  reparsing raw `argv` values.
- Session lifecycle, data-transfer, and multicast handlers consume the
  validated values supplied by the framework. For example,
  `session_update_dt_tag_rounds` accepts either individual integers or a single
  hex blob, logical-link identifiers are parsed as bytes (0-255), and
  application configuration setters reuse the typed session ID that was already
  checked at dispatch time.
