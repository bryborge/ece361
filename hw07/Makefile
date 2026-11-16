CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -g -I.
BUILD = build

all: $(BUILD)/balance

$(BUILD)/balance: $(BUILD)/balance.o $(BUILD)/istack.o
	$(CC) $(CFLAGS) -o $@ $^

$(BUILD)/tests/test_istack: $(BUILD)/tests/test_istack.o $(BUILD)/istack.o
	$(CC) $(CFLAGS) -o $@ $^

test: $(BUILD)/tests/test_istack $(BUILD)/balance
	./$(BUILD)/tests/test_istack
	sh regress.sh $(BUILD)/balance

# Builds into a directory of its own, so that sanitized objects and plain ones
# are never linked together.
memcheck:
	$(MAKE) BUILD=build/memcheck CFLAGS="$(CFLAGS) -fsanitize=address,undefined -fno-sanitize-recover=undefined" test

$(BUILD)/%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD)/balance.o: balance.c istack.h
$(BUILD)/istack.o: istack.c istack.h
$(BUILD)/tests/test_istack.o: tests/test_istack.c istack.h

clean:
	rm -rf build

.PHONY: all clean memcheck test
