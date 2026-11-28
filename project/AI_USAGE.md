# AI Usage

## Tools

| Tool | Model | Used for |
|------|-------|----------|
| Claude Code | Opus 5.5 | listing what was left against the assignment and `SPEC.md`, writing `report.h`, `report.c`, their tests, `--report` in `main.c`, scenario 6, the new sections of `SPEC.md`, and drafts of the README changes and this file |

## Description

I asked Claude to compare the project with the assignment and `SPEC.md` and
list what was left. The list was: the report and its tests, `--report` in
`main.c`, scenario 6, four sections of `SPEC.md`, this file, two broken links
in the README, and a question about the layout of `project/`.

Homework 9 says that projects with S4 as their second structure should turn
`aux.c` into a hash table. I asked Claude to review `aux.c` with that in mind.
It pointed out that my second structure is S1, a stack, so the note didn't
apply, and that `aux.c` already met everything the assignment asks of the
stack.

Claude wrote the report one function or declaration at a time and stopped
after each one for my review. Its first attempt put a struct and three
functions into `report.c` at once, so I had it back them out and explain the plan before
writing any code. After that, I set two rules for the rest of the work:

- **Top-down.** A function is written before the helpers it calls, so that
  nothing is written before something uses it.
- **The layout of my `bst` repository.** Helper prototypes after the types,
  then the public functions, then the helper definitions, each with its
  comment. The test files keep their existing layout until I reorganize them.

I made these decisions about the report:

- **Get time order by walking the store and sorting.** The store is ordered
  by temperature, and the report can only use `store.h`. The other option was
  a time-order walk in the store itself, which would have meant a second
  ordering inside it only for the report.
- **Do the arithmetic in whole tenths of a degree,** with a running sum, and
  round halves away from zero. Floats would have left it to chance whether an
  average of exactly 15.05 printed as 15.0 or 15.1.
- **Print lines like `2026-10-06 08:00:19 avg=15.1`,** in the same format as
  the trace.
- **Keep the arithmetic and printing inside `print_averages`.** Claude had
  planned three more helpers for converting, rounding, and printing. I asked
  whether they were premature, and with each one only a line or two long,
  they were.

For `--report`, I agreed to accept only the two forms the assignment shows,
to switch the trace off with a flag passed down to `run_tick`, and to print
the report in `main` after the replay. Claude designed scenario 6 so that
three of its averages land exactly on a half, one of them below zero. It
checked the expected output with a separate calculation in Python, and
checked that the unit tests fail when the sort is removed from `report.c`.

Claude drafted the new sections of `SPEC.md`: the report, where the events
are made, the drawing, and the paragraph on the given states. I rewrote the
opening of that paragraph, and I trimmed the comments in `report.h` and
`report.c` myself.

## What it got wrong

**Too much code at once.** Claude's first attempt at `report.c` wrote a
struct, a callback, a comparison function, and a helper in one step, before
I had agreed to the plan. I had it back them all out.

**Functions written before anything used them.** Its second attempt wrote
the walk callback and the comparison function first, bottom-up, so for a
while neither had a caller. I had it back those out and start from
`report_print`.

**A wrong claim about a test.** Claude said the test with exactly 20 readings
couldn't detect a missing sort, because every reading is in its one window.
When it removed the sort to check, that test failed too: the average was the
same, but the line carried the wrong timestamp. Claude corrected itself.
