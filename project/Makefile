CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -g -fsanitize=address,undefined -fno-sanitize-recover=undefined -I. -Iiom361_r4 -MMD -MP
LDLIBS = -lm

SRCS = main.c sensor.c fsm.c store.c aux.c report.c iom361_r4/iom361_r4.c
OBJS = $(SRCS:%.c=build/%.o)

all build: build/garage

build/garage: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

TEST_SRCS = tests/test_main.c tests/test_sensor.c tests/test_fsm.c tests/test_aux.c \
            tests/test_store.c tests/test_report.c tests/test_replay.c
TEST_OBJS = $(TEST_SRCS:%.c=build/%.o) build/sensor.o build/fsm.o build/aux.o \
            build/store.o build/report.o build/iom361_r4/iom361_r4.o

# --wrap=realloc sends every realloc call to tests/test_aux.c, so that the aux
# tests can see the history grow.
build/tests/run_tests: $(TEST_OBJS)
	$(CC) $(CFLAGS) -Wl,--wrap=realloc -o $@ $^ $(LDLIBS)

# The replay tests run build/garage, so it has to be built first.
test: build/tests/run_tests build/garage
	./$<

build/%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -rf build/

-include $(OBJS:.o=.d) $(TEST_OBJS:.o=.d)

.PHONY: all build clean test
