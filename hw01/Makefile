CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -g -I.
HEADERS = bits.h          # add status.h in Part 3, not before: a missing prerequisite breaks make
OBJS = bits.o             # add status.o in Part 3

all: $(OBJS)

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

tests/test_bits: tests/test_bits.o bits.o
	$(CC) $(CFLAGS) -o $@ $^

test: tests/test_bits
	./tests/test_bits

clean:
	rm -f *.o tests/*.o tests/test_bits

.PHONY: all test clean
