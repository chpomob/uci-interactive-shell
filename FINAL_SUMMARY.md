# UCI Interactive Shell - Complete Feature Summary

**Hardware-first focus:** This project centers on robust UCI control of real hardware; the simulator only supports protocol validation and debugging.

## Overview
The UCI Interactive Shell has been enhanced from a simple simulation to a comprehensive UWB protocol implementation that supports both simulation mode and real hardware communication via character device files.

## Complete Feature Set

### 1. Protocol Compliance
- **Step 1**: Aligned all UCI protocol definitions with Android UWB specification
- **Proper Header Format**: Implemented correct UCI packet header structure [GID|PBF|MT][Opcode|R][Reserved][Length]
- **Message Types**: Support for COMMAND, RESPONSE, NOTIFICATION, DATA message types
- **Group IDs**: Complete GID support (CORE, SESSION_CONFIG, SESSION_CONTROL, DATA_CONTROL, TEST)
- **Opcodes**: Full opcode coverage for all core UCI commands

### 2. Enhanced Response Handling
- **Step 2**: Added complete response parsing for all basic UCI commands
- **CORE_DEVICE_INFO_RSP**: Full device information with UCI/MAC/PHY versions
- **Error Handling**: Proper status code interpretation and reporting
- **Payload Parsing**: Correct parsing of complex response payloads

### 3. Comprehensive TLV Support
- **Step 3**: Implemented complete TLV (Type-Length-Value) handling
- **SET_CONFIG/GET_CONFIG**: Dynamic configuration management with proper TLV format
- **Multi-TLV Support**: Handle multiple configuration parameters in single commands
- **Configuration Types**: Support for DEVICE_STATE, LOW_POWER_MODE and other config IDs
- **New Commands**: get_device_state, set_device_active, set_device_ready

### 4. Session Management
- **Step 4**: Complete session lifecycle management with persistent state
- **Session Context**: Track session states, configurations, and parameters
- **Session Commands**: session_init, session_deinit, session_start, session_stop, get_session_state
- **State Tracking**: Proper session state transitions (INIT, ACTIVE, IDLE, DEINIT)
- **Configuration Storage**: Persistent configuration storage per session

### 5. Notification Handling
- **Step 5**: Complete notification parsing and human-readable display
- **Device Status NTF**: Device state change notifications with meaningful descriptions
- **Session Status NTF**: Session state changes with reason codes
- **Data Credit NTF**: Data transfer credit availability notifications
- **Error Notifications**: Generic error and specific error condition notifications

### 6. Ranging Notification Support
- **Step 6**: Comprehensive UWB ranging notification handling
- **TWO_WAY Ranging**: Single and multi-target distance measurement parsing
- **OWR_AOA Ranging**: One-way ranging with angle-of-arrival measurements
- **Distance/Angle Reporting**: Human-readable distance (cm) and angle (degrees) values
- **Quality Metrics**: NLOS indicators, RSSI, FoM (Figure of Merit) values
- **MAC Address Support**: Both SHORT_ADDRESS and EXTENDED_ADDRESS formats

### 7. Hardware Communication Support
- **Step 7**: Real UWB hardware communication via character device files
- **Chardev Interface**: Direct communication with UWB devices through /dev/ttyUSB* or similar
- **Packet Framing**: Proper UCI packet construction and transmission
- **Response Handling**: Real-time reception and parsing of UCI responses
- **Asynchronous Notifications**: Support for unsolicited notifications from hardware
- **Error Recovery**: Timeout handling and communication error recovery
- **Test Framework**: Script for simulating hardware communication without real device
- **Verbose Mode**: Detailed debugging output for hardware communication

## Key Technical Improvements

### Architecture
- **Modular Design**: Separated simulation and hardware modes with common interface
- **Clean API**: Well-defined function interfaces for UCI packet handling
- **Resource Management**: Proper allocation and cleanup of system resources
- **Error Handling**: Comprehensive error checking and graceful failure handling

### Performance
- **Memory Efficient**: Minimal memory footprint with proper allocation/deallocation
- **Fast Parsing**: Optimized packet parsing with bounds checking
- **Low Latency**: Efficient communication with hardware devices

### Compatibility
- **Android UWB Spec**: Fully compliant with Android UWB protocol specification
- **Cross-Platform**: Portable C implementation with standard library dependencies
- **Extensible**: Easy to add new commands and notification types

## Available Commands

### Device Management
- `get_device_info` - Retrieve device information (UCI/MAC/PHY versions)
- `device_reset` - Reset the UWB device
- `get_caps_info` - Get device capabilities information
- `set_config` - Set device configuration parameters
- `get_config` - Get device configuration parameters
- `get_device_state` - Get current device state
- `set_device_active` - Set device to ACTIVE state
- `set_device_ready` - Set device to READY state

### Session Management
- `session_init` - Initialize a new UWB session
- `session_deinit` - Deinitialize an existing session
- `session_start` - Start a UWB session
- `session_stop` - Stop a UWB session
- `get_session_state` - Get current session state

### Application Configuration
- `set_app_config` - Set session application configuration
- `get_app_config` - Get session application configuration

### Simulation Commands
- `simulate_notification` - Simulate device status notification
- `simulate_session_status` - Simulate session status notification
- `simulate_data_credit` - Simulate data credit notification
- `simulate_ranging` - Simulate single-target ranging notification
- `simulate_multi_target_ranging` - Simulate multi-target ranging notification
- `demo_session_flow` - Demonstrate complete session lifecycle

### Hardware Communication
- `mode_hw <device_path>` / `hw_init <device_path>` (alias `hw_connect`) - Connect to a hardware transport
- `mode_sim` / `mode_info` - Switch between transports and check the active mode
- `hw_send <mt> <pbf> <gid> <oid> [payload_bytes...]` - Stream raw UCI packets to the connected device
- Standard commands (`get_device_info`, `session_start 1`, etc.) automatically target hardware once mode_hw is active

## Usage Examples

### Simulation Mode
```bash
./uci-shell
> get_device_info
> session_init
> session_start
> simulate_ranging
> session_stop
> quit
```

### Hardware Mode (with real UWB device)
```bash
./uci-shell
> mode_hw /dev/ttyUSB0          # Connect to hardware transport
> get_device_info               # All standard commands now talk to hardware
> session_init 1 fira_ranging   # Create a session on hardware
> session_start 1               # Begin live ranging
> hw_send 01 00 00 02           # Send CORE_DEVICE_INFO manually when needed
> mode_sim                      # Return to simulation mode
> quit
```

## Development and Testing

The UCI Interactive Shell is designed for both development and production use:

1. **Development**: Use simulation mode to develop UWB applications without hardware
2. **Testing**: Use comprehensive test scripts to validate UCI protocol implementation
3. **Production**: Use hardware mode to communicate with real UWB devices

The implementation provides a seamless transition from simulation to real hardware, making it ideal for UWB application development and testing.Unified UCI command interface implementation complete!
