# UCI Header Specification and Implementation

**Hardware-first focus:** This project centers on robust UCI control of real hardware; the simulator only supports protocol validation and debugging.

## Overview

This document describes the UCI (Ultra-Wideband Communication Interface) packet header structure as implemented in this codebase, with emphasis on the correct **LSB (Least Significant Bit first)** bit ordering used in the Android UWB specification.

## Header Structure

UCI packets consist of a 4-byte header followed by a variable-length payload:

```
Byte 0: [GID:4][PBF:1][MT:3]
Byte 1: [Opcode:6][Reserved:2]
Byte 2: Reserved (0x00)
Byte 3: Payload Length
```

### Byte 0 Layout (LSB First)

```
Bit:    7   6   5   4   3   2   1   0
        +-----------------------+---+-------+
        |  MT   |PBF|     GID       |
        +-----------------------+---+-------+
```

- **Bits [3:0]**: Group ID (GID) - 4 bits, LSB position
- **Bit 4**: Packet Boundary Flag (PBF) - 1 bit
- **Bits [7:5]**: Message Type (MT) - 3 bits, MSB position

### Byte 1 Layout (LSB First)

```
Bit:    7   6   5   4   3   2   1   0
        +-------+-----------------------+
        | Rsvd  |       Opcode          |
        +-------+-----------------------+
```

- **Bits [5:0]**: Opcode ID - 6 bits, LSB position
- **Bits [7:6]**: Reserved - 2 bits, MSB position

## Implementation

### Header Packing Functions

The correct implementation uses these helper functions defined in `include/uci.h`:

```c
static inline unsigned char uci_pack_first_byte(unsigned char message_type,
                                                unsigned char packet_boundary,
                                                unsigned char group_id) {
    return (unsigned char)((group_id & 0x0F) |
                           ((packet_boundary & 0x01) << 4) |
                           ((message_type & 0x07) << 5));
}

static inline unsigned char uci_pack_second_byte(unsigned char opcode_id) {
    return (unsigned char)(opcode_id & 0x3F);  // opcode occupies lower 6 bits
}
```

### Header Unpacking Functions

```c
static inline unsigned char get_gid(const struct uci_packet_header *header) {
    return header->first_byte & 0x0F;
}

static inline unsigned char get_pbf(const struct uci_packet_header *header) {
    return (header->first_byte >> 4) & 0x01;
}

static inline unsigned char get_mt(const struct uci_packet_header *header) {
    return (header->first_byte >> 5) & 0x07;
}

static inline unsigned char get_opcode(const struct uci_packet_header *header) {
    return header->second_byte & 0x3F;
}
```

### Complete Header Creation

Use the `set_header_values()` function which internally uses the correct packing:

```c
set_header_values(header, message_type, packet_boundary, group_id, opcode_id, payload_length);
```

## Real Packet Examples

### Example 1: RANGING_DATA_NTF (RANGING_DATA group notification)

```
Raw bytes: 6B 03 00 21
```

**Analysis:**
- Byte 0 = 0x6B = 0b01101011
  - GID (bits [3:0]) = 0b1011 = 0x0B (RANGING_DATA)
  - PBF (bit 4) = 0b0 = 0x0 (COMPLETE)
  - MT (bits [7:5]) = 0b011 = 0x3 (NOTIFICATION)
- Byte 1 = 0x03 = Opcode 0x03 (RANGE_DATA_NTF_OPCODE)
- Byte 2 = 0x00 = Reserved
- Byte 3 = 0x21 = 33 bytes payload

### Example 2: SESSION_CONTROL Notification

```
Raw bytes: 62 00 00 38
```

**Analysis:**
- Byte 0 = 0x62 = 0b01100010
  - GID (bits [3:0]) = 0b0010 = 0x02 (SESSION_CONTROL)
  - PBF (bit 4) = 0b0 = 0x0 (COMPLETE)
  - MT (bits [7:5]) = 0b011 = 0x3 (NOTIFICATION)
- Byte 1 = 0x00 = Opcode 0x00 (SESSION_START/SESSION_INFO_NTF)
- Byte 2 = 0x00 = Reserved
- Byte 3 = 0x38 = 56 bytes payload

### Example 3: CORE_DEVICE_INFO Command

```
Raw bytes: 20 02 00 00
```

**Analysis:**
- Byte 0 = 0x20 = 0b00100000
  - GID (bits [3:0]) = 0b0000 = 0x00 (CORE)
  - PBF (bit 4) = 0b0 = 0x0 (COMPLETE)
  - MT (bits [7:5]) = 0b001 = 0x1 (COMMAND)
- Byte 1 = 0x02 = Opcode 0x02 (CORE_DEVICE_INFO)
- Byte 2 = 0x00 = Reserved
- Byte 3 = 0x00 = 0 bytes payload

### Example 4: SESSION_INIT Command

```
Raw bytes: 21 00 00 05
```

**Analysis:**
- Byte 0 = 0x21 = 0b00100001
  - GID (bits [3:0]) = 0b0001 = 0x01 (SESSION_CONFIG)
  - PBF (bit 4) = 0b0 = 0x0 (COMPLETE)
  - MT (bits [7:5]) = 0b001 = 0x1 (COMMAND)
- Byte 1 = 0x00 = Opcode 0x00 (SESSION_INIT)
- Byte 2 = 0x00 = Reserved
- Byte 3 = 0x05 = 5 bytes payload

## Field Values

### Message Type (MT)

| Value | Name | Description |
|-------|------|-------------|
| 0x00 | DATA | Data packet |
| 0x01 | COMMAND | Command from host to device |
| 0x02 | RESPONSE | Response from device to host |
| 0x03 | NOTIFICATION | Unsolicited notification from device |

### Group ID (GID)

| Value | Name | Description |
|-------|------|-------------|
| 0x00 | CORE | Core device management |
| 0x01 | SESSION_CONFIG | Session configuration |
| 0x02 | SESSION_CONTROL | Session control |
| 0x03 | DATA_CONTROL | Data transfer control |
| 0x0B | RANGING_DATA | Ranging data notifications |
| 0x0C | VENDOR_ANDROID | Android vendor extensions |
| 0x0D | TEST | Test commands |

### Packet Boundary Flag (PBF)

| Value | Name | Description |
|-------|------|-------------|
| 0x00 | COMPLETE | Complete packet (not fragmented) |
| 0x01 | NOT_COMPLETE | Fragmented packet, more fragments follow |

## Common Mistakes to Avoid

### ❌ WRONG: MSB-first interpretation

```c
// WRONG: This would put MT in the LSB position
unsigned char wrong = (gid << 5) | (pbf << 4) | mt;
```

### ✅ CORRECT: LSB-first interpretation

```c
// CORRECT: GID in LSB position, MT in MSB position
unsigned char correct = (gid & 0x0F) | ((pbf & 0x01) << 4) | ((mt & 0x07) << 5);
```

## Testing Header Implementation

The `analyze_packet` command in the UCI shell can be used to verify header parsing:

```bash
./uci-shell
> analyze_packet 6B 03 00 21
```

Expected output:
```
Message Type (MT): 0x3 (NOTIFICATION)
Packet Boundary Flag (PBF): 0x0 (COMPLETE)
Group ID (GID): 0xB (RANGING_DATA)
Opcode: 0x03
```

## References

- Android UWB Specification: `uci_analysis/uwb/src/rust/uwb_uci_packets/uci_packets.pdl`
- Implementation: `include/uci.h`
- Test cases: `tests/test_uci_functions.c`
- Real packet parser: `parse_real_logs.c`
