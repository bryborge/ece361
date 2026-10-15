# AI Usage

## Tools

| Tool | Model | Used for |
|------|-------|----------|
| Claude Code | Opus 5.5 | Implementing `minmax`, `find_above`, and `reverse`, header comments, unit tests, Makefile, README |

## Description

I wrote `swap_float` and the doc comment stubs in `ptr.c` myself, then had
Claude Code implement the remaining functions one at a time, each with its
tests, stopping after each one so I could read and understand the code before
committing it. Claude also wrote the header comments in `ptr.h`, the test
harness in `tests/test_ptr.c` (reusing the `CHECK` macro from HW2), reworked
the Makefile to build the test binary, and wrote `README.md`. I kept all git
operations to myself.

Two behaviors came out of my review. The spec does not say what `find_above`
does with a `NULL` array; Claude raised it, and I chose to return `NULL`, to
match how `minmax` and `reverse` treat `NULL`. I then asked whether
`find_above` should also return `NULL` when `n` is too small, made that change
myself, and had Claude add the test.

## What it got wrong

**Claimed a negative `n` was already handled.** When it first wrote
`find_above`, Claude said that for `n < 1` "the loop never runs" and the
function falls through to `return NULL`. That is true for `n == 0`, but for a
negative `n` the loop condition computes `a + n`, a pointer before the start of
the array, which is undefined behavior in C even without a dereference. It only
said so once I asked whether `find_above` should check `n`. The guard is now
`a == NULL || n < 1`.
