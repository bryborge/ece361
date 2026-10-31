# AI Usage

## Tools

| Tool | Model | Used for |
|------|-------|----------|
| Claude Code | Opus 5.5 | Design interview, PRD and pseudocode, implementing `lock.c`, `main.c` and the tests, Makefile, README |

## Description

I scaffolded `hw05/` myself, with `lock.h` holding the interface from the
assignment and empty files for the rest. Before any code, I had Claude Code
read the assignment and quiz me on every decision it leaves open: how
`lock_event` knows the next digit (a `next_digit` array indexed by state),
what a digit means while the door is open, how bad arguments are handled
(asserts, so `-1` only ever means "not an event"), whether `C` and `T` count,
what `main` does with characters that are not events, the two table entries
the description of the lock does not settle, and how the tests check the
table and the key sequences. I made each call. Claude wrote the decisions up
in `docs/PRD.md` and the algorithms as pseudocode in `docs/algorithms.md`.

At first I planned to write the code myself from that pseudocode, but I
changed my mind and had Claude write it, as in the other homeworks, while I
reviewed. It worked test first, one function at a time: tests that fail,
then the code that makes them pass, then a cleanup, stopping after each
function so I could read it. For the tests that guard a specific design
choice (the order of the checks in `lock_event`, the bounds checks, the
sequences), it ran them against deliberately broken copies of `lock.c` to
show that they catch the bug. I managed git, and asked it to make two of the
commits.

Several choices came out of my review. I asked why `event_name` and
`action_name` existed (the assignment names them only in a comment on
`state_name`), whether `stddef.h` was needed (only for `NULL`), and why the
bounds checks cast to `unsigned` (see below). I had the declarations moved
above the functions, kept the explicit `'\0'` for `OPEN` in `next_digit`,
asked for a braced `if`/`else` where it had two one-liners, and rewrote or
shortened several comments in `lock.h` and `lock.c` myself.

## What it got wrong

**A reason that was not true.** It recommended writing the bounds checks as
`(unsigned)s < N_STATES`, saying that `s < 0` on an enum would make gcc warn
under `-Wextra` and break the no-warnings rule. When I asked whether the cast
was needed, it tested the claim and found that gcc 14.2 gives no such warning
for an enum. On gcc the cast also did nothing, since gcc already makes these
enums unsigned, and it had no comment saying why it was there. The checks are
now `s >= 0 && s < N_STATES`, and the PRD is corrected.

**Casts that did nothing.** `main.c` had `(char)c` and two `(event_t)e` casts.
When I asked about the first, it admitted it had added them out of habit
without checking they were needed. It removed all three and confirmed the
build had no warnings, the output was the same, and the compiled code was
identical.

**A broken `.gitignore`.** While setting up a `compile_flags.txt` so the
editor could find `lock.h`, it appended a line to a `.gitignore` that had no
newline at the end, which merged the new line with `build/` and briefly
stopped `build/` from being ignored. It noticed from `git status` and
rewrote the file.
