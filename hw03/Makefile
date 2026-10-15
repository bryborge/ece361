CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -g -I. -fsanitize=address

all: build/tests/test_ptr

build/tests/test_ptr: build/tests/test_ptr.o build/ptr.o
	$(CC) $(CFLAGS) -o $@ $^

test: build/tests/test_ptr
	./$<

build/%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c -o $@ $<

build/ptr.o: ptr.c ptr.h
build/tests/test_ptr.o: tests/test_ptr.c ptr.h

clean:
	rm -rf build

.PHONY: all clean test
