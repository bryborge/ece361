CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -g -I. -fsanitize=address,undefined

all: build/lock

build/lock: build/main.o build/lock.o
	$(CC) $(CFLAGS) -o $@ $^

build/tests/test_lock: build/tests/test_lock.o build/lock.o
	$(CC) $(CFLAGS) -o $@ $^

build/tests/test_check: build/tests/test_check.o build/check.o build/lock.o
	$(CC) $(CFLAGS) -o $@ $^

test: build/tests/test_lock
	./$<

bonus: build/tests/test_check
	./$<

build/%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c -o $@ $<

build/main.o: main.c lock.h
build/lock.o: lock.c lock.h
build/tests/test_lock.o: tests/test_lock.c lock.h
build/check.o: check.c check.h lock.h
build/tests/test_check.o: tests/test_check.c check.h lock.h

clean:
	rm -rf build

.PHONY: all bonus clean test
