CC=gcc
CFLAGS=-Isrc -Wall -Wextra -std=c11

SRC=$(wildcard src/*.c)
OBJ=$(SRC:.c=.o)

TARGET=uci-shell
TEST_TARGET=test_chardev

.PHONY: all clean install test

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ)

src/main.o: src/main.c src/uci.h src/uci_functions.h
src/uci.o: src/uci.c src/uci.h src/uci_functions.h

clean:
	rm -f $(OBJ) $(TARGET) $(TEST_TARGET)

install: $(TARGET)
	install -m 755 $(TARGET) /usr/local/bin/uci-shell
	install -m 644 README.md /usr/local/share/doc/uci-shell/
	install -m 755 test_hardware_interface.sh /usr/local/bin/uci-hw-test
	mkdir -p /usr/local/share/doc/uci-shell/uci_analysis
	cp -r uci_analysis/* /usr/local/share/doc/uci-shell/uci_analysis/

test: $(TEST_TARGET)

$(TEST_TARGET): src/uci_hw_chardev.o
	$(CC) $(CFLAGS) -o $(TEST_TARGET) test_chardev.c src/uci_hw_chardev.o
