CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -g

build: build/test_bits

build/test_bits: build/tests/test_bits.o build/bits.o
	$(CC) $(CFLAGS) -o $@ $^

build/%.o: %.c bits.h
	mkdir -p $(@D)
	$(CC) $(CFLAGS) -I. -c $< -o $@

test: build/test_bits
	./build/test_bits

clean:
	rm -rf build

.PHONY: build test clean
