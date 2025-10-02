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

- Send a raw command: `send 00 00 01` (send command with MT=0, GID=0, OID=1)
- Get device information: `get_device_info`
- Get capabilities: `get_caps_info`
- Initialize a session: `session_init`

## Protocol Information

The UCI protocol is a communication interface used for Ultra-Wideband (UWB) devices, particularly defined by the FiRa Consortium and is also used in some Android devices for UWB applications. This shell simulates communication with a UWB controller.

## Project Structure

- `src/main.c` - Main interactive shell implementation
- `src/uci.c` - UCI packet processing and command handling
- `src/uci.h` - UCI data structures
- `src/uci_functions.h` - Function declarations
- `src/uci_pdl.h` - UCI protocol definitions and constants
