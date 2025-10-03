# UCI Protocol Analysis

## Overview

The UCI (Ultra-wideband Communication Interface) protocol is defined by the FiRa Consortium and used in Ultra-Wideband (UWB) communication systems. This analysis compares the implementation found in the Android UWB repository with our current project.

## Protocol Structure

### Message Types
- `DATA` (0x00)
- `COMMAND` (0x01) 
- `RESPONSE` (0x02)
- `NOTIFICATION` (0x03)

### Packet Header Format
```
Bits: [2:0] Message Type (MT)
      [3]   Packet Boundary Flag (PBF)
      [7:4] Group ID (GID) for control packets; Data Packet Format for data packets
```

### Group IDs
- `CORE` (0x00) - Core device management
- `SESSION_CONFIG` (0x01) - Session configuration
- `SESSION_CONTROL` (0x02) - Session control operations
- `DATA_CONTROL` (0x03) - Data transfer operations
- `TEST` (0x0d) - Test operations
- `VENDOR_ANDROID` (0x0c) - Android vendor commands
- Vendor-specific ranges (0x09, 0x0a, 0x0b, 0x0e, 0x0f)

## Key Commands and Operations

### Core Commands (GID = 0x00)
- `CORE_DEVICE_RESET` (0x00) - Reset the UWB device
- `CORE_DEVICE_STATUS_NTF` (0x01) - Device status notification
- `CORE_DEVICE_INFO` (0x02) - Get device information
- `CORE_GET_CAPS_INFO` (0x03) - Get device capabilities
- `CORE_SET_CONFIG` (0x04) - Set device configuration
- `CORE_GET_CONFIG` (0x05) - Get device configuration
- `CORE_QUERY_UWBS_TIMESTAMP` (0x08) - Query UWB timestamp

### Session Configuration Commands (GID = 0x01)
- `SESSION_INIT` (0x00) - Initialize a ranging session
- `SESSION_DEINIT` (0x01) - Deinitialize a ranging session
- `SESSION_SET_APP_CONFIG` (0x03) - Set application configuration
- `SESSION_GET_APP_CONFIG` (0x04) - Get application configuration
- `SESSION_GET_STATE` (0x06) - Get session state

### Session Control Commands (GID = 0x02)
- `SESSION_START` (0x00) - Start ranging session
- `SESSION_STOP` (0x01) - Stop ranging session
- `SESSION_GET_RANGING_COUNT` (0x03) - Get ranging count

## Key Differences from Our Implementation

### 1. Packet Boundary Flag
- **Android**: Uses `PacketBoundaryFlag` enum with `COMPLETE` (0x00) and `NOT_COMPLETE` (0x01)
- **Our Project**: Uses similar concept but with different naming (`PBF`)

### 2. Message Types
- **Android**: Uses `MessageType` enum with 3 bits (COMMAND=0x01, RESPONSE=0x02, NOTIFICATION=0x03, DATA=0x00)
- **Our Project**: Uses similar message types but defined as constants

### 3. Status Codes
- **Android**: Comprehensive status codes including:
  - Generic: `UCI_STATUS_OK`, `UCI_STATUS_REJECTED`, `UCI_STATUS_FAILED`, etc.
  - Session-specific: `UCI_STATUS_SESSION_NOT_EXIST`, `UCI_STATUS_SESSION_ACTIVE`, etc.
  - Ranging-specific: `UCI_STATUS_RANGING_RX_TIMEOUT`, `UCI_STATUS_RANGING_TX_FAILED`, etc.
  - Vendor-specific ranges

### 4. Configuration Types
- **Android**: Extensive configuration TLV types with over 70 different configuration IDs
- **Our Project**: Limited to basic configurations

### 5. Data Packets
- **Android**: Includes data packet formats (`DATA_SND`, `DATA_RCV`, `RADAR_DATA_MESSAGE`)
- **Our Project**: Primarily focused on control packets

## Packet Structure Examples

### Core Device Info Request/Response
```
GetDeviceInfoCmd: No payload
GetDeviceInfoRsp: 
  - status: StatusCode
  - uci_version: 16-bit
  - mac_version: 16-bit  
  - phy_version: 16-bit
  - uci_test_version: 16-bit
  - vendor_spec_info: variable array
```

### Session Initialization
```
SessionInitCmd:
  - session_id: 32-bit
  - session_type: SessionType enum

SessionInitRsp:
  - status: StatusCode
  - session_handle: 32-bit (for FIRA v2.0)
```

## Protocol Enhancements Needed

Based on the Android implementation, our project could benefit from:

1. **More Comprehensive TLV Support**: Support for all configuration TLV types
2. **Proper Packet Fragmentation**: Handling of multi-packet messages
3. **Extended Status Codes**: More detailed error reporting
4. **Data Transfer Packets**: Support for actual UWB data transfer
5. **Session Management**: Complete session lifecycle support
6. **Test Commands**: RF testing capabilities

## Key Takeaways

The Android UWB repository provides a full, comprehensive implementation of the UCI protocol as specified by the FiRa Consortium. Our current implementation is a basic simulation that covers the fundamental packet structure but lacks the full feature set of the actual specification.

The protocol is designed to be extensible with vendor-specific command ranges and has detailed specifications for ranging, data transfer, and testing operations.