# Contributing to UCI Interactive Shell

Thank you for your interest! Please follow these guidelines.

## Quick Start

```bash
# Build
make

# Run all tests
make test

# Build just the binary
make uci-shell

# Get help
make help
```

## Code Style

- **Language**: C11 (`-std=c11`)
- **Format**: Google style, Allman braces, 4-space indent, 120 char line limit
  - `.clang-format` file present in repository root — configure your editor
  - `.editorconfig` present for editor integration (Vim, VS Code, etc.)
- **Compiler flags**: `-Wall -Wextra -g -DHAVE_READLINE`
- **Zero warnings**: Every file must compile without warnings under `-Wall -Wextra`

## Commit Conventions

- **Subject line**: ≤ 72 chars, imperative mood (`add`, `fix`, `remove`, `refactor`)
- **Scope prefix** (optional but preferred): `ci:`, `docs:`, `src/`, `include/`, `test/`,
  `make:`, `style:`
- **Body**: Explain _why_ the change is needed. Reference any related
  `uci_pdl.h` constant or `COMMAND_REFERENCE.md` entry when relevant.

Examples:

```
Add `analyze_packet --tlv` flag for raw TLV hex output

The existing analyzer only shows decoded field names. Adding --tlv
exposes the raw hex bytes between the message header and the
session handle, which is needed for vendor-specific payload debugging.

refs: include/uci_pdl.h UCI_PACKET_TLV_TYPE
```

## Adding a New Command

See `docs/ARCHITECTURE.md` §2 ("Command Framework") for the full procedure.
The short form:

1. Add a row to the appropriate `g_cmd_table[]` in `src/uci_cmd_framework_*.c`
2. Write the handler function in the appropriate `src/uci_cmd*.c`
3. Register the argument template in the existing table entry
4. No documentation write-up needed — the framework auto-generates help text
5. Add a regression test in the appropriate `tests/test_*.c`

## Writing Tests

All tests live under `tests/`:

- Each `.c` test file runs its own test runner and reports `RESULT: ALL TESTS PASSED`.
- Tests compile with `$(CC) $(CFLAGS) -o test_foo tests/test_foo.c $(LIBS)`.
- New tests should go in an existing file if the scope matches, or a new `tests/test_<name>.c`
  otherwise.
- All new test binary names must be added to the `Makefile` `.PHONY` list and the `test:` target.

Common patterns:

```c
// Assertion
TEST_ASSERT(condition, "short message");
TEST_ASSERT_EQ(expected, actual, "description");
TEST_ASSERT_STR_EQ(expected, actual);
TEST_ASSERT_BUF_EQ(expected, actual, len);

// Test runner
#define RUNTEST(fn) printf("  %s ... %s\n", #fn, fn() ? "PASSED" : "FAILED"); ok &= fn()
```

## Review Checklist

- [ ] `make help` still shows all targets
- [ ] `make clean && make -Werror` compiles without warnings
- [ ] `make test` passes (all suites)
- [ ] `src/main.o` rebuilds if headers changed
- [ ] New commands appear in `help` output automatically
- [ ] Protocol constants come from `include/uci_pdl.h`, not embedded literals
- [ ] `.clang-format` / `.editorconfig` rules followed (or rules updated if needed)

## Reporting Issues

- Include: OS, `make --version`, `gcc --version`, test failures
- For protocol bugs: wire packet bytes (`analyze_packet <hex>`) and expected vs. actual output
