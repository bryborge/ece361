# Homework 4

## The tally ADT

> In Homework 4, we learn about abstract data types: a tally that counts how
> often each name occurs, such as the events a device sees. Callers use it only
> through its header; only `tally.c` knows what is inside.

### What is this?

A tally holds up to `TALLY_NAMES` (16) different names, each up to
`TALLY_LEN` (15) characters, with a count for each, and up to `TALLY_MAX`
(4) tallies can exist at once. Callers use it only through `tally.h`; see
[What the opaque type prevents](#what-the-opaque-type-prevents).

Inside, `tally.c` keeps a static pool of `TALLY_MAX` tallies, each marked
in use or free: `tally_destroy` only marks a tally free, and
`tally_create` empties it when it hands it out again. A tally is an array
of entries (a name and a count) kept in `strcmp` order. To add a new name,
`tally_add` finds the first entry whose name comes after it, moves that
entry and every later one up by one, starting from the last so that no
entry is overwritten before it has moved, and puts the new name in the
gap.

| File | Responsibility |
|------|----------------|
| `tally.h`, `tally.c` | The tally ADT. |
| `events.c`           | Counts the event names in a log read from stdin. |
| `data/door.txt`      | A sample garage door log for `events`. |
| `tests/test_tally.c` | Unit tests for every function in `tally`. |
| `Makefile`           | Builds `events` and the tests into `build/`. |
| `docs/PRD.md`        | The assignment's requirements, the lecture's recommendations, and the decisions made beyond them. |
| `AI_USAGE.md`        | Which AI tools were used, for what, and what they got wrong. |

### Build and test

```sh
make        # builds build/events
make test   # builds (if needed) and runs build/tests/test_tally
make clean  # removes build/
```

Everything is built with `gcc -std=c11 -Wall -Wextra` and compiles with no
warnings. `build/tests/test_tally` prints one line per check (`PASS` or
`FAIL`) followed by a summary, and exits nonzero if any check failed.

Everything is also built with `-fsanitize=address,undefined`, so an
out-of-bounds access or a `NULL` dereference aborts with a report, and
undefined behavior such as signed overflow prints a report, instead of either
passing silently.

### Usage

```sh
build/events < data/door.txt
```

`events` adds each line of its input to one tally, then prints the tally and
a last line, `names: N, lines skipped: S`.

- Blank lines and lines that start with `#` are ignored: they are neither
  names nor counted in `S`.
- `S` counts the lines `tally_add` refused: a line longer than `TALLY_LEN`
  (15) characters, however long, or a new name once the tally holds
  `TALLY_NAMES` (16) names.
- A `\r` at the end of a line is removed, so a file with Windows (CRLF) line
  endings gives the same result.

For `data/door.txt`, where `OBSTRUCTION_STOP` is 16 characters and so too
long:

```
ALARM 1
CLOSE 3
LOCK 1
OPEN 3
names: 4, lines skipped: 1
```

### Input ranges and boundary behavior

Every function that takes a `tally_t` assumes it came from `tally_create` and
has not been destroyed since. Using a tally after `tally_destroy` is not
detected.

**`tally_t *tally_create(void)`**
- Returns a new, empty tally, even when it reuses a destroyed one.
- Returns `NULL` once all `TALLY_MAX` are in use.

**`void tally_destroy(tally_t *t)`**
- Gives the tally back, so a later `tally_create` can hand it out again.
- Does nothing if `t` is `NULL`.

**`int tally_add(tally_t *t, const char *name)`**
- Returns `1` the first time `name` is added, then `2`, `3`, and so on.
- Returns `-1` and changes nothing if `t` or `name` is `NULL`, `name` is empty
  or longer than `TALLY_LEN`, or `name` is new and the tally already holds
  `TALLY_NAMES` names. A full tally still counts names it already holds.
- A name longer than `TALLY_LEN` is refused, never cut short.
- Names are compared exactly: `OPEN`, `open`, and `OPEN ` are three names.

**`int tally_count(const tally_t *t, const char *name)`**
- Returns `0` if `name` was never added, or if `t` or `name` is `NULL`.

**`int tally_size(const tally_t *t)`**
- Returns `0` to `TALLY_NAMES`; `0` if `t` is `NULL`.

**`void tally_print(const tally_t *t, FILE *out)`**
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
