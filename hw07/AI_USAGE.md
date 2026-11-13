# AI Usage

## Tools

| Tool | Model | Used for |
|------|-------|----------|
| Claude Code | Opus 5.5 | Design interview, PRD, writing `istack.h`, `istack.c`, `balance.c`, the tests, the regression cases, the Makefile and `regress.sh`, and drafts of this file and the README |

## Description

I scaffolded `hw07/` with empty files. Before any code, I had Claude Code read
the assignment and the week 7 slides and quiz me on what the assignment leaves
open: where the binaries go (in place, matching the sheet's `./balance`, after
first choosing `build/` as in hw06), whether a plain `make` uses the sanitizers
(no, so that `make memcheck` really rebuilds), how `expected/` records the exit
status (one file, the output then `exit N`), which stream the result line goes
to (stdout), what `balance` does when it fails itself (stderr, exit 2), what a
column counts (bytes), and what `istack_to_array` does with a `NULL` `n`
(returns `NULL`). I made each call. Claude wrote the decisions up in
`docs/PRD.md`, with a section of likely Shine questions.

Claude then wrote the code in pieces and stopped after each one for my review:
the stack, then `balance`, then the Makefile, tests and regression cases, then
the README. It wrote each expected regression output by hand before running
`balance` on it, and checked that the tests can fail: a wrong expected file
fails `regress.sh`, and a copy of `balance` with its `istack_destroy` removed
fails `make memcheck`.

In review, I had Claude walk me through the overflow check in `istack_push`,
then add parentheses to its division and a comment on why it is there, and add
a comment block to every function in `istack.c` that lacked one.

I made and ran the three Part 4 bugs myself. My captures started partway into
the reports, so Claude reproduced bugs (1) and (2) in a throwaway copy, with
the same edits, to get the first lines, and drafted the sentence for each. The
reports showed something I hadn't expected: destroying a stack twice is
reported as a use after free, not a double free, because the second destroy
reads `s->data` out of the already freed struct before it can call `free`.

## What it got wrong

**Wrong cross-references in the PRD.** Its first draft pointed to section 8 for
the decisions beyond the spec, which are in section 9. It caught and fixed this
itself before I reviewed the PRD.
