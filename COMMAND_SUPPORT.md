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
  this repository. Those values are kept aligned with Android UWB definitions
  and cross-checked against the Cherry C headers shipped in the local Qorvo
  SDK tree.
- `include/uci_opcode_constants.h` is the authoritative source for Qorvo and
  Android vendor opcode values. Qorvo values follow the QM SDK.
- For `GID 0x0E`, this repository now follows the Cherry C-header assignment
  `QORVO_MAC`. The Python Qorvo tools still expose `ConfigManager` at the same
  numeric value; that overlap is treated as a documented SDK inconsistency, not
  as this repository's public protocol basis.
- Shared semantic helpers in `uci_packet_utils` are the preferred way to decode
  protocol enums in CLI/analyzer code, so repeated status, device-state,
  session-state, and session-reason switch blocks do not drift apart.
- The shared `uci_packet_utils` lookup tables now also provide the label and
  description metadata used by `uci_ui_packet_decoder.c`, replacing the older
  private UI copies of status/device/session/data-transfer interpretation.
- Shared plain-text decode helpers in `uci_decode_utils` now own the
  human-readable status/state printing used by `uci.c` and
  `uci_packet_analyzer.c`.
- `src/uci_plain_decoders.c` now uses the same shared decode helpers for UCI
  status and data-transfer status output, replacing most of the local
  response-status switch blocks that previously lived in that file.
- Plain response/notification payload decoders now live in
  `src/uci_plain_decoders.c`, so `uci.c` no longer carries the full plain-text
  decoder block alongside simulation and transport logic.
- Dead simulation shim files (`src/uci_cmd_simulation.c` and the unused
  `include/uci_cmd_simulation.h`) have been removed; the maintained simulation
  command surface is `src/uci_cmd_handlers_simulation.c` plus
  `src/uci_cmd_framework_simulation.c`.
- The framework wrapper adapter layer has been removed. Session and simulation
  command definitions now call typed handlers directly instead of routing
  through compatibility macros that rebuilt old `argc/argv` semantics.
- `send_uci_command()` now constructs one canonical control packet and feeds
  that packet into simulation or hardware transport, instead of rebuilding
  command headers separately in each path.
- New decoding or command logic must use those named constants instead of local
  literals. `tests/test_protocol_definitions.c` and
  `tests/test_protocol_fixtures.c` exist to catch drift.
- `tests/test_transport_parity.c` verifies that representative handlers emit
  byte-identical control packets in simulation mode and hardware mode before
  any device response is processed.
- `tests/test_protocol_definitions.c` now also pins the exported lookup-table
  labels/descriptions that back analyzer and UI semantic output.
- `tests/test_uci_functions.c` now exercises `uci_analyze_packet_core()` with a
  real `SESSION_INFO_NTF` packet, closing the gap where decoder helper tests
  could pass while the analyzer dispatch table still contained dead branches.
- The active decode surface now follows Cherry for `SESSION_CONTROL +
  SESSION_INFO_NTF`: the packet is presented as `RANGE_DATA_NTF
  (SESSION_INFO_NTF)` rather than as a separate session-info-only notification
  family, while the wire opcode remains `UCI_OID_SESSION_INFO`. The decoded
  header now follows Cherry's client handler more closely: the second 32-bit
  field is shown as session handle, and the fixed header includes the
  `measurement_count` byte at offset 24.
- Shared header helpers now also follow Cherry's packet-length rules: control
  packets keep their 8-bit payload length in byte 3, while `DATA` packets use
  bytes 2-3 as a 16-bit little-endian payload length. `uci_send_data_message()`
  now accepts payloads up to the UCI 16-bit length limit while still emitting
  255-byte transport/log fragments locally.
- The hardware transport path now preserves Cherry-style `DATA` fragment
  behavior on receive: `DATA` packets are returned packet-by-packet even when
  `PBF=NOT_COMPLETE`, instead of being combined through the non-DATA
  reassembly buffer. The same transport layer still builds 255-byte outbound
  `DATA` wire fragments for larger payloads.
- Segmented non-`DATA` messages now reassemble in `parse_uci_packet()` rather
  than in the hardware transport layer. That keeps the transport path close to
  raw Cherry wire behavior while still giving analyzer/plain/UI decoders a
  single logical control packet once all fragments arrive.
- Standard command handlers now also support a dedicated TCP transport mode for
  the external simulator project. Packet construction remains shared, and the
  transport decision happens only after the canonical UCI packet has been
  built.
- `tests/test_analyzer_dispatch.c` now covers representative live dispatch for
  `COMMAND`, `RESPONSE`, and `NOTIFICATION` packets in the CORE,
  `SESSION_CONFIG`, and `SESSION_CONTROL` families, and now also asserts the
  family-specific fallback strings for unsupported CORE, SESSION, and Android
  opcodes so dead branches and accidental generic fallbacks are visible. It
  also audits the top-level branch signatures in `src/uci_packet_analyzer.c`
  so duplicate `MT/GID` routes are treated as a test failure instead of
  waiting for runtime symptoms.
- `tests/test_uci_functions.c` now compares plain and UI decoder output for
  representative `CORE_DEVICE_INFO_RSP`, `SESSION_STATUS_NTF`, and
  `SESSION_INFO_NTF` payloads, enforcing shared semantic labels and values even
  when the UI path uses richer formatting.
- `tests/test_command_framework_validation.c` now dispatches
  `simulate_notification`, `simulate_session_status`, and `simulate_ranging`
  through the real framework and asserts that the analyzer output matches the
  emitted notification family. That gives you an end-to-end check from command
  parsing to simulated packet construction to decode output.
- `tests/test_analyzer_dispatch.c` now also covers malformed packets at the
  analyzer boundary. It asserts short-header rejection, truncated-payload
  clamping, and that decoders receive the clamped payload length instead of the
  stale header length.
- `tests/test_cherry_alignment.c` reads the local Cherry headers and Cherry
  client sources directly so standard FiRa GIDs/OIDs, Qorvo `EXT2` opcodes,
  Cherry `QORVO_MAC`/`QORVO_CALIB` assignments, and the `SESSION_CONTROL` +
  `SESSION_INFO` wire mapping cannot silently drift away from the checked-in
  SDK sources. The same suite now also asserts Cherry's
  `uci_rsp_range_data_ntf_handler` binding for that packet family.
- `tests/test_uci_functions.c` now also pins Cherry-style `DATA` header
  encoding/decoding and verifies that the data-message builder/send path
  preserves payloads above 255 bytes instead of falling back to the old
  control-packet length ceiling.
- `tests/test_hw_fragmentation.c` now exercises the real hardware-interface
  fragmentation helpers directly, pinning Cherry-style 255-byte outbound
  `DATA` fragmentation and inbound `DATA` fragment passthrough without relying
  on a real device.
- `tests/test_uci_functions.c` now also exercises parser-level segmented
  control-message reassembly and mismatch handling, so full multi-fragment
  control packets are validated separately from `DATA` transport behavior.

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
- Simulation helpers now follow the same typed-dispatch model, so
  `simulate_notification`, `simulate_session_status`, and the vendor simulation
  helpers no longer carry a separate raw-argv adapter path.
