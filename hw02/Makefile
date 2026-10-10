CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -fsanitize=address

SRCS = main.c reading.c stats.c histogram.c
OBJS = $(SRCS:%.c=build/%.o)

all build: build/readings

build/readings: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

build/test_stats: build/tests/test_stats.o build/stats.o
	$(CC) $(CFLAGS) -o $@ $^

test: build/test_stats build/readings
	build/test_stats
	sh regress.sh

build/%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c -o $@ $<

build/main.o:      main.c reading.h stats.h histogram.h
build/reading.o:   reading.c reading.h
build/stats.o:     stats.c stats.h
build/histogram.o: histogram.c histogram.h
build/tests/test_stats.o: tests/test_stats.c stats.h

clean:
	rm -rf build/

.PHONY: all build clean test
