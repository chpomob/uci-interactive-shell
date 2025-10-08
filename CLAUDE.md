# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

UCI Interactive Shell is a comprehensive UWB (Ultra-Wideband) device communication tool that implements the UCI (Ultra-wideband Communication Interface) protocol. The project supports both simulation mode for testing and real hardware communication via character device files.

## Build and Test Commands

### Building
```bash
make                    # Build uci-shell and all test binaries
make clean              # Remove all build artifacts and coverage files
```

### Running
```bash
./uci-shell             # Launch interactive shell (simulation mode by default)
```

### Testing
```bash
make unit-test          # Run unit tests for core UCI functions
make config-test        # Run configuration manager tests
make hw-interface-test  # Run hardware interface tests
make session-manager-test  # Run session management tests
make coverage           # Build with coverage, run tests, generate reports in coverage/
```

### Hardware Testing
```bash
./test_hardware_interface.sh    # Test character device flows (requires hardware)
```

## Architecture Overview

### Layered Architecture
The codebase follows a modular architecture with clear separation of concerns:

1. **CLI Layer** (`src/main.c`, `src/uci_cli.c`, `src/uci_cli_completion.c`)
   - Interactive shell with readline support for command history and tab completion
   - Command parsing and dispatching
   - Alias management for user-friendly commands

2. **Protocol Layer** (`src/uci.c`, `src/uci_packet_utils.c`, `include/uci_pdl.h`)
   - UCI packet construction and parsing following Android UWB specification
   - Header encoding/decoding with proper bit-field manipulation
   - Message type, group ID, and opcode handling

3. **Session Management** (`src/uci.c` - session functions)
   - Persistent session state tracking (up to 10 concurrent sessions)
   - Session lifecycle: init → configure → start → stop → deinit
   - Configuration storage using TLV (Type-Length-Value) encoding
   - Multicast controlee management

4. **Hardware Interface Layer** (`src/uci_hw_interface.c`, `src/uci_hw_chardev.c`)
   - Character device communication (/dev/ttyUSB*, /dev/ttyACM*)
   - Packet framing and transmission
   - Response timeout and error handling

5. **Configuration Manager** (`src/uci_config_manager.c`)
   - Application and device configuration parameter management
   - Parameter validation with range checking
   - Human-readable parameter naming and descriptions

6. **UI/Decoder Layer** (`src/uci_ui.c`, `src/uci_ui_packet_decoder.c`)
   - Colorized terminal output
   - Human-readable packet decoding for all UCI message types
   - Ranging notification parsing with distance/angle measurements

### UCI Packet Structure
UCI packets follow the Android UWB specification format:
- **Header (4 bytes)**:
  - Byte 0: `[GID:4][PBF:1][MT:3]` where GID is in bits[3:0]
  - Byte 1: `[Opcode:6][Reserved:2]` where Opcode is in bits[5:0]
  - Byte 2: Reserved
  - Byte 3: Payload Length
- **Payload**: Variable length, depends on command/response type

Use helper functions in `include/uci.h`:
- `uci_pack_first_byte()` / `uci_pack_second_byte()` for encoding
- `get_gid()`, `get_mt()`, `get_opcode()` for decoding

### Endianness Handling
**CRITICAL**: The UCI protocol uses **little-endian** byte order for multi-byte values:
- Session IDs (4 bytes)
- Short addresses (2 bytes)
- Configuration parameter values

Always use proper endianness conversion when building commands or parsing responses. See `src/uci_packet_utils.c` for reference implementations.

### Session State Management
Sessions are stored in global array `uci_sessions[MAX_SESSIONS]` with fields:
- `session_id`: 4-byte unique identifier
- `session_type`: FIRA_RANGING_SESSION, CCC_RANGING_SESSION, etc.
- `session_state`: SESSION_STATE_INIT, ACTIVE, IDLE, etc.
- `configs[]`: Array of TLV configuration entries
- Session lifecycle functions: `find_session_by_id()`, `find_free_session_slot()`, `store_session_config()`

## Code Style and Conventions

- **C11 standard** with four-space indentation and K&R braces
- **Naming**: `snake_case` for functions/locals, `g_` prefix for globals, `UPPERCASE` for macros
- **Static helpers**: Keep helper functions `static` and co-locate with usage
- **Headers**: All headers in `include/`, matching `.c` filenames in `src/`
- **Warnings**: Code must compile cleanly with `-Wall -Wextra`

## Key Files Reference

- `include/uci_pdl.h` - Protocol constants (message types, group IDs, opcodes, status codes)
- `include/uci.h` - Core data structures and packet header manipulation
- `include/uci_functions.h` - Session management and notification handling
- `src/uci.c` - Main protocol implementation (~4000 lines)
- `src/main.c` - Interactive shell and command dispatching
- `tests/test_runner.h` - Test framework helpers

## Protocol Documentation

Comprehensive UCI protocol analysis based on Android AOSP implementation:
- `uci_analysis/UCI_PROTOCOL_ANALYSIS.md` - Detailed protocol specification
- `uci_analysis/SUMMARY.md` - Protocol element deep dive
- `uci_analysis/uwb/src/rust/uwb_uci_packets/uci_packets.pdl` - Official PDL definition

## Important Notes from agent.md

- Match imperative commit style (`Fix command autocompletion issue`), ~72 chars
- Mock hardware boundaries at interface layer in tests
- Document device assumptions in hardware test scripts
- Regenerate coverage when adding branches: `make coverage`
- Keep protocol analysis updates in separate commits
