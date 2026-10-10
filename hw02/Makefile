CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -fsanitize=address

SRCS = main.c reading.c stats.c histogram.c
OBJS = $(SRCS:%.c=build/%.o)

all build: build/readings

build/readings: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

build/%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c -o $@ $<

build/main.o:      main.c reading.h stats.h histogram.h
build/reading.o:   reading.c reading.h
build/stats.o:     stats.c stats.h
build/histogram.o: histogram.c histogram.h

clean:
	rm -rf build/

.PHONY: all build clean
