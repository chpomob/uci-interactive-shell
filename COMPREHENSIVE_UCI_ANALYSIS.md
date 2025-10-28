# Comprehensive UCI Log and Packet Analysis

**Hardware-first focus:** This project centers on robust UCI control of real hardware; the simulator only supports protocol validation and debugging.

## Summary of UCI Data Sources Found

### 1. Real Log Files
- **File**: `logs/uwb_range_ntf.log`
- **Content**: Actual UWB ranging data from real hardware
- **Packet Count**: 12 hex-encoded UCI packets
- **Key Findings**:
  - RANGING_DATA notifications (GID=0x0B) showing distance measurements
  - Distances: 0cm, 4cm, 7cm, 12cm showing movement over time
  - Session ID 42 consistently used
  - Sequence numbers: 8, 9, 10, 11, 12

### 2. PDL Specification Test Packets  
- **File**: `uci_analysis/uwb/src/rust/uwb_uci_packets/uci_packets.pdl`
- **Content**: Official UCI packet definitions with test vectors
- **Packet Count**: 81 test packet examples covering all UCI commands
- **Coverage**: All UCI groups (CORE, SESSION_CONFIG, SESSION_CONTROL, VENDOR_ANDROID, TEST)

### 3. PCAPNG Logging Framework
- **Files**: `uci_analysis/uwb/src/rust/uwb_core/src/uci/*.rs`
- **Content**: Framework for capturing UCI packets in PCAPNG format
- **Features**: Complete packet capture with timestamps and metadata

## Packet Analysis Results

### Real Log Packets (from uwb_range_ntf.log)
```
NOTIFICATION -> RANGING_DATA (0x03) - Range data notifications
NOTIFICATION -> SESSION_CONTROL (0x00) - Session start notifications  
COMMAND -> VENDOR_SPECIFIC_A (0x00) - Vendor-specific commands
```

### PDL Test Packets Distribution
- **Commands**: 26 (32%)
- **Responses**: 30 (37%) 
- **Notifications**: 22 (27%)
- **Data Packets**: 3 (4%)

- **CORE Group**: 14 packets (17%)
- **SESSION_CONFIG Group**: 27 packets (33%)
- **SESSION_CONTROL Group**: 17 packets (21%)
- **TEST Group**: 16 packets (20%)
- **VENDOR_ANDROID Group**: 6 packets (7%)
- **VENDOR_RESERVED_B**: Likely used for RANGING_DATA (as seen in real logs)

## Key Insights

1. **Real UWB Behavior**: The real logs show actual ranging measurements between devices with changing distance (0cm → 7cm → 4cm → 7cm → 12cm), confirming these are from live UWB ranging operations.

2. **Standard Compliance**: The PDL specification provides complete compliance testing vectors covering the entire UCI command set.

3. **Vendor Extensions**: Different vendors use reserved GID ranges (0x09-0x0F) for proprietary extensions, with 0x0B commonly used for ranging data as seen in our real logs.

4. **Complete Ecosystem**: The codebase includes tools for packet generation (uci_packet_generator.py), analysis (our scripts), and capture (PCAPNG logger).

## Next Steps

1. Create PCAPNG captures from the real log data
2. Extend our C parser to handle more packet types
3. Implement packet analysis tools for the ranging data
4. Add support for vendor-specific extensions
5. Create visualization tools for UWB ranging data