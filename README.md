# UCI Interactive Shell

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

- `quit` - Exit the shell
- `send <mt> <gid> <oid> [payload bytes]` - Send a raw UCI command with message type, group ID, opcode ID, and optional payload
- `get_device_info` - Get device information
- `device_reset` - Reset the device
- `get_caps_info` - Get device capabilities information
- `set_config` - Set device configuration
- `get_config` - Get device configuration
- `session_init` - Initialize a ranging session
- `session_deinit` - Deinitialize a ranging session
- `session_start` - Start a ranging session
- `session_stop` - Stop a ranging session

## Command Examples

### Basic Commands
- Get device information: `get_device_info` or `device_info`
- Reset device: `device_reset`
- Get capabilities: `get_caps_info`
- Get/set device state: `get_device_state`, `set_device_active`, `set_device_ready`
- Device power control: `device_on` (alias for set_device_active), `device_off` (alias for set_device_ready)

### Session Management
- Initialize a session: `session_init 1 fira_ranging` (session ID 1, type fira_ranging)
- Start ranging: `session_start 1` 
- Stop ranging: `session_stop 1`
- Get session status: `get_session_state 1`
- Deinitialize session: `session_deinit 1`

### Configuration Management
- Set device configuration: `set_config device_state active`
- Get device configuration: `get_config device_state`
- Set application configuration: `set_app_config 1 device_type responder`
- Get application configuration: `get_app_config 1 device_type`

### Hardware Commands
- Connect to hardware: `hw_connect /dev/ttyUSB0`
- Get hardware device info: `hw_get_device_info`
- Send raw command: `hw_send 01 00 00 02` (MT=1, PBF=0, GID=0, OID=2 for CORE_DEVICE_INFO)
- Send raw packet bytes: `hw_send_raw 20 08 00 00`

### Simulation Commands
- Simulate ranging notification: `simulate_ranging`
- Simulate multi-target ranging: `simulate_multi_target_ranging`
- Run session demo: `demo_session_flow`
- Simulate other notifications: `simulate_notification`, `simulate_session_status`, `simulate_data_credit`

## Protocol Information

The UCI protocol is a communication interface used for Ultra-Wideband (UWB) devices, particularly defined by the FiRa Consortium and is also used in some Android devices for UWB applications. This shell simulates communication with a UWB controller.

## Project Status

This project has been enhanced from a simple UCI protocol simulation to a comprehensive UWB communication tool that supports both simulation mode and real hardware communication.

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
> hw_connect /dev/ttyUSB0
> hw_send 01 00 00 02  # Send CORE_DEVICE_INFO command
```

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
> hw_connect /dev/ttyUSB0          # Connect to hardware device
> hw_get_device_info               # Get device info from hardware
> hw_device_reset                  # Reset hardware device
> hw_get_device_state              # Check hardware device state
> hw_session_init 1 ranging        # Initialize ranging session
> hw_set_app_config 1 device_type responder  # Configure as responder
> hw_set_app_config 1 channel 5    # Set channel
> hw_session_start 1               # Start hardware session
> hw_get_session_state 1           # Check session state on hardware
> hw_session_stop 1                # Stop hardware session
> hw_session_deinit 1              # Deinitialize hardware session
> quit
```

#### Command with Raw Packets (Advanced)
```bash
./uci-shell
> hw_send 01 00 00 02             # Send CORE_DEVICE_INFO (MT=0x01, GID=0x00, OID=0x02)
> hw_send_raw 20 08 00 00         # Send raw packet bytes (alternative format)
> hw_send 01 00 00 04 01 00 01 02 # Send SET_CONFIG to set device to ACTIVE (num_tlvs=1, cfg_id=0, len=1, value=2)
```

For complete technical details of all improvements, see:
- `FINAL_SUMMARY.md` - Comprehensive feature summary and usage guide
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

## UCI Protocol Analysis

For a comprehensive analysis of the UCI protocol based on the Android Open Source Project implementation, see:
- `uci_analysis/UCI_PROTOCOL_ANALYSIS.md` - Detailed comparison with Android UWB implementation
- `uci_analysis/SUMMARY.md` - Deep dive summary of protocol elements
- `uci_analysis/uci_packet_generator.py` - Python script to generate proper UCI packets
- `uci_analysis/uwb/` - Full Android UWB repository for reference

The Android repository provides the complete UCI specification in Protocol Definition Language (PDL) format in `uci_analysis/uwb/src/rust/uwb_uci_packets/uci_packets.pdl`
