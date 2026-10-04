CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -g -I.

all: build/tests/test_bits

build/%.o: %.c bits.h
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c -o $@ $<

build/tests/test_bits: build/tests/test_bits.o build/bits.o

test: build/tests/test_bits
	@./$<

clean:
	rm -rf build

.PHONY: all test clean
