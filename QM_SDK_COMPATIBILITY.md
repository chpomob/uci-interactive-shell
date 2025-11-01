# QM SDK Compatibility Guide

## Overview

This document explains the UCI Interactive Shell's approach to handling compatibility with Qorvo QM SDK implementations, particularly regarding Group ID (GID) assignments.

## Standard UCI Specification vs QM SDK Differences

### Standard UCI Specification
In the standard FiRa UCI specification:
- **GID 0x0B** is designated for **RANGING_DATA** notifications
- This is used for ranging data notifications like `RANGE_DATA_NTF` (Opcode 0x03)

### QM SDK Variations
Some Qorvo QM SDK implementations may use GID 0x0B differently:
- **GID 0x0B** may be used for **vendor-specific commands** instead of ranging data
- Ranging data notifications may use a different mechanism or GID

## Implementation Approach

The UCI Interactive Shell follows the standard UCI specification by default:

```
#define CORE            0x00
#define SESSION_CONFIG  0x01
#define SESSION_CONTROL 0x02
#define DATA_CONTROL    0x03
#define RANGING_DATA    0x0B  // Standard Fira UCI GID for ranging data notifications
#define VENDOR_ANDROID  0x0C
#define TEST            0x0D
```

## QM SDK Compatibility Options

### Build-Time Configuration
The Makefile includes a commented build option for QM SDK compatibility:

```makefile
# QM SDK Compatibility Build Options
# Uncomment the following line to enable QM SDK compatibility mode
# QM_SDK_COMPAT=-DQM_SDK_COMPAT
```

### Runtime Adaptation
Developers working with QM SDK implementations should:

1. **Verify GID Usage**: Check the specific QM SDK documentation for GID assignments
2. **Adjust Packet Creation**: Modify packet creation to use the correct GIDs
3. **Update Decoders**: Ensure decoders match the actual GID/opcodes used

## Recommendations for QM SDK Integration

### 1. Verification Step
Before integrating with QM SDK:
```bash
# Analyze actual packets from QM SDK implementation
./uci-shell analyze_packet <hex_dump>
```

### 2. Custom Mapping
If GID 0x0B is used for vendor commands in your QM SDK:
- Map ranging data notifications to a different GID
- Or modify the vendor command handling to differentiate from standard ranging data

### 3. Conditional Compilation
Use the QM_SDK_COMPAT flag to conditionally compile different GID mappings:

```c
#ifdef QM_SDK_COMPAT
#define QM_SDK_RANGING_DATA_GID 0x0C  // Example alternative GID
#else
#define QM_SDK_RANGING_DATA_GID RANGING_DATA  // Standard 0x0B
#endif
```

## Testing Compatibility

### Standard Test
```bash
# Test with standard UCI specification
./uci-shell simulate_ranging
```

### Vendor-Specific Test
```bash
# Test with vendor-specific packet formats
./uci-shell analyze_packet <qm_sdk_packet_hex>
```

## Conclusion

The UCI Interactive Shell maintains compliance with the standard FiRa UCI specification while providing hooks for vendor-specific adaptations. Developers integrating with QM SDK should:

1. Verify the specific GID mappings used in their QM SDK version
2. Use the provided build options for compatibility adaptations
3. Test with actual packets from their QM SDK implementation
4. Adjust GID/opcodes as needed for proper interoperability

The modular design allows for easy adaptation to different vendor implementations while preserving standard UCI compliance for interoperability with other UCI-compliant systems.