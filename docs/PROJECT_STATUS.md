# Project Status – 2025-11-11

## Repository Snapshot
- Branch: `master`
- Latest commit: `39eac10 Teach CLI to use command definitions` (2025-11-11)
- Remote tracking: `origin/master` (no local divergence reported by `git status -sb`)
- Working tree: clean (`git status -sb` prints only `## master...origin/master`)
- Build/tests: not rerun in this session; execute `make` and the `tests/test_final.sh` wrapper before distributing new binaries.

## Recent Activity
- `39eac10` – connected the interactive shell (`src/uci_cli.c`) and tab completion (`src/uci_cli_completion.c`) to declarative command definitions so every command is resolved from shared metadata.
- `3f0b5c6` – centralized the previously hard-coded CLI command metadata into reusable definition tables (`src/uci_cmd_framework_*.c`).
- `e429505` – introduced the device command bridge that exposes hardware/device definitions through the new framework.
- `1606cd4` – modularized the session command bridge, splitting lifecycle, configuration, and logical-link handlers into framework-managed tables.
- `a56301e` – isolated simulation-only commands behind the same bridge so demos/test hooks share the typed interface.

## Current Focus
The mainline now routes command discovery, help text, and completion through `uci_command_framework.c`. Every command is described in a definition table (`src/uci_cmd_framework_{device,session,simulation}.c`, `src/uci_cmd_framework_bridge.c`), and the CLI uses that metadata to enforce parameter counts, basic type validation, and transport requirements before handing off to the legacy handlers in `src/uci_cmd_*`. Readline completion is also backed by the same tables, which eliminates duplicate command lists and keeps aliases consistent.

## Outstanding Work / Next Tasks
1. **Consume typed parameters in handlers** – many of the framework adapters (for example `handle_set_config_command_new` in `src/uci_cmd_core_new.c`) still index directly into `argv` and ignore the validated values that `uci_command_framework.c` already parses. Gradually update the handlers to accept typed structs or helper contexts so we can delete the legacy parsing logic.
2. **Expand automated coverage for the command framework** – there are no unit tests that load the command tables, iterate over aliases, or exercise `uci_cmd_dispatch`. Add focused tests in `tests/` to guard against missing definitions or incorrect validation ranges.
3. **Document and exercise hardware mode regressions** – the new dispatcher guards hardware-only commands via `CLI_CMD_FLAG_REQUIRES_HW_MODE`, but there are no end-to-end tests that boot the shell in hardware mode (even with a stub transport). Add a loopback harness under `tests/` or `demo_validation` to ensure the guardrails hold.
4. **Prune legacy CLI artifacts** – transitional `.bak` sources (e.g. `src/uci_cmd_session.c.bak`, `src/uci_ui_packet_decoder.c.before_fix`) and unused command tables remain in the tree. Once the framework migration is stable, remove the stale files to avoid confusion.

## Testing Status
No automated test suite or build was run after commit `39eac10` inside this workspace snapshot. To verify before release:
1. `make -j` – ensures the CLI and supporting libraries still compile.
2. `./tests/test_final.sh` – runs the aggregated regression suite (functional/config/session/security cases).
3. Optional: `./uci-shell --help` and a short interactive run in both `mode_sim` and `mode_hw` to confirm the new command metadata is visible in help/completion output.
