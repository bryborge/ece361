CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -g -I.
BUILD = build

all: $(BUILD)/treestats

$(BUILD)/treestats: $(BUILD)/treestats.o $(BUILD)/bst.o
	$(CC) $(CFLAGS) -o $@ $^

$(BUILD)/tests/test_bst: $(BUILD)/tests/test_bst.o $(BUILD)/bst.o
	$(CC) $(CFLAGS) -o $@ $^

test: $(BUILD)/tests/test_bst
	./$(BUILD)/tests/test_bst

memcheck:
	$(MAKE) BUILD=build/memcheck CFLAGS="$(CFLAGS) -fsanitize=address,undefined -fno-sanitize-recover=undefined" test

$(BUILD)/%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD)/treestats.o: treestats.c bst.h
$(BUILD)/bst.o: bst.c bst.h
$(BUILD)/tests/test_bst.o: tests/test_bst.c bst.h

clean:
	rm -rf build

.PHONY: all clean memcheck test
