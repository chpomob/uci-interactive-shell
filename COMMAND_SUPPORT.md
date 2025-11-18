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
parameter validation rules before handing control to the legacy handlers in
`src/uci_cmd_*`.

### Migration Status
- Help output and readline completion already use the shared definitions, so any
  change to a command name/alias instantly propagates.
- Most handlers still rely on transitional wrappers in
  `src/uci_cmd_core_new.c` (and similar files) that read from `argv` instead of
  the parsed values. The next step is to migrate handlers to typed parameters so
  the legacy parsing code can be deleted.
- Session lifecycle, data-transfer, and multicast handlers now consume the
  validated values supplied by the framework. For example,
  `session_update_dt_tag_rounds` accepts either individual integers or a single
  hex blob, logical-link identifiers are parsed as bytes (0-255), and
  application configuration setters reuse the typed session ID that was already
  checked at dispatch time.
