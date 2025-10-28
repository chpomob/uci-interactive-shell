# Agent Guide: `tests/`

**Hardware-first focus:** This project centers on robust UCI control of real hardware; the simulator only supports protocol validation and debugging.

## Test Harness Overview
- `test_runner.h`: Minimal macros for suite/case logging and assertions; failures jump to `test_case_end` labels.
- `test_uci_functions.c`: End-to-end simulation coverage for core/session/vendor commands.
- `test_config_manager.c`: Validates TLV metadata, parsing, and storage edge cases.
- `test_session_manager.c`: Exercises session bookkeeping helpers.
- Hardware-dependent suites have been removed to keep CI hardware-free.

## Conventions
- Keep tests self-contained; initialize global state (e.g., `init_uci_sessions()`, `uci_config_init()`) within each case.
- Prefer explicit assertions with descriptive failure messages. Use helper writers (`write_u16_le` etc.) to keep byte arrays readable.
- Tests intentionally print simulated packet traces; ensure new assertions still leave output manageable.

## Adding Coverage
1. Identify regression path (command branch, TLV decode, etc.).
2. Seed necessary state (`uci_sessions`, config manager, etc.).
3. Exercise behavior via public functions (e.g., `send_uci_command` rather than private helpers) so tests reflect real usage.
4. Assert on both state changes and reported status codes.
5. Run `make unit-test` (or relevant target) and ensure zero failures.

## Debugging Tips
- When diagnosing complex payloads, temporarily log hex dumps; wrap in `#ifdef DEBUG` or remove before landing.
- For flaky tests, verify global state cleanup—most helpers require explicit resets between cases.

Document any notable testing gaps or assumptions in `COMMAND_SUPPORT.md` or the commit message to keep maintainers aligned.
