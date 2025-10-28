# UCI Enhancement Plan: Implement Missing HUS Commands (Historical)

**Hardware-first focus:** This project centers on robust UCI control of real hardware; the simulator only supports protocol validation and debugging.

> **Status Update:** The hybrid controller and controlee commands described in
> this plan are now implemented in the interactive shell. The steps below are
> preserved as background material and can be repurposed for future refinements
> such as richer validation or extended error code support.

## 1. Problem Statement

Our current UCI implementation is missing two critical Session Config commands:
1. **SESSION_SET_HUS_CONTROLLER_CONFIG** (Opcode 0x0C) - Sets Hybrid UWB System controller configuration
2. **SESSION_SET_HUS_CONTROLEE_CONFIG** (Opcode 0x0D) - Sets Hybrid UWB System controlee configuration

These commands are part of the official Qorvo UWB SDK specification and are essential for:
- Full Android UWB stack compatibility
- Hybrid positioning system support (combining UWB with other positioning technologies)
- Industry-standard UCI protocol compliance

## 2. Official Specification Details

### SESSION_SET_HUS_CONTROLLER_CONFIG (Opcode 0x0C)
```
packet SessionSetHybridControllerConfigCmd : SessionConfigCommand (opcode = 0x0C) {
    session_token: 32,
    number_of_phases: 8,
    phase_list: ControllerPhaseList[],
}

struct ControllerPhaseList {
    session_token: 32,
    start_slot_index: 16,
    end_slot_index: 16,
    control: 8,
    mac_address: 8[],
}

Response:
packet SessionSetHybridControllerConfigRsp : SessionConfigResponse (opcode = 0x0C) {
    status: StatusCode,
}
```

### SESSION_SET_HUS_CONTROLEE_CONFIG (Opcode 0x0D)
```
packet SessionSetHybridControleeConfigCmd : SessionConfigCommand (opcode = 0x0D) {
    session_token: 32,
    _count_(controlee_phase_list): 8,
    controlee_phase_list: ControleePhaseList[],
}

struct ControleePhaseList {
    session_token: 32,
}

Response:
packet SessionSetHybridControleeConfigRsp : SessionConfigResponse (opcode = 0x0D) {
    status: StatusCode,
}
```

## 3. Implementation Plan

### Phase 1: Define Constants and Structures
1. Add missing opcode constants to the source files:
   ```c
   #define SESSION_SET_HUS_CONTROLLER_CONFIG 0x0C
   #define SESSION_SET_HUS_CONTROLEE_CONFIG 0x0D
   ```

2. Define supporting data structures:
   ```c
   typedef struct {
       unsigned int session_token;
       unsigned short start_slot_index;
       unsigned short end_slot_index;
       unsigned char control;
       unsigned char mac_address_len;
       unsigned char* mac_address;  // Variable length array
   } controller_phase_list_t;

   typedef struct {
       unsigned int session_token;
   } controlee_phase_list_t;
   ```

### Phase 2: Implement Command Handlers
1. Add SESSION_SET_HUS_CONTROLLER_CONFIG handler:
   - Parse incoming ControllerPhaseList array
   - Validate session tokens and parameters
   - Store configuration in session context
   - Generate appropriate response

2. Add SESSION_SET_HUS_CONTROLEE_CONFIG handler:
   - Parse incoming ControleePhaseList array
   - Validate session tokens
   - Store configuration in session context
   - Generate appropriate response

### Phase 3: Add Error Handling
1. Implement proper error responses for:
   - Invalid session tokens
   - Malformed payloads
   - Buffer overflows
   - Unsupported configurations

2. Add appropriate error notifications:
   ```c
   // HUS-specific error codes from specification
   #define ERROR_HUS_NOT_ENOUGH_SLOTS 0x26
   #define ERROR_HUS_CFP_PHASE_TOO_SHORT 0x27
   #define ERROR_HUS_CAP_PHASE_TOO_SHORT 0x28
   #define ERROR_HUS_OTHERS 0x29
   ```

### Phase 4: Testing and Validation
1. Create unit tests for both commands:
   - Successful configuration scenarios
   - Error handling cases
   - Boundary condition testing

2. Validate against official test vectors:
   ```
   // SESSION_SET_HUS_CONTROLLER_CONFIG test vector
   "\x21\x0C\x00\x23\x03\x00\x00\x01\x00\x02\x00\x00\x00\x00\x00\x00\x00\x00\x01\x00\x00\x05\x01\x00\x19\x00\x00\x30\x00\x02\x00\x00\x03\x1A\x00\x32\x00\x00\x30\x00"

   // SESSION_SET_HUS_CONTROLEE_CONFIG test vector  
   "\x21\x0D\x00\x0F\x03\x00\x00\x01\x02\x01\x00\x00\x05\x02\x02\x00\x00\x03\x02"
   ```

## 4. Benefits of Implementation

### Technical Benefits
- **Full UCI Protocol Compliance**: Complete implementation of all standard UCI commands
- **Android Compatibility**: Support for Android UWB hybrid positioning features
- **Industry Standard Alignment**: Compliance with Qorvo UWB SDK specifications
- **Robust Error Handling**: Proper validation and error reporting for HUS configurations

### Functional Benefits
- **Hybrid Positioning Support**: Enable combination of UWB with other positioning technologies
- **Enhanced Use Cases**: Support for complex multi-session coordination scenarios
- **Better Device Management**: Improved session configuration capabilities

### Development Benefits
- **Future-Proofing**: Ready for advanced UWB use cases
- **Code Completeness**: Eliminate gaps in UCI command set
- **Documentation Clarity**: Clear implementation of HUS features

## 5. Implementation Timeline

### Week 1: Foundation
- Add constants and data structures
- Implement basic command handlers
- Add initial error handling

### Week 2: Robustness
- Complete error handling implementation
- Add comprehensive validation
- Implement proper session integration

### Week 3: Testing
- Create unit tests
- Validate against official test vectors
- Performance and stress testing

### Week 4: Documentation and Review
- Update documentation
- Code review and optimization
- Integration testing

## 6. Risk Mitigation

### Compatibility Risks
- Maintain backward compatibility with existing code
- Ensure no regression in existing functionality
- Follow existing code patterns and conventions

### Complexity Risks
- Start with minimal viable implementation
- Incrementally add features
- Thorough testing at each stage

### Integration Risks
- Test with existing session management infrastructure
- Validate proper interaction with notification system
- Ensure proper memory management

## 7. Success Criteria

### Technical Criteria
- ✅ Both HUS commands implemented with full specification compliance
- ✅ All error conditions properly handled
- ✅ Memory management optimized
- ✅ Code integrated seamlessly with existing session management

### Testing Criteria
- ✅ 100% unit test coverage for new functionality
- ✅ All official test vectors pass
- ✅ No regression in existing test suite
- ✅ Performance benchmarks met

### Quality Criteria
- ✅ Code follows existing patterns and conventions
- ✅ Comprehensive documentation
- ✅ Clear error messages and logging
- ✅ Industry-standard implementation quality

## 8. Next Steps

1. **Immediate Action**: Begin Phase 1 implementation - define constants and structures
2. **Medium Term**: Proceed with command handler implementation
3. **Long Term**: Complete testing and validation, prepare for production deployment
