# ECE 361 HW1, Part 2: bit-manipulation library. Design and implementation plan

Status: accepted.
Suggested location in the repo: `docs/hw01-part2-plan.md` (outside `hw01/`, so `hw01/` holds only the handout's deliverables). Part 3 gets its own plan doc.

## 1. Goal and scope

Write `bits.h` and `bits.c` with the four Part 2 functions (`print_binary`, `get_field`, `set_field`, `sign_extend`) plus one extra public helper, `format_binary`, which exists so `print_binary` can be tested.

In scope: everything Part 2 forces. That means the Makefile targets needed to build and test these functions, the `tests/test_bits.c` cases for them, and the Part 2 sections of `hw01/README.md`.

Out of scope here: `status_t` / `status_unpack` (Part 3) and the status-word tests. The Makefile and test file are laid out so they can be extended later.

## 2. Constraint: the toolbox

Requirements come from the HW1 handout. Techniques come only from the "C Programming Review" and "Week 1" slides.

**Allowed (from the slides):** `<stdint.h>` exact-width types and `INT32_MIN`/`INT32_MAX`, `<inttypes.h>`, `<stdio.h>` (`printf`, `putchar`), `<stdbool.h>`, `<limits.h>`, bitwise operators with `1u` shifts, `#define` / `enum` / `static const`, `static` helpers, include guards, arrays and `sizeof`, the make pattern (`$@ $^ $<`, `%.o: %.c`, `.PHONY`), the `CHECK` macro, `-std=c11 -Wall -Wextra -g -c -o`.

**Accepted suggestions (outside the slides, explicitly approved):**

| # | Item | Where it is used |
|---|------|------------------|
| 1 | `strcmp` from `<string.h>` | `tests/test_bits.c` only, to compare formatted strings |
| 2 | `-I.` compiler flag | `CFLAGS` in the Makefile, so the test can write `#include "bits.h"` |

**Offered and withdrawn or not adopted:** `freopen` stdout capture (withdrawn: standard C cannot restore stdout, see D3), `UINT32_MAX`, `size_t` via `<stddef.h>`, `gcc -MMD` auto-dependencies.

**Deliberately avoided because not in the slides:** `assert.h`, sanitizers, struct-based test tables, `0b` literals, `CPPFLAGS`.

## 3. Design tree

Each line is `ID decision. Choice (rejected alternative)`.

```
ROOT  bits.h / bits.c: print_binary, get_field, set_field, sign_extend  (+ format_binary)
|
+- A  Scope: Part 2 + what it forces (tests, Makefile targets, README Part 2 sections)
|
+- B  Contract
|  +- B1  Out of range, get_field / set_field: REJECT
|  |      get_field -> 0, set_field -> word unchanged        (not: truncate to the word)
|  |  +- B1a  Signalling: silent, return value only          (not: message on stderr)
|  |  +- B1b  Same rule everywhere
|  |         sign_extend -> 0, format_binary -> "", print_binary prints nothing
|  |                                                          (not: clamp width in those two)
|  +- B2  print_binary: no trailing newline                   (not: print one)
|  |  +- B2a  Width not a multiple of 4: group from the right,
|  |         short leading group, no padding. (0x2C, 6) -> "10 1100"
|  |                                                          (not: pad to a multiple of 4)
|  +- B3  Value wider than field: mask to the low `width` bits   (fixed by the handout)
|
+- C  Implementation (bits.c)
|  +- C1  Mask for width 32: explicit branch
|  |      (width == WORD_BITS) ? ALL_ONES : (1u << width) - 1u
|  |                                                          (not: 0xFFFFFFFFu >> (32 - width))
|  |  +- C1a  One `static uint32_t low_mask(int width)` helper (not: inline in each function)
|  |  +- C1b  Validity: `static bool` helpers, checks ordered so the sum cannot overflow
|  |         (not: guard clauses, int64_t widening, macro)
|  +- C2  sign_extend: mask, OR in ~mask if the sign bit is set, return converts to int32_t
|  |                                                          (not: negate-by-hand, explicit cast)
|  +- C3  Constants: enum { WORD_BITS = 32 } and static const uint32_t ALL_ONES in bits.c;
|         public enum BIN_BUF_SIZE in bits.h                   (not: #define, UINT32_MAX)
|
+- D  Testing (tests/test_bits.c)
|  +- D1  Harness: slide CHECK macro + total counter + summary line + exit status
|  +- D2  Depth: hand-derived boundary cases only             (not: exhaustive cross-check)
|  +- D3  print_binary is untestable through stdout alone -> public format_binary,
|  |      print_binary is a thin wrapper                      (not: freopen capture, eyeball only)
|  |  +- D3b  format_binary returns void; empty string means failure
|  |  +- D3c  Buffer too small: all or nothing (empty string)  (not: snprintf-style truncation)
|  |  +- D3d  BIN_BUF_SIZE = 32 + 7 + 1 as a public enum in bits.h
|  |  +- D3e  Buffer size parameter is `int size`, like get_line   (not: size_t)
|  |  +- D3f  The wrapper gets one labeled demo line, not PASS/FAIL
|  +- D4  String comparison: strcmp                           [ACCEPTED SUGGESTION #1]
|
+- E  Build and repo
|  +- E1  Pattern rule `%.o: %.c $(HEADERS)`; HEADERS lists all headers
|  |      (just bits.h until Part 3)
|  +- E2  Include path: `-I.` in CFLAGS, test uses #include "bits.h"
|         [ACCEPTED SUGGESTION #2]                            (not: #include "../bits.h")
|
+- F  Process
|  +- F1  Order: spec -> tests (expected values by hand) -> code, one function at a time
|  +- F2  AI: small asks with my README as the spec; I read every diff and write the
|  |      boundary tests myself; AI_USAGE.md records this planning session too
|  +- F3  Commits: I make every commit myself and author every message. An AI assistant
|         stops at each checkpoint and never runs git add, commit or push.
|
+- G  This doc: Markdown outside hw01/, named hw01-part2-plan.md
```

### Why `-I.` and not `../bits.h` (E2)

| | `-I.` with `#include "bits.h"` | `#include "../bits.h"` |
|---|---|---|
| Source stays valid if the file moves | Yes | No, the path encodes the directory layout |
| Matches how other files include it | Yes | No, the one file with a relative path |
| Compiles outside the Makefile | No, `gcc tests/test_bits.c` fails without the flag | Yes, from `hw01/` |
| Toolbox | Accepted suggestion #2 | Inside the toolbox |
| Later work | Same pattern for `status.h` and for reusing `bits.h` from `project/` | One more `../` per header and directory |

For quote includes, gcc searches the including file's own directory first (`tests/`), then `-I` directories. That is why `-I.` is what makes `"bits.h"` resolve from inside `tests/`. The flag goes in `CFLAGS`, matching the slides' Makefile; `CPPFLAGS` would be the conventional variable for preprocessor flags but is not used in the slides.

## 4. Contract (source text for hw01/README.md)

Bits are numbered from 0, the least significant bit. Constants: `WORD_BITS` = 32.

| Function | Valid input | On invalid input |
|----------|-------------|------------------|
| `uint32_t get_field(uint32_t word, int pos, int width)` | width 1..32, pos 0..31, pos + width <= 32 | returns 0 |
| `uint32_t set_field(uint32_t word, int pos, int width, uint32_t value)` | same as get_field | returns `word` unchanged |
| `int32_t sign_extend(uint32_t value, int width)` | width 1..32 | returns 0 |
| `void format_binary(char *buf, int size, uint32_t x, int width)` | width 1..32 and size >= width + (width - 1) / 4 + 1 | if size >= 1, `buf` is `""`; if size < 1, nothing is written |
| `void print_binary(uint32_t x, int width)` | width 1..32 | prints nothing |

Rules shared by all functions:

- Bits of `x`, `value` or `word` outside the selected width or field are ignored (or, for `set_field`, left unchanged in `word`).
- Rejection is silent. The caller cannot tell a rejected `get_field` from a legitimate 0; this is accepted and documented.
- Any `int` is allowed as an argument, including negative values and `INT_MAX`. No input causes undefined behavior.
- `print_binary` prints no newline. Groups of four from the right, separated by a space, no padding: `print_binary(0x2C, 8)` prints `0010 1100`; width 6 prints `10 1100`.
- `sign_extend` converts a `uint32_t` above `INT32_MAX` to `int32_t`. In C that conversion is implementation-defined, not undefined; gcc wraps modulo 2^32. The README states this assumption.
- `format_binary` is not required by the handout. It exists so the string logic can be tested, because the test program cannot read its own stdout.

## 5. Module design

### bits.h

- Include guard `BITS_H`, `#include <stdint.h>`.
- `enum { BIN_BUF_SIZE = 32 + 7 + 1 };` with a comment: 32 digits, 7 separating spaces, 1 terminator.
- Prototypes for the five functions, with the Section 4 contract as comments above each.

### bits.c

Includes: `<stdio.h>`, `<stdbool.h>`, `"bits.h"`.

Private (all `static`):

| Name | Definition |
|------|------------|
| `enum { WORD_BITS = 32 };` | word width |
| `static const uint32_t ALL_ONES = 0xFFFFFFFFu;` | needed because 0xFFFFFFFF does not fit an `enum` (int) |
| `static bool width_ok(int width)` | `width >= 1 && width <= WORD_BITS` |
| `static bool field_ok(int pos, int width)` | `width_ok(width) && pos >= 0 && pos <= WORD_BITS - 1 && pos + width <= WORD_BITS`. Order matters: by the time the sum is evaluated, both operands are bounded (at most 31 + 32), so it cannot overflow. |
| `static uint32_t low_mask(int width)` | precondition 1..32. `(width == WORD_BITS) ? ALL_ONES : (1u << width) - 1u`. The branch exists because `1u << 32` is undefined behavior. |

Public functions:

1. `get_field`: if `!field_ok`, return 0. Otherwise `(word >> pos) & low_mask(width)`.
2. `set_field`: if `!field_ok`, return `word`. Otherwise `m = low_mask(width)`, result `(word & ~(m << pos)) | ((value & m) << pos)`.
3. `sign_extend`: if `!width_ok`, return 0. Otherwise `m = low_mask(width)`, `v = value & m`; if `v & (1u << (width - 1))` then `v |= ~m`; return `v`. The return converts it to `int32_t`; an explicit `(int32_t)` cast would compile to the same code, so it is left out.
4. `format_binary`:
   - if `size < 1`, return without writing;
   - `buf[0] = '\0'`;
   - if `!width_ok(width)`, return;
   - `needed = width + (width - 1) / 4 + 1`; if `size < needed`, return;
   - for `i` from `width - 1` down to 0: write `'0' + ((x >> i) & 1u)`; then, if `i > 0 && i % 4 == 0`, write `' '`;
   - write `'\0'`.
   - Checks: width 8 needs 10 bytes, width 6 needs 8, width 32 needs 40.
5. `print_binary`: `char buf[BIN_BUF_SIZE]; format_binary(buf, BIN_BUF_SIZE, x, width); printf("%s", buf);`

Why each choice is explainable without notes: `1u` and unsigned shifts avoid undefined and implementation-defined behavior from the Week 1 "Single bits" slide; the width-32 branch avoids `1u << 32`; validity checks are ordered per the "check before you add" overflow slide.

## 6. Test plan (tests/test_bits.c)

**Harness.** The slide `CHECK(cond)` macro, extended: a `static int total` and `fails`, `PASS` / `FAIL` lines with the condition text and `__LINE__`, a final line such as `N run, M failed`, and `return fails != 0;`.

**Includes.** `<stdio.h>`, `<stdint.h>`, `<limits.h>`, `<string.h>` (for `strcmp`, accepted suggestion #1), `"bits.h"` (found through `-I.`, accepted suggestion #2).

**Rules.**
- Expected values are worked out on paper first, never copied from the function's own output.
- Unsigned literals get a `u` suffix, so `-Wextra` stays silent.
- Use `INT32_MIN` / `INT32_MAX` rather than spelled-out literals.
- Tests call only the public functions, never the `static` helpers.

**Cases (write each group before implementing its function).**

`get_field`
- Slide example: `get_field(0xB6C5, 4, 4) == 0xC`
- Width 1: `(0x1u, 0, 1) == 1`; `(0x80000000u, 31, 1) == 1`; `(0x7FFFFFFFu, 31, 1) == 0`
- Width 32: `(0xDEADBEEFu, 0, 32) == 0xDEADBEEFu`
- Edges: `(0xF0000000u, 28, 4) == 0xF`; `(0xABCD1234u, 16, 16) == 0xABCD`
- Reject (word `0xFFFFFFFFu`, expect 0): width 0, 33, -1; pos -1, 32; pos 28 width 5; pos 31 width 2
- Overflow guard: pos and width both `INT_MAX`; `INT_MIN` for each

`set_field`
- Slide example: `set_field(0xB6C5, 4, 4, 3) == 0xB635`
- Value too wide: `set_field(0xFFFFFF0Fu, 4, 4, 0x1F5) == 0xFFFFFF5F`
- Width 1, pos 31: `(0, 31, 1, 1) == 0x80000000u`; `(0xFFFFFFFFu, 31, 1, 0) == 0x7FFFFFFFu`
- Width 1, value 2: `(0xFFFFFFFFu, 0, 1, 2) == 0xFFFFFFFEu`
- Width 32: `(0x12345678u, 0, 32, 0xDEADBEEFu) == 0xDEADBEEFu`
- Round trip: `get_field(set_field(0, 12, 8, 0xAB), 12, 8) == 0xAB`, and the set value is `0x000AB000`
- Reject (word `0x12345678u`, expect unchanged): width 0, 33; pos -1, 32; pos 28 width 5; both `INT_MAX`

`sign_extend`
- Handout: `(0xF8, 8) == -8`
- Positive: `(0x7F, 8) == 127`
- Most negative: `(0x80, 8) == -128`; `(0x80000000u, 32) == INT32_MIN`
- Others: `(0xFFFFFFFFu, 32) == -1`; `(0x7FFFFFFFu, 32) == INT32_MAX`
- Width 1: `(1, 1) == -1`; `(0, 1) == 0`
- Value too wide: `(0x1F8, 8) == -8`; `(0x100, 8) == 0`
- Reject (value `0xFF`, expect 0): width 0, 33, -1

`format_binary` (compare with `strcmp(buf, expected) == 0`)
- Handout: `(0x2C, 8)` gives `"0010 1100"`
- Odd width: `(0x2C, 6)` gives `"10 1100"`; width 5 of `0x16` gives `"1 0110"`
- Width 1: `(1, 1)` gives `"1"`; `(2, 1)` gives `"0"`
- Width 32: `0x80000001u` gives `"1000 0000 0000 0000 0000 0000 0000 0001"`; `0xFFFFFFFFu` gives eight `1111` groups
- Bits above width ignored: `(0x12C, 8)` gives `"0010 1100"`
- Reject: width 0, 33, -1 give `""`
- Buffer sizes for width 8 (needs 10): size 10 works and leaves byte 10 as the sentinel (catches an off-by-one overrun); size 9 gives `""`; size 1 gives `""`; size 0 leaves a sentinel byte untouched. Use sentinel-filled buffers.

`print_binary`
- One labeled demo line printed by the test program, for example `print_binary(0x2C, 8) -> 0010 1100`. Not counted as PASS/FAIL; the README says so.

## 7. Build and repo

Makefile shape (recipes start with a TAB):

```make
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
```

- The `-o $@` in the compile rule differs from the slide's recipe on purpose: without it, `gcc -c tests/test_bits.c` writes `test_bits.o` into the current directory, not `tests/`. Both `-o` and `$@` are in the slides.
- `-I.` adds `hw01/` to the include search path, so `tests/test_bits.c` can include `"bits.h"`. Compiling the test by hand then needs the flag too: `gcc -std=c11 -Wall -Wextra -I. -c tests/test_bits.c -o tests/test_bits.o`.
- A nonzero exit from `./tests/test_bits` makes `make test` fail, which satisfies the exit-code requirement.
- `.gitignore`: `*.o` and `tests/test_bits` (the slide says to keep objects and executables out of the repo).

## 8. Implementation sequence

**Commits (F3).** You make every commit and write every message. The steps below mark **checkpoints** where a commit is natural. At a checkpoint, any AI assistant stops, says what changed, and leaves the working tree as it is: it does not stage, commit or push. A checkpoint is reached when the chunk builds with no warnings and, where tests exist, `make test` passes. That gives you a clean point to look over the diff, poke at the code, and decide on the commit message. Commits are yours either way (the syllabus: an agent's commits are your commits), so this keeps the history in your own words.

**Setup**
0. On paper: re-derive the boundary cases (width 32, pos 31 width 1, `INT32_MIN`). Check them against the studio notes.
1. Write the Part 2 section of `hw01/README.md` (Section 4) and `bits.h` with contract comments. **Checkpoint.**
2. Add the Makefile and an initial `tests/test_bits.c` with the harness. Confirm `make`, `make test`, `make clean` run with no warnings. **Checkpoint.**

**Per-function loop** (in this order: `low_mask` / `width_ok` / `field_ok` with `get_field`, then `set_field`, then `sign_extend`, then `format_binary`, then `print_binary` plus its demo line)
3. Write the function's tests from Section 6, see them fail, implement, run `make test`. **Checkpoint** after each function. A throwaway stub that returns a wrong value will warn about unused parameters under `-Wextra`; silence it with `(void)param;` and delete that when the function is real.

**Finish**
4. Finish `hw01/README.md`: build and test commands, valid ranges, boundary behavior, out-of-range behavior, the `sign_extend` conversion note, why `format_binary` exists, what the demo line is.
5. Write `hw01/AI_USAGE.md`: tools used, what for (this planning session, plus any code help), and one real thing a model got wrong and how it was found. Only write that item if it actually happened.
6. Clean-clone check: clone the repo into a temp folder, run `make`, `make test`, `make clean`, and confirm there are no warnings and `git status` is clean.
7. Commit what remains and push (you do both).
8. Shine rehearsal (Section 9).

## 9. Shine rehearsal (no tools, no notes)

Be able to say these out loud:
- What `get_field` returns at width 32, and why `1u << 32` is the tricky case.
- Why `1u` and not `1`, and why shifts are done on unsigned values.
- Why the validity check is ordered, and what happens with `INT_MAX` arguments.
- Why `sign_extend`'s return converts to `int32_t`, why that is implementation-defined, and what `sign_extend(0x80000000u, 32)` returns.
- Why `format_binary` exists and why an empty string means failure.
- Walk through `print_binary(0x2C, 6)` and its output.
- What `-I.` does, why the test needs it, and the tradeoff against `#include "../bits.h"` (Section 3).

Modify drills (change one thing by hand, then say what breaks):
- Make width 0 valid and return 0.
- Change `set_field` to reject by returning 0.
- Switch the reject policy to truncation.
- Add `clear_field(word, pos, width)`.

## 10. Risks and open notes

- `sign_extend` relies on gcc's wraparound for the final conversion. Documented, and the `INT32_MIN` test covers it.
- `status.h` must not appear in the Makefile before it exists.
- `format_binary` makes the library five functions, not the handout's four. The README must explain it; the four required functions still behave as specified.
- `strcmp` and `-I.` are the only things outside the slides. `strcmp` appears only in tests; `-I.` appears only in the Makefile.
- With `-I.`, `tests/test_bits.c` compiles only through the Makefile (or with the flag added by hand). The handout says Shine builds and tests `hw01/`, which implies `make test`; if something compiled the test file directly, it would fail on the include.
- The handout does not say how Shine captures test output. This design avoids depending on it: PASS/FAIL and the summary go to stdout and the exit code is correct.
