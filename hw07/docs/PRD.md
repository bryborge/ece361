# HW7 PRD: A growing stack, and a bracket checker

Source: *Homework 7, ECE 361, Fall 2026, Revision 0* (Nov 10, 2026), cited
as **[HW]**, and the week 7 Tuesday slides, *Dynamic memory and the stack
ADT*, cited as **[L]** with a slide number.

Requirements (sections 2 to 7) come from the assignment and are graded.
Section 8 gives implementation recommendations: they are the suggested way to
meet the requirements, not requirements themselves. Section 9 records what the
assignment leaves open and how it was decided.

## 1. Overview

A stack of `int` on one heap array that starts with room for 8 values and
doubles when full, built on `malloc`, `realloc` and `free`. A bracket checker,
`balance`, uses it (Karumanchi 4.7, problem 1). Then memory bugs are made on
purpose for the sanitizers to catch, and undone. [HW]

| Milestone | Date |
|-----------|------|
| Code pushed to `hw07/` | Sun, Nov 15, 2026, 11:59pm |
| Shine window opens | Mon, Nov 16, 2026, morning |
| Shine due | Wed, Nov 18, 2026, 11:59pm |

Grading is Satisfactory / Not Yet. AI tools are allowed. The Shine is about
your own code, starts with a warm-up on one of your functions, and allows no
notes, no second screen, no AI and no other person; the webcam must be on.
[HW]

**Who writes what.** We settled the design together. Claude writes the code
and the documentation, in small pieces, and stops after each one for review.
The user reviews each piece and manages git. Part 4's bugs are made and run by
the user (section 9).

## 2. Deliverables

All in `hw07/` at the deadline. [HW]

| File | Contents |
|------|----------|
| `istack.h` | The interface in section 3.1, with a comment per function. |
| `istack.c` | `struct istack` and the nine functions. |
| `balance.c` | The program in section 4. |
| `tests/test_istack.c` | The unit tests in section 5.1. |
| `inputs/`, `expected/` | At least six regression cases (section 5.2). |
| `regress.sh` | Runs `./balance` on each input and compares with `expected/`. |
| `Makefile` | The targets in section 5.3. |
| `README.md` | How to build and test, then Part 4 (section 6). |
| `AI_USAGE.md` | Not specified in HW7. As in hw06: the tools used, what for, and what they got wrong. Claude drafts it with the README, and the user reviews it. |

Also, not a deliverable: `.gitignore` for the in-place build files (section 9).
The scaffold's empty `main.c` is renamed to `balance.c`.

## 3. Part 1: The istack ADT

### 3.1 `istack.h`

Exactly this interface, with an include guard and a comment per function on
what it takes, returns and assumes. The name is not `stack_t`, which macOS
already defines. [HW]

```c
#define ISTACK_START 8                  /* room at first; doubles when full */
typedef struct istack istack_t;         /* opaque */
istack_t *istack_create(void);
void      istack_destroy(istack_t *s);
int       istack_push(istack_t *s, int v);
int       istack_pop(istack_t *s, int *v);
int       istack_peek(const istack_t *s, int *v);
size_t    istack_count(const istack_t *s);
bool      istack_is_empty(const istack_t *s);
size_t    istack_capacity(const istack_t *s);
int      *istack_to_array(const istack_t *s, size_t *n);
```

The header needs `<stdbool.h>` and `<stddef.h>` for `bool` and `size_t`.
`istack_to_array`'s comment says that the caller owns the array and must free
it. [HW, L17]

### 3.2 Behavior

| Function | Behavior |
|----------|----------|
| `istack_create` | A new, empty stack with room for `ISTACK_START` values. `NULL` if memory runs out, and nothing leaked. |
| `istack_destroy` | Frees the stack and everything it holds. `NULL` does nothing. |
| `istack_push` | Pushes `v`. If the stack is full, first doubles the room with `realloc`. 0, or −1 if `s` is `NULL` or memory runs out; then the stack is unchanged. |
| `istack_pop`, `istack_peek` | The top value into `*v` (`v` may be `NULL`); pop also removes it. 0, or −1 if `s` is `NULL` or empty; then `*v` is unchanged. |
| `istack_count`, `istack_is_empty`, `istack_capacity` | Values held, whether none, the room right now. For `NULL`: 0, `true`, 0. |
| `istack_to_array` | A new array of the values, bottom first, and their number in `*n`. The caller owns it and frees it. `NULL` and `*n = 0` if `s` is `NULL` or empty, or memory runs out. If `n` is `NULL`: `NULL`, nothing allocated (section 9). |

[HW, Table 1]

### 3.3 `istack.c`

- `struct istack` is completed here and nowhere else. [HW]
- The values live in one array on the heap; the room never shrinks. [HW]
- `realloc` goes into a second pointer, so that a failure loses nothing. [HW]
- Every `malloc` has its `free`. [HW]

## 4. Part 2: `balance.c`

`./balance < file` reads standard input one character at a time. It pushes
each `(`, `[`, `{` and pops for each `)`, `]`, `}`. Every bracket counts, also
in comments and strings. It stops at the first error and prints exactly one
line. Lines and columns count from 1. [HW]

| Outcome | Line | Exit status |
|---------|------|-------------|
| Balanced | `balanced: 6 pairs, deepest 3` | 0 |
| Wrong closer | `line 1, column 8: found ')', expected ']'` | 1 |
| Closer with nothing open | `line 1, column 12: found ')', nothing open` | 1 |
| Openers left at the end | `end of input: 2 still open, innermost '('` | 1 |

`deepest` is the most brackets open at once. `balance` destroys its stack on
every path. [HW]

Decided beyond the spec (section 9):

| Case | stderr | Exit status |
|------|--------|-------------|
| Arguments given | `usage: balance < file` | 2 |
| `istack_create` or `istack_push` fails | `balance: out of memory` | 2 |
| Reading stdin fails | `balance: read error` | 2 |

All four result lines go to stdout. A column is a byte: tab, `'\r'` and each
byte of a UTF-8 character count as one.

## 5. Part 3: Tests, memory checks, Makefile

### 5.1 `tests/test_istack.c`

At least [HW]:

| # | Case |
|---|------|
| 1 | Pop and peek on an empty stack leave `*v` alone. |
| 2 | Last in, first out. |
| 3 | Full at 8, and the 9th push doubles the room to 16 and keeps every value. |
| 4 | 1000 pushes and pops. |
| 5 | `istack_to_array` is bottom first, and a copy. |
| 6 | `NULL` arguments. |
| 7 | Two stacks are independent. |
| 8 | Destroying a stack that is not empty. |

PASS or FAIL per test, a summary, and a nonzero exit status on failure. [HW]

### 5.2 Regression test

At least six files in `inputs/`: balanced, deeper than 8, each of the three
errors, and an empty file. Their expected output and exit status are in
`expected/`, compared by `regress.sh`, as in HW6. [HW]

The format is one file per input, `expected/NAME.txt`: balance's stdout, then
a last line `exit N` (section 9).

### 5.3 `Makefile`

| Target | Does |
|--------|------|
| `make` | Builds `balance`, with no warnings. |
| `make test` | Runs both tests: `tests/test_istack` and `regress.sh`. |
| `make memcheck` | Rebuilds everything with `-fsanitize=address,undefined` and runs `make test`. |
| `make clean` | Removes everything that was built. |

It finds leaks on Linux only. On macOS: `leaks --atExit -- ./tests/test_istack`.
[HW]

## 6. Part 4: Break it on purpose

In `README.md`, after how to build and test: make each bug below in turn, run
it under `make memcheck` (or `leaks` for the leak on macOS), and copy the
**first line** of the report into the README with one sentence on what
happened in memory. [HW]

1. Destroy the same stack twice.
2. Push onto a stack after destroying it.
3. Leave out `istack_destroy` in `balance.c`.

Then undo each bug: the code pushed must pass `make memcheck` cleanly. [HW]

## 7. Acceptance criteria (Satisfactory)

- [ ] Every deliverable in section 2 is in the repository at the deadline.
- [ ] `make` builds with no warnings.
- [ ] `make test` and `make memcheck` pass with no sanitizer report.
- [ ] `make clean` works.
- [ ] The room starts at 8 and doubles.
- [ ] The Shine is completed by its deadline and meets the bar.

[HW, Deliverable, Shine, and requirements]

## 8. Implementation recommendations

- **The struct** is three fields: `int *data`, `size_t count`, `size_t cap`.
  Two blocks: the struct never moves, the array may. [L18]
- **Create outside in, destroy inside out.** If the array's `malloc` fails,
  free the struct before returning `NULL`. Destroy frees `s->data`, then `s`,
  because the struct holds the only pointer to the array. [L20, L21]
- **Push** checks `s->cap > SIZE_MAX / 2 / sizeof *s->data` before doubling,
  so that `2 * cap * sizeof` cannot wrap (`<stdint.h>`). [L12, L28]
- **Sizes follow the pointer:** `n * sizeof *p`, not `n * sizeof(int)`. [L10]
- **`istack_to_array`** needs no overflow check: `count <= cap`, and `cap`
  bytes were already allocated. Copy with `memcpy`.
- **balance** keeps `line` and `col` counters, advanced per byte read; the
  position printed is the position of the offending closer. `pairs` counts
  successful matches; `deepest` is updated after each push. The pushed value
  is the opener's character code; the closer it expects is derived from it.
- **One exit path in balance**, so that `istack_destroy` is visibly called on
  every path, including the errors.
- **Tests** use hw06's `CHECK` macro: PASS/FAIL per check, a summary, and
  `return fails != 0`.
- **The sanitizer flags reach the link** as well as the compile: the link rule
  uses `$(CFLAGS)`, as in hw06.

## 9. Decisions beyond the spec

### 9.1 Decided

| Question | Decision | Why |
|----------|----------|-----|
| Build output | In place: `./balance`, `./tests/test_istack`, `*.o` next to the sources. | Matches the spec's commands exactly, including the macOS `leaks` line; no deviation needed. |
| Ignoring build output | `hw07/.gitignore`: `balance`, `tests/test_istack`, `*.o`. | Keeps hw07 self-contained; not inert, since it changes `git status`. |
| Sanitizers in `make` | No. `CFLAGS = -std=c11 -Wall -Wextra -g`. `memcheck: clean` runs `$(MAKE) CFLAGS="$(CFLAGS) -fsanitize=address,undefined -fno-sanitize-recover=undefined" test`. | The spec's "rebuilds" only means something if the normal build is plain. No-recover makes undefined behavior fail the run instead of only printing. After `memcheck`, the binaries stay sanitized until `make clean`; the README says so. |
| Expected-file format | `expected/NAME.txt`: stdout, then `exit N`. | One case, one file; one diff checks both output and status. |
| Result stream | All four result lines on stdout; `regress.sh` captures stdout only. | The exit status separates the outcomes. A sanitizer report on stderr stays out of the diff, and still fails the case through the exit status. |
| balance's own failures | One line on stderr, exit 2: usage, out of memory, read error. Stack destroyed on every path. | Keeps 1 meaning "unbalanced" and 2 "couldn't decide", as `diff` and `grep` do. |
| What a column counts | Bytes, from 1, reset after `'\n'`. | Follows "one character at a time" with `getchar`. Tab expansion or UTF-8 decoding would go past the spec. |
| `istack_to_array` with `n == NULL` | Returns `NULL`, allocates nothing. | Without `n` the copy is unusable; fits the spec's NULL-fails-cleanly rule. Covered by the NULL-arguments test. |
| Part 4 | The user makes and runs each bug; Claude writes it up from the output. Bugs (1) and (2) as a temporary check in `test_istack.c`; bug (3) in `balance.c`. | Seeing the reports first-hand is Shine preparation. |
| `AI_USAGE.md` | Claude drafts it with the README, and the user reviews it. | As in hw05 and hw06. |

### 9.2 Still open

Nothing.

## 10. Out of scope

The project's next step (S1: `aux.c` is this stack, holding events; S5: this
growing array, kept sorted; S2, S3: the same create/destroy pattern; S4:
`aux.h` only). Not graded and not on the Shine. [HW]

## 11. Shine preparation

Sample question from the assignment [HW]:

- "Your push calls `realloc` into a second pointer. What goes wrong with
  `s->data = realloc(s->data, …)`?"

  On failure `realloc` returns `NULL` and leaves the old block where it was,
  still allocated. Assigning straight back overwrites `s->data`, the only
  pointer to that block: the block leaks, and the stack's values are lost with
  it. The stack is then broken, `count` says there are values and `data` is
  `NULL`, so the next push or pop dereferences `NULL`. With a second pointer,
  `s->data` is only replaced after the check, and on failure the stack is
  unchanged, as the spec requires. The bug passes every test, sanitizers
  included, because `realloc` almost never fails there. [L31]

Questions the design invites:

- Why are there two blocks, and why must `istack_destroy` free `s->data`
  before `s`? [L18, L21]
- Why does `istack_create` free `s` when the array's `malloc` fails? [L20]
- What is the overflow check in push for, and why is it written as a
  division? [L12, L28]
- Why double rather than add 8? Doubling copies 8 + 16 + … + 512 = 1016 values
  for 1000 pushes, under 2n; adding a constant copies about n²/2k. The price is
  up to half the room unused. [L32]
- Who frees the array from `istack_to_array`, and why doesn't `istack_count`
  free anything? [L17, L19]
- Why is `istack_peek`'s parameter `const istack_t *` but `istack_pop`'s is
  not? [L17]
- What does `balance` push, and how does it know which closer to expect?
- Where is the stack destroyed when balance stops early on an error?
- After `free(p)`, what does `p` hold, and why is using it a bug that often
  "works"? [L9, L22]
