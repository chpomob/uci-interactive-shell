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

- Core typed handlers in `src/uci_cmd_core_new.c` now consume validated parsed
  parameters for `set_power`, `get_config`, `set_config`, and
  `validate_arguments`.
- Vendor packet analysis in `src/uci_packet_analyzer_vendor.c` now references
  centralized Qorvo opcode constants instead of local numeric literals.
- `tests/test_protocol_definitions.c` pins the current Android/Qorvo constant
  mappings and checks command metadata plus representative typed dispatch flows.
- `make test` now runs the canonical non-hardware regression suites, including
  the protocol definition suite.

## Next Consolidation Targets

1. Remove remaining `_new` transitional naming once the typed handlers fully
   replace the old bridge semantics.
2. Collapse remaining literal protocol values in decoders and response builders
   into shared named constants.
3. Expand hardware-integration assertions around vendor notifications and
   session lifecycle once the target device profile is locked.
