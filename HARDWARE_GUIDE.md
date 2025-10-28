# UCI Hardware Communication Guide

**Hardware-first focus:** This project centers on robust UCI control of real hardware; the simulator only supports protocol validation and debugging.

## Overview

This guide explains how to use the UCI Interactive Shell with real UWB hardware devices through character device files.

## Hardware Setup

### Supported Devices
The UCI Interactive Shell supports any UWB device that communicates via UCI protocol over a character device interface, including:
- Android-compatible UWB chips (e.g., Qorvo DW3000, NXP SR150, etc.)
- USB-UWB adapters that expose a serial interface
- UART-connected UWB modules
- Bluetooth-to-UWB bridge devices

### Device Connections
Common device paths for UWB hardware:
- `/dev/ttyUSB0` - USB-UWB adapters
- `/dev/ttyACM0` - CDC-ACM UWB devices
- `/dev/ttyS0` - Serial/UART UWB modules
- Custom paths for platform-specific UWB drivers

## Connecting to Hardware

### Method 1: Interactive Shell
```bash
# Start the interactive shell
./uci-shell

# Connect to the UWB device transport
> mode_hw /dev/ttyUSB0
> mode_info                     # Confirm HARDWARE mode

# Issue standard commands over hardware
> get_device_info
> device_reset
> session_init 1 fira_ranging
> session_start 1
> get_session_state 1
> session_stop 1
> mode_sim                      # Return to simulation when done
> quit
```

### Method 2: Command Line Arguments
```bash
# Direct hardware communication (future enhancement)
./uci-shell --hw-connect /dev/ttyUSB0 --command "get_device_info"
```

## Hardware Communication Protocol

### UCI Packet Format
The UCI Interactive Shell handles all UCI packet formatting according to the Android UWB specification:

```
UCI Control Packet Header:
[Byte 0]: [GID:4][PBF:1][MT:3] 
[Byte 1]: [Opcode:6][Reserved:2]
[Byte 2]: Reserved
[Byte 3]: Payload Length

UCI Data Packet Header:
[Byte 0]: [Data Packet Format:4][PBF:1][MT:3]
[Byte 1]: Reserved
[Byte 2]: Reserved  
[Byte 3]: Payload Length
```

### Message Types (MT)
- `0x00` - DATA
- `0x01` - COMMAND
- `0x02` - RESPONSE
- `0x03` - NOTIFICATION

### Packet Boundary Flags (PBF)
- `0x00` - COMPLETE
- `0x01` - START
- `0x02` - CONT
- `0x03` - END

### Group IDs (GID)
- `0x00` - CORE
- `0x01` - SESSION_CONFIG
- `0x02` - SESSION_CONTROL
- `0x03` - DATA_CONTROL
- `0x0C` - VENDOR_ANDROID
- `0x0D` - TEST

## Common UCI Commands for Hardware

### Device Management
```bash
# Get device information
> get_device_info

# Reset device
> device_reset

# Get device capabilities
> get_caps_info

# Set device configuration
> set_config device_state active
> set_config low_power_mode on

# Get device configuration
> get_config device_state
```

### Session Management
```bash
# Initialize session (ID 1, FiRa ranging)
> session_init 1 fira_ranging

# Start session
> session_start 1

# Stop session
> session_stop 1

# Get session state
> get_session_state 1

# Deinitialize session
> session_deinit 1
```

### Application Configuration
```bash
# Set application configuration
> set_app_config 1 device_type responder
> set_app_config 1 channel 5

# Get application configuration
> get_app_config 1 device_type
```

## Testing Hardware Communication

### Using the Built-in Test Scripts
Hardware-specific helper scripts have been removed from the repository. Use the manual testing approaches below or craft project-specific harnesses as needed.

### Manual Testing with Terminal Tools
```bash
# Monitor UWB device communication
sudo screen /dev/ttyUSB0 115200

# Or using minicom
sudo minicom -D /dev/ttyUSB0 -b 115200

# Or using picocom
sudo picocom -b 115200 /dev/ttyUSB0
```

### Creating Virtual UWB Devices for Testing
```bash
# Create a virtual UWB device pair for testing
socat -d -d pty,raw,echo=0,link=/tmp/uwb_master pty,raw,echo=0,link=/tmp/uwb_slave &

# Connect to the virtual device
./uci-shell
> mode_hw /tmp/uwb_slave
> get_device_info
```

## Troubleshooting Hardware Issues

### Common Problems and Solutions

1. **Permission Denied Errors**
   ```bash
   # Add user to dialout group
   sudo usermod -a -G dialout $USER
   
   # Or run with sudo
   sudo ./uci-shell
   ```

2. **Device Not Found**
   ```bash
   # Check available serial devices
   ls /dev/ttyUSB* /dev/ttyACM*
   
   # Check if device is properly connected
   dmesg | tail
   ```

3. **Communication Failures**
   ```bash
   # Check device configuration
   stty -F /dev/ttyUSB0
   
   # Reset device configuration
   stty -F /dev/ttyUSB0 115200 raw -echo -echoe -echok
   ```

4. **No Response from Device**
   ```bash
   # Verify device is powered and functioning
   # Check wiring and connections
   # Ensure correct baud rate (typically 115200 for UWB devices)
   ```

## Debugging Hardware Communication

### Enabling Verbose Mode
```bash
# Start shell with verbose output
./uci-shell
> mode_hw /dev/ttyUSB0
# Verbose output will show:
# - Packet headers and payloads
# - Communication status
# - Error conditions
# - Device responses
```

### Analyzing UCI Packets
The shell displays UCI packets in human-readable format:
```
Sending UCI packet:
  Header: 20 08 00 00
  Payload: 
Simulating UCI response...
Received UCI packet:
  MT: 0x2
  PBF: 0x0
  GID: 0x0
  Opcode: 0x02
  Payload Length: 9
  Payload: 00 01 00 02 00 02 00 01 00 
  Status: 0x00
  UCI Version: 0x0100
  MAC Version: 0x0200
  PHY Version: 0x0200
  UCI Test Version: 0x0100
```

### Logging Communication
```bash
# Log all communication to file
./uci-shell 2>&1 | tee uci_communication.log

# Filter for specific packets
./uci-shell 2>&1 | grep -E "(Sending|Received|Header|Payload)"
```

## Advanced Hardware Features

### Multi-Device Support
The UCI Interactive Shell can manage multiple UWB devices simultaneously:
```bash
# Connect to multiple devices
./uci-shell
> mode_hw /dev/ttyUSB0  # Primary device
> mode_hw /dev/ttyUSB1  # Secondary device (future enhancement)
```

### Ranging Data Processing
The shell processes ranging notifications with full precision:
```bash
> simulate_ranging
=== Simulating UWB Ranging Notification ===
Received UCI packet:
  MT: 0x3
  PBF: 0x0
  GID: 0x2
  Opcode: 0x00
  Payload Length: 41
  Sequence Number: 1
  Session Token: 0x01020304
  RCR Indicator: 0x01
  Current Ranging Interval: 100 ms
  Ranging Measurement Type: 0x01 (TWO_WAY)
  MAC Address Indicator: SHORT_ADDRESS
  HUS Primary Session ID: 0x00000000
  Ranging Measurements (offset=20, payload_len=41):
    Number of Two-Way Measurements: 1
    Measurement 1:
      MAC Address: 0x1234
      Status: 0x00 (OK)
      NLOS: NO
      Distance: 100 cm
      AoA Azimuth: 20 degrees (FoM: 8)
      AoA Elevation: 5 degrees (FoM: 7)
      Destination AoA Azimuth: 16 degrees (FoM: 6)
      Destination AoA Elevation: 3 degrees (FoM: 9)
      Slot Index: 2
      RSSI: -32 dBm
=== Ranging Simulation Complete ===
```

### Session Management
Complete session lifecycle management with persistent state:
```bash
# Initialize session
> session_init 01 02 03 04 00  # Session ID 0x01020304, FIRA_RANGING_SESSION

# Set application configuration
> set_app_config 01 02 03 04 02 00 01 01 04 00 01 02  # Multiple config TLVs

# Start ranging session
> session_start 01 02 03 04

# Monitor session status
> get_session_state 01 02 03 04

# Stop ranging session
> session_stop 01 02 03 04

# Deinitialize session
> session_deinit 01 02 03 04
```

## Best Practices for Hardware Development

1. **Always Initialize Properly**
   ```bash
   # Start with device reset
   > device_reset
   
   # Get device info to verify connectivity
   > get_device_info
   
   # Initialize session before use
   > session_init <session_id> <session_type>
   ```

2. **Handle Notifications Gracefully**
   ```bash
   # Expect and process notifications
   > session_start <session_id>
   # Will receive SESSION_STATUS_NTF with ACTIVE state
   ```

3. **Use Proper Error Checking**
   ```bash
   # Check return status of all commands
   > get_device_info
   # Look for UCI_STATUS_OK in response
   ```

4. **Manage Session Lifecycle**
   ```bash
   # Follow proper session flow:
   # 1. session_init
   # 2. set_app_config
   # 3. session_start
   # 4. (perform ranging operations)
   # 5. session_stop
   # 6. session_deinit
   ```

5. **Configure Appropriately**
   ```bash
   # Set proper application configuration for your use case
   > set_app_config <session_id> <num_tlvs> <cfg_id> <len> <value>...
   ```

## Future Enhancements

Planned improvements for hardware communication:
- Support for multiple concurrent UWB devices
- Asynchronous notification handling with callback system
- Advanced ranging data visualization and logging
- Integration with positioning algorithms
- Support for data transfer sessions
- Enhanced error recovery and retry mechanisms
- Platform-specific optimizations for different UWB chips
