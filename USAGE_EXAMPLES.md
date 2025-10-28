# UCI Interactive Shell - Usage Examples

**Hardware-first focus:** This project centers on robust UCI control of real hardware; the simulator only supports protocol validation and debugging.

This document provides comprehensive examples of how to use the UCI Interactive Shell, highlighting its unified command interface that works seamlessly in both simulation and hardware modes.

## Table of Contents
1. [Basic Usage](#basic-usage)
2. [Device Management](#device-management)
3. [Session Management](#session-management)
4. [Multicast List Management](#multicast-list-management)
5. [Hardware Communication](#hardware-communication)
6. [Unified Command Interface](#unified-command-interface)
7. [Simulation Features](#simulation-features)

## Basic Usage

Start the UCI Interactive Shell:

```bash
./uci-shell
```

The shell provides tab completion and command history:

```bash
> [TAB][TAB]  # Shows all available commands
> history     # Shows command history
> alias       # Shows all defined aliases
```

## Device Management

### Get Device Information
```bash
> get_device_info
# or the alias:
> device_info
```

### Device State Management
```bash
# Check current device state
> get_device_state

# Set device to active state
> set_device_active
# or as a power command:
> device_on

# Set device to ready state
> set_device_ready
# or as a power command:
> device_off

# Set device power state with friendly names
> set_power active
> set_power ready
> set_power sleep
```

### Device Configuration
```bash
# Set device configuration
> set_config device_state active
> set_config low_power_mode on

# Get specific configuration
> get_config device_state
> get_config low_power_mode

# Get all configurations
> get_config
```

## Session Management

### Session Lifecycle
```bash
# Initialize a ranging session (ID: 1, Type: fira_ranging)
> session_init 1 fira_ranging
# or with friendly alias:
> session_new 1 ranging

# Set application configuration for the session
> set_app_config 1 device_type responder
> set_app_config 1 channel 5
> set_app_config 1 ranging_usage ranging

# Start ranging session
> session_start 1
# or with friendly alias:
> start_ranging 1

# Check session status
> get_session_state 1
# or with friendly alias:
> session_status 1

# Stop the ranging session
> session_stop 1
# or with friendly alias:
> stop_ranging 1

# Deinitialize the session
> session_deinit 1
# or with friendly alias:
> session_close 1
```

### Multicast List Management
```bash
# Add a device to the multicast list
> session_update_multicast_list 1 add 0x1234 0x00000001

# Remove a device from the multicast list
> session_update_multicast_list 1 remove 0x5678 0x00000002

# Add a device with a short key
> session_update_multicast_list 1 add_short_key 0xABCD 0x00000003

# Add a device with a long key
> session_update_multicast_list 1 add_long_key 0xEF01 0x00000004
```

## Hardware Communication

### Connecting to Hardware
```bash
# Initialize hardware interface with a device path
> hw_init /dev/ttyUSB0

# Or switch to hardware mode directly
> mode_hw /dev/ttyUSB0

# Check current mode
> mode_info
# or
> current_mode
```

### Using Hardware Commands
```bash
# After mode_hw, standard commands talk to hardware
> mode_hw /dev/ttyUSB0
> get_device_info
> session_init 1 fira_ranging
> session_start 1

# Send a raw command directly when needed (MT=01, PBF=00, GID=00, OID=02 = CORE_DEVICE_INFO)
> hw_send 01 00 00 02

# Return to simulation mode when finished
> mode_sim
```

### Switching Between Modes
```bash
# Switch to simulation mode
> mode_sim

# Switch to hardware mode
> mode_hw /dev/ttyUSB0

# Check current mode
> current_mode
```

## Unified Command Interface

The UCI Interactive Shell features a unified command interface that works in both simulation and hardware modes:

### Unified Device Commands
```bash
# These commands work in both modes:
> get_device_info      # Works in sim/hw mode
> device_reset         # Works in sim/hw mode
> get_caps_info        # Works in sim/hw mode
> set_config device_state active  # Works in sim/hw mode
> get_config device_state         # Works in sim/hw mode
```

### Unified Session Commands
```bash
# These commands work in both modes:
> session_init 1 fira_ranging  # Works in sim/hw mode
> session_start 1             # Works in sim/hw mode
> session_stop 1              # Works in sim/hw mode
> session_deinit 1            # Works in sim/hw mode
> get_session_state 1         # Works in sim/hw mode
> set_app_config 1 channel 5  # Works in sim/hw mode
```

## Simulation Features

### Simulate Notifications
```bash
# Simulate a device status notification
> simulate_notification device_status active

# Simulate a session status notification
> simulate_session_status 1 active mgmt_cmd

# Simulate a data credit notification
> simulate_data_credit
```

### Simulate Ranging
```bash
# Simulate ranging measurements
> simulate_ranging

# Simulate multi-target ranging
> simulate_multi_target_ranging

# Run a complete session flow demonstration
> demo_session_flow
```

### Packet Analysis
```bash
# Analyze hex packet bytes
> analyze_packet 20 08 00 00
> analyze_packet 21 00 00 05 00 00 00 01 00
```

## Advanced Examples

### Complete Ranging Session (Simulation Mode)
```bash
> get_device_info
> device_reset
> set_device_active
> session_init 1 fira_ranging
> set_app_config 1 device_type responder
> set_app_config 1 channel 5
> session_start 1
> simulate_ranging
> get_session_state 1
> session_stop 1
> session_deinit 1
> set_device_ready
> quit
```

### Complete Ranging Session (Hardware Mode)
```bash
> mode_hw /dev/ttyUSB0
> get_device_info
> device_reset
> set_device_active
> session_init 1 fira_ranging
> set_app_config 1 device_type responder
> set_app_config 1 channel 5
> session_start 1
> get_session_state 1
> session_stop 1
> session_deinit 1
> set_device_ready
> quit
```

### Mode Switching Example
```bash
# Start in simulation mode
> get_device_info
> session_init 1 fira_ranging
> session_start 1

# Switch to hardware mode (with real device)
> mode_hw /dev/ttyUSB0
> get_device_info
> session_init 2 fira_ranging
> session_start 2

# Switch back to simulation
> mode_sim
> simulate_ranging
> quit
```

## Command Aliases

The shell supports command aliases for convenience:

```bash
# Create an alias
> alias ranging_flow "session_init 1 ranging && set_app_config 1 channel 5 && session_start 1"

# Use the alias
> ranging_flow

# List all aliases
> alias

# Remove an alias
> unalias ranging_flow
```

## Troubleshooting

### Common Issues and Solutions

1. **Hardware not found:**
   ```bash
   > mode_hw /dev/ttyUSB0
   # If this fails, check permissions:
   > ls -la /dev/ttyUSB*
   # You may need to run as root or add user to dialout group
   ```

2. **Command not found:**
   ```bash
   > unknown_command
   # Check available commands with help
   > help
   ```

3. **Response timeout:**
   ```bash
   # This may indicate hardware is not responding
   # Check connections and try again
   # Verify device is powered and responding
   ```

## Helpful Commands

```bash
> help                    # Show all available commands
> complete sess           # Show commands starting with 'sess'
> history                 # Show command history
> mode_info               # Show current mode (sim/hw)
> analyze_packet 20 08 00 00   # Analyze raw packet
```

This unified interface allows you to develop and test UWB applications in simulation mode, then seamlessly switch to real hardware for testing, without changing your command scripts or workflow.