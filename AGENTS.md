# Repository Guidelines

## Project Structure & Module Organization
Core implementation lives in `src/` (CLI handlers, protocol logic, hardware/simulation bridges). Public headers are in `include/`. Canonical tests are in `tests/` (unit and command-framework coverage). Main build orchestration is in `Makefile`; `Makefile.ui` is for UI-focused workflows.  
Top-level demo scripts and ad-hoc experiments exist (for example `demo_*.sh`, `test_*.c` at repo root), but contributors should prefer `src/`, `include/`, and `tests/` for production changes.

## Build, Test, and Development Commands
- `make`  
Builds `uci-shell` and runs the primary test targets.
- `make uci-shell`  
Builds only the CLI binary.
- `./uci-shell`  
Starts interactive shell locally.
- `make unit-test`  
Runs `test_uci_functions` (core protocol and behavior checks).
- `make command-handler-test`  
Runs command handler validation.
- `make command-framework-validation-test`  
Runs typed parsing/CLI dispatch regression checks.
- `make clean`  
Removes objects, binaries, and coverage artifacts.

## Coding Style & Naming Conventions
Use C11 with warnings enabled (`-Wall -Wextra`). Follow existing style: 4-space indentation, braces on same line for functions/conditionals, and clear early-return validation.  
Naming patterns:
- Functions: `snake_case` (for example `handle_hw_send_command`)
- Globals: `g_*`
- Constants/macros: `UPPER_SNAKE_CASE`
Keep new code consistent with adjacent files; avoid mixing formatting styles in the same patch.

## Testing Guidelines
Tests use the in-repo harness (`tests/test_runner.h`) with `TEST_CASE`, `ASSERT_*`, and explicit pass/fail summaries.  
Add or update tests with every behavior change, especially for CLI output, argument parsing, and protocol field handling.  
Prefer targeted runs while iterating, then run at least:
- `make unit-test`
- `make command-handler-test`
- `make command-framework-validation-test`

## Commit & Pull Request Guidelines
History favors short imperative subjects (for example `Fix ...`, `Add ...`, `Remove ...`). Keep subject lines specific and scoped to behavior.  
PRs should include:
- What changed and why
- Test commands executed and outcomes
- Any user-visible CLI/output changes
- Linked issue/task when applicable
