# UCI Interactive Shell

**Hardware-first focus:** This project centers on robust UCI control of real hardware; the simulator only supports protocol validation and debugging.

An interactive shell to communicate with a UWB (Ultra-Wideband) device using the UCI (Ultra-wideband Communication Interface) protocol.

## Overview

This project provides an interactive shell interface for testing and communicating with UWB devices using the UCI protocol. The UCI protocol is used in Ultra-Wideband communication systems, particularly in mobile and IoT devices for ranging and positioning applications.

## Building

To build the project, simply run:

```bash
make
```

## Running

To run the interactive shell:

```bash
./uci-shell
```

## Available Commands

Once the shell is running, you can use the following commands:

### General / UI
- `help` – Discover commands and their usage (now backed by the typed framework, so the output always mirrors the active command tables)
- `analyze_packet [--verbose] [--tlv] [--compare] [--examples] [--help] <bytes...>` – Run the enhanced packet analyzer on hex input
- `quit` – Exit the shell

### Core Device Management
- `get_device_info`, `device_reset`
- `get_caps_info`, `get_device_state`
- `set_config <config> <value>`, `get_config <config>`
- `set_device_active`, `set_device_ready`, `device_on`, `device_off`, `device_suspend`

### Session Configuration & Control
- `session_init <id> <type>`, `session_deinit <id>`
- `session_start <id>`, `session_stop <id>`, `get_session_state <id>`
- `set_app_config <id> <param> <value>`, `get_app_config <id> <param>`
- `session_update_multicast_list <id> <action> <short> <subsession>`
- `session_update_dt_tag_rounds <id> [round_index ... | hex_bytes]`
- `session_data_transfer_phase_config <id> <repetition> <control> <size> [payload...]`
- `session_query_data_size_in_ranging <id>`
- `session_set_hybrid_controller_config <id> [hex-config]`
- `session_set_hybrid_controlee_config <id> [hex-config]`
- `session_send_data <id> <dest> <seq> <payload>`

### Simulation Utilities
- `simulate_ranging`, `simulate_multi_target_ranging`
- `simulate_notification`, `simulate_session_status`, `simulate_data_credit`
- `demo_session_flow`

### Hardware Mode
- `mode_hw <device_path>`, `hw_init <device_path>` (alias `hw_connect`)
- `mode_sim` / `mode_info` – switch modes and display active transport
- `hw_send <mt> <pbf> <gid> <oid> [payload...]` – stream raw hex to the connected device
- After connecting, reuse the standard device/session commands (e.g. `get_device_info`, `session_start 1`) to operate on hardware.

## Command Examples

### Basic Commands
- Get device information: `get_device_info` or `device_info`
- Reset device: `device_reset`
- Get capabilities: `get_caps_info`
- Get/set device state: `get_device_state`, `set_device_active`, `set_device_ready`
- Device power control: `device_on` (alias for set_device_active), `device_off` (alias for set_device_ready)

### Session Management
- Initialize a session: `session_init 1 fira_ranging`
- Configure session parameters: `set_app_config 1 channel 5`
- Update DT-Tag rounds: `session_update_dt_tag_rounds 1 0 1 2` (or `session_update_dt_tag_rounds 1 000102` for hex bytes)
- Configure data transfer: `session_data_transfer_phase_config 1 1 0x02 4 AA BB CC DD`
- Start ranging: `session_start 1`
- Get session status: `get_session_state 1`
- Stop ranging: `session_stop 1`
- Deinitialize session: `session_deinit 1`

### Configuration Management
- Discover device configs (supports both modern and legacy syntax):
  - `show_device_configs filter=state detail=full`
  - `show_device_configs --filter state --full`
  - `show_device_configs id=0x25`
- Discover application configs for a particular session:
  - `show_app_configs filter=channel`
  - `show_app_configs id=0x00 detail=summary`
  - `show_app_configs --id 0x00 --filter device`
- Set device configuration: `set_config device_state active`
- Get device configuration: `get_config device_state`
- Set application configuration: `set_app_config 1 device_type responder`
- Get application configuration: `get_app_config 1 device_type`

### Hardware Commands
- Connect to hardware: `mode_hw /dev/ttyUSB0` (alias `hw_init`)
- Inspect active transport: `mode_info`
- Send raw command: `hw_send 01 00 00 02` (MT=1, PBF=0, GID=0, OID=2 for CORE_DEVICE_INFO)
- Once connected, issue any standard command (`get_device_info`, `session_start 1`, etc.) to act on hardware.

### Simulation Commands
- Simulate ranging notification: `simulate_ranging`
- Simulate multi-target ranging: `simulate_multi_target_ranging`
- Run session demo: `demo_session_flow`
- Simulate other notifications: `simulate_notification`, `simulate_session_status`, `simulate_data_credit`

## Protocol Information

The UCI protocol is a communication interface used for Ultra-Wideband (UWB) devices, particularly defined by the FiRa Consortium and is also used in some Android devices for UWB applications. This shell simulates communication with a UWB controller.

## Project Status

The shell is in consolidation mode. The active architecture is the declarative
command framework in `src/uci_cmd_framework_*` and `src/uci_command_framework.c`.
Help text, completion, validation, and dispatch now read from those shared
command tables.

### Source Of Truth
- Standard UCI message, group, status, and opcode definitions are pinned to the
  Android UWB code definitions in `include/uci_pdl.h`.
- Qorvo vendor groups and opcodes are pinned to QM SDK values in
  `include/uci_opcode_constants.h`.
- Shared enum decoding helpers in `include/uci_packet_utils.h` own the plain
  CLI/analyzer mapping for status, device-state, session-state, and
  session-reason values.
- Tests in `tests/test_protocol_definitions.c` and
  `tests/test_protocol_fixtures.c` enforce those mappings, the command metadata
  that depends on them, and representative byte-level packet fixtures.
- `tests/test_transport_parity.c` verifies that the same command handlers emit
  identical control-packet bytes in simulation mode and hardware mode before
  any hardware response is involved.

### Current Consolidation Work
1. Keep protocol constants centralized and remove local literal fallbacks.
2. Route typed command handlers through validated parsed parameters instead of
   reparsing `argv`.
3. Expand regression coverage for constant mappings, command definitions, and
   hardware/simulation parity.

### Complete Feature Set
- **Protocol Compliance**: Fully aligned with Android UWB specification
- **Enhanced Response Handling**: Complete parsing of all UCI responses
- **Comprehensive TLV Support**: Full Type-Length-Value configuration management
- **Session Management**: Complete session lifecycle with persistent state
- **Notification Handling**: Human-readable display of all UCI notifications
- **Ranging Support**: Full UWB ranging notification parsing with distance/angle measurements
- **Hardware Communication**: Real UWB device support via character device files (/dev/ttyUSB*)

### Hardware Support
The UCI Interactive Shell can communicate with real UWB hardware through character device files:
```bash
./uci-shell
> mode_hw /dev/ttyUSB0
> hw_send 01 00 00 02  # Send CORE_DEVICE_INFO command
```

### Hardware Integration Test Suite
An opt-in hardware integration suite is available and safe for CI/local runs when no device is attached.
```bash
# Builds and runs; auto-skips if UCI_HW_DEVICE is not set.
make hardware-integration-test

# Run against real hardware
UCI_HW_DEVICE=/dev/ttyUSB0 make hardware-integration-test

# Optional destructive reset coverage
UCI_HW_DEVICE=/dev/ttyUSB0 UCI_HW_INCLUDE_RESET=1 make hardware-integration-test
```

A minimal user-facing hardware acceptance smoke script is also available:
```bash
# Non-destructive CLI bring-up flow
UCI_HW_DEVICE=/dev/ttyUSB0 make hardware-acceptance-smoke

# Include session_start only when the target device already has a known-good
# app-config baseline for the chosen session profile.
UCI_HW_DEVICE=/dev/ttyUSB0 UCI_HW_INCLUDE_SESSION_START=1 make hardware-acceptance-smoke
```

Environment variables:
- `UCI_HW_DEVICE` (required to run real-hardware checks)
- `UCI_HW_TIMEOUT_MS` (optional, default `1500`)
- `UCI_HW_VERBOSE` (optional, `1` enables transport debug logs)
- `UCI_HW_INCLUDE_RESET` (optional, `1` enables `CORE_DEVICE_RESET` test)
- `UCI_HW_SCRIPT_TIMEOUT_SEC` (optional, smoke-script timeout, default `20`)
- `UCI_HW_SESSION_ID` and `UCI_HW_SESSION_TYPE` (optional, smoke-script session settings)
- `UCI_HW_INCLUDE_SESSION_START` (optional, `1` includes `session_start` in the smoke script)

### Usage Examples

#### Basic Simulation Mode
```bash
./uci-shell
> get_device_info
> device_reset
> get_device_state
> set_device_active
> session_init 1 fira_ranging
> set_app_config 1 device_type responder
> set_app_config 1 channel 5
> session_start 1
> simulate_ranging
> get_session_state 1
> session_stop 1
> session_deinit 1
> quit
```

#### Complete Session Flow Example
```bash
./uci-shell
> get_device_info                 # Get basic device info
> get_caps_info                   # Get device capabilities
> device_reset                     # Reset the device
> get_device_state                 # Check current state
> set_device_active                # Set to active state
> session_init 1 fira_ranging      # Initialize session with ID 1
> set_app_config 1 device_type responder    # Set device as responder
> set_app_config 1 channel 5       # Set to channel 5
> set_app_config 1 ranging_usage ranging   # Set ranging usage
> session_update_multicast_list 1 add 0x1234 0x00000001
> session_update_dt_tag_rounds 1 0 1          # or session_update_dt_tag_rounds 1 0001
> session_data_transfer_phase_config 1 1 0x02 0
> session_start 1                  # Start the session
> simulate_ranging                 # Simulate ranging measurement
> get_session_state 1              # Check session state
> session_stop 1                   # Stop the session
> session_deinit 1                 # Deinitialize session
> set_device_ready                 # Set back to ready state
> quit                             # Exit shell
```

#### Hardware Mode (with real UWB device)
```bash
./uci-shell
> mode_hw /dev/ttyUSB0             # Connect to the hardware transport
> mode_info                        # Confirm we are in HARDWARE mode
> get_device_info                  # Query the device over hardware
> device_reset                     # Reset the controller
> set_device_active                # Bring the device to ACTIVE
> session_init 1 fira_ranging      # Create a ranging session on hardware
> set_app_config 1 device_type responder
> set_app_config 1 channel 5
> session_start 1                  # Begin ranging on hardware
> get_session_state 1              # Observe live session state
> session_stop 1
> session_deinit 1
> mode_sim                         # Return to simulation mode when finished
> quit
```

#### Command with Raw Packets (Advanced)
```bash
./uci-shell
> hw_send 01 00 00 02             # Send CORE_DEVICE_INFO (MT=0x01, GID=0x00, OID=0x02)
> hw_send 01 00 00 04 01 00 01 02 # Send SET_CONFIG to set device to ACTIVE (num_tlvs=1, cfg_id=0, len=1, value=2)
> hw_send 01 00 00 05 01 00       # Request GET_CONFIG for device_state
```

For complete technical details of all improvements, see:
- `docs/PROJECT_STATUS.md` - Current branch snapshot, recent commits, and next steps
- `docs/UCI_STATE_OF_THE_ART_SECURITY_FINAL_SUMMARY.md` - End-to-end security hardening summary
- `uci_analysis/UCI_PROTOCOL_ANALYSIS.md` - Detailed UCI protocol specification
- `uci_analysis/SUMMARY.md` - Deep dive into protocol elements
- `uci_analysis/uci_packet_generator.py` - Python script for generating UCI packets

## Project Structure

- `src/main.c` - Main interactive shell implementation
- `src/uci.c` - UCI packet processing and command handling
- `src/uci.h` - UCI data structures
- `src/uci_functions.h` - Function declarations
- `src/uci_pdl.h` - UCI protocol definitions and constants
- `src/uci_hw_interface.h/c` - Hardware interface layer
- `src/uci_hw_chardev.h/c` - Character device communication layer
- `src/uci_command_framework.c` - Parameter validation and unified dispatcher shared by simulation/hardware modes
- `src/uci_cmd_framework_{bridge,device,session,simulation}.c` - Declarative command definition tables that power CLI help, completion, and handlers

## UCI Protocol Analysis

For a comprehensive analysis of the UCI protocol based on the Android Open Source Project implementation, see:
- `uci_analysis/UCI_PROTOCOL_ANALYSIS.md` - Detailed comparison with Android UWB implementation
- `uci_analysis/SUMMARY.md` - Deep dive summary of protocol elements
- `uci_analysis/uci_packet_generator.py` - Python script to generate proper UCI packets
- `uci_analysis/uwb/` - Full Android UWB repository for reference

The Android repository provides the complete UCI specification in Protocol Definition Language (PDL) format in `uci_analysis/uwb/src/rust/uwb_uci_packets/uci_packets.pdl`
