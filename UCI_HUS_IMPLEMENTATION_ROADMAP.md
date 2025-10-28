# UCI HUS Command Implementation Roadmap (Historical)

**Hardware-first focus:** This project centers on robust UCI control of real hardware; the simulator only supports protocol validation and debugging.

> **Status Update (October 2025):** The hybrid controller (`SESSION_SET_HYBRID_CONTROLLER_CONFIG`)
> and controlee (`SESSION_SET_HYBRID_CONTROLEE_CONFIG`) commands are now
> implemented end-to-end, including CLI entry points and enhanced response
> decoders. The material below is retained for reference should we revisit the
> original planning assumptions.

## Phase 1: Immediate Actions (Week 1)

### 1. Define Constants and Data Structures
- Add SESSION_SET_HUS_CONTROLLER_CONFIG (0x0C) and SESSION_SET_HUS_CONTROLEE_CONFIG (0x0D) constants
- Define controller_phase_list_t and controlee_phase_list_t structures
- Add HUS-specific error codes (0x26-0x29)

### 2. Implement Basic Command Handlers
- Add placeholder handlers for both HUS commands
- Set up proper command routing
- Implement basic response generation

### 3. Integrate with Existing Infrastructure
- Connect to session management system
- Hook into notification framework
- Ensure proper error handling

## Phase 2: Full Implementation (Week 2-3)

### 4. Complete HUS_CONTROLLER_CONFIG Handler
- Parse ControllerPhaseList arrays
- Validate session tokens and parameters
- Store configuration in session context
- Add comprehensive error handling
- Implement proper response generation

### 5. Complete HUS_CONTROLEE_CONFIG Handler
- Parse ControleePhaseList arrays
- Validate session tokens
- Store configuration in session context
- Add comprehensive error handling
- Implement proper response generation

### 6. Add Session Context Extensions
- Extend uci_session structure to store HUS configuration
- Add accessor functions for HUS data
- Implement proper initialization and cleanup

## Phase 3: Testing and Validation (Week 4)

### 7. Create Unit Tests
- Test successful configuration scenarios
- Test error handling cases
- Test boundary conditions
- Validate against official test vectors

### 8. Integration Testing
- Test with existing session management
- Test with notification system
- Test memory management
- Performance benchmarking

### 9. Documentation and Examples
- Update API documentation
- Create usage examples
- Add configuration guides

## Implementation Checklist

### Constants to Add:
- [x] SESSION_SET_HUS_CONTROLLER_CONFIG = 0x0C
- [x] SESSION_SET_HUS_CONTROLEE_CONFIG = 0x0D
- [ ] ERROR_HUS_NOT_ENOUGH_SLOTS = 0x26
- [ ] ERROR_HUS_CFP_PHASE_TOO_SHORT = 0x27
- [ ] ERROR_HUS_CAP_PHASE_TOO_SHORT = 0x28
- [ ] ERROR_HUS_OTHERS = 0x29

### Data Structures to Define:
- [ ] controller_phase_list_t
- [ ] controlee_phase_list_t

### Functions to Implement:
- [x] handle_session_set_hus_controller_config()
- [x] handle_session_set_hus_controlee_config()
- [ ] parse_controller_phase_list()
- [ ] parse_controlee_phase_list()
- [ ] validate_hus_configuration()

### Tests to Create:
- [ ] Basic HUS command handling
- [ ] Error condition testing
- [ ] Boundary condition testing
- [ ] Memory leak testing
- [ ] Integration with existing sessions

## File Locations for Changes

### src/uci.c (Main Implementation)
1. Add constants near line 180 (after other #define statements)
2. Add command handlers between SESSION_UPDATE_ACTIVE_ROUNDS_DT_TAG and SESSION_DATA_TRANSFER_PHASE_CONFIG
3. Add helper functions for parsing and validation

### include/uci.h (Header Updates)
1. Add structure definitions
2. Add function prototypes
3. Add any necessary macros

### tests/test_uci_functions.c (Test Cases)
1. Add new test cases for HUS commands
2. Add test vectors from official specification
3. Add error condition tests

## Risk Mitigation Strategy

### Start Small Approach:
1. Begin with minimal viable implementation
2. Add features incrementally
3. Test at each step
4. Validate compatibility continuously

### Backward Compatibility:
1. Ensure no existing functionality is broken
2. Maintain existing API contracts
3. Follow existing code patterns
4. Preserve existing test suite behavior

### Quality Assurance:
1. Write comprehensive unit tests first
2. Validate against official specification
3. Perform stress testing
4. Monitor memory usage and leaks

## Success Criteria

### Technical Requirements:
- ✅ Both HUS commands implemented fully
- ✅ All official specification requirements met
- ✅ No regressions in existing functionality
- ✅ 100% unit test coverage for new features

### Performance Requirements:
- ✅ Response times comparable to existing commands
- ✅ Memory usage within acceptable limits
- ✅ No resource leaks

### Quality Requirements:
- ✅ Code follows existing patterns and conventions
- ✅ Comprehensive error handling
- ✅ Clear documentation
- ✅ Proper logging and debugging support

## Next Steps

1. **Today**: Create branch for HUS implementation
2. **Tomorrow**: Begin Phase 1 implementation
3. **This Week**: Complete basic command handlers
4. **Next Week**: Implement full functionality
5. **Following Week**: Testing and validation

By following this roadmap, we can systematically implement the missing HUS commands while maintaining the quality and stability of our existing UCI implementation.
