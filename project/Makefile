CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -g -fsanitize=address,undefined -Iiom361_r4 -MMD -MP

SRCS = main.c sensor.c fsm.c store.c aux.c report.c iom361_r4/iom361_r4.c
OBJS = $(SRCS:%.c=build/%.o)

all build: build/garage

build/garage: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

build/%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -rf build/

-include $(OBJS:.o=.d)

.PHONY: all build clean
