CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -g -I.
SANITIZE = -fsanitize=address,undefined -fno-sanitize-recover=undefined
BUILD = build

all: $(BUILD)/tests/test_ht

$(BUILD)/tests/test_ht: $(BUILD)/tests/test_ht.o $(BUILD)/ht.o
	$(CC) $(CFLAGS) -o $@ $^

test: $(BUILD)/tests/test_ht
	./$(BUILD)/tests/test_ht

memcheck:
	$(MAKE) BUILD=build/memcheck CFLAGS="$(CFLAGS) $(SANITIZE)" test

$(BUILD)/%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD)/ht.o: ht.c ht.h
$(BUILD)/tests/test_ht.o: tests/test_ht.c ht.h

clean:
	rm -rf build

.PHONY: all clean memcheck test
