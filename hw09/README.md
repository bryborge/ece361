# A Hash Table That Counts

> A hash table of whole numbers that keeps track of how many times each one
> was added, with tests, and a look at why the number of buckets matters.

### What is this?

`ht` is a small hash table library. You add numbers to it, and it remembers
how many times each number was added. It can tell you that count for any
number, how many different numbers it holds, and how long its longest chain
is.

The table has a fixed number of buckets, chosen when it's created. A number
goes in bucket `key % nbuckets`. When two numbers land in the same bucket,
the bucket keeps them in a linked list, called a chain. A new number goes at
the front of its bucket's chain. Adding a number that's already there just
adds 1 to its count.

| File | What it's for |
|------|---------------|
| `ht.h`, `ht.c` | The hash table library. The structs are defined only in `ht.c`, so other code can't reach inside them. |
| `tests/test_ht.c` | Unit tests for the library. |
| `AI_USAGE.md` | How I used AI on this homework. |

### Build and test

Run these from `hw09/`:

```sh
make           # builds the test program, build/tests/test_ht
make test      # builds if needed, then runs the unit tests
make memcheck  # runs the unit tests again, built with the sanitizers
make clean     # deletes build/
```

The code compiles with `gcc -std=c11 -Wall -Wextra` and no warnings.

`make memcheck` builds with AddressSanitizer and UndefinedBehaviorSanitizer
(`-fsanitize=address,undefined -fno-sanitize-recover=undefined`). With these,
a memory error, a leak, or undefined behavior stops the tests with an error
instead of going unnoticed. The sanitized build goes in its own folder,
`build/memcheck/`, so it never gets mixed up with the normal build.

The unit tests print `PASS` or `FAIL` for each check and a total at the end.
If any check fails, the exit status is nonzero.

### Why the number of buckets matters

The assignment's Table 2 adds the keys 10, 20, 30, up to 10000, once with
1000 buckets and once with 1009. The tests check both.

With 1000 buckets the longest chain is 10 because every key is a
multiple of 10, and so is its remainder after dividing by 1000, which puts
all 1000 keys in just 100 buckets (0, 10, 20, up to 990), 10 keys each.
1009 is a prime number, so it shares no factor with 10, and the 1000 keys
all leave different remainders, giving each key a bucket of its own.
