# Interesting UCI Implementation Files in Qorvo SDK

**Hardware-first focus:** This project centers on robust UCI control of real hardware; the simulator only supports protocol validation and debugging.

This document lists the most interesting and useful UCI-related files from the Qorvo SDK that could provide valuable insights for your UCI implementation.

## Core UCI Implementation Files

### 1. Main UCI Core Implementation
- **Location**: `/Samples/Cherry/uci/uci_core/src/uci.c`
- **Description**: The main UCI implementation containing core functionality
- **Interest Level**: HIGH
- **Why Useful**: Contains the core UCI protocol implementation, state machines, and message handling

### 2. UCI Internal Headers
- **Location**: `/Samples/Cherry/uci/uci_core/src/uci_internal.h`
- **Description**: Internal header file with implementation details
- **Interest Level**: HIGH
- **Why Useful**: Provides insight into internal structures and helper functions

### 3. UCI Message Handling
- **Location**: `/Samples/Cherry/uci/uci_core/src/uci_message.c`
- **Description**: Implementation of UCI message processing
- **Interest Level**: HIGH
- **Why Useful**: Shows how UCI messages are parsed, constructed, and handled

## UCI Header Files (API Definitions)

### 4. Main UCI API Header
- **Location**: `/Samples/Cherry/uci/uci_core/include/uci/uci.h`
- **Description**: Main UCI API definitions and public interface
- **Interest Level**: HIGH
- **Why Useful**: Defines the complete UCI API and data structures

### 5. UCI Message Header
- **Location**: `/Samples/Cherry/uci/uci_core/include/uci/uci_message.h`
- **Description**: UCI message structure definitions
- **Interest Level**: HIGH
- **Why Useful**: Detailed definitions of UCI message formats and structures

### 6. FiRa Specification Header
- **Location**: `/Samples/Cherry/uci/uci_core/include/uci/uci_spec_fira.h`
- **Description**: Complete FiRa consortium UCI specification implementation
- **Interest Level**: VERY HIGH
- **Why Useful**: Contains all FiRa-defined UCI commands, responses, and notifications with detailed documentation

### 7. Qorvo Extensions Header
- **Location**: `/Samples/Cherry/uci/uci_core/include/uci/uci_spec_qorvo.h`
- **Description**: Qorvo-specific UCI extensions and vendor commands
- **Interest Level**: MEDIUM
- **Why Useful**: Shows vendor-specific extensions that may be relevant for advanced features

### 8. MCPS Specification Header
- **Location**: `/Samples/Cherry/uci/uci_core/include/uci/uci_spec_mcps.h`
- **Description**: MAC Control Protocol Specification definitions
- **Interest Level**: MEDIUM
- **Why Useful**: Additional protocol specifications that complement UCI

## UCI Transport Layer

### 9. File Descriptor Transport Implementation
- **Location**: `/Samples/Cherry/uci/uci_transport/src/uci_transport_fd.c`
- **Description**: UCI transport implementation using file descriptors
- **Interest Level**: HIGH
- **Why Useful**: Shows how UCI messages are transported over different interfaces

### 10. UART Transport Implementation
- **Location**: `/Samples/Cherry/uci/uci_transport/src/uci_transport_uart.c`
- **Description**: UCI transport implementation for UART communication
- **Interest Level**: HIGH
- **Why Useful**: Reference for implementing UART-based UCI communication

### 11. HS-SPI Transport Implementation
- **Location**: `/Samples/Cherry/uci/uci_transport/src/uci_transport_hsspi.c`
- **Description**: UCI transport implementation for high-speed SPI
- **Interest Level**: MEDIUM
- **Why Useful**: Example of SPI-based transport implementation

### 12. Qorvo Utils Transport Implementation
- **Location**: `/Samples/Cherry/uci/uci_transport/src/uci_transport_qmutils.c`
- **Description**: UCI transport using Qorvo utility libraries
- **Interest Level**: MEDIUM
- **Why Useful**: Integration example with Qorvo-specific hardware abstraction

## Python UCI Implementation

### 13. Python UCI Core Module
- **Location**: `/Samples/Python/UWB-Qorvo-Tools/lib/uwb-uci/uci/core.py`
- **Description**: Python implementation of UCI core functionality
- **Interest Level**: HIGH
- **Why Useful**: Clean, readable implementation that's easier to understand than C code

### 14. Python FiRa Implementation
- **Location**: `/Samples/Python/UWB-Qorvo-Tools/lib/uwb-uci/uci/fira.py`
- **Description**: Python implementation of FiRa UCI specification
- **Interest Level**: VERY HIGH
- **Why Useful**: Complete, well-documented implementation of FiRa UCI commands with examples

### 15. Python Qorvo Extensions
- **Location**: `/Samples/Python/UWB-Qorvo-Tools/lib/uwb-uci/uci/qorvo.py`
- **Description**: Python implementation of Qorvo-specific UCI extensions
- **Interest Level**: HIGH
- **Why Useful**: Shows vendor-specific extensions in a readable format

### 16. Python Message Definitions
- **Location**: `/Samples/Python/UWB-Qorvo-Tools/lib/uwb-uci/uci/fira_msg.py`
- **Description**: Python definitions of UCI FiRa messages
- **Interest Level**: VERY HIGH
- **Why Useful**: Complete message definitions with clear documentation and examples

## Linux Kernel Implementation

### 17. UCI Character Device Driver
- **Location**: `/Platform/Linux/Drivers/linux-drivers/kernel/drivers/base/qm/qm3x_uci_dev_main.c`
- **Description**: Linux kernel character device driver for UCI communication
- **Interest Level**: HIGH
- **Why Useful**: Shows how UCI is implemented at the kernel level for direct hardware access

### 18. UCI IOCTL Definitions
- **Location**: `/Platform/Linux/Drivers/linux-drivers/kernel/drivers/base/qm/qm3x_uci_dev_ioctl.h`
- **Description**: IOCTL definitions for UCI character device
- **Interest Level**: HIGH
- **Why Useful**: Shows how applications communicate with UCI driver through IOCTLs

### 19. UCI Transport Implementation
- **Location**: `/Platform/Linux/Drivers/linux-drivers/kernel/drivers/base/qm/qm3x_transport.c`
- **Description**: UCI transport layer implementation in kernel
- **Interest Level**: HIGH
- **Why Useful**: Low-level transport implementation showing hardware communication

## UCI Utilities and Tools

### 20. UCI Bridge Application
- **Location**: `/Samples/Cherry/examples/uci_bridge/uci_bridge_app.c`
- **Description**: UCI bridge application that forwards UCI commands
- **Interest Level**: HIGH
- **Why Useful**: Example of how to create a UCI bridge/proxy application

### 21. UCI Test Framework
- **Location**: `/Samples/Cherry/qm-utils/qmutils/utest/test_uci.cc`
- **Description**: Unit tests for UCI functionality
- **Interest Level**: HIGH
- **Why Useful**: Shows how to properly test UCI implementations and expected behaviors

### 22. Python UCI Utilities
- **Location**: `/Samples/Python/UWB-Qorvo-Tools/scripts/utils/uqt_ls/uqt_ls.py`
- **Description**: Python utility for listing UCI devices
- **Interest Level**: MEDIUM
- **Why Useful**: Example of Python-based UCI device management tools

### 23. UCI Radar Demo Applications
- **Location**: `/Samples/Python/UWB-Qorvo-Tools/scripts/radar/radar_simple_demo/radar_simple_demo.py`
- **Description**: Simple radar demo using UCI protocol
- **Interest Level**: MEDIUM
- **Why Useful**: Complete application example showing UCI usage in real-world scenarios

### 24. UCI Configuration Utilities
- **Location**: `/Samples/Python/UWB-Qorvo-Tools/scripts/device/set_cal/set_cal.py`
- **Description**: Device calibration configuration utility
- **Interest Level**: MEDIUM
- **Why Useful**: Example of device configuration through UCI commands

## Key Areas to Focus On

### High-Priority Files for Understanding UCI Protocol:
1. `/Samples/Cherry/uci/uci_core/include/uci/uci_spec_fira.h` - Complete FiRa specification
2. `/Samples/Python/UWB-Qorvo-Tools/lib/uwb-uci/uci/fira_msg.py` - Well-documented message definitions
3. `/Samples/Python/UWB-Qorvo-Tools/lib/uwb-uci/uci/core.py` - Clean UCI core implementation
4. `/Samples/Cherry/uci/uci_core/src/uci.c` - Main UCI implementation

### Medium-Priority Files for Advanced Features:
1. `/Samples/Cherry/uci/uci_core/include/uci/uci_spec_qorvo.h` - Vendor extensions
2. `/Samples/Cherry/uci/uci_transport/src/uci_transport_*` - Various transport implementations
3. `/Platform/Linux/Drivers/linux-drivers/kernel/drivers/base/qm/*` - Kernel-level implementation

### Useful Utilities and Tools:
1. `/Samples/Cherry/examples/uci_bridge/uci_bridge_app.c` - UCI bridge implementation
2. `/Samples/Cherry/qm-utils/qmutils/utest/test_uci.cc` - UCI testing framework
3. `/Samples/Python/UWB-Qorvo-Tools/lib/uwb-uci/uci/fira.py` - Complete FiRa command implementation

These files provide comprehensive coverage of the UCI protocol implementation from high-level APIs to low-level hardware communication, offering valuable insights for enhancing your own UCI implementation.