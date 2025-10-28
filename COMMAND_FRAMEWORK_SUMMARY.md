# UCI Command Framework Design and Implementation Plan

## Current Architecture Overview

The UCI Interactive Shell currently has commands implemented in a distributed manner across multiple modules:
- `src/uci_cmd_core.c` - Core device commands
- `src/uci_cmd_session.c` - Session management commands
- `src/uci_cmd_hardware.c` - Hardware interface commands
- And others...

### Issues Identified:
1. **Inconsistent parameter validation** - Each command implements its own parsing
2. **Duplicated payload construction** - Similar patterns scattered across files
3. **Inconsistent error handling** - Different error reporting styles
4. **Manual command registration** - Commands manually added to the main command table

## New Framework Architecture

### Core Components

1. **Parameter Definition System**
   - Standardized parameter types (UINT8, UINT32, SESSION_ID, etc.)
   - Validation rules with min/max values
   - Required/optional flagging

2. **Unified Command Handler Interface**
   - Single function signature for all command handlers
   - Automatic parameter parsing based on command definition
   - Standardized error handling

3. **Command Definition Macros**
   - Macros to define commands with their parameters and validation rules
   - Automatic payload creation based on parameter types

### Framework Files Created

1. `include/uci_command_framework.h` - Framework definitions and interfaces
2. `src/uci_command_framework.c` - Implementation of the framework
3. `src/uci_command_definitions.c` - Command definitions using the new framework
4. `src/uci_cmd_core_new.c` - Example implementation using new framework
5. `include/uci_cmd_core_new.h` - Header for new core command implementations

### Framework Features

1. **Standardized Command Definition**: Commands defined with structured parameter definitions
2. **Automatic Parameter Validation**: Framework validates parameters based on type and constraints
3. **Unified Error Handling**: Consistent error reporting across all commands
4. **Automatic Payload Construction**: Framework builds UCI packet payloads automatically
5. **Flexible Parameter Types**: Support for various data types (uint8, uint32, hex strings, etc.)

### Implementation Status

#### ✅ Completed:
- Core framework architecture designed
- Parameter validation system implemented
- Unified command handler interface
- Example command implementations (core commands)
- Framework compilation with existing codebase

#### 🔄 In Progress:
- Integration with existing command system
- Testing of framework functionality

## Framework Usage Example

The new framework allows commands to be defined more consistently:

```c
// Parameter definitions
static const uci_param_def_t session_init_params[] = {
    {
        .name = "session_id",
        .type = PARAM_TYPE_UINT32,
        .flags = PARAM_FLAG_REQUIRED,
        .min_value = 0,
        .max_value = 0xFFFFFFFF,
        .description = "Unique identifier for the session"
    },
    {
        .name = "session_type",
        .type = PARAM_TYPE_SESSION_TYPE,
        .flags = PARAM_FLAG_REQUIRED,
        .description = "Type of session (fira_ranging, ranging_and_data, etc.)"
    }
};

// Command definition using the framework
static const uci_command_def_t session_init_cmd = {
    .name = "session_init",
    .aliases = { "session_new", NULL },
    .group = CLI_GROUP_SESSION,
    .flags = CLI_CMD_FLAG_NONE,
    .description = "Initialize a ranging session",
    .params = session_init_params,
    .param_count = 2,
    .handler = handle_session_init_command_new,
};
```

## Next Steps for Full Unification

1. **Gradual Migration**: Migrate commands one module at a time to use the new framework
2. **Bridge Implementation**: Create bridge functions to connect the new framework with the existing CLI dispatcher
3. **Testing**: Extensive testing of the new framework with all existing functionality
4. **Documentation**: Complete documentation for the new command architecture
5. **Refinement**: Optimize and refine the framework based on usage experience

## Benefits of the New Framework

1. **Consistency**: All commands follow the same parameter validation and handling patterns
2. **Maintainability**: Changes to command handling logic affect all commands uniformly
3. **Extensibility**: Adding new commands is simplified and consistent
4. **Reliability**: Built-in validation reduces common parameter parsing errors
5. **Code Reuse**: Eliminates duplicated validation and payload construction logic

## Integration Strategy

The framework is designed to work alongside the existing command system, allowing for gradual migration of individual command modules. This minimizes risk while providing a clear path toward full unification.

The new command framework has been successfully implemented and compiles with the existing codebase. The architecture is ready for gradual migration of existing command modules.