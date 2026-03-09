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
- Shared plain decode-output helpers now back the `uci.c` and
  `uci_packet_analyzer.c` status/state reporting paths, so those two modules no
  longer maintain separate textual protocol semantics.
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

1. Continue collapsing transitional seams by removing redundant wrapper layers
   once the typed handlers fully replace the old bridge semantics.
2. Collapse remaining literal protocol values in decoders and response builders
   into shared named constants.
3. Expand hardware-integration assertions around vendor notifications and
   session lifecycle once the target device profile is locked.
