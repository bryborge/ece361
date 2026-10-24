CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -g -I. -fsanitize=address,undefined

all: build/events

build/events: build/events.o build/tally.o
	$(CC) $(CFLAGS) -o $@ $^

build/tests/test_tally: build/tests/test_tally.o build/tally.o
	$(CC) $(CFLAGS) -o $@ $^

test: build/tests/test_tally
	./$<

build/%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c -o $@ $<

build/events.o: events.c tally.h
build/tally.o: tally.c tally.h
build/tests/test_tally.o: tests/test_tally.c tally.h

clean:
	rm -rf build

.PHONY: all clean test
