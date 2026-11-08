CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -g -I. -Iexternal -fsanitize=address,undefined -fno-sanitize-recover=undefined

MODULES = build/external/iom361_r4.o build/panel.o

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

build/main.o: main.c external/iom361_r4.h panel.h
build/external/iom361_r4.o: external/iom361_r4.c external/iom361_r4.h
build/panel.o: panel.c panel.h
build/tests/test_hw06.o: tests/test_hw06.c external/iom361_r4.h panel.h

clean:
	rm -rf build

.PHONY: all clean test
