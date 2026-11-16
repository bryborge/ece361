# AI Usage

## Tools

| Tool | Model | Used for |
|------|-------|----------|
| Claude Code | Opus 5.5 | Design interview, PRD, writing `istack.h`, `istack.c`, `balance.c`, the tests, the regression cases, the Makefile and `regress.sh`, and drafts of this file and the README |
| Claude Code | Fable 5.2 | A cleanup pass after the build: comparing hw07 with my earlier homeworks, moving the build into `build/`, small edits to `balance.c`, `istack.c` and the tests, fixing `make memcheck`, and a draft of the additions to this file |

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

After the build, I had Claude Code, now on Fable 5.2, compare hw07 with hw05
and hw06 and list what didn't follow their patterns. I reviewed the list and
had it apply eight small items: a named `is_opener` in `balance.c`, a clearer
variable name, `int main(void)` as in hw05, comments on each step of the loop,
one statement of the overflow reason in `istack_push` where there had been two,
`bool` flags in the tests, and two formatting fixes. `balance` used to reject
extra arguments with a usage line and exit status 2. With `int main(void)` it
ignores them.

I also reversed two of my earlier calls. The binaries now go in `build/`, as in
hw03 to hw06, and the README lists that as a deviation from the sheet. And
`make memcheck` now builds into `build/memcheck/` and no longer starts from
`make clean`. A plain `make` still doesn't use the sanitizers.

Afterwards I removed the comment blocks above the functions in `istack.c`, and
cut back and rewrote the comments Claude had added to `balance.c`.

## What it got wrong

**Wrong cross-references in the PRD.** Opus's first draft pointed to section 8
for the decisions beyond the spec, which are in section 9. It caught and fixed
this itself before I reviewed the PRD.

**A decision I made without seeing its cost.** During the design interview I
chose to build in place, to match the sheet's `./balance`. Opus did not make
the consequences of that choice clear enough for me, and I reversed it in the
cleanup pass.

**A `make memcheck` that broke `make test`.** Opus's Makefile kept the
sanitized objects and the plain ones in the same place, and counted on
`make clean` to keep them apart. After a `make memcheck`, changing one source
file and running `make test` recompiled only that file without the sanitizers,
and the link failed with undefined `__asan` and `__ubsan` symbols. Fable moved
this Makefile into `build/` without noticing the flaw. It came out only when
`make test` failed for me, and Fable then gave the sanitized build a directory
of its own.

**A command I hadn't asked for.** I asked Fable only to plan the cleanup. While
copying hw07 to a scratch directory to run the tests, it ran `make clean` in my
real `hw07/` and removed my built binaries. It told me so in the same reply,
and `make` restored them.
