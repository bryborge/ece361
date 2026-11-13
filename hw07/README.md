# A Growing Stack and a Bracket Checker

> A stack of `int` on one heap array that starts with room for 8 and doubles
> when full, built on `malloc`, `realloc` and `free`, and a bracket checker
> that uses it.

### What is this?

Homework 7 is the first dynamic memory of the course. `istack` is an opaque
stack ADT: the caller holds an `istack_t *` and never sees the struct, whose
array `realloc` may move as it grows. `balance` reads a file on standard input
and uses the stack to check that its brackets match.

| File | What it's for |
|------|---------------|
| `istack.h`, `istack.c` | The stack: create, destroy, push, pop, peek, count, is_empty, capacity and to_array. |
| `balance.c` | The bracket checker. |
| `tests/test_istack.c` | Unit tests for `istack.c`. |
| `inputs/`, `expected/` | Seven inputs for `balance`, and the output and exit status each should give, written by hand. |
| `regress.sh` | Runs `./balance` on each input and diffs the result with its expected file. |
| `AI_USAGE.md` | Which AI tools I used, for what, and what they got wrong. |

### Build and test

Run these from `hw07/`:

```sh
make           # builds ./balance
make test      # builds (if needed), then runs the unit tests and regress.sh
make memcheck  # rebuilds everything with the sanitizers, then runs make test
make clean     # removes everything the build made
```

Everything is compiled with `gcc -std=c11 -Wall -Wextra`, with no warnings.
`make memcheck` adds `-fsanitize=address,undefined
-fno-sanitize-recover=undefined`, so a memory error, a leak or undefined
behavior fails the tests instead of only printing a report. It starts with
`make clean`, so that every object is rebuilt with the sanitizers, and the
binaries stay sanitized until the next `make clean`.

AddressSanitizer finds leaks on Linux only. On macOS, find them with:

```sh
leaks --atExit -- ./tests/test_istack
```

The unit tests print `PASS` or `FAIL` per check, and `regress.sh` per input.
Both exit nonzero on a failure.

### Usage

```sh
./balance < inputs/balanced.txt
```

It reads standard input one byte at a time, pushes each `(`, `[`, `{` and pops
at each `)`, `]`, `}`. Every bracket counts, also in comments and strings. It
stops at the first error and prints one line on stdout:

| Line | Exit status |
|------|-------------|
| `balanced: 6 pairs, deepest 3` | 0 |
| `line 1, column 8: found ')', expected ']'` | 1 |
| `line 1, column 12: found ')', nothing open` | 1 |
| `end of input: 2 still open, innermost '('` | 1 |

`deepest` is the most brackets open at once. Lines and columns count from 1,
and a column is a byte: a tab, a `'\r'` and each byte of a UTF-8 character
count as one.

When `balance` itself fails, it says so in one line on stderr and exits with
status 2, so that 1 always means "not balanced":

```
usage: balance < file
balance: out of memory
balance: read error
```

### Regression cases

Each file in `expected/` holds `balance`'s stdout, then a line `exit N` with
its exit status, so that one diff checks both.

| Input | Checks |
|-------|--------|
| `balanced.txt` | Brackets in a comment and a string count: 6 pairs, deepest 3. |
| `deep.txt` | 10 brackets open at once, so the stack grows past 8. |
| `mismatch.txt` | A wrong closer. |
| `mismatch_line2.txt` | A wrong closer on line 2: the line count, and the column starting over. |
| `nothing_open.txt` | A closer with nothing open. |
| `still_open.txt` | Two brackets still open at the end, across two lines. |
| `empty.txt` | An empty file is balanced: 0 pairs, deepest 0. |

### Part 4: Breaking it on purpose

Each bug was made in turn, run under `make memcheck`, and undone. The line
quoted is the first line of the report.

**(1) Destroying the same stack twice.** In `tests/test_istack.c`, a second
`istack_destroy(s);` right after the one in `check_destroy_not_empty`.

```
==64185==ERROR: AddressSanitizer: heap-use-after-free on address 0x503000000400 at pc 0x5570246ba493 bp 0x7ffd898c7f40 sp 0x7ffd898c7f38
```

The first destroy freed the array and then the struct, so the second one read
`s->data` out of the freed struct (offset 0, `istack.c:45`) to pass it to
`free`; ASan stopped at that read, a use after free, one step before the double
free itself would have happened.

**(2) Pushing onto a stack after destroying it.** In `tests/test_istack.c`,
`istack_push(s, 1);` right after `istack_destroy(s);` in
`check_destroy_not_empty`.

```
==64218==ERROR: AddressSanitizer: heap-use-after-free on address 0x503000000408 at pc 0x5599eaaa6549 bp 0x7ffc996e5210 sp 0x7ffc996e5208
```

`istack_destroy` freed the struct, but the caller's `s` still held its address,
so `istack_push` read `s->count` (offset 8 of the freed 24-byte struct,
`istack.c:57`) through a dangling pointer.

**(3) Leaving out `istack_destroy` in `balance.c`.** The one call, in `main`.

```
==63587==ERROR: LeakSanitizer: detected memory leaks
```

`main` returned with the stack still allocated and no pointer left to it: the
24-byte struct leaked directly, and its 32-byte array (8 ints) indirectly,
because the array's only pointer was inside the leaked struct.

### Known gaps

**The out-of-memory paths have no test.** A failed `malloc` in
`istack_create`, a failed `realloc` in `istack_push`, the size check before
doubling, and a failed `malloc` in `istack_to_array` all return their failure
with the stack unchanged, but making an allocation fail on demand would take a
fake allocator. The code guarantees these, and a test doesn't. The same holds
for `balance: read error`.
