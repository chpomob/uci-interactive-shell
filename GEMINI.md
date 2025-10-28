# Gemini Project Analysis: UCI Interactive Shell

**Hardware-first focus:** This project centers on robust UCI control of real hardware; the simulator only supports protocol validation and debugging.

## Project Overview

This project is an interactive command-line shell for communicating with Ultra-Wideband (UWB) devices using the UCI (Ultra-wideband Communication Interface) protocol. It provides a unified interface for both simulation and hardware modes, allowing developers to test and interact with UWB devices.

> **Preservation Reminder:** Keep `uci_analysis/qm35-sdk/` intact; the snapshot fuels protocol analysis workflows.

The project is written in C and uses the `readline` library for the interactive shell. It is built with a `Makefile` and includes a suite of unit tests.

**Key Features:**

*   **Interactive Shell:** Provides a command-line interface with tab completion and history.
*   **UCI Protocol Implementation:** Implements the UCI protocol for communicating with UWB devices with full Android UWB specification compliance.
*   **Simulation and Hardware Modes:** Supports both a simulation mode for testing without hardware and a hardware mode for communicating with real UWB devices through a character device (e.g., `/dev/ttyUSB0`).
*   **Protocol Compliance:** Fully aligned with Android UWB specification with proper header format [GID|PBF|MT][Opcode|R][Reserved][Length].
*   **Enhanced Response Handling:** Complete parsing of all UCI responses with proper status code interpretation and payload parsing.
*   **Comprehensive TLV Support:** Full Type-Length-Value configuration management for SET_CONFIG/GET_CONFIG operations with multi-TLV support.
*   **Session Management:** Complete session lifecycle with persistent state tracking and configuration storage.
*   **Notification Handling:** Human-readable display of all UCI notifications including device status, session status, and data credit notifications.
*   **Ranging Support:** Full UWB ranging notification parsing with distance/angle measurements (TWO_WAY and OWR_AOA), quality metrics, and MAC address support.
*   **Hardware Communication:** Real UWB device support via character device files with proper packet framing and error recovery.
*   **Packet Analysis:** Includes a command to analyze raw UCI packets.
*   **Unit Tests:** Includes a suite of unit tests for testing various components of the application.
*   **Code Coverage:** Can generate code coverage reports using `gcov`.

## Building and Running

### Building the Project

To build the project, run the following command:

```bash
make
```

This will compile the source code and create the `uci-shell` executable. It will also build and run all unit tests.

### Running the Interactive Shell

To run the interactive shell, execute the following command:

```bash
./uci-shell
```

### Running the Tests

To run the tests, you can use the following commands:

*   `make test`: Builds and runs a test for the character device.
*   `make unit-test`: Builds and runs the unit tests for the UCI functions.
*   `make config-test`: Builds and runs the unit tests for the configuration manager.
*   Hardware interface test target has been removed; use simulation suites instead.
*   `make session-manager-test`: Builds and runs the unit tests for the session manager.
*   `make all`: Builds the main application and runs all the unit tests.

### Generating Code Coverage

To generate code coverage reports, run the following command:

```bash
make coverage
```

The coverage reports will be generated in the `coverage` directory.

## Development Conventions

*   **Coding Style:** The code follows a consistent C coding style with proper error handling.
*   **Headers:** Header files are located in the `include` directory with clear function declarations.
*   **Source Code:** Source code is located in the `src` directory with modular organization by functionality.
*   **Tests:** Tests are located in the `tests` directory with dedicated test suites for each module.
*   **Makefile:** The project uses a `Makefile` for building and testing with multiple targets.
*   **Documentation:** The project is well-documented with a `README.md` file and other markdown files.

## Key Files

*   `src/main.c`: The main entry point of the interactive shell. It contains the main loop for reading and processing user commands.
*   `src/uci.c`: Contains the core implementation of the UCI protocol, including command and response handling.
*   `src/uci_cmd_core.c`: Core UCI command implementations (device_info, device_reset, caps_info, etc.)
*   `src/uci_cmd_session.c`: Session management command implementations (session_init, session_start, etc.)
*   `src/uci_cmd_session_config.c`: Application configuration command implementations (set_app_config, get_app_config, etc.)
*   `src/uci_cmd_hardware.c`: Hardware communication command implementations (mode_hw, hw_send, etc.)
*   `src/uci_cmd_simulation.c`: Simulation command implementations (simulate_ranging, demo_session_flow, etc.)
*   `src/uci_config_manager.c`: Configuration management with TLV support for device and session configuration
*   `src/uci_hw_interface.c`: Hardware communication interface for sending and receiving UCI packets to/from UWB devices
*   `src/uci_hw_chardev.c`: Low-level character device communication implementation
*   `src/uci_packet_utils.c`: UCI packet construction and parsing utilities
*   `src/uci_packet_analyzer.c`: Raw UCI packet analysis functionality
*   `src/uci_response_core.c`: Core response parsing implementations for all UCI responses
*   `include/uci.h`: Defines the core data structures and function prototypes for the UCI protocol implementation
*   `include/uci_functions.h`: Main function declarations for the UCI implementation
*   `include/uci_pdl.h`: UCI protocol definitions and constants based on Android PDL specifications
*   `include/uci_config_manager.h`: Header file for configuration manager with TLV support
*   `include/uci_hw_interface.h`: Header file for the hardware communication interface
*   `include/uci_hw_chardev.h`: Header file for character device communication
*   `Makefile`: The build script for the project with targets for building, testing, and coverage generation
*   `README.md`: The main documentation file for the project with comprehensive usage examples

## Usage

The interactive shell can be used in two primary modes: simulation mode and hardware mode.

### Simulation Mode

In simulation mode, the shell simulates the behavior of a UWB device and can be used for testing and development purposes without requiring real hardware.

To start the shell in simulation mode, simply run:

```bash
./uci-shell
```

Example commands in simulation mode:
*   `get_device_info` - Get device information
*   `session_init 1 fira_ranging` - Initialize a ranging session
*   `session_start 1` - Start the ranging session
*   `simulate_ranging` - Simulate ranging measurements
*   `session_stop 1` - Stop the ranging session
*   `session_deinit 1` - Deinitialize the session

### Hardware Mode

In hardware mode, the shell communicates with a real UWB device through a character device (e.g., `/dev/ttyUSB0`).

To start the shell in hardware mode:

```bash
./uci-shell
> mode_hw /dev/ttyUSB0
```

Once connected to hardware, continue using the standard commands:
*   `get_device_info` - Query the connected device
*   `session_init 1 fira_ranging` - Initialize a ranging session on hardware
*   `session_start 1` - Start the ranging session on hardware
*   `hw_send 01 00 00 02` - Manually transmit a raw UCI command when needed

## Advanced Features

### Ranging Notifications
The system supports comprehensive ranging notification parsing including:
*   TWO_WAY Ranging: Single and multi-target distance measurements with human-readable values
*   OWR_AOA Ranging: One-way ranging with angle-of-arrival measurements
*   Distance/Angle Reporting: Human-readable distance (cm) and angle (degrees) values
*   Quality Metrics: NLOS indicators, RSSI, FoM (Figure of Merit) values
*   MAC Address Support: Both SHORT_ADDRESS and EXTENDED_ADDRESS formats

### Configuration Management
The system provides complete configuration management with:
*   Multi-TLV support for setting multiple parameters in single commands
*   Device and session level configurations
*   Persistent configuration storage
*   Support for all standard configuration parameters

### Error Handling
*   Comprehensive error status interpretation
*   Graceful failure handling
*   Proper resource cleanup on errors
*   Detailed debugging output for hardware communication

## Project Architecture

The project follows a modular architecture with clean separation of concerns:

*   **Core Protocol Layer**: Handles basic UCI packet construction/parsing
*   **Command Processing Layer**: Manages different command groups (Core, Session, Hardware, etc.)
*   **Configuration Manager**: Handles TLV-based configuration management
*   **Session Manager**: Tracks session states and configurations
*   **Hardware Interface Layer**: Provides abstraction between simulation and hardware modes
*   **UI Layer**: Interactive command-line interface with readline support

## Testing and Quality

The project includes comprehensive testing capabilities:
*   Unit tests for individual components
*   Integration tests for complete workflows
*   Hardware simulation framework for testing without real devices
*   Code coverage analysis to ensure high test coverage
*   Automated test execution through Makefile targets

## Future Improvements

*   **Asynchronous Event Handling:** The current implementation processes UCI events synchronously. A future improvement would be to handle events asynchronously to better simulate a real-world scenario.
*   **More Comprehensive Hardware Testing:** The current hardware testing is limited. More comprehensive testing with a wider range of UWB devices would be beneficial.
*   **Support for More UCI Features:** While the current implementation supports a comprehensive subset of the UCI protocol, expansion to support additional features would be valuable.
*   **Enhanced UI Features:** More advanced CLI features like command history, configuration profiles, and visual ranging displays.
