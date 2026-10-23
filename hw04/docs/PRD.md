# HW4 PRD: The tally ADT

Sources: *Homework 4, ECE 361, Fall 2026, Revision 0* (Oct 20, 2026), cited
as **[HW]**, and the Week 4 Tuesday lecture *Structs and Abstract Data Types*,
cited as **[L*n*]** for slide *n*.

Requirements (sections 2 to 6) come from the assignment and are graded.
Implementation recommendations (section 7) come from the lecture: they are
the suggested way to meet the requirements, not requirements themselves.
Section 8 covers what neither source settles: what has been decided, and what
is still open.

## 1. Overview

A **tally** counts how often each name occurs, such as the events a device
sees. It is the course's first abstract data type: callers use it only through
`tally.h`, and only `tally.c` knows what is inside. [HW]

| Milestone | Date |
|-----------|------|
| Code pushed to `hw04/` | Sun, Oct 25, 2026, 11:59pm |
| Shine window opens | Mon, Oct 26, 2026, morning |
| Shine due | Wed, Oct 28, 2026, 11:59pm |

Grading is Satisfactory / Not Yet. AI tools are allowed for the code; the
Shine allows no notes, no second screen, no AI, and no other person, with the
webcam on. [HW]

## 2. Deliverables

All in `hw04/` at the deadline. [HW]

| File | Contents |
|------|----------|
| `tally.h` | The interface in section 3, with a comment per function. |
| `tally.c` | The only definition of `struct tally`; the pool. |
| `events.c` | The program in section 4.1. |
| `data/` | At least one input file of your own, e.g. `data/door.txt`. |
| `tests/test_tally.c` | The tests in section 4.2. |
| `Makefile` | The targets in section 4.3. |
| `README.md` | How to build and test, then Part 3 (section 5). |
| `AI_USAGE.md` | Tools used, for what, and one thing a model got wrong. |

## 3. Part 1: The tally ADT

### 3.1 `tally.h`

Declare exactly this interface, with a comment per function on what it takes,
returns, and assumes. [HW]

```c
#define TALLY_MAX    4    /* tallies that can exist at the same time */
#define TALLY_NAMES  16   /* different names one tally can hold */
#define TALLY_LEN    15   /* longest name, without the '\0' */

typedef struct tally tally_t;      /* opaque */

tally_t *tally_create(void);
void     tally_destroy(tally_t *t);
int      tally_add(tally_t *t, const char *name);
int      tally_count(const tally_t *t, const char *name);
int      tally_size(const tally_t *t);
void     tally_print(const tally_t *t, FILE *out);
```

### 3.2 Behavior

| Function | Normal result | Error and edge cases |
|----------|---------------|----------------------|
| `tally_create` | A new, empty tally. | `NULL` if `TALLY_MAX` tallies are already in use. |
| `tally_destroy` | Gives the tally back so `tally_create` can hand it out again. | `NULL` does nothing. |
| `tally_add` | Counts one more occurrence of `name`; returns its new count. | Returns `-1` **and changes nothing** if `t` is `NULL`, `name` is `NULL`, `name` is empty, `name` is longer than `TALLY_LEN`, or `name` is new and the tally already holds `TALLY_NAMES` names. |
| `tally_count` | How often `name` was added. | `0` if never added, or for `NULL`. |
| `tally_size` | How many different names the tally holds. | `0` for `NULL`. |
| `tally_print` | One line per name, `name count`, in `strcmp` order, to `out`. | Not specified; decided: prints nothing if `t` or `out` is `NULL` (section 8.1). |

[HW, Table 1]

Note the asymmetry in the full-tally case: a **new** name is refused, but an
**existing** name still counts. [HW, Part 2 test list]

### 3.3 `tally.c` constraints

- **Opaque type.** `struct tally` is completed in `tally.c` and nowhere else.
  [HW]
- **Sorted storage.** `struct tally` holds an array of entries, each a name and
  a count, kept sorted by name. A new name goes where it belongs; the later
  entries move up by one. [HW]
- **No `malloc`.** Tallies live in a `static` array of `TALLY_MAX` inside
  `tally.c`, each marked in use or free. There is no global outside the pool.
  [HW, Requirements for Satisfactory]
- **Safe name copies.** The tally copies each name. Check the length first and
  never write past an array. No `strcpy` or `strncpy` without a length check.
  [HW]

## 4. Part 2: Program, tests, Makefile

### 4.1 `events.c`

1. Read standard input line by line with `fgets`.
2. Remove the newline.
3. Skip blank lines and lines that start with `#`.
4. Add every other line to one tally.
5. Print the tally, then one last line:
   `names: N, lines skipped: S`, where `S` counts the lines `tally_add`
   refused.

Example invocation: `./events < data/door.txt`. [HW]

`S` counts refusals only. Blank and `#` lines are skipped before `tally_add`
and so are not in `S`. [HW, by the wording "the lines `tally_add` refused"]

For how overlong lines, CRLF endings and whitespace-only lines are handled,
see section 8.

Expected shape of the tally output [L16]:

```
ALARM 1
CLOSE 3
LOCK 1
OPEN 3
```

### 4.2 `tests/test_tally.c`

Test every function. At minimum [HW]:

| # | Case |
|---|------|
| 1 | Counting up: repeated adds return 1, 2, 3, ... |
| 2 | A name never added counts 0. |
| 3 | An empty name is refused. |
| 4 | A name of exactly `TALLY_LEN` characters is accepted. |
| 5 | A name of `TALLY_LEN + 1` characters is refused. |
| 6 | `NULL` arguments, for every function that takes a pointer. |
| 7 | A full tally: a new name is refused; an existing name still counts. |
| 8 | `TALLY_MAX` tallies can be created; one more returns `NULL`. |
| 9 | Create after destroy succeeds. |
| 10 | Two tallies are independent. |
| 11 | Print order is `strcmp` order. |

Output: `PASS` or `FAIL` per test, then a summary. Exit status is nonzero if
any test fails. [HW]

### 4.3 `Makefile`

| Target | Does |
|--------|------|
| `make` | Builds `events` with `gcc -std=c11 -Wall -Wextra`, with no warnings. |
| `make test` | Builds and runs the tests. |
| `make clean` | Removes everything that was built. |

Each `.o` rule lists the headers its source includes as prerequisites. [HW]

## 5. Part 3: What the opaque type prevents

In `README.md`, after the build and test instructions [HW]:

1. In a file that includes only `tally.h`, try `t->n` (or any member of your
   struct).
2. Copy the compiler's error message into the README.
3. Explain in one paragraph what the opaque type prevents and why that is
   useful.
4. Remove the offending line: the pushed code must compile.

The lecture's expected errors [L18]:

```
clang: error: incomplete definition of type 'tally_t' (aka 'struct tally')
gcc:   error: invalid use of incomplete typedef 'tally_t' {aka 'struct tally'}
```

Points the lecture gives for the paragraph [L15, L17, L18, L25]:

- The compiler enforces opacity simply by not having the struct body; copying
  the definition into a caller would defeat it there.
- Callers cannot break the invariants (sortedness, `n`, `in_use`), e.g. by
  writing the equivalent of `c.count = 99`.
- Tallies come only from `tally_create`, so the pool stays the single source
  of truth.
- The header is the contract; `tally.c` can change its inside (as `store.c`
  will in week 8) without changing any caller.

## 6. Acceptance criteria (Satisfactory)

- [ ] Every deliverable in section 2 is in the repository at the deadline.
- [ ] `make` builds with no errors and no warnings.
- [ ] `make test` runs and all tests pass.
- [ ] `make clean` works.
- [ ] Only `tally.c` completes `struct tally`.
- [ ] No `malloc`, and no global outside the pool.
- [ ] Names are kept sorted and copied safely.
- [ ] The Shine is completed by its deadline and meets the bar.

[HW, Requirements for Satisfactory]

## 7. Implementation recommendations

From the lecture. These are suggestions, and the Shine will ask you to explain
whichever choices you made.

### 7.1 Layout [L19]

```c
struct entry { char name[TALLY_LEN + 1]; int count; };

struct tally {
    int          in_use;   /* handed out by tally_create? */
    int          n;        /* entries in use, sorted by name */
    struct entry e[TALLY_NAMES];
};
```

`name` is `TALLY_LEN + 1` bytes: room for the `'\0'`. [L4]

### 7.2 Pool and `tally_create` [L21, L22]

```c
static struct tally pool[TALLY_MAX];   /* private to tally.c, lives all run */

tally_t *tally_create(void) {
    for (int i = 0; i < TALLY_MAX; i++)
        if (!pool[i].in_use) {
            pool[i].in_use = 1;
            pool[i].n = 0;
            return &pool[i];
        }
    return NULL;                       /* all in use */
}
```

`static` at file scope gives internal linkage (no other file can name `pool`)
and static storage duration (it lives the whole run). It does not make the
data read-only; only `const` does that. [L22]

Resetting `n` in `tally_create` is what makes a reused slot start empty.

### 7.3 Safe copy in `tally_add` [L7]

```c
size_t len = strlen(name);
if (len > TALLY_LEN)                   /* refuse it, do not cut it */
    return -1;
memcpy(e->name, name, len + 1);        /* the letters and the '\0' */
```

`strncpy` does not guarantee a `'\0'` when the source fills the buffer. Either
`strncpy` or `memcpy` can be made safe; the lecture treats the choice as
style. [L7]

### 7.4 Insertion [L19, L20]

Find `pos`, the index of the first name that sorts after the new one, then
shift from the end toward `pos`:

```c
for (int k = t->n; k > pos; k--)       /* make room at pos */
    t->e[k] = t->e[k - 1];
```

**Why backwards:** shifting from the end means each entry is copied into an
empty slot before its own slot is overwritten. Shifting forward from `pos`
would copy `e[pos]` over `e[pos + 1]` and lose it. Struct assignment copies
the whole entry, `name` array included. [L11, L20]

Worked example, inserting `LOCK` with `n = 3` (`pos = 2`) [L20]:

| Step | e[0] | e[1] | e[2] | e[3] |
|------|------|------|------|------|
| before | ALARM 1 | CLOSE 3 | OPEN 3 | (empty) |
| shift, k = 3 | ALARM 1 | CLOSE 3 | OPEN 3 | OPEN 3 |
| insert at pos | ALARM 1 | CLOSE 3 | LOCK 1 | OPEN 3 |

Do every refusal check (`NULL`, empty, too long, full and new) before
shifting, so a refused add changes nothing.

### 7.5 Reading lines in `events.c` [L8]

```c
char line[64];
while (fgets(line, sizeof line, stdin) != NULL) {
    size_t len = strlen(line);
    if (len > 0 && line[len - 1] == '\n')
        line[--len] = '\0';            /* "OPEN\n" becomes "OPEN" */
    ...
}
```

Size the buffer with `sizeof`, and strip the `'\n'` before comparing or
storing.

### 7.6 String pitfalls [L5, L6]

| Want | Write | Not | Because |
|------|-------|-----|---------|
| equal? | `strcmp(a, b) == 0` | `a == b` | compares addresses |
| first in the alphabet? | `strcmp(a, b) < 0` | `a < b` | compares addresses |
| length | `strlen(a)` | `sizeof a` | 8 for a pointer parameter |
| a copy | `strlen`, then `memcpy` | `a = b` | does not compile |

Use `const char *` for string literals and for names the function only reads.

### 7.7 Header design decisions [L23]

| Decide | For the tally |
|--------|---------------|
| names | every function starts with `tally_` |
| ownership | the tally copies the name; the caller keeps its buffer |
| errors | `-1` from `tally_add`, `0` from `tally_count`, `NULL` from create |
| limits | `TALLY_MAX`, `TALLY_NAMES`, `TALLY_LEN`, in the header |
| const | count, size, and print take `const tally_t *` |

## 8. Decisions beyond the spec

Neither source settles these. Each decided behavior is documented in
`tally.h`, `events.c` or the README.

### 8.1 Decided

| Question | Decision | Where |
|----------|----------|-------|
| `tally_print` with a `NULL` `t` or `out` | Prints nothing. Table 1 gives no `NULL` rule for print, so this follows the other functions. | `tally.h`, `tally.c`; tested |
| Input lines longer than the `fgets` buffer (64 bytes) | The rest of the line is read and thrown away. The whole line counts as one line, which `tally_add` refuses, so it adds 1 to `S`. | `events.c`, README |
| Windows (CRLF) line endings | One `'\r'` at the end of a line is removed after the `'\n'`, so a CRLF file gives the same output as an LF file. A line holding only `"\r"` becomes blank and is skipped. | `events.c`, README |
| Using a tally after `tally_destroy` | Not detected. The header assumes every `tally_t` came from `tally_create` and has not been destroyed since. | `tally.h`, README |
| Comparing names | Names must match exactly: `OPEN`, `open` and `OPEN ` are three different names. The order is `strcmp` order, not dictionary order. | README; tested |

### 8.2 Still open

1. **Fail-fast on misuse.** A second `tally_destroy` on the same tally, or a
   pointer that did not come from the pool, is not detected. The pool makes
   both easy to detect: compare `t` with `==` against each slot (`<` between
   unrelated pointers is undefined behavior) and `assert` that the slot is in
   use. The case for adding it: a second `tally_destroy` frees a slot that
   `tally_create` may already have handed to a new owner. The case against:
   the spec doesn't ask for it, and `assert` hasn't come up in class yet.
2. **Whitespace-only lines.** `"   "` is counted as a name. "Blank" is not
   defined in the spec. Skipping such lines is the friendlier reading; keeping
   them is consistent with matching names exactly (section 8.1).
3. **Sanitizers in the default build.** `CFLAGS` includes
   `-fsanitize=address,undefined`. If the grader's `gcc` is installed without
   `libasan` (on some distributions it is a separate package), `make` fails
   to link, which would fail "make builds with no errors". The safe option is
   to use the sanitizers only for `make test`.

## 9. Out of scope

The project scaffolding (`project/sensor.h` with `reading_t`,
`project/store.h` with `record_t` and an opaque `store_t`, and `store.c` as a
sorted array) is not part of HW4, is not graded, and is not asked about in the
HW4 Shine. [HW, L26]

## 10. Shine preparation

Sample questions from the assignment [HW]:

- "Walk me through your `tally_add` when a new name goes in the middle: what
  moves, and why in that order?" (section 7.4)
- "Why can `events.c` not look inside a `tally_t`, and what would break if it
  could?" (section 5)

Other questions the lecture poses that may come up: why `sizeof` and `strlen`
differ by one [L3], what `const` does on a struct-pointer parameter [L10], and
what `static` means for `pool` [L22].
