# UCI analyze_packet Command Enhancement Proposal

**Hardware-first focus:** This project centers on robust UCI control of real hardware; the simulator only supports protocol validation and debugging.

## Overview
This document proposes enhancements to the existing `analyze_packet` command based on QM SDK patterns and insights to improve the analysis capabilities of the UCI Interactive Shell.

## Current Implementation Status
The `analyze_packet` command already has:
- ✅ Hex byte parsing capabilities
- ✅ Colorized output via UI enhancements
- ✅ Support for various UCI packet types (CORE, SESSION_CONFIG, SESSION_CONTROL, DATA, etc.)
- ✅ Proper header field extraction and display
- ✅ Segmentation/fragmentation handling

## Proposed Enhancements Based on QM SDK Patterns

### 1. Enhanced TLV Analysis
The QM SDK uses shared helpers like `tvs_to_bytes`, `tlvs_from_bytes` for TLV conversions. We should enhance the analyzer to:

```c
// Proposed addition to uci_packet_analyzer.c
static void analyze_tlv_structure(unsigned char* payload, int payload_len, int indent_level) {
    if (payload_len < 2) {
        return; // Not enough for a TLV
    }
    
    // Track TLV processing for better analysis
    int offset = 0;
    int tlv_count = 0;
    
    while (offset + 2 <= payload_len) {
        unsigned char tlv_type = payload[offset];
        unsigned char tlv_len = payload[offset + 1];
        offset += 2;
        
        if (offset + tlv_len > payload_len) {
            if (ui_color_enabled) {
                printf("%s%*sWarning: Incomplete TLV at offset %d%s\n", 
                       ANSI_COLOR_YELLOW, indent_level * 4, "", offset - 2, ANSI_RESET);
            } else {
                printf("%*sWarning: Incomplete TLV at offset %d\n", indent_level * 4, "", offset - 2);
            }
            break;
        }
        
        printf("%*sTLV[%d]: Type=0x%02X, Length=%d, Value: ", 
               indent_level * 4, "", tlv_count, tlv_type, tlv_len);
        
        for (int i = 0; i < tlv_len; i++) {
            printf("%02X ", payload[offset + i]);
        }
        printf("\n");
        
        // Try to interpret common configuration TLVs
        interpret_common_tlv(tlv_type, payload + offset, tlv_len, indent_level);
        
        offset += tlv_len;
        tlv_count++;
    }
}
```

### 2. Enhanced Data Message Analysis
Following the QM SDK's approach to data message handling, improve the analysis for different data packet formats:

```c
// Enhanced data message payload analysis with better structure identification
static void enhanced_analyze_data_message_payload(unsigned char dpf,
                                                 unsigned char pbf,
                                                 const unsigned char *payload,
                                                 int payload_len) {
    // Use the builder/parser context approach from QM SDK
    if (dpf == DATA_PACKET_FORMAT_SEND && payload_len >= UCI_DATA_MESSAGE_SND_HEADER) {
        // Extract fields using builder approach
        uint32_t session_handle = read_u32_le(payload);
        uint64_t destination_address = read_u64_le(payload + 4);
        uint16_t sequence_number = read_u16_le(payload + 12);
        uint16_t declared_len = read_u16_le(payload + 14);
        
        // Show field interpretation in analysis
        printf("  Data Message SND Analysis:\n");
        printf("    Session Handle: 0x%08X (Valid: %s)\n", session_handle, 
               is_valid_session_handle(session_handle) ? "Yes" : "No");
        printf("    Destination: 0x%016llX\n", (unsigned long long)destination_address);
        printf("    Seq No: %u\n", sequence_number);
        printf("    Declared Length: %u bytes\n", declared_len);
        
        // Show actual vs declared length comparison
        size_t actual_payload = (payload_len > UCI_DATA_MESSAGE_SND_HEADER) ? 
                                (payload_len - UCI_DATA_MESSAGE_SND_HEADER) : 0;
        printf("    Actual Payload: %zu bytes %s\n", actual_payload,
               (actual_payload == declared_len) ? "(Matches)" : 
               (actual_payload < declared_len) ? "(Truncated)" : "(Overflow)");
    }
}
```

### 3. Enhanced Error Reporting
Following the QM SDK's approach to error handling and status reporting:

```c
// Enhanced error analysis with more specific information
static void enhanced_error_analysis(unsigned char status_code) {
    printf("  Status Analysis: ");
    
    switch (status_code) {
        case UCI_STATUS_OK:
            printf("Operation completed successfully\n");
            break;
        case UCI_STATUS_REJECTED:
            printf("Request rejected by device (possible state issue)\n");
            break;
        case UCI_STATUS_INVALID_PARAM:
            printf("Invalid parameter(s) provided\n");
            break;
        case UCI_STATUS_UNKNOWN_GID:
            printf("Unknown Group ID - check GID value and device capabilities\n");
            break;
        case UCI_STATUS_UNKNOWN_OID:
            printf("Unknown Opcode ID - check OID value and device support\n");
            break;
        default:
            printf("Unknown status code 0x%02X\n", status_code);
            break;
    }
}
```

### 4. Enhanced Session State Analysis
Add contextual analysis based on session state:

```c
// Function to analyze session-related packets with context
static void analyze_session_context(unsigned char* payload, int payload_len, 
                                   unsigned char opcode, unsigned char gid) {
    if ((gid == SESSION_CONFIG || gid == SESSION_CONTROL) && payload_len >= 4) {
        uint32_t session_token = read_u32_le(payload);
        printf("  Session Context Analysis:\n");
        printf("    Session Token: 0x%08X\n", session_token);
        
        // Check if session exists and its state
        int session_idx = find_session_by_token_or_id(session_token);
        if (session_idx >= 0) {
            printf("    Session Status: Active (Index: %d)\n", session_idx);
            printf("    Session State: %s\n", get_session_state_string(session_idx));
        } else {
            printf("    Session Status: Not found in current session table\n");
        }
    }
}
```

### 5. Enhanced Command Line Analysis Options
Add more analysis options to the existing `analyze_packet` command:

**New usage patterns:**
```
> analyze_packet 20 08 00 00                    # Basic analysis (current)
> analyze_packet -v 20 08 00 00               # Verbose analysis 
> analyze_packet -t 20 08 00 00               # TLV analysis only
> analyze_packet -d 21 00 00 05 00 00 00 01 00  # Data message details
```

### 6. Additional Analysis Features

**Packet Chain Analysis:** Following the QM SDK's `uci_packet_recv()` pattern of tracking partially assembled chains:

```c
// Function to analyze fragmentation chains based on PBF
static void analyze_fragmentation_chain(unsigned char pbf, unsigned char* packet, size_t packet_len) {
    printf("  Fragmentation Analysis:\n");
    switch(pbf) {
        case COMPLETE:
            printf("    Single packet - no fragmentation\n");
            break;
        case NOT_COMPLETE:
            printf("    Start of fragmented message - expecting more fragments\n");
            break;
        default:
            printf("    Fragment part (PBF: 0x%02X) - continuation expected\n", pbf);
            break;
    }
}
```

## Implementation Plan

### Phase 1: Core Enhancement
1. Add detailed TLV analysis functions
2. Enhance error code interpretation
3. Add session context analysis

### Phase 2: Advanced Features  
1. Add verbose analysis option
2. Implement packet chain analysis
3. Add data message structure analysis

### Phase 3: User Experience
1. Add more command line options
2. Improve color coding for different analysis types
3. Add summary statistics

## Benefits

1. **Better Protocol Understanding**: More detailed analysis helps developers understand UCI protocol behavior
2. **Debugging Support**: Enhanced error analysis makes debugging easier
3. **Verification**: Better analysis capabilities help verify correct UCI behavior
4. **Learning**: Detailed output helps new users understand UCI structures

## Integration Notes

The enhancements should be backward compatible with the existing `analyze_packet` command, adding new features as optional enhancements rather than fundamental changes to the current behavior.

These enhancements will bring the `analyze_packet` command more in line with the sophisticated analysis capabilities found in the QM SDK reference implementations.