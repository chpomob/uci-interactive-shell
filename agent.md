# Agent Guide: UCI Interactive Shell

## Project Snapshot
- **Purpose**: Interactive CLI for exercising the Android-aligned UCI protocol against simulated or real UWB controllers.
- **Primary language**: C (C11); command-line UX built on GNU Readline.
- **Key entrypoints**: `src/main.c` (CLI loop), `src/uci.c` (protocol core), `src/uci_hw_interface.c` + `src/uci_hw_chardev.c` (hardware transport), `src/uci_config_manager.c` (TLV catalog + storage).
- **Documentation**: `README.md` for user flow, `FINAL_SUMMARY.md` for feature matrix, `COMMAND_SUPPORT.md` for coverage/limitations, `uci_analysis/` for protocol reference notes.

## Build & Test Workflow
- `make` builds `uci-shell` plus unit/config/session/hw-interface test binaries.
- Focused targets:
  - `make unit-test`
  - `make config-test`
  - `make session-manager-test`
  - `make hw-interface-test`
- Unit harness prints to stdout; capture regressions by grepping for `FAILED`.
- Clean artifacts with `make clean` (also removes generated binaries produced by tests).

## Editing Guardrails
- Honor C coding style in repo (no non-ASCII unless required).
- Keep CLI output user-friendly; avoid verbose logging unless behind a verbose flag.
- Multi-byte TLVs are preserved for device config (`CORE_SET/GET_CONFIG`); maintain compatibility with `uci_config_manager` when altering TLV parsing.
- Simulation path should never crash on malformed input—return proper `UCI_STATUS_*` codes and optionally enqueue `CORE_GENERIC_ERROR_NTF`.
- Hardware interactions must tolerate missing devices; default to graceful fallbacks so the shell remains usable without `/dev/tty*` access.

## Architecture Quick Notes
- `send_uci_command` constructs responses inline for simulation mode; ensure new command handling both prints and reuses decoding helpers where possible.
- Session state persists in `uci_sessions` (see `include/uci.h`); helper functions in `uci_functions.c` maintain bookkeeping.
- Config manager (`uci_config_manager.c`) seeds defaults on `uci_config_init` and accepts TLV storage via setter/getter APIs. Prefer reusing these rather than manual arrays.
- Hardware facade provides two layers: legacy FD helpers in `uci_hw.c` and fragment-aware channel in `uci_hw_interface.c`; new work should target the interface layer.

## Testing Expectations
- Extend existing suites when touching protocol logic. `tests/test_uci_functions.c` is the canonical place to exercise simulated command flows.
- Add regression assertions before altering decoding code—most parser bugs are caught via these tests.
- For new TLVs/config paths, cover both happy-path and invalid-parameter cases (ensures status propagation stays correct).

## Common Pitfalls
- Forgetting to bump response payload length when appending TLV data—always update header `payload_len` after writing bytes.
- Mixing `malloc`/`free` with stack allocations inside the CLI loop; prefer stack buffers unless absolutely necessary.
- Neglecting to enqueue notifications after session state changes (see `enqueue_session_status_notification`).
- Attempting to open hardware devices in environments without serial access; wrap new hardware logic with `uci_hw_interface_is_connected` checks.

## Suggested Launch Sequence for Agents
1. Skim `README.md` and `FINAL_SUMMARY.md` for feature intent.
2. Review `src/uci.c` command switch once before editing; many branches share helpers.
3. Run `make unit-test` prior to submission; add targeted tests for new behaviors.
4. Document significant protocol assumptions in `COMMAND_SUPPORT.md` to keep limitations synchronized.

## Helpful References in Tree
- `include/uci_pdl.h`: Enumerations mirroring Android UCI spec.
- `uci_analysis/uci_packet_generator.py`: Python helper for crafting raw packets during testing.
- `HARDWARE_GUIDE.md`: Practical steps for character-device setup.

Keep this file short and focused—add new observations as the code evolves so future agents ramp quickly.
