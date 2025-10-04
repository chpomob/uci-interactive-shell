CC=gcc
CFLAGS=-Iinclude -Wall -Wextra -std=c11
LIBS=-lreadline

SRC=$(wildcard src/*.c)
OBJ=$(SRC:.c=.o)

TARGET=uci-shell
TEST_TARGET=test_chardev
UNIT_TEST_TARGET=test_uci_functions

.PHONY: all clean install test unit-test

all: $(TARGET) unit-test

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ) $(LIBS)

src/main.o: src/main.c include/uci.h include/uci_functions.h
src/uci.o: src/uci.c include/uci.h include/uci_functions.h

clean:
	rm -f $(OBJ) $(TARGET) $(TEST_TARGET) $(UNIT_TEST_TARGET) tests/*.o tests/*.d

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

tests/test_uci_functions.o: tests/test_uci_functions.c tests/test_runner.h include/uci.h include/uci_functions.h
