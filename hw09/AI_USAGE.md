# AI Usage

## Tools

| Tool | Model | Used for |
|------|-------|----------|
| Claude Code | Opus 5.5 | Planning the work, writing the comments in `ht.h`, writing `ht.c`, the tests and the Makefile, and drafts of the README and this file. |

## Description

I set up `hw09/` with empty files and wrote the declarations in `ht.h`, as
the assignment gives them. I gave Claude the assignment and the week 9
lecture slides and asked it for a plan, which I then changed before any code
was written.

Claude's plan was to write one function at a time, each with its tests, and
stop after each one so I could review it. It started with `ht_create` and
`ht_destroy`, so every test could clean up after itself and `make memcheck`
would only report real problems. It also pointed out that `ht.h` included
`<stdlib.h>` when it only needs `<stddef.h>`, and it wrote the comment on
each function that the assignment asks for.

I made these decisions:

- **Work from the slides,** not from Karumanchi's code.
- **Keep a running count of different keys** in the table, instead of
  counting them each time `ht_size` is called. In hw08 I chose to count each
  time, so the size could never get out of sync with the tree. Here there's
  no delete, so only one line of code ever changes the count, and recounting
  would mean walking every bucket for no real benefit.
- **Write the chain search inside each function** that needs it (`ht_add`
  and `ht_count`), instead of sharing one helper function between them.
- **Test only what the assignment asks for.**

Claude copied the Makefile and the test setup from hw08.

## What it got wrong

Nothing that I caught during review.
