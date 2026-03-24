# Project Status – 2026-03-08

## Current Direction

The project is in consolidation mode. The command framework is the primary
execution path, and changes are currently limited to:

- centralizing authoritative UCI definitions
- removing transitional handler behavior that reparses CLI arguments
- enforcing those assumptions with automated tests

## Protocol Source Of Truth

- Standard UCI constants in `include/uci_pdl.h` follow Android UWB definitions.
- The same header now also pins Qorvo vendor-group values against the Cherry C
  headers under `uci_analysis/uwb/Samples/Cherry/uci/uci_core/include/uci`,
  including the explicit choice that `GID 0x0E` means `QORVO_MAC`.
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
- `tests/test_cherry_alignment.c` now reads the local Cherry headers and client
  sources directly, enforcing that the currently supported standard FiRa and
  Qorvo `EXT2` definitions still match the SDK sources and that the repository
  keeps its documented `GID 0x0E` basis choice explicit even though the Python
  Qorvo tools expose `ConfigManager` at that same value.
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
- The active decoder surface for `SESSION_CONTROL + SESSION_INFO_NTF` now
  follows Cherry's `range_data_ntf` model: the packet is still the same wire
  opcode, but analyzer/plain/UI output presents it as
  `RANGE_DATA_NTF (SESSION_INFO_NTF)` and no longer treats byte 8 as an
  `RCR` role field. The remaining plain/core fallback decoders now also use the
  Cherry 25-byte fixed header and label the second 32-bit field as session
  handle rather than a local session token.
- Shared packet-header helpers now follow Cherry's message-type-specific length
  semantics: control packets keep an 8-bit payload length, while `DATA`
  packets use a 16-bit little-endian length in bytes 2-3. The analyzer and
  hardware character-device receive path now decode that 16-bit `DATA` length
  correctly instead of truncating it back to 8 bits, and
  `uci_send_data_message()` no longer rejects payloads above the old 255-byte
  control-packet ceiling.
- The hardware transport path now matches Cherry more closely for normal
  `DATA` packet runtime flow: outbound packets still split into 255-byte wire
  fragments, but inbound `DATA` fragments are no longer reassembled through the
  non-DATA fragment buffer. They are forwarded packet-by-packet so MAC/data
  consumers can observe the original fragment boundaries.
- Segmented non-`DATA` runtime support is now present end-to-end. Hardware-mode
  receive loops drain raw fragments, `parse_uci_packet()` owns reassembly for
  control/response/notification fragments, and the analyzer now accepts a
  decoded header-plus-payload view so reassembled packets go through the same
  decode path without fabricating fake on-wire payload bytes.
- The analyzer/UI decoder seam is now const-correct: UI decoder entrypoints
  take read-only payload buffers, which removes the previous compiler warnings
  about discarded const qualifiers and makes the decode contract explicit.
- A dedicated table-driven analyzer dispatch suite now runs representative live
  packets through `uci_analyze_packet_core()` for CORE and SESSION command,
  response, and notification paths, and now also verifies the expected
  family-specific fallback output for unsupported CORE, SESSION, and Android
  opcodes. The same suite now audits the analyzer source for unique top-level
  dispatch routes and expected duplicated shared notification cases, which
  blocks the specific class of shadowed branch regressions that previously
  slipped through. Android response routing is still only covered through the
  generic analyzer fallback path, which is now an explicit tested behavior
  rather than an unexamined gap.
- Plain/UI semantic parity is now regression-tested for representative
  `CORE_DEVICE_INFO_RSP`, `SESSION_STATUS_NTF`, and `SESSION_INFO_NTF` payloads.
  Those tests intentionally compare shared labels and values rather than banner
  text, which keeps decoder meaning aligned without forcing identical
  presentation layers.
- Framework-level simulation commands now have end-to-end analyzer coverage.
  `simulate_notification`, `simulate_session_status`, and `simulate_ranging`
  are dispatched through the real command framework in tests, and their
  resulting analyzer output is asserted directly. That closes the gap between
  packet fixtures and the actual simulation command path.
- Malformed analyzer entrypoint behavior is now regression-tested. Short-header
  packets are rejected cleanly, truncated payloads emit a clamp warning, and
  the analyzer now updates the decoder-visible payload length after clamping so
  truncated packets cannot be decoded using stale header lengths.
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
- The unit suite now also pins Cherry-style `DATA` header encoding/decoding and
  a `DATA_MESSAGE_SND` generation path with an application payload above
  255 bytes, so the repository cannot silently fall back to 8-bit-only data
  packet behavior.
- A dedicated hardware-fragmentation suite now exercises the transport
  fragmentation helpers directly, proving both Cherry-style 255-byte outbound
  `DATA` chunking and inbound `DATA` fragment passthrough.
- The unit suite now also covers segmented non-`DATA` parser behavior,
  including successful multi-fragment `SESSION_CONTROL` reassembly and
  deliberate final-fragment mismatch handling.
- A new transport parity suite now runs representative handlers through both
  simulation mode and a stubbed hardware transport to prove the emitted command
  bytes match before real-device testing.
- External transport is now explicit rather than hardware-only. The shell has
  separate simulation, hardware chardev, and TCP simulator modes, while packet
  construction still flows through the same canonical builder path.
- A minimal real-device acceptance script now exercises the CLI hardware flow
  for `mode_hw`, `get_device_info`, `get_caps_info`, `session_init`,
  `get_session_state`, and `session_deinit`, with `session_start` kept opt-in
  until a target app-config baseline is pinned.
- An opt-in TCP simulator integration target now launches the sibling
  `uci_device_simulator` binary and validates a real `mode_tcp` shell session
  end-to-end, including `CORE_DEVICE_RESET`, `CORE_QUERY_UWBS_TIMESTAMP`,
  `CORE_SET_CONFIG`, `CORE_GET_CONFIG`, `SESSION_SET_APP_CONFIG`, and
  `SESSION_GET_APP_CONFIG`, without turning cross-repo network tests into part
  of default `make test`.
  The current scripted coverage now exercises `device_state`,
  `low_power_mode`, `device_pan_id`, `CORE_QUERY_UWBS_TIMESTAMP`,
  `SESSION_QUERY_DATA_SIZE_IN_RANGING`, `ranging_round_usage`,
  `sts_config`, `channel_number`, `no_of_controlee`,
  `device_mac_address`, `dst_mac_address`, `slot_duration`,
  `ranging_duration`, `sts_index`, `mac_fcs_type`,
  `ranging_round_control`, `aoa_result_req`, `rng_data_ntf`,
  `rng_data_ntf_proximity_near`, `rng_data_ntf_proximity_far`,
  `result_report_config`, `rframe_config`, `rssi_reporting`,
  `preamble_code_index`, `sfd_id`, `psdu_data_rate`,
  `preamble_duration`, `link_layer_mode`, `data_repetition_count`,
  `ranging_time_struct`, `slots_per_rr`, `tx_adaptive_payload_power`,
  `rng_data_ntf_aoa_bound`, `responder_slot_index`, `prf_mode`,
  `cap_size_range`, `tx_jitter_window_size`, `scheduled_mode`,
  `key_rotation`, `key_rotation_rate`, `session_priority`,
  with `ranging_round_usage` now aligned to the FiRa/Cherry round-usage enum
  instead of the older local `ranging/data` shorthand, and `sts_config` now
  aligned to the five-value Cherry/FIra STS enum instead of the older local
  shortened mapping. `ranging_time_struct` is now conservatively constrained to
  `0..1` on the CLI/simulator path because the local Qorvo/Cherry sources only
  prove block-based scheduling as the concrete non-RFU time-structure concept.
  `data_repetition_count` now also drives repeated
  application-data transfer progression in the TCP simulator path.
  `slots_per_rr` is now constrained to `1..255`, the TCP integration now
  exercises the invalid `0` case, and emitted range-data slot indices now
  follow the configured session responder-slot state instead of staying fixed.
  `scheduled_mode` now exposes the Cherry/FiRa three-value enum on the CLI
  (`CONTENTION_BASED`, `TIME_SCHEDULED`, `HYBRID`), while the default simulator
  profile intentionally accepts only `TIME_SCHEDULED` because that is the only
  scheduling mode it currently models. `cap_size_range` is now pinned against
  the simulator’s typed min/max validation path too: the TCP integration first
  exercises an invalid non-zero contention-style value, then restores the
  default neutral `0x0000` baseline used by the time-scheduled profile.
  `device_type`
  and `device_role` are also now aligned to
  the FiRa/Cherry controller/controlee and responder/initiator semantics
  instead of the older swapped local labels. `multi_node_mode` is now also
  aligned to the FiRa/Cherry `UNICAST`, `ONE_TO_MANY`, and `MANY_TO_MANY`
  topology semantics instead of the older local `anycast` / `multicast`
  labels, while keeping those older tokens as CLI compatibility aliases.
  `vendor_id`, `static_sts_iv`, `number_of_sts_segments`,
  `max_rr_retry`, `uwb_initiation_time`,
  `in_band_termination_attempt_count`, `sub_session_id`, `bprf_phr_data_rate`,
  `max_number_of_measurements`, `ul_tdoa_tx_interval`,
  `ul_tdoa_random_window`, `sts_length`, `suspend_ranging_rounds`,
  `ul_tdoa_ntf_report_config`, `ul_tdoa_device_id`, `ul_tdoa_tx_timestamp`,
  `dl_tdoa_anchor_cfo`, `dl_tdoa_anchor_location`,
  `dl_tdoa_tx_active_ranging_rounds`, `dl_tdoa_block_striding`,
  `dl_tdoa_time_reference_anchor`, `min_frames_per_rr`, `mtu_size`, `inter_frame_interval`,
  `dl_tdoa_ranging_method`, `mac_address_mode`,
  `hopping_mode`, `block_stride_length`, `dl_tdoa_tx_timestamp_conf`,
  `dl_tdoa_hop_count`, `session_key`, `subsession_key`,
  `session_data_transfer_status_ntf_config`, `session_time_base`,
  `dl_tdoa_responder_tof`, `secure_ranging_nefa_level`,
  `secure_ranging_csw_length`, `application_data_endpoint`,
  `owr_aoa_measurement_ntf_period`,
  `SESSION_UPDATE_CONTROLLER_MULTICAST_LIST`, and
  `SESSION_UPDATE_ACTIVE_ROUNDS_DT_TAG`, and
  `SESSION_DATA_TRANSFER_PHASE_CONFIG` over the TCP simulator path.
  `block_stride_length` is now also exercised with the same scheduler
  constraint the simulator enforces: non-zero stride is rejected until
  block-based, time-scheduled operation is configured, then accepted and
  returned by `get_app_config`.
  `responder_slot_index` now follows the same model: the TCP path first checks
  the invalid case where the chosen responder slot falls outside the configured
  `slots_per_rr`, then verifies the accepted in-range value.
  The same TCP coverage now treats `get_app_config` as a broader retrieval
  command: one requested param, multiple requested params, or no requested
  params meaning "return all stored supported app-config TLVs", with explicit
  assertions for the expanded default-profile TLV set. That zero-count/all path
  now also validates segmented control-response reassembly, because the full
  standard app-config surface no longer fits in a single 8-bit-length UCI
  control packet.
  The same TCP path now also validates `DATA_MESSAGE_SND` delivery to the
  simulator and the resulting `SESSION_DATA_CREDIT_NTF` plus
  `SESSION_DATA_TRANSFER_STATUS_NTF` decode path.
  It now also covers the logical-link lifecycle end to end:
  `SESSION_LOGICAL_LINK_CREATE`, `SESSION_LOGICAL_LINK_GET_PARAM`,
  `SESSION_LOGICAL_LINK_CLOSE`, and the corresponding UWBS notifications.
  The same script can now be run with `UCI_TCP_SIM_SCENARIO=ranging_stream` to
  assert that the shell decodes a Cherry-aligned
  `RANGE_DATA_NTF (SESSION_INFO_NTF)` stream emitted by the simulator and that
  `SESSION_STOP` suppresses any later range-data packets.
  The same path now also aligns the validated `rng_data_ntf` behavior:
  `0x00` suppresses range-data notifications, `0x01` enables them, `0x02`
  emits while the simulated distance stays within the configured proximity
  window, and `0x05` emits only on proximity enter/leave transitions. The
  AoA-dependent modes remain stored and decodable, but are not yet enforced by
  the simulator runtime. `RESULT_REPORT_CONFIG` is now also behavioral on that
  simulator path while keeping the packet layout stable: with the currently
  scripted `result_report_config=7`, AoA values remain visible but AoA FoM
  fields are suppressed, which the shell integration now pins through the
  decoded `FoM 0%` output. The same path now also pins the current
  Qorvo-like validation rule for that parameter: values using bits outside the
  documented low four report flags are rejected with `INVALID_PARAM`.
  `RANGING_INTERVAL` is now also behavioral on that
  path: the simulator uses the session `ranging_duration` app-config both for
  the emitted `Current Ranging Interval` field and for future ranging-stream
  timing, including the first post-start range-data notification. The same TCP
  path now also pins interval validation: `ranging_duration` values below the
  current Qorvo-like profile minimum (`50 ms`) are rejected with
  `INVALID_RANGE`.
- `make test` now runs the canonical non-hardware regression suites, including
  the protocol definition suite.

## Next Consolidation Targets

1. Collapse remaining literal protocol values in decoders and response builders
   into shared named constants.
2. Expand hardware-integration assertions around vendor notifications and
   session lifecycle once the target device profile is locked.
- TCP simulator interoperability now expects Cherry-compatible `SESSION_SET_APP_CONFIG` success responses (`status + zero config-status entries`) instead of the older per-config success list.
