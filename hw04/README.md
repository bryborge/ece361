# Homework 4

## The tally ADT

> In Homework 4, we learn about abstract data types: a tally that counts how
> often each name occurs, such as the events a device sees. Callers use it only
> through its header; only `tally.c` knows what is inside.

### What is this?

A tally holds up to `TALLY_NAMES` (16) different names, each up to
`TALLY_LEN` (15) characters, with a count for each. `tally_t` is opaque:
`tally.h` declares it without a body, and only `tally.c` completes
`struct tally`.

Inside, a tally is an array of entries (a name and a count) kept in
alphabetical order by name. To add a new name, the tally finds the first entry
whose name comes after it, slides all entries after it down one slot to open a
gap, and puts the new name in the gap.

| File | Responsibility |
|------|----------------|
| `tally.h`, `tally.c` | The tally ADT. |
| `events.c`           | Counts the event names in a log read from stdin. |
| `data/door.txt`      | A sample garage door log for `events`. |
| `tests/test_tally.c` | Unit tests for every function in `tally`. |

| Function | What it does |
|----------|--------------|
| `tally_t *tally_create(void)` | Returns a new, empty tally, or `NULL` if `TALLY_MAX` tallies are already in use. |
| `void tally_destroy(tally_t *t)` | Gives the tally back so `tally_create` can hand it out again. |
| `int tally_add(tally_t *t, const char *name)` | Counts one more occurrence of `name` and returns its new count, or `-1`. |
| `int tally_count(const tally_t *t, const char *name)` | Returns how often `name` was added. |
| `int tally_size(const tally_t *t)` | Returns how many different names the tally holds. |
| `void tally_print(const tally_t *t, FILE *out)` | Writes one `name count` line per name, in `strcmp` order. |

### Usage

```sh
build/events < data/door.txt
```

`events` adds each line of its input to one tally, then prints the tally and a
last line with the number of names and the number of lines `tally_add`
refused. Blank lines and lines that start with `#` are ignored and not
counted. A line too long for the 64-byte input buffer is read to its end and
counts as one refused line. For `data/door.txt`, where `OBSTRUCTION_STOP` is
16 characters and so too long:

```
ALARM 1
CLOSE 3
LOCK 1
OPEN 3
names: 4, lines skipped: 1
```

### Build and test

```sh
make        # builds build/events
make test   # builds (if needed) and runs build/tests/test_tally
make clean  # removes build/
```

### Input ranges and boundary behavior

Every function that takes a `tally_t` assumes it came from `tally_create` and
has not been destroyed since. Using a tally after `tally_destroy` is not
detected.

**`tally_create()`**
- Hands out the first free tally in the pool, emptied.
- Returns `NULL` once all `TALLY_MAX` are in use.

**`tally_destroy(t)`**
- Does nothing if `t` is `NULL`.
- Only marks the tally free; `tally_create` empties it when it hands it out
  again.

**`tally_add(t, name)`**
- Returns `1` the first time `name` is added, then `2`, `3`, and so on.
- Returns `-1` and changes nothing if `t` or `name` is `NULL`, `name` is empty
  or longer than `TALLY_LEN`, or `name` is new and the tally already holds
  `TALLY_NAMES` names. A full tally still counts names it already holds.
- A name longer than `TALLY_LEN` is refused, never cut short.
- Names are compared exactly: `OPEN`, `open`, and `OPEN ` are three names.

**`tally_count(t, name)`**
- Returns `0` if `name` was never added, or if `t` or `name` is `NULL`.

**`tally_size(t)`**
- Returns `0` to `TALLY_NAMES`; `0` if `t` is `NULL`.

**`tally_print(t, out)`**
- Prints nothing for an empty tally, or if `t` or `out` is `NULL`.
- The order is `strcmp` order, not dictionary order: a prefix comes first
  (`OPEN` before `OPENED`), and every uppercase letter comes before every
  lowercase one (`OPEN` before `open`).

### What the opaque type prevents

To see it, I put this in a file that includes only `tally.h`:

```c
#include "tally.h"

int peek(const tally_t *t) {
    return t->n;
}
```

and compiled it with `gcc -std=c11 -Wall -Wextra -c` (gcc 14.2):

```
peek.c: In function ‘peek’:
peek.c:4:13: error: invalid use of incomplete typedef ‘tally_t’ {aka ‘const struct tally’}
    4 |     return t->n;
      |             ^~
```

`tally.h` only says that a `struct tally` exists, never what is in it, so
outside `tally.c` the compiler has no fields to look up and refuses any access
to them. The same goes for declaring a `tally_t` variable or taking
`sizeof(tally_t)`: the compiler does not know its size, so every tally has to
come from `tally_create`, which hands out slots from the pool. This means no
caller can break the rules the tally depends on: it cannot set `n` past
`TALLY_NAMES`, put a name out of order so that lookups miss it, write a name
without its `'\0'`, or reuse a slot that is still in use. Only `tally.c` can
break those rules, so a bug in a tally is a bug in `tally.c`, and that is the
only file to read when hunting it. It also means the inside can change, for
example from a sorted array to a tree, without changing a line in `events.c`
or the tests, as long as the functions keep their promises.
