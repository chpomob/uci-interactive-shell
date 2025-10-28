# Repository Guidelines

**Hardware-first focus:** This project centers on robust UCI control of real hardware; the simulator only supports protocol validation and debugging.

## Project Structure & Module Organization
Core CLI logic lives in `src/`, covering the main loop, command handlers, packet parsing, and hardware layers; add new symbols beside their peers. Public headers reside in `include/` and must stay guarded and consistent with the C sources. Tests mirror production structure under `tests/`, with harnesses such as `tests/test_session_manager.c`. Use helper scripts like `analyze_real_logs.py`, `final_analysis_report.py`, and the demos in `demo_enhanced_ui/` to support protocol analysis, while longer research belongs in `docs/` or `uci_analysis/`.

## Build, Test, and Development Commands
- `make`: compile `uci-shell` and every test binary.
- `./uci-shell`: launch the interactive shell after a successful build.
- `make unit-test` / `make config-test` / `make session-manager-test` / `make security-test`: exercise focused suites; watch for `FAILED` lines.
- `make coverage`: rebuild with GCOV and review `coverage/src_coverage.txt` and `coverage/tests_coverage.txt`.
- `make clean`: drop objects, binaries, and coverage traces to reset the tree.

## Coding Style & Naming Conventions
Compile with `gcc -std=c11 -Wall -Wextra`; new code must remain warning-free. Follow the four-space indentation and brace-on-new-line style shown in `src/main.c`. Exported APIs take the `uci_`, `uci_cmd_`, or `uci_hw_` prefixes, while internal helpers stay in descriptive `snake_case`. Update headers when a source file exposes a new symbol and keep identifiers ASCII unless the protocol mandates otherwise.

## Testing Guidelines
Add suites under `tests/test_<area>.c` and register them in the Makefile next to similar targets. Unit binaries exit non-zero on failure, so retain stdout when reporting issues. Use `make <suite>-test` while iterating and finish with a full `make` to ensure every binary links. Regenerate coverage for protocol-heavy updates and share relevant GCOV snippets during review.

## Commit & Pull Request Guidelines
Write imperative, concise commit subjects (e.g., "Add session manager timeout guard"). Group related edits together and keep generated artifacts such as `uci-shell`, object files, and coverage reports out of version control. Pull requests should state the problem, outline the solution, list executed `make` targets, and reference any `docs/` or `uci_analysis/` updates. Include logs or screenshots whenever UX or hardware behavior changes.

## Security & Hardware Notes
Character-device transports ship with conservative defaults; document expected device paths and any needed `udev` tweaks. Reference `demo_uci_security.sh` when introducing adapters so reviewers understand the setup. Store raw capture files outside the repository and summarize findings in `UCI_SECURITY_GUIDE.md` or related docs.

## Repository Preservation Notes
- Keep `uci_analysis/qm35-sdk/` intact; this SDK snapshot supports ongoing protocol analysis.
