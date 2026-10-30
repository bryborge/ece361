CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -g -I. -fsanitize=address,undefined

all: build/lock

build/lock: build/main.o build/lock.o
	$(CC) $(CFLAGS) -o $@ $^

build/tests/test_lock: build/tests/test_lock.o build/lock.o
	$(CC) $(CFLAGS) -o $@ $^

test: build/tests/test_lock
	./$<

build/%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c -o $@ $<

build/main.o: main.c lock.h
build/lock.o: lock.c lock.h
build/tests/test_lock.o: tests/test_lock.c lock.h

clean:
	rm -rf build

.PHONY: all clean test
