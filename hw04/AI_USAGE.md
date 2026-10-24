# AI Usage

## Tools

| Tool | Model | Used for |
|------|-------|----------|
| Claude Code | Opus 5.5 | Planning, implementing `tally.c` and `events.c`, header comments, unit tests, Makefile, README |

## Description

I scaffolded `hw04/` myself, with `tally.h` holding the interface from the
assignment and empty files for the rest. Before any code, I had Claude Code
read the lecture slides and the assignment and quiz me on every decision the
spec leaves open: the struct layout, how names are searched and copied, what
`tally_destroy` and `tally_print` do with edge cases, how `events.c` treats
comments and overlong lines, and how the tests and Makefile are organized. I
made each call, and we settled on a plan of small units, one function each.

Claude then implemented the plan one unit at a time, stopping after each so I
could read and understand it before committing it: the header comments, the
pool and `tally_create`, `tally_destroy`, `tally_add` with its `find_pos`
helper, `tally_count`, `tally_size`, `tally_print`, `events.c` with
`data/door.txt`, and the documentation. For tests that guard a specific behavior
(the backward shift, refusing rather than cutting long names, `tally_create`
emptying a reused tally), it ran them against deliberately broken copies of
`tally.c` to show that they catch the bug. I kept all git operations to
myself.

Several choices came out of my review. I had the binary built into `build/`
like the other homeworks, pulled the repeated pool setup in the tests into
`create_all` and `destroy_all`, renamed a helper to `entry_matches`, chose a
linear search over a binary search for 16 entries, and reworded several
comments in `tally.h` and `tally.c` myself.

## What it got wrong

**A test that could not fail for the bug it was meant to catch.** The first
version of the counting test inserted a new name in front of only one entry.
With one entry moving, shifting from the front and shifting from the end do
the same thing, so the test would pass even if the loop in `tally_add` ran in
the wrong direction. Claude found this by running the test against a copy of
`tally.c` with the loop reversed, and added a third name so that two entries
move. Its first long-name test had the same kind of gap: the 16-character name
was all `B`s, so a `tally_add` that cut names down to 15 characters instead of
refusing them would still pass. It changed the name to the 15-character name
plus one more letter, which makes a cut-down name show up as an extra count.

**Added a check I did not need.** In `events.c` it guarded the input buffer
size with a `_Static_assert`, a C11 feature we have not covered in class. With
the sizes in the program it could never fire and generated no code. When I
asked what it did and whether it was necessary, it agreed it was not, and the
assumption is now a comment on the buffer.

**Comments that did not read clearly.** Its description of what `find_pos`
returns explained how the loop stops instead of what the index means, and I
had to ask for a rewrite, which I then reworded further.
