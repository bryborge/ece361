CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -fsanitize=address

SRCS = readings.c histogram.c reading.c stats.c
OBJS = $(SRCS:%.c=build/%.o)

all build: build/readings

build/readings: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

build/test_stats: build/tests/test_stats.o build/stats.o
	$(CC) $(CFLAGS) -o $@ $^

build/test_histogram: build/tests/test_histogram.o build/histogram.o
	$(CC) $(CFLAGS) -o $@ $^

build/test_reading: build/tests/test_reading.o build/reading.o
	$(CC) $(CFLAGS) -o $@ $^

test: build/test_stats build/test_histogram build/test_reading build/readings
	build/test_stats
	build/test_histogram
	build/test_reading
	sh regress.sh

build/%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c -o $@ $<

build/stats.o: stats.c stats.h
build/tests/test_stats.o: tests/test_stats.c stats.h
build/histogram.o: histogram.c histogram.h
build/tests/test_histogram.o: tests/test_histogram.c histogram.h
build/reading.o: reading.c reading.h
build/tests/test_reading.o: tests/test_reading.c reading.h

clean:
	rm -rf build/

.PHONY: all build clean test
