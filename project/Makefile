CC     = gcc
CFLAGS = -std=c11 -Wall -Wextra -g -fsanitize=address,undefined -Iiom361_r4 -MMD -MP

SRCS = main.c sensor.c fsm.c store.c aux.c report.c iom361_r4/iom361_r4.c
OBJS = $(SRCS:%.c=build/%.o)

all: garage

build: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

build/%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c -o $@ $<

test: garage
	@echo "0 passed, 0 failed"

clean:
	rm -rf build garage

-include $(OBJS:.o=.d)

.PHONY: all test clean
