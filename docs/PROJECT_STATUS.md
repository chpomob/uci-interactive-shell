# Project Status – 2025-11-19

## Repository Snapshot
- Branch: `master`
- Latest commit: `74cfd77 Remove legacy session CLI parsing` (2025-11-18)
- Remote tracking: `origin/master` (`git status -sb` shows `## master...origin/master [ahead 7]`)
- Working tree: dirty (local edits in `src/uci.c`, `src/uci_response_core.c`, `src/uci_cmd_handlers.c`, `src/uci_cmd_handlers_session.c`, and this document)
- Build/tests: `make -j`, `./test_command_handlers`, and `./test_command_framework_validation` all ran on 2025-11-19 and passed.

## Recent Activity
- `74cfd77` – deleted the last legacy session CLI parser so every session_* command now executes through typed framework handlers.
- `961477e` – documented the typed session command parameters/aliases to keep the CLI help output aligned with the framework definitions.
- `c3f1dc0` – migrated the session transport commands (start/stop/send_data/etc.) to consume parsed parameters surfaced by `uci_command_framework.c`.
- `6ba33f5` – added typed session-config handlers, wiring each parameterized command into shared TLV parsing utilities.
- `7a4c52f` – switched the original session handlers to reuse typed values, reducing duplicated validation logic.
- _Uncommitted_ – routed all hardware/mode CLI commands through typed handlers (`src/uci_cmd_hardware_new.c`) and deleted the redundant argc/argv shims.
- _Uncommitted_ – moved `help`, `analyze_packet`, and the config-listing commands onto typed handlers, added key=value option parsing for `show_*_configs`, and removed the obsolete `src/uci_cmd_handlers.c`.

## Current Focus
The interactive shell now resolves command metadata exclusively through `uci_command_framework.c`. Session/session-config, hardware/device, and general CLI commands execute via typed handlers (`src/uci_cmd_handlers_session.c`, `src/uci_cmd_hardware_new.c`, `src/uci_cmd_core_new.c`, `src/uci_cmd_framework_bridge.c`). The last legacy argc/argv bridge (`src/uci_cmd_handlers.c`) has been deleted, and configuration-listing commands accept structured key=value options while still tolerating legacy flags for compatibility.

## Outstanding Work / Next Tasks
1. **Document the new option syntax** – refresh CLI help/docs so `show_*_configs` users know they can pass `filter=...`, `id=...`, and `detail=full` (while legacy `--id`/`--filter` continue to work).
2. **Prune remaining dead artifacts** – search for unused `.before_fix` files, duplicate CLI tables, and unused helper declarations in headers/tests.
3. **Extend automated coverage** – add regression tests targeting the typed hardware/general commands and the new config-listing option parser.
4. **Audit for further consolidation** – now that `src/uci_cmd_handlers.c` is gone, look for other duplicated helpers (key parsing, error reporting) that can move into shared utilities.

## Testing Status
`make -j`, `./test_command_handlers`, and `./test_command_framework_validation` were executed on 2025-11-19 after the typed hardware migration; all builds/tests passed. No other suites (e.g., `tests/test_final.sh`) have been run in this workspace since commit `74cfd77`.
