# AI Usage

## Tools

| Tool | Model | Used for |
|------|-------|----------|
| Claude Code | Opus 5.5 | Scaffold, design interview, PRD, implementing every module and the tests, Makefile, README, a draft of this file |

## Description

Claude scaffolded `hw06/` from the assignment. I had it remove the `.gitkeep` files,
and move the binary to `build/replay`. That second one is my own deviation from the
sheet's `./replay`, and the README says so.

Before any code, Claude quizzed me on every decision the assignment leaves open: how
`scenario_load` gets its input (a `FILE *`, so that it only ever returns 0 or a line
number), what happens before the first line (the first tick must be 0), what counts
as a wrong line, and how `replay` fails. I made each call, and Claude wrote them up
in `docs/PRD.md`.

I had Claude write the implementation while I reviewed. It worked test first, one
module at a time: failing tests, then the code, then a cleanup, stopping after each
one so I could read it. It then ran the tests against deliberately broken copies of
each module, to show they catch the bug. It wrote the expected traces by hand, before
`main.c` existed, so that they could not just be copied from its output. I told it
when to stage and commit.

## What it got wrong

**A claim it had not tested.** It told me UBSan would catch a `1 << 31` bug in
`panel_led`, and wrote that into the PRD and a test comment. The mutation run showed
otherwise: UBSan prints a report, and the program still exits 0, so the bug passes
`make test`. I added `-fno-sanitize-recover=undefined`, and that bug now fails.

**A test that proved less than it looked like.** With the line buffer one byte too
short, the longest CRLF line gets split in two, and every later line number is off by
one. The test only checked that the load succeeded, so it passed. A mutation run
caught this, and Claude added checks on the line number.

**More code than the problem needed.** Its fix for `-0.0` was to add `0.0f` after
`round`. Slide 8's `lround` returns an integer, which has no negative zero, so the
extra line was not needed.

**A claim it had not checked.** It said all three of my project scenarios start at
tick 0, when it had only opened one. It checked the other two before I relied on it,
and the claim held.
