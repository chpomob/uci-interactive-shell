# UCI Protocol Deep Dive: Android Implementation vs Our Project

**Hardware-first focus:** This project centers on robust UCI control of real hardware; the simulator only supports protocol validation and debugging.

## Overview

This document provides a comprehensive analysis of the UCI (Ultra-wideband Communication Interface) protocol based on the Android Open Source Project implementation and how it compares to our current project.

## Android UWB Repository Structure

### Location
- Repository: https://android.googlesource.com/platform/external/uwb
- Key file: `src/rust/uwb_uci_packets/uci_packets.pdl`
- Implementation: `src/rust/uwb_uci_packets/src/lib.rs`

### Protocol Definition Language (PDL)
The Android implementation uses a Protocol Definition Language to specify all valid UCI packet formats, which gets compiled into Rust code for parsing and generation.

## Key Protocol Elements

### Message Types (MT)
- `DATA` (0): Data packets
- `COMMAND` (1): Commands sent to UWB controller
- `RESPONSE` (2): Responses from UWB controller
- `NOTIFICATION` (3): Unsolicited notifications from controller

### Group IDs (GID)
- `CORE` (0x00): Basic device management
- `SESSION_CONFIG` (0x01): Session configuration operations
- `SESSION_CONTROL` (0x02): Session control operations
- `DATA_CONTROL` (0x03): Data transfer operations
- `TEST` (0x0D): Testing operations
- `VENDOR_ANDROID` (0x0C): Android-specific commands

### Packet Format

#### Control Packet Header
```
Byte 0: [GID:4][PBF:1][MT:3]
  - GID: Group ID (4 bits)
  - PBF: Packet Boundary Flag (1 bit) 
  - MT: Message Type (3 bits)

Byte 1: [Opcode:6][R:2] 
  - Opcode: Operation code (6 bits)
  - R: Reserved (2 bits)

Byte 2: Reserved

Byte 3: Payload Length
```

## Comparison with Our Project

### Current Implementation Strengths
1. **Basic Structure**: Correct header format with MT, GID, OID (opcode)
2. **Core Commands**: Implementation of basic UCI commands (get device info, session ops)
3. **Packet Parsing**: Basic functionality to parse and display UCI packets
4. **Simulation**: Ability to simulate responses to commands

### Missing Elements from Android Spec
1. **Complete Opcode Support**: Limited to a subset of all possible opcodes
2. **Data Packets**: Missing DATA_SND/DATA_RCV packet types
3. **Proper TLV Handling**: Limited configuration TLV support
4. **Session Management**: Incomplete session lifecycle support
5. **Error Handling**: Limited status code support
6. **Fragmentation**: No support for fragmented packets  
7. **Vendor Commands**: Missing Android vendor-specific commands

### Header Structure Differences
- **Android**: Uses UciPacketHal, UciControlPacketHal abstractions with proper PDL
- **Our Project**: Simpler struct-based approach

## Key Takeaways

### For Protocol Understanding
1. The UCI protocol is extensive with over 70 configuration types and comprehensive status codes
2. The protocol supports complex session management, ranging operations, and data transfer
3. Proper byte-level format is critical: `[GID|PBF|MT][OPCODE|RES][RES][LEN][PAYLOAD]`

### For Project Enhancement
1. **Expand Configuration Support**: Implement all AppConfigTlvType and CapTlvType values
2. **Add Data Transfer**: Implement UCI data packet formats (DATA_SND/DATA_RCV)
3. **Complete Status Codes**: Support full range of StatusCode values
4. **Session Operations**: Complete SessionConfig and SessionControl command sets
5. **Testing Commands**: Add RF testing capabilities from TEST group

### Learning Resources
1. **Primary Spec**: `uci_analysis/uwb/src/rust/uwb_uci_packets/uci_packets.pdl`
2. **Analysis**: `uci_analysis/UCI_PROTOCOL_ANALYSIS.md`
3. **Packet Generator**: `uci_analysis/uci_packet_generator.py`

## Conclusion

The Android UWB repository provides a production-ready, comprehensive implementation of the UCI protocol that serves as an excellent reference for understanding the complete specification. Our project provides a functional simulation but would benefit from incorporating more of the complete protocol specification, particularly around configuration TLVs, data transfer operations, and complete status/error handling.

The Python packet generator demonstrates how to properly format UCI packets according to the Android specification, which can be used for testing and development.