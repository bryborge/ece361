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

clean:
	rm -rf build/

.PHONY: all build clean
