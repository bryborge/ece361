CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -g -I. -fsanitize=address,undefined -fno-sanitize-recover=undefined
LDLIBS = -lm

MODULES = build/sensor.o build/panel.o build/scenario.o build/iom361_r4.o

all: build/replay

build/replay: build/main.o $(MODULES)
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

build/tests/test_hw06: build/tests/test_hw06.o $(MODULES)
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

test: build/tests/test_hw06 build/replay
	./build/tests/test_hw06
	sh regress.sh

build/%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c -o $@ $<

build/main.o: main.c iom361_r4.h panel.h scenario.h sensor.h
build/sensor.o: sensor.c iom361_r4.h sensor.h
build/panel.o: panel.c iom361_r4.h panel.h
build/scenario.o: scenario.c scenario.h
build/iom361_r4.o: iom361_r4.c iom361_r4.h
build/tests/test_hw06.o: tests/test_hw06.c iom361_r4.h panel.h scenario.h sensor.h

clean:
	rm -rf build

.PHONY: all clean test
