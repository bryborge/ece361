CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -g -fsanitize=address,undefined -I. -Iiom361_r4 -MMD -MP
LDLIBS = -lm

SRCS = main.c sensor.c fsm.c store.c aux.c report.c iom361_r4/iom361_r4.c
OBJS = $(SRCS:%.c=build/%.o)

all build: build/garage

build/garage: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

build/tests/test_store: build/tests/test_store.o build/store.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

test: build/tests/test_store
	./$<

build/%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -rf build/

-include $(OBJS:.o=.d) build/tests/test_store.d

.PHONY: all build clean test
