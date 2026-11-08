CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -g -Ilib -fsanitize=address,undefined -fno-sanitize-recover=undefined

MODULES = build/lib/iom361_r4.o

all: build/replay

build/replay: build/main.o $(MODULES)
	$(CC) $(CFLAGS) -o $@ $^

build/tests/test_hw06: build/tests/test_hw06.o $(MODULES)
	$(CC) $(CFLAGS) -o $@ $^

test: build/tests/test_hw06
	./build/tests/test_hw06

build/%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c -o $@ $<

build/main.o: main.c lib/iom361_r4.h
build/lib/iom361_r4.o: lib/iom361_r4.c lib/iom361_r4.h
build/tests/test_hw06.o: tests/test_hw06.c lib/iom361_r4.h

clean:
	rm -rf build

.PHONY: all clean test
