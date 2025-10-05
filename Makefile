CC=gcc
CFLAGS=-Iinclude -Wall -Wextra -std=c11 -Wno-unused-label
LIBS=-lreadline

SRC=$(wildcard src/*.c)
OBJ=$(SRC:.c=.o)

TARGET=uci-shell
TEST_TARGET=test_chardev
UNIT_TEST_TARGET=test_uci_functions
CONFIG_TEST_TARGET=test_config_manager
HW_INTERFACE_TEST_TARGET=test_hw_interface
SESSION_MANAGER_TEST_TARGET=test_session_manager

.PHONY: all clean install test unit-test config-test hw-interface-test session-manager-test coverage

all: $(TARGET) unit-test config-test hw-interface-test session-manager-test

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ) $(LIBS)

src/main.o: src/main.c include/uci.h include/uci_functions.h
src/uci.o: src/uci.c include/uci.h include/uci_functions.h

clean:
	rm -f $(OBJ) $(TARGET) $(TEST_TARGET) $(UNIT_TEST_TARGET) $(CONFIG_TEST_TARGET) $(SESSION_MANAGER_TEST_TARGET) tests/*.o tests/*.d
	rm -f src/*.gcno src/*.gcda tests/*.gcno tests/*.gcda *.gcov *.gcda *.gcno
	rm -rf coverage

install: $(TARGET)
	install -m 755 $(TARGET) /usr/local/bin/uci-shell
	install -m 644 README.md /usr/local/share/doc/uci-shell/
	install -m 755 test_hardware_interface.sh /usr/local/bin/uci-hw-test
	mkdir -p /usr/local/share/doc/uci-shell/uci_analysis
	cp -r uci_analysis/* /usr/local/share/doc/uci-shell/uci_analysis/

test: $(TEST_TARGET)

$(TEST_TARGET): src/uci_hw_chardev.o
	$(CC) $(CFLAGS) -o $(TEST_TARGET) test_chardev.c src/uci_hw_chardev.o

unit-test: $(UNIT_TEST_TARGET)
	./$(UNIT_TEST_TARGET)

$(UNIT_TEST_TARGET): tests/test_uci_functions.o $(filter-out src/main.o,$(OBJ))
	$(CC) $(CFLAGS) -o $(UNIT_TEST_TARGET) tests/test_uci_functions.c $(filter-out src/main.o,$(OBJ)) $(LIBS)

config-test: $(CONFIG_TEST_TARGET)
	./$(CONFIG_TEST_TARGET)

$(CONFIG_TEST_TARGET): tests/test_config_manager.o $(filter-out src/main.o,$(OBJ))
	$(CC) $(CFLAGS) -o $(CONFIG_TEST_TARGET) tests/test_config_manager.c $(filter-out src/main.o,$(OBJ)) $(LIBS)

hw-interface-test: $(HW_INTERFACE_TEST_TARGET)
	./$(HW_INTERFACE_TEST_TARGET)

$(HW_INTERFACE_TEST_TARGET): tests/test_hw_interface.o $(filter-out src/main.o,$(OBJ))
	$(CC) $(CFLAGS) -o $(HW_INTERFACE_TEST_TARGET) tests/test_hw_interface.c $(filter-out src/main.o,$(OBJ)) $(LIBS)

session-manager-test: $(SESSION_MANAGER_TEST_TARGET)
	./$(SESSION_MANAGER_TEST_TARGET)

$(SESSION_MANAGER_TEST_TARGET): tests/test_session_manager.o $(filter-out src/main.o,$(OBJ))
	$(CC) $(CFLAGS) -o $(SESSION_MANAGER_TEST_TARGET) tests/test_session_manager.c $(filter-out src/main.o,$(OBJ)) $(LIBS)

tests/test_uci_functions.o: tests/test_uci_functions.c tests/test_runner.h include/uci.h include/uci_functions.h
tests/test_config_manager.o: tests/test_config_manager.c tests/test_runner.h include/uci.h include/uci_config_manager.h
tests/test_hw_interface.o: tests/test_hw_interface.c tests/test_runner.h include/uci.h include/uci_hw_interface.h
tests/test_session_manager.o: tests/test_session_manager.c tests/test_runner.h include/uci.h include/uci_functions.h

coverage: clean
	$(MAKE) CFLAGS="$(CFLAGS) --coverage" LIBS="$(LIBS) --coverage" unit-test config-test session-manager-test
	mkdir -p coverage
	gcov -o src src/*.gcno > coverage/src_coverage.txt
	gcov -o tests tests/*.gcno > coverage/tests_coverage.txt
	mv -f *.gcov coverage/ 2>/dev/null || true
	@echo "Coverage reports written to coverage/src_coverage.txt and coverage/tests_coverage.txt"
