CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -g -I.

HEADERS = bits.h status.h
OBJS = build/tests/test_bits.o build/bits.o build/status.o

all: build/tests/test_bits

build/%.o: %.c $(HEADERS)
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c -o $@ $<

build/tests/test_bits: $(OBJS)

test: build/tests/test_bits
	@./$<

clean:
	rm -rf build

.PHONY: all test clean
