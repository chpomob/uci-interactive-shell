CC=gcc
CFLAGS=-Isrc -Wall -Wextra -std=c11

SRC=$(wildcard src/*.c)
OBJ=$(SRC:.c=.o)

TARGET=uci-shell

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ)

src/main.o: src/main.c src/uci.h src/uci_functions.h
src/uci.o: src/uci.c src/uci.h src/uci_functions.h

clean:
	rm -f $(OBJ) $(TARGET)
