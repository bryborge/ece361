# HW2 Refactor: Requirements and Plan

Source of truth: `ece361_hw02_fall_2026_r1.pdf` (Homework 2, Revision 0, Oct 6 2026). Due Sun Oct 11, 11:59pm. This document covers Parts 1 to 3 and the hw02 README/AI_USAGE deliverables. Part 4 (`project/`) and the optional bonus are out of scope here.

## Contents

- [1. Goal](#1-goal)
- [2. What is wrong with `readings.c` today](#2-what-is-wrong-with-readingsc-today)
- [3. Module design](#3-module-design)
  - [3.1 Diagram](#31-diagram)
  - [3.2 Proposed interfaces](#32-proposed-interfaces)
  - [3.3 Why these choices](#33-why-these-choices)
- [4. Hard requirements](#4-hard-requirements)
  - [4.1 From the assignment](#41-from-the-assignment)
  - [4.2 Byte-for-byte traps](#42-byte-for-byte-traps)
- [5. How the work is done](#5-how-the-work-is-done)
  - [5.1 Makefile shape](#51-makefile-shape)
  - [5.2 Unit tests](#52-unit-tests)
- [6. Decisions](#6-decisions)
- [7. Shine preparation](#7-shine-preparation)

## 1. Goal

Split the one-file program `readings.c` into four modules plus `main.c` without changing one byte of what it prints to stdout. `regress.sh` is the judge: it must pass after every step of the work, not just at the end.

## 2. What is wrong with `readings.c` today

| # | Problem | Where | Rule it breaks |
|---|---|---|---|
| P1 | Six globals (`ticks`, `temps`, `hums`, `count`, `skipped`, `threshold`) | lines 30 to 35 | Rule 1: no globals |
| P2 | `max_temp(int i)` recurses over the global `temps` and the global `count`, so it cannot be reused for humidity or tested on its own array | lines 38 to 43 | Rule 2: one recursive maximum |
| P3 | The min/max/mean loop is written twice (temperature, then humidity), and temperature max is computed a different way (recursion) than humidity max (loop) | lines 84 to 105 | Rule 2: one function per statistic, called twice |
| P4 | `main` does everything: argument parsing, input parsing, statistics, run detection, histogram | lines 45 to 148 | Table 2 module split |
| P5 | Nothing can be unit tested: there are no functions with inputs and outputs | whole file | Part 3 unit tests |

## 3. Module design

### 3.1 Diagram

```
          +--------+
          |  main  |
          +--------+
         /    |     \
        v     v      v
 +---------+ +-------+ +-----------+
 | reading | | stats | | histogram |
 +---------+ +-------+ +-----------+
```

Only `main` calls other modules. The three leaf modules do not know about each other, so there is no cycle and each can be compiled and tested alone. `stats` does not know what a tick is: it works on plain `float` arrays and returns indices, and `main` turns indices into ticks.

### 3.2 Proposed interfaces

These follow the decisions in section 6. Names and comment wording are still open to edits during review.

**reading.h**

```c
/* Reads "tick temperature humidity" lines from stdin into the three arrays.
 * Skips blank lines and lines whose first non-blank character is '#'.
 * A line that does not hold three numbers is counted as skipped, not stored.
 * Stops after max readings; if another non-blank, non-comment line follows,
 * prints a warning to stderr and stops reading.
 * Takes: arrays with room for max entries, max >= 0.
 * Returns: count, the number of readings stored (0 to max), and skipped,
 * the number of lines that were not blank, not comments, and not readings. */
struct reading_counts {
    int count;
    int skipped;
};
struct reading_counts reading_read(int ticks[], float temps[], float hums[], int max);
```

`LINE_LEN` and the whitespace/comment check move into `reading.c` as private details (`#define` and a `static` helper such as `is_blank_or_comment`).

**stats.h**

```c
/* Smallest value in a[0..n-1]. Assumes n >= 1. */
float stats_min(const float a[], int n);

/* Largest value in a[0..n-1], computed recursively: one stack frame per
 * element, so very large n overflows the stack. Assumes n >= 1. */
float stats_max(const float a[], int n);

/* Mean of a[0..n-1], summed in float from index 0 upward. Assumes n >= 1. */
float stats_mean(const float a[], int n);

/* Longest run of consecutive values strictly greater than threshold.
 * A value equal to threshold ends a run. On a tie, the first run wins.
 * Returns: length, the run length (0 if no value is above threshold), and
 * start, the index of the run's first value (-1 if length is 0).
 * Assumes n >= 0. */
struct stats_run {
    int length;
    int start;
};
struct stats_run stats_longest_run_above(const float a[], int n, float threshold);
```

**histogram.h**

```c
/* Prints the histogram of a[0..n-1] to stdout: "histogram:" then one line per
 * non-empty 5 C bin from 0 to 100 C. Values below 0 go to the first bin,
 * values at or above 100 go to the last. Assumes n >= 0. */
void histogram_print(const float a[], int n);
```

`NUM_BINS`, `BIN_WIDTH`, and the bin-index calculation become private to `histogram.c` (a `static int bin_index(float t)` helper).

**main.c** owns `MAX_READINGS`, the three arrays, `count`, `skipped`, and `threshold` as locals. It parses `argv`, calls `reading_read`, prints the counts, returns early on zero readings, then calls `stats_*` for temperature and humidity, `stats_longest_run_above`, and `histogram_print`, printing with the exact format strings from `readings.c`.

### 3.3 Why these choices

- **Plain arrays plus a length, not a struct.** Rule 1 says the readings live in `main` and are passed in. Three parallel arrays match the starter and need no new types. `stats` taking `const float a[]` is what lets one function serve both temperature and humidity (fixes P2, P3).
- **A small struct for the second result.** `reading_read` produces a count and a skipped count; `stats_longest_run_above` produces a length and a start. Each returns both in a two-field struct, so no pointers are needed before HW3 teaches them, and the call site reads `r.length`, not a positional argument. The struct names carry the module prefix (`reading_counts`, `stats_run`) like the functions do. The assignment's "Looking ahead" says HW3 is about this problem, so switching to out-pointers later means changing two signatures and their callers (see section 6).
- **`stats` returns an index, not ticks.** Keeps `stats` free of input-format knowledge and keeps the module graph a tree. `main` computes `ticks[r.start]` and `ticks[r.start + r.length - 1]`.
- **Warning stays in `reading.c`.** It is about input, it goes to stderr (not checked by `regress.sh`), and moving it to `main` would need another result.
- **Read from stdin directly.** No `FILE *` parameter: nothing in this homework reads from anywhere else.
- **`n >= 1` is a precondition, not a check.** `main` already returns before any statistic when there are no readings. The header says so (Rule 3); the functions do not repeat the check.

## 4. Hard requirements

### 4.1 From the assignment

Each item restates the PDF; the source is in brackets. "Rule n" is the numbered list on page 2 under "Your design has to meet these rules".

- R1. No global variables. The readings live in `main` and are passed to the functions that need them. [Rule 1]
- R2. One recursive maximum in `stats.c` that takes an array and its length, used for both temperature and humidity; `max_temp` is replaced. The PDF gives `float stats_max(const float a[], int n)` as an example, not a required signature. [Rule 2]
- R3. Every header has an include guard, declares only what other files call, and has a comment per function stating what it takes, returns, and assumes. [Rule 3]
- R4. Every function used in only one `.c` file is `static`. Public functions carry their module prefix. [Rule 4]
- R5. Output byte for byte identical to `expected/` for every input. [Rule 5; checked by `sh regress.sh`, Part 3]
- R6. `make` builds with `gcc -std=c11 -Wall -Wextra` and no warnings, one `.o` per `.c`. [Part 3, bullet 1]
- R7. Each `.o` depends on its `.c` and every header it includes. Touching `stats.h` rebuilds every file that includes it; touching `histogram.c` rebuilds only `histogram.o` and the program. [Part 3, bullet 1]
- R8. `make clean` removes everything the build produced. [Part 3, bullet 1]
- R9. `tests/test_stats.c` prints PASS or FAIL per test and a summary, and exits nonzero if any test fails. At least: one value; the maximum at the start and at the end; a value exactly equal to the threshold (it ends a run); the recursive maximum on 10,000 values. [Part 3, bullet 2]
- R10. `make test` builds and runs the unit tests, then runs `sh regress.sh`; any difference is a failure. [Part 3, bullet 3]
- R11. `hw02/README.md` (build and test instructions, module diagram) and `hw02/AI_USAGE.md` (tools, uses, one thing a model got wrong and how it was found). [README and AI usage, page 3]

### 4.2 Byte-for-byte traps

Not in the PDF. Derived by reading `readings.c` against `data/` and `expected/`: these are the places a reasonable-looking rewrite would change the output. Each must be preserved exactly.

- **Float arithmetic.** Sums are `float`, accumulated from index 0 upward, then divided by the `int` count. Summing in `double`, summing backwards, or using a recursive sum can change the last printed digit of the mean.
- **Format strings.** Copy every `printf` format from `readings.c` verbatim, including the two spaces after `skipped:`, `%%RH`, and the field widths `%6.1f`, `%6.2f`, `%5.1f`.
- **Run rule.** `>` not `>=` (30.0 with threshold 30 ends a run, see `edge.txt`), and `run > best` so the first of two equal runs wins.
- **Histogram clamping.** `(int)(t / 5.0f)` truncates toward zero, so -3.5 lands in bin 0 anyway; the explicit `t < 0` check catches values below -5. 100.0 computes bin 20 and is clamped to 19. Empty bins are not printed.
- **Input loop.** Leading spaces and tabs are skipped before the blank/comment check; a line with only a tab is blank. The capacity check happens before parsing, so the warning fires only when a 1001st non-blank, non-comment line exists, even if that line is bad.
- **Threshold parsing.** `strtof` with the `*end != '\0'` check, and the two error messages and usage line on stderr, unchanged.
- **Empty input.** Print `readings:`, `skipped:`, then `no readings, no summary`, and return 0 before any statistic is called.

## 5. How the work is done

Rule for every step: `make && sh regress.sh` passes before moving on. If it fails, the step is wrong, not the expected file. `expected/`, `data/`, and `regress.sh` are not edited.

Review and version control:

- The student runs every git command (add, commit, push). The implementer runs none, not even read-only ones.
- When a step (or a small group of steps) is done and passing, the implementer stops and notifies the student that it is ready for review, naming the files changed and the regression result.
- The student reads, edits, and asks questions as they like. Work on the next step starts only after the student says to continue; the student commits on their own schedule.
- Edits the student makes during review are the new baseline. The implementer builds on them and does not revert them.

Current state: the existing `Makefile` builds `build/readings` from `readings.c` with `-fsanitize=address`, and `regress.sh` runs `build/readings`. Baseline passes (4 of 4).

1. **stats module, max first.** Create `stats.h`/`stats.c` with `stats_max`. Add `stats.c` to the Makefile. In `readings.c`, replace `max_temp(0)` with `stats_max(temps, count)` and delete `max_temp`. Regress.
2. **stats: min, mean, both series.** Add `stats_min` and `stats_mean`. Replace both loops in `readings.c` with three calls each. Regress (this is where the float trap bites).
3. **stats: longest run.** Add `stats_longest_run_above`; replace the run loop. Regress with special attention to `edge_25`.
4. **histogram module.** Create `histogram.h`/`histogram.c`, move the bin constants and loop, replace with `histogram_print(temps, count)`. Regress.
5. **reading module.** Create `reading.h`/`reading.c`, move the input loop and `LINE_LEN`. Regress.
6. **Remove globals and rename.** Move the remaining globals into `main` as locals, rename `readings.c` to `main.c`, update the Makefile. Regress. Confirm with `grep` that no file-scope variables remain.
7. **Makefile header dependencies.** Write the per-object dependency lines (R7). Verify by `touch stats.h; make` (rebuilds `main.o`, `stats.o`, link) and `touch histogram.c; make` (rebuilds `histogram.o`, link only).
8. **Unit tests.** Write `tests/test_stats.c` linking only `build/stats.o`. Add `test` target that builds and runs it, then `sh regress.sh`. Confirm a deliberately broken assertion makes `make test` exit nonzero, then revert it.
9. **Docs.** `README.md` (build, test, diagram from 3.1) and `AI_USAGE.md`.
10. **Final check.** `make clean && make` (no warnings), `make test` (all pass), `make clean` leaves only sources.
11. **Ask about ASan.** Ask the student specifically whether to remove `-fsanitize=address` from `CFLAGS` before submitting (decision 2 in section 6). Do not change it without an answer.

### 5.1 Makefile shape

Explicit dependency lines rather than `gcc -MMD` auto-generation, because Shine asks "you change `stats.h`, which files does make rebuild?" and the answer should be readable in the Makefile:

```make
build/main.o:       main.c reading.h stats.h histogram.h
build/reading.o:    reading.c reading.h
build/stats.o:      stats.c stats.h
build/histogram.o:  histogram.c histogram.h
build/test_stats.o: tests/test_stats.c stats.h
```

The existing pattern rule `build/%.o: %.c` keeps supplying the recipe; these lines only add prerequisites. The test object needs its own rule (source is under `tests/`) and `-I.` to find `stats.h`.

### 5.2 Unit tests

A small `check(name, condition)` helper that prints `PASS  name` or `FAIL  name` (matching `regress.sh` style) and counts failures; `main` prints `N passed, M failed` and returns 1 if any failed.

| Test | Input | Expect |
|---|---|---|
| one value | `{42.0}` | min, max, mean all 42.0 |
| max at start | `{9, 1, 2, 3}` | `stats_max` = 9 |
| max at end | `{1, 2, 3, 9}` | `stats_max` = 9 |
| equal to threshold ends run | `{31, 31, 30, 31}`, threshold 30 | length 2, start 0 |
| first of equal runs wins | `{31, 0, 31}`, threshold 30 | length 1, start 0 |
| never above | `{1, 2}`, threshold 30 | length 0, start -1 |
| recursive max, 10,000 values | `a[i] = i`, then one large value placed mid-array | that value |

The 10,000-value array is a local `static float` inside the test function or a local array; 10,000 floats (40 KB) fits on the stack, and 10,000 recursion frames are well within the default 8 MB stack, including under ASan's larger frames.

## 6. Decisions

1. **Second result: struct, not pointer.** `reading_read` returns `struct reading_counts`, `stats_longest_run_above` returns `struct stats_run`. Defensible with what has been taught so far, and small to change to out-pointers if HW3 calls for it. Arrays passed to `reading_read` are still filled through array decay (a pointer underneath); be ready to say so on Shine.
2. **ASan: keep during implementation, decide at the end.** `-fsanitize=address` stays in `CFLAGS` while refactoring because it catches out-of-bounds reads that can still print correct output. When implementation is complete, the student is asked specifically whether to remove it (step 11). Risk to weigh then: the grader's machine may lack the ASan runtime, which would fail the build. It is never used for the bonus `depth.c` measurements, since ASan changes frame sizes and code generation.
3. **Program location: `build/readings`.** The assignment says `make` builds `readings`; the Makefile and `regress.sh` already use `build/readings`. Keep `build/`, and `README.md` says so so the Shine grader runs the right path.
4. **`readings.c` goes away.** It is not in the deliverable list and becomes `main.c` in step 6. Git history keeps the original.

## 7. Shine preparation

Each of these should be answerable from the code alone:

- Draw the call stack of `stats_max` on three values; what happens on a million (stack overflow, one frame per element).
- Why `bin_index` and `is_blank_or_comment` are `static`, and what a name collision across files would do at link time without it.
- Which objects rebuild after touching `stats.h` (section 5.1).
- How `reading_read` fills `main`'s arrays when C passes arguments by value (an array argument is the address of its first element), and why the two counts come back in a struct instead.
- Which project module resembles which hw02 module (`report` reads an array and computes statistics like `stats`; `sensor` produces readings like `reading`, from registers instead of stdin).
