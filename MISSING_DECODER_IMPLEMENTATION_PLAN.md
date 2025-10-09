# Missing UCI Packet Decoder Implementation Plan

## Overview

Based on our analysis, we've successfully implemented the missing SESSION_CONFIG_RESPONSE decoders for opcodes 0x0B, 0x0C, and 0x0D. We still have 8 remaining "No specific decoder" messages that need to be addressed:

1. **VENDOR_ANDROID Commands** (4 messages)
2. **TEST Commands** (3 messages)  
3. **VENDOR_ANDROID_NOTIFICATION** (1 message)

## Remaining Missing Decoders

### 1. VENDOR_ANDROID Commands (GID=12)
- Opcode 0x00: ANDROID_GET_POWER_STATS
- Opcode 0x01: ANDROID_SET_COUNTRY_CODE
- Opcode 0x11: ANDROID_RADAR_SET_APP_CONFIG
- Opcode 0x12: ANDROID_RADAR_GET_APP_CONFIG

### 2. TEST Commands (GID=13)
- Opcode 0x00: RF_TEST_CONFIG_SET
- Opcode 0x02: RF_TEST_PER_RX
- Opcode 0x07: RF_TEST_STOP_SESSION

### 3. VENDOR_ANDROID_NOTIFICATION (GID=12)
- Opcode 0x02: ANDROID_FIRA_RANGE_DIAGNOSTICS_NTF

## Implementation Plan

### Phase 1: VENDOR_ANDROID Command Decoders

#### 1.1 ANDROID_GET_POWER_STATS (Opcode 0x00)
**Packet Structure:**
```
packet AndroidGetPowerStatsCmd : VendorAndroidCommand (opcode = 0x0) {
    // No additional fields
}

packet AndroidGetPowerStatsRsp : VendorAndroidResponse (opcode = 0x0) {
    status: StatusCode,
    // Additional power stats fields would be here
}
```

#### 1.2 ANDROID_SET_COUNTRY_CODE (Opcode 0x01)
**Packet Structure:**
```
packet AndroidSetCountryCodeCmd : VendorAndroidCommand (opcode = 0x1) {
    country_code: 8[2], // 2-character ISO country code
}

packet AndroidSetCountryCodeRsp : VendorAndroidResponse (opcode = 0x1) {
    status: StatusCode,
}
```

#### 1.3 ANDROID_RADAR_SET_APP_CONFIG (Opcode 0x11)
**Packet Structure:**
```
packet AndroidRadarSetAppConfigCmd : VendorAndroidCommand (opcode = 0x11) {
    session_token: 32,
    // Configuration parameters
}

packet AndroidRadarSetAppConfigRsp : VendorAndroidResponse (opcode = 0x11) {
    status: StatusCode,
}
```

#### 1.4 ANDROID_RADAR_GET_APP_CONFIG (Opcode 0x12)
**Packet Structure:**
```
packet AndroidRadarGetAppConfigCmd : VendorAndroidCommand (opcode = 0x12) {
    session_token: 32,
    // Request parameters
}

packet AndroidRadarGetAppConfigRsp : VendorAndroidResponse (opcode = 0x12) {
    status: StatusCode,
    // Configuration response data
}
```

### Phase 2: TEST Command Decoders

#### 2.1 RF_TEST_CONFIG_SET (Opcode 0x00)
**Packet Structure:**
```
packet SessionSetRfTestConfigCmd : TestCommand (opcode = 0x00) {
    session_token: 32,
    _count_(tlvs): 8,
    tlvs: RfTestConfigTlv[],
}

packet SessionSetRfTestConfigRsp : TestResponse (opcode = 0x00) {
    status: StatusCode,
    _count_(cfg_status): 8,
    cfg_status: RfTestConfigStatus[],
}
```

#### 2.2 RF_TEST_PER_RX (Opcode 0x02)
**Packet Structure:**
```
packet TestPerRxCmd : TestCommand (opcode = 0x02) {
    psdu_data: 8[],
}

packet TestPerRxRsp : TestResponse (opcode = 0x02) {
    status: StatusCode,
}

packet TestPerRxNtf : TestNotification (opcode = 0x02) {
    status: StatusCode,
    attempts: 32,
    acq_detect: 32,
    acq_reject: 32,
    rx_fail: 32,
    sync_cir_ready: 32,
    sfd_fail: 32,
    sfd_found: 32,
    phr_dec_error: 32,
    phr_bit_error: 32,
    // Additional fields...
}
```

#### 2.3 RF_TEST_STOP_SESSION (Opcode 0x07)
**Packet Structure:**
```
packet StopRfTestCmd : TestCommand (opcode = 0x07) {
    // No additional fields
}

packet StopRfTestRsp : TestResponse (opcode = 0x07) {
    status: StatusCode,
}
```

### Phase 3: VENDOR_ANDROID_NOTIFICATION Decoder

#### 3.1 ANDROID_FIRA_RANGE_DIAGNOSTICS_NTF (Opcode 0x02)
**Packet Structure:**
```
packet AndroidRangeDiagnosticsNtf : AndroidNotification (opcode = 0x2) {
    session_token: 32,
    sequence_number: 32,
    _count_(frame_reports): 8,
    frame_reports: FrameReport[],
}
```

## Implementation Priority

### High Priority (Next Implementation):
1. **ANDROID_GET_POWER_STATS** (Opcode 0x00) - Essential for Android power management
2. **ANDROID_SET_COUNTRY_CODE** (Opcode 0x01) - Critical for regulatory compliance
3. **RF_TEST_STOP_SESSION** (Opcode 0x07) - Important for device testing

### Medium Priority:
4. **RF_TEST_CONFIG_SET** (Opcode 0x00) - Important for device configuration
5. **RF_TEST_PER_RX** (Opcode 0x02) - Important for device testing
6. **ANDROID_RADAR_SET_APP_CONFIG** (Opcode 0x11) - Useful for radar functionality
7. **ANDROID_RADAR_GET_APP_CONFIG** (Opcode 0x12) - Useful for radar functionality

### Low Priority:
8. **ANDROID_FIRA_RANGE_DIAGNOSTICS_NTF** (Opcode 0x02) - Useful for diagnostics but not critical

## Implementation Approach

### 1. Add Function Declarations
Add decoder function declarations to `include/uci_ui_packet_decoder.h`:
```c
// VENDOR_ANDROID decoders
void ui_decode_android_get_power_stats_cmd(unsigned char* payload, int payload_len);
void ui_decode_android_get_power_stats_rsp(unsigned char* payload, int payload_len);
void ui_decode_android_set_country_code_cmd(unsigned char* payload, int payload_len);
void ui_decode_android_set_country_code_rsp(unsigned char* payload, int payload_len);
void ui_decode_android_radar_set_app_config_cmd(unsigned char* payload, int payload_len);
void ui_decode_android_radar_set_app_config_rsp(unsigned char* payload, int payload_len);
void ui_decode_android_radar_get_app_config_cmd(unsigned char* payload, int payload_len);
void ui_decode_android_radar_get_app_config_rsp(unsigned char* payload, int payload_len);

// TEST decoders
void ui_decode_rf_test_config_set_cmd(unsigned char* payload, int payload_len);
void ui_decode_rf_test_config_set_rsp(unsigned char* payload, int payload_len);
void ui_decode_rf_test_per_rx_cmd(unsigned char* payload, int payload_len);
void ui_decode_rf_test_per_rx_rsp(unsigned char* payload, int payload_len);
void ui_decode_rf_test_stop_session_cmd(unsigned char* payload, int payload_len);
void ui_decode_rf_test_stop_session_rsp(unsigned char* payload, int payload_len);

// VENDOR_ANDROID_NOTIFICATION decoders
void ui_decode_android_range_diagnostics_ntf(unsigned char* payload, int payload_len);
```

### 2. Add Function Implementations
Add implementations to `src/uci_ui_packet_decoder.c` following the existing pattern.

### 3. Add Switch Cases
Add appropriate switch cases to `src/uci_packet_analyzer.c` in the respective sections:
- `mt == COMMAND && gid == VENDOR_ANDROID`
- `mt == RESPONSE && gid == VENDOR_ANDROID`
- `mt == NOTIFICATION && gid == VENDOR_ANDROID`
- `mt == COMMAND && gid == TEST`
- `mt == RESPONSE && gid == TEST`
- `mt == NOTIFICATION && gid == TEST`

## Expected Outcomes

After implementing these missing decoders:

1. **Eliminate all "No specific decoder" messages** from test output
2. **Achieve 100% UCI packet decoder coverage** for all implemented commands
3. **Improve Android UWB compatibility** with complete vendor command support
4. **Enhance device testing capabilities** with complete TEST command support
5. **Provide better debugging and diagnostics** with comprehensive packet analysis
6. **Maintain full backward compatibility** with existing functionality

## Test Validation

Each new decoder implementation should be validated with:
1. **Positive test cases** with valid packet payloads
2. **Negative test cases** with invalid/malformed payloads  
3. **Boundary condition tests** with edge case values
4. **Integration testing** with the existing test suite

The implementation should result in elimination of all 8 remaining "No specific decoder" messages from our test output.