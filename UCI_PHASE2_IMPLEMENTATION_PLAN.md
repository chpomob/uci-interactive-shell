# UCI Implementation Enhancement - Phase 2 Implementation Plan

## Overview

Based on our Phase 1 success, we now have 8 remaining "No specific decoder" messages that need to be addressed in Phase 2. These represent critical missing functionality for Android UWB compatibility and device testing capabilities.

## Remaining Missing Decoders (8 Messages)

### High Priority (Critical for Android UWB Compatibility)
1. **ANDROID_GET_POWER_STATS** (GID=12, Opcode 0x00) - Essential for Android power management
2. **ANDROID_SET_COUNTRY_CODE** (GID=12, Opcode 0x01) - Critical for regulatory compliance
3. **RF_TEST_STOP_SESSION** (GID=13, Opcode 0x07) - Important for device testing

### Medium Priority (Useful for Enhanced Functionality)
4. **RF_TEST_CONFIG_SET** (GID=13, Opcode 0x00) - Important for device configuration
5. **RF_TEST_PER_RX** (GID=13, Opcode 0x02) - Important for device testing
6. **ANDROID_RADAR_SET_APP_CONFIG** (GID=12, Opcode 0x11) - Useful for radar functionality
7. **ANDROID_RADAR_GET_APP_CONFIG** (GID=12, Opcode 0x12) - Useful for radar functionality

### Low Priority (Nice to Have)
8. **ANDROID_FIRA_RANGE_DIAGNOSTICS_NTF** (GID=12, Opcode 0x02) - Useful for diagnostics

## Detailed Implementation Specifications

### 1. ANDROID_GET_POWER_STATS (GID=12, Opcode 0x00)
**Purpose**: Retrieve power consumption statistics for the UWB device
**Specification**:
```
packet AndroidGetPowerStatsCmd : VendorAndroidCommand (opcode = 0x0) {
    // No additional fields
}

packet AndroidGetPowerStatsRsp : VendorAndroidResponse (opcode = 0x0) {
    status: StatusCode,
    // Additional power stats fields would be here
}
```

**Priority**: HIGH - Essential for Android power management
**Estimated Effort**: 2-3 days

### 2. ANDROID_SET_COUNTRY_CODE (GID=12, Opcode 0x01)
**Purpose**: Set the regulatory region/country code for compliance
**Specification**:
```
packet AndroidSetCountryCodeCmd : VendorAndroidCommand (opcode = 0x1) {
    country_code: 8[2], // 2-character ISO country code
}

packet AndroidSetCountryCodeRsp : VendorAndroidResponse (opcode = 0x1) {
    status: StatusCode,
}
```

**Priority**: HIGH - Critical for regulatory compliance
**Estimated Effort**: 2-3 days

### 3. RF_TEST_STOP_SESSION (GID=13, Opcode 0x07)
**Purpose**: Stop an ongoing RF test session
**Specification**:
```
packet StopRfTestCmd : TestCommand (opcode = 0x07) { // RF_TEST_STOP_SESSION
    // No additional fields
}

packet StopRfTestRsp : TestResponse (opcode = 0x07) {  // RF_TEST_STOP_SESSION
    status: StatusCode,
}
```

**Priority**: HIGH - Important for device testing
**Estimated Effort**: 1-2 days

### 4. RF_TEST_CONFIG_SET (GID=13, Opcode 0x00)
**Purpose**: Configure RF test parameters
**Specification**:
```
packet SessionSetRfTestConfigCmd : TestCommand (opcode = 0x00) {  // RF_TEST_CONFIG_SET
    session_token: 32, // Session ID or Session Handle (based on UWBS version)
    _count_(tlvs): 8,
    tlvs: RfTestConfigTlv[],
}

packet SessionSetRfTestConfigRsp : TestResponse (opcode = 0x00) { // RF_TEST_CONFIG_SET
    status: StatusCode,
    _count_(cfg_status): 8,
    cfg_status: RfTestConfigStatus[],
}
```

**Priority**: MEDIUM - Important for device configuration
**Estimated Effort**: 3-4 days

### 5. RF_TEST_PER_RX (GID=13, Opcode 0x02)
**Purpose**: Configure PER (Packet Error Rate) receiver test
**Specification**:
```
packet TestPerRxCmd : TestCommand (opcode = 0x03) { // RF_TEST_PER_RX
    psdu_data : 8[],
}

packet TestPerRxRsp : TestResponse (opcode = 0x03) { // RF_TEST_PER_RX
    status: StatusCode,
}

packet TestPerRxNtf : TestNotification (opcode = 0x03) { // RF_TEST_PER_RX
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

**Priority**: MEDIUM - Important for device testing
**Estimated Effort**: 4-5 days

### 6. ANDROID_RADAR_SET_APP_CONFIG (GID=12, Opcode 0x11)
**Purpose**: Set radar application configuration parameters
**Specification**:
```
packet AndroidRadarSetAppConfigCmd : VendorAndroidCommand (opcode = 0x11) {
    session_token: 32,
    // Configuration parameters
}

packet AndroidRadarSetAppConfigRsp : VendorAndroidResponse (opcode = 0x11) {
    status: StatusCode,
}
```

**Priority**: MEDIUM - Useful for radar functionality
**Estimated Effort**: 2-3 days

### 7. ANDROID_RADAR_GET_APP_CONFIG (GID=12, Opcode 0x12)
**Purpose**: Get radar application configuration parameters
**Specification**:
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

**Priority**: MEDIUM - Useful for radar functionality
**Estimated Effort**: 2-3 days

### 8. ANDROID_FIRA_RANGE_DIAGNOSTICS_NTF (GID=12, Opcode 0x02)
**Purpose**: Receive FiRa range diagnostics notifications
**Specification**:
```
packet AndroidRangeDiagnosticsNtf : AndroidNotification (opcode = 0x2) { //FIRA_RANGE_DIAGNOSTICS
    session_token: 32, // Session ID or Session Handle (based on UWBS version)
    sequence_number: 32,
    _count_(frame_reports): 8,
    frame_reports: FrameReport[],
}
```

**Priority**: LOW - Useful for diagnostics
**Estimated Effort**: 2-3 days

## Phase 2 Implementation Timeline

### Week 1-2: High Priority Implementation
- Implement ANDROID_GET_POWER_STATS decoder
- Implement ANDROID_SET_COUNTRY_CODE decoder
- Implement RF_TEST_STOP_SESSION decoder
- Add comprehensive unit tests for all three
- Validate against official specification

### Week 3-4: Medium Priority Implementation
- Implement RF_TEST_CONFIG_SET decoder
- Implement RF_TEST_PER_RX decoder
- Add comprehensive unit tests
- Validate against official specification

### Week 5-6: Remaining Implementation
- Implement ANDROID_RADAR_SET_APP_CONFIG decoder
- Implement ANDROID_RADAR_GET_APP_CONFIG decoder
- Implement ANDROID_FIRA_RANGE_DIAGNOSTICS_NTF decoder
- Add comprehensive unit tests
- Validate against official specification

## Risk Assessment

### Technical Risks
1. **Complex Data Structures**: Some commands require complex TLV parsing
2. **Memory Management**: Variable-length arrays require careful memory handling
3. **Error Handling**: Comprehensive error scenarios need thorough testing

### Mitigation Strategies
1. Start with simplified implementations and gradually add complexity
2. Implement comprehensive unit tests before integration
3. Add thorough validation and bounds checking
4. Follow incremental development approach with frequent testing

## Success Metrics

### Technical Requirements
- ✅ All 8 missing decoders implemented with full specification compliance
- ✅ Zero regressions in existing functionality
- ✅ 100% unit test coverage for new features
- ✅ All official test vectors pass

### Performance Requirements
- ✅ Response times comparable to existing decoders
- ✅ Memory usage within acceptable limits
- ✅ No resource leaks

### Quality Requirements
- ✅ Code follows existing patterns and conventions
- ✅ Comprehensive error handling
- ✅ Clear documentation
- ✅ Proper logging and debugging support

## Next Steps

1. **Immediate**: Begin Phase 2 implementation with high-priority decoders
2. **Short-term**: Complete high-priority implementations (Week 1-2)
3. **Medium-term**: Implement medium-priority decoders (Week 3-4)
4. **Long-term**: Complete all remaining decoders and validation (Week 5-6)