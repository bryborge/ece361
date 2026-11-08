CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -g -I. -Iexternal -fsanitize=address,undefined -fno-sanitize-recover=undefined
LDLIBS = -lm

MODULES = build/external/iom361_r4.o build/panel.o build/scenario.o build/sensor.o

all: build/replay

build/replay: build/main.o $(MODULES)
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

build/tests/test_hw06: build/tests/test_hw06.o $(MODULES)
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

test: build/tests/test_hw06
	./build/tests/test_hw06

build/%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c -o $@ $<

build/main.o: main.c external/iom361_r4.h panel.h scenario.h sensor.h
build/external/iom361_r4.o: external/iom361_r4.c external/iom361_r4.h
build/panel.o: panel.c external/iom361_r4.h panel.h
build/scenario.o: scenario.c scenario.h
build/sensor.o: sensor.c external/iom361_r4.h sensor.h
build/tests/test_hw06.o: tests/test_hw06.c external/iom361_r4.h panel.h scenario.h sensor.h

clean:
	rm -rf build

.PHONY: all clean test
