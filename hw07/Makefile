CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -g -I.

all: balance

balance: balance.o istack.o
	$(CC) $(CFLAGS) -o $@ $^

tests/test_istack: tests/test_istack.o istack.o
	$(CC) $(CFLAGS) -o $@ $^

test: tests/test_istack balance
	./tests/test_istack
	sh regress.sh

# Starts from clean so that every object is rebuilt with the sanitizers. The
# binaries stay sanitized until the next make clean.
memcheck: clean
	$(MAKE) CFLAGS="$(CFLAGS) -fsanitize=address,undefined -fno-sanitize-recover=undefined" test

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

balance.o: balance.c istack.h
istack.o: istack.c istack.h
tests/test_istack.o: tests/test_istack.c istack.h

clean:
	rm -f balance tests/test_istack *.o tests/*.o

.PHONY: all test memcheck clean
