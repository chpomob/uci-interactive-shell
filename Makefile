CC=gcc
CFLAGS=-Iinclude -Wall -Wextra -std=c11 -Wno-unused-label -g -DHAVE_READLINE -D_GNU_SOURCE -fPIC $(QM_SDK_COMPAT)
LDFLAGS=-Wl,--no-as-needed
LIBS=-lreadline

# QM SDK Compatibility Build Options
# Uncomment the following line to enable QM SDK compatibility mode
# QM_SDK_COMPAT=-DQM_SDK_COMPAT

SRC=$(wildcard src/*.c)
OBJ=$(SRC:.c=.o)

TARGET=uci-shell
UNIT_TEST_TARGET=test_uci_functions
CONFIG_TEST_TARGET=test_config_manager
SESSION_MANAGER_TEST_TARGET=test_session_manager
SECURITY_TEST_TARGET=test_uci_security
COMMAND_GENERATION_TEST_TARGET=test_command_generation

COMMAND_HANDLER_TEST_TARGET=test_command_handlers
COMMAND_FRAMEWORK_VALIDATION_TEST_TARGET=test_command_framework_validation
HARDWARE_INTEGRATION_TEST_TARGET=test_hardware_integration
HW_FRAGMENTATION_TEST_TARGET=test_hw_fragmentation
PROTOCOL_DEFINITIONS_TEST_TARGET=test_protocol_definitions
PROTOCOL_FIXTURES_TEST_TARGET=test_protocol_fixtures
TRANSPORT_PARITY_TEST_TARGET=test_transport_parity
ANALYZER_DISPATCH_TEST_TARGET=test_analyzer_dispatch
CHERRY_ALIGNMENT_TEST_TARGET=test_cherry_alignment
PACKET_STRUCTURES_TEST_TARGET=test_packet_structures

# ────────────────────────────────────────────────────────────────────────────────
# Default first target: show help instead of building everything              #
# ────────────────────────────────────────────────────────────────────────────────
.PHONY: all clean install test unit-test config-test session-manager-test security-test command-generation-test command-handler-test command-framework-validation-test hardware-integration-test hw-fragmentation-test hardware-acceptance-smoke tcp-simulator-integration-test protocol-definitions-test protocol-fixtures-test transport-parity-test analyzer-dispatch-test cherry-alignment-test packet-structures-test coverage help

all: $(TARGET) unit-test config-test session-manager-test security-test command-generation-test command-handler-test command-framework-validation-test hw-fragmentation-test protocol-definitions-test protocol-fixtures-test transport-parity-test analyzer-dispatch-test cherry-alignment-test packet-structures-test

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(TARGET) $(OBJ) $(LIBS)

src/%.o: src/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

src/main.o: src/main.c include/uci.h include/uci_functions.h
src/uci.o: src/uci.c include/uci.h include/uci_functions.h
src/uci_ui_packet_decoder.o: src/uci_ui_packet_decoder.c include/uci_ui_packet_decoder.h
src/uci_cmd_session_config_ext.o: src/uci_cmd_session_config_ext.c include/uci_cmd_session_config_ext.h include/uci.h include/uci_functions.h

# Help screen
help:
	@echo 'UCI Interactive Shell — build & test commands'
	@echo ''
	@echo 'Build & install:'
	@echo '  make                Build project and run all tests'
	@echo '  help                Show this help message'
	@echo '  uci-shell           Build the interactive shell binary'
	@echo '  install             Install uci-shell to /usr/local/bin'
	@echo '  clean               Remove object files, binaries, and coverage data'
	@echo ''
	@echo 'Run tests (can also be run individually after building):'
	@echo '  test                Build and run all tests'
	@echo '  unit-test           Run uci_functions unit tests'
	@echo '  config-test         Run config_manager unit tests'
	@echo '  session-manager-test   Run session_manager unit tests'
	@echo '  security-test       Run uci_security unit tests'
	@echo '  command-generation-test   Run command_generation unit tests'
	@echo '  command-handler-test    Run command_handlers unit tests'
	@echo '  command-framework-validation-test   Run command_framework_validation unit tests'
	@echo '  hardware-integration-test     Run hardware_integration unit tests'
	@echo '  hw-fragmentation-test     Run hw_fragmentation unit tests'
	@echo '  protocol-definitions-test     Run protocol_definitions tests'
	@echo '  protocol-fixtures-test    Run protocol_fixtures tests'
	@echo '  transport-parity-test     Run transport_parity tests'
	@echo '  analyzer-dispatch-test  Run analyzer_dispatch tests'
	@echo '  cherry-alignment-test       Run cherry_alignment tests'
	@echo '  packet-structures-test    Run packet_structures tests'
	@echo ''
	@echo 'Other:'
	@echo '  coverage            Build with --coverage and produce coverage reports'
	@echo '  hardware-acceptance-smoke   Run hardware acceptance smoke test'
	@echo '  tcp-simulator-integration-test   Run shell ↔ simulator integration test'

clean:
	rm -f $(OBJ) $(TARGET) $(UNIT_TEST_TARGET) $(CONFIG_TEST_TARGET) $(SESSION_MANAGER_TEST_TARGET) $(SECURITY_TEST_TARGET) $(HARDWARE_INTEGRATION_TEST_TARGET) $(HW_FRAGMENTATION_TEST_TARGET) $(CHERRY_ALIGNMENT_TEST_TARGET) tests/*.o tests/*.d
	rm -f src/*.gcno src/*.gcda tests/*.gcno tests/*.gcda *.gcov *.gcda *.gcno
	rm -rf coverage

install: $(TARGET)
	install -m 755 $(TARGET) /usr/local/bin/uci-shell
	install -m 644 README.md /usr/local/share/doc/uci-shell/
	mkdir -p /usr/local/share/doc/uci-shell/uci_analysis
	cp -r uci_analysis/* /usr/local/share/doc/uci-shell/uci_analysis/

test: unit-test config-test session-manager-test security-test command-generation-test command-handler-test command-framework-validation-test hw-fragmentation-test protocol-definitions-test protocol-fixtures-test transport-parity-test analyzer-dispatch-test cherry-alignment-test packet-structures-test


unit-test: $(UNIT_TEST_TARGET)
	./$(UNIT_TEST_TARGET)

$(UNIT_TEST_TARGET): tests/test_uci_functions.o $(filter-out src/main.o,$(OBJ))
	$(CC) $(CFLAGS) -o $(UNIT_TEST_TARGET) tests/test_uci_functions.c $(filter-out src/main.o,$(OBJ)) $(LIBS)

config-test: $(CONFIG_TEST_TARGET)
	./$(CONFIG_TEST_TARGET)

$(CONFIG_TEST_TARGET): tests/test_config_manager.o $(filter-out src/main.o,$(OBJ))
	$(CC) $(CFLAGS) -o $(CONFIG_TEST_TARGET) tests/test_config_manager.c $(filter-out src/main.o,$(OBJ)) $(LIBS)

session-manager-test: $(SESSION_MANAGER_TEST_TARGET)
	./$(SESSION_MANAGER_TEST_TARGET)

$(SESSION_MANAGER_TEST_TARGET): tests/test_session_manager.o $(filter-out src/main.o,$(OBJ))
	$(CC) $(CFLAGS) -o $(SESSION_MANAGER_TEST_TARGET) tests/test_session_manager.c $(filter-out src/main.o,$(OBJ)) $(LIBS)

security-test: $(SECURITY_TEST_TARGET)
	./$(SECURITY_TEST_TARGET)

$(SECURITY_TEST_TARGET): tests/test_uci_security.o
	$(CC) $(CFLAGS) -o $(SECURITY_TEST_TARGET) tests/test_uci_security.c $(LIBS)

command-generation-test: $(COMMAND_GENERATION_TEST_TARGET)
	./$(COMMAND_GENERATION_TEST_TARGET)


$(COMMAND_GENERATION_TEST_TARGET): tests/test_command_generation.c tests/test_helpers.c $(filter-out src/main.o,$(OBJ))
	$(CC) $(CFLAGS) -o $(COMMAND_GENERATION_TEST_TARGET) tests/test_command_generation.c tests/test_helpers.c $(filter-out src/main.o,$(OBJ)) $(LIBS)

command-handler-test: $(COMMAND_HANDLER_TEST_TARGET)
	./$(COMMAND_HANDLER_TEST_TARGET)

$(COMMAND_HANDLER_TEST_TARGET): tests/test_command_handlers.c $(filter-out src/main.o,$(OBJ))
	$(CC) $(CFLAGS) -o $(COMMAND_HANDLER_TEST_TARGET) tests/test_command_handlers.c $(filter-out src/main.o,$(OBJ)) $(LIBS)

command-framework-validation-test: $(COMMAND_FRAMEWORK_VALIDATION_TEST_TARGET)
	./$(COMMAND_FRAMEWORK_VALIDATION_TEST_TARGET)

$(COMMAND_FRAMEWORK_VALIDATION_TEST_TARGET): tests/test_command_framework_validation.c $(filter-out src/main.o,$(OBJ))
	$(CC) $(CFLAGS) -o $(COMMAND_FRAMEWORK_VALIDATION_TEST_TARGET) tests/test_command_framework_validation.c $(filter-out src/main.o,$(OBJ)) $(LIBS)

hardware-integration-test: $(HARDWARE_INTEGRATION_TEST_TARGET)
	./$(HARDWARE_INTEGRATION_TEST_TARGET)

$(HARDWARE_INTEGRATION_TEST_TARGET): tests/test_hardware_integration.c $(filter-out src/main.o,$(OBJ))
	$(CC) $(CFLAGS) -o $(HARDWARE_INTEGRATION_TEST_TARGET) tests/test_hardware_integration.c $(filter-out src/main.o,$(OBJ)) $(LIBS)

hw-fragmentation-test: $(HW_FRAGMENTATION_TEST_TARGET)
	./$(HW_FRAGMENTATION_TEST_TARGET)

$(HW_FRAGMENTATION_TEST_TARGET): tests/test_hw_fragmentation.c $(filter-out src/main.o,$(OBJ))
	$(CC) $(CFLAGS) -o $(HW_FRAGMENTATION_TEST_TARGET) tests/test_hw_fragmentation.c $(filter-out src/main.o,$(OBJ)) $(LIBS)

hardware-acceptance-smoke: $(TARGET)
	./hardware_acceptance_smoke.sh

tcp-simulator-integration-test: $(TARGET)
	./tcp_simulator_integration_test.sh

protocol-definitions-test: $(PROTOCOL_DEFINITIONS_TEST_TARGET)
	./$(PROTOCOL_DEFINITIONS_TEST_TARGET)

$(PROTOCOL_DEFINITIONS_TEST_TARGET): tests/test_protocol_definitions.c $(filter-out src/main.o,$(OBJ))
	$(CC) $(CFLAGS) -o $(PROTOCOL_DEFINITIONS_TEST_TARGET) tests/test_protocol_definitions.c $(filter-out src/main.o,$(OBJ)) $(LIBS)

protocol-fixtures-test: $(PROTOCOL_FIXTURES_TEST_TARGET)
	./$(PROTOCOL_FIXTURES_TEST_TARGET)

$(PROTOCOL_FIXTURES_TEST_TARGET): tests/test_protocol_fixtures.c $(filter-out src/main.o,$(OBJ))
	$(CC) $(CFLAGS) -o $(PROTOCOL_FIXTURES_TEST_TARGET) tests/test_protocol_fixtures.c $(filter-out src/main.o,$(OBJ)) $(LIBS)

transport-parity-test: $(TRANSPORT_PARITY_TEST_TARGET)
	./$(TRANSPORT_PARITY_TEST_TARGET)

$(TRANSPORT_PARITY_TEST_TARGET): tests/test_transport_parity.c $(filter-out src/main.o src/uci_hw_chardev.o,$(OBJ))
	$(CC) $(CFLAGS) -o $(TRANSPORT_PARITY_TEST_TARGET) tests/test_transport_parity.c $(filter-out src/main.o src/uci_hw_chardev.o,$(OBJ)) $(LIBS)

analyzer-dispatch-test: $(ANALYZER_DISPATCH_TEST_TARGET)
	./$(ANALYZER_DISPATCH_TEST_TARGET)

$(ANALYZER_DISPATCH_TEST_TARGET): tests/test_analyzer_dispatch.c $(filter-out src/main.o,$(OBJ))
	$(CC) $(CFLAGS) -o $(ANALYZER_DISPATCH_TEST_TARGET) tests/test_analyzer_dispatch.c $(filter-out src/main.o,$(OBJ)) $(LIBS)

cherry-alignment-test: $(CHERRY_ALIGNMENT_TEST_TARGET)
	./$(CHERRY_ALIGNMENT_TEST_TARGET)

$(CHERRY_ALIGNMENT_TEST_TARGET): tests/test_cherry_alignment.c $(filter-out src/main.o,$(OBJ))
	$(CC) $(CFLAGS) -o $(CHERRY_ALIGNMENT_TEST_TARGET) tests/test_cherry_alignment.c $(filter-out src/main.o,$(OBJ)) $(LIBS)

packet-structures-test: $(PACKET_STRUCTURES_TEST_TARGET)
	./$(PACKET_STRUCTURES_TEST_TARGET)

$(PACKET_STRUCTURES_TEST_TARGET): tests/test_packet_structures.c $(filter-out src/main.o,$(OBJ))
	$(CC) $(CFLAGS) -o $(PACKET_STRUCTURES_TEST_TARGET) tests/test_packet_structures.c $(filter-out src/main.o,$(OBJ)) $(LIBS)



tests/test_uci_functions.o: tests/test_uci_functions.c tests/test_runner.h include/uci.h include/uci_functions.h
tests/test_config_manager.o: tests/test_config_manager.c tests/test_runner.h include/uci.h include/uci_config_manager.h
tests/test_session_manager.o: tests/test_session_manager.c tests/test_runner.h include/uci.h include/uci_functions.h
tests/test_uci_security.o: tests/test_uci_security.c include/uci_security.h
tests/uci_globals_test.o: tests/uci_globals_test.c include/uci_globals.h
tests/test_packet_structures.o: tests/test_packet_structures.c tests/test_runner.h include/uci_packet_structures.h include/uci.h

coverage: clean
	$(MAKE) CFLAGS="$(CFLAGS) --coverage" LIBS="$(LIBS) --coverage" unit-test config-test session-manager-test security-test
	mkdir -p coverage
	gcov -o src src/*.gcno > coverage/src_coverage.txt
	gcov -o tests tests/*.gcno > coverage/tests_coverage.txt
	mv -f *.gcov coverage/ 2>/dev/null || true
	@echo "Coverage reports written to coverage/src_coverage.txt and coverage/tests_coverage.txt"
