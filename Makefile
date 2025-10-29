CC=gcc
CFLAGS=-Iinclude -Wall -Wextra -std=c11 -Wno-unused-label -g  # Added debug flag
LIBS=

SRC=$(wildcard src/*.c)
OBJ=$(SRC:.c=.o)

TARGET=uci-shell
TEST_TARGET=test_chardev
MUTUALIZATION_TEST_TARGET=test_mutualization
UNIT_TEST_TARGET=test_uci_functions
CONFIG_TEST_TARGET=test_config_manager
SESSION_MANAGER_TEST_TARGET=test_session_manager
SECURITY_TEST_TARGET=test_uci_security

.PHONY: all clean install test unit-test config-test session-manager-test security-test coverage

all: $(TARGET) unit-test config-test session-manager-test security-test test-mutualization

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ) $(LIBS)

test-mutualization: $(MUTUALIZATION_TEST_TARGET)
	./$(MUTUALIZATION_TEST_TARGET)

$(MUTUALIZATION_TEST_TARGET): test_mutualization.c $(filter-out src/main.o src/uci_globals.o,$(OBJ)) tests/uci_globals_test.o
	$(CC) $(CFLAGS) -o $(MUTUALIZATION_TEST_TARGET) test_mutualization.c $(filter-out src/main.o src/uci_globals.o,$(OBJ)) tests/uci_globals_test.o $(LIBS)

src/main.o: src/main.c include/uci.h include/uci_functions.h
src/uci.o: src/uci.c include/uci.h include/uci_functions.h
src/uci_ui_packet_decoder.o: src/uci_ui_packet_decoder.c include/uci_ui_packet_decoder.h
src/uci_cmd_session_config_ext.o: src/uci_cmd_session_config_ext.c include/uci_cmd_session_config_ext.h include/uci.h include/uci_functions.h

clean:
	rm -f $(OBJ) $(TARGET) $(TEST_TARGET) $(UNIT_TEST_TARGET) $(CONFIG_TEST_TARGET) $(SESSION_MANAGER_TEST_TARGET) $(SECURITY_TEST_TARGET) tests/*.o tests/*.d
	rm -f src/*.gcno src/*.gcda tests/*.gcno tests/*.gcda *.gcov *.gcda *.gcno
	rm -rf coverage

install: $(TARGET)
	install -m 755 $(TARGET) /usr/local/bin/uci-shell
	install -m 644 README.md /usr/local/share/doc/uci-shell/
	mkdir -p /usr/local/share/doc/uci-shell/uci_analysis
	cp -r uci_analysis/* /usr/local/share/doc/uci-shell/uci_analysis/

test: $(TEST_TARGET)

$(TEST_TARGET): src/uci_hw_chardev.o
	$(CC) $(CFLAGS) -o $(TEST_TARGET) test_chardev.c src/uci_hw_chardev.o

unit-test: $(UNIT_TEST_TARGET)
	./$(UNIT_TEST_TARGET)

$(UNIT_TEST_TARGET): tests/test_uci_functions.o $(filter-out src/main.o,$(OBJ)) tests/stubs.o
	$(CC) $(CFLAGS) -o $(UNIT_TEST_TARGET) tests/test_uci_functions.c $(filter-out src/main.o,$(OBJ)) tests/stubs.o $(LIBS)

config-test: $(CONFIG_TEST_TARGET)
	./$(CONFIG_TEST_TARGET)

$(CONFIG_TEST_TARGET): tests/test_config_manager.o $(filter-out src/main.o,$(OBJ)) tests/stubs.o
	$(CC) $(CFLAGS) -o $(CONFIG_TEST_TARGET) tests/test_config_manager.c $(filter-out src/main.o,$(OBJ)) tests/stubs.o $(LIBS)

session-manager-test: $(SESSION_MANAGER_TEST_TARGET)
	./$(SESSION_MANAGER_TEST_TARGET)

$(SESSION_MANAGER_TEST_TARGET): tests/test_session_manager.o $(filter-out src/main.o,$(OBJ)) tests/stubs.o
	$(CC) $(CFLAGS) -o $(SESSION_MANAGER_TEST_TARGET) tests/test_session_manager.c $(filter-out src/main.o,$(OBJ)) tests/stubs.o $(LIBS)

security-test: $(SECURITY_TEST_TARGET)
	./$(SECURITY_TEST_TARGET)

$(SECURITY_TEST_TARGET): tests/test_uci_security.o
	$(CC) $(CFLAGS) -o $(SECURITY_TEST_TARGET) tests/test_uci_security.c $(LIBS)

tests/test_uci_functions.o: tests/test_uci_functions.c tests/test_runner.h include/uci.h include/uci_functions.h
tests/test_config_manager.o: tests/test_config_manager.c tests/test_runner.h include/uci.h include/uci_config_manager.h
tests/test_session_manager.o: tests/test_session_manager.c tests/test_runner.h include/uci.h include/uci_functions.h
tests/test_uci_security.o: tests/test_uci_security.c include/uci_security.h
tests/stubs.o: tests/stubs.c include/uci.h include/uci_cli.h
tests/uci_globals_test.o: tests/uci_globals_test.c include/uci_globals.h

coverage: clean
	$(MAKE) CFLAGS="$(CFLAGS) --coverage" LIBS="$(LIBS) --coverage" unit-test config-test session-manager-test security-test
	mkdir -p coverage
	gcov -o src src/*.gcno > coverage/src_coverage.txt
	gcov -o tests tests/*.gcno > coverage/tests_coverage.txt
	mv -f *.gcov coverage/ 2>/dev/null || true
	@echo "Coverage reports written to coverage/src_coverage.txt and coverage/tests_coverage.txt"
