# Project Status – 2026-03-08

## Current Direction

The project is in consolidation mode. The command framework is the primary
execution path, and changes are currently limited to:

- centralizing authoritative UCI definitions
- removing transitional handler behavior that reparses CLI arguments
- enforcing those assumptions with automated tests

## Protocol Source Of Truth

- Standard UCI constants in `include/uci_pdl.h` follow Android UWB definitions.
- Vendor-specific opcode constants in `include/uci_opcode_constants.h` follow
  Qorvo QM SDK values, with Android vendor opcodes kept alongside them.
- Code should use named constants from those headers instead of embedding local
  opcode literals.

## Validated Improvements

- Core typed handlers in `src/uci_cmd_core_typed.c` now consume validated parsed
  parameters for `set_power`, `get_config`, `set_config`, and
  `validate_arguments`.
- Vendor packet analysis in `src/uci_packet_analyzer_vendor.c` now references
  centralized Qorvo opcode constants instead of local numeric literals.
- `tests/test_protocol_definitions.c` pins the current Android/Qorvo constant
  mappings and checks command metadata plus representative typed dispatch flows.
- Shared enum helpers now cover device state, status, session state, session
  reason, and session type decoding in the plain CLI/analyzer paths, reducing
  duplicated protocol switch statements without collapsing the richer UI lookup
  tables.
- Shared protocol lookup tables in `src/uci_packet_utils.c` now also back the
  UI packet decoder labels/descriptions for status, device state, session
  state, session reason, and data-transfer status, so those semantics have one
  maintained source.
- Shared plain decode-output helpers now back the `uci.c` and
  `uci_packet_analyzer.c` status/state reporting paths, so those two modules no
  longer maintain separate textual protocol semantics.
- The plain response decoder now reuses those shared decode helpers for UCI
  status and data-transfer status output, which removes most of the repeated
  status-label switch blocks from `src/uci_plain_decoders.c`.
- Analyzer dispatch now has a direct regression check for
  `SESSION_CONTROL + SESSION_INFO_NTF`, which guards against stale duplicate
  branches in `uci_packet_analyzer.c` that helper-only decoder tests would not
  catch.
- A dedicated table-driven analyzer dispatch suite now runs representative live
  packets through `uci_analyze_packet_core()` for CORE and SESSION command,
  response, and notification paths, and now also verifies the expected
  family-specific fallback output for unsupported CORE, SESSION, and Android
  opcodes. Android response routing is still only covered through the generic
  analyzer fallback path, which is now an explicit tested behavior rather than
  an unexamined gap.
- Plain response/notification payload decoding now lives in
  `src/uci_plain_decoders.c`, which trims decoder responsibility out of
  `uci.c` and adds a direct regression check for the plain session-state path.
- Dead simulation shim files (`src/uci_cmd_simulation.c` and the unused
  `include/uci_cmd_simulation.h`) have been deleted so the simulation command
  path now has one obvious implementation route.
- The framework wrapper adapter layer has been removed. Session and simulation
  command tables now dispatch straight to typed handlers, so validated
  framework parameters are the only command-entry model left in active code.
- Control-command transport now builds a canonical packet once in
  `send_uci_command()` and hands that packet to either simulation or hardware,
  reducing header/payload drift across transport paths.
- Fixture-driven protocol tests now pin representative CORE commands/responses
  and SESSION notifications at the exact byte level before hardware bring-up.
- A new transport parity suite now runs representative handlers through both
  simulation mode and a stubbed hardware transport to prove the emitted command
  bytes match before real-device testing.
- A minimal real-device acceptance script now exercises the CLI hardware flow
  for `mode_hw`, `get_device_info`, `get_caps_info`, `session_init`,
  `get_session_state`, and `session_deinit`, with `session_start` kept opt-in
  until a target app-config baseline is pinned.
- `make test` now runs the canonical non-hardware regression suites, including
  the protocol definition suite.

## Next Consolidation Targets

1. Collapse remaining literal protocol values in decoders and response builders
   into shared named constants.
2. Expand hardware-integration assertions around vendor notifications and
   session lifecycle once the target device profile is locked.
