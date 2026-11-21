# AI Usage

## Tools

| Tool | Model | Used for |
|------|-------|----------|
| Claude Code | Opus 5.5 | Setting up the Makefile and test file, editing the comments in `bst.h`, writing `bst.c` (except the crash fix), the tests (except the crash test), `treestats.c`, and drafts of the README and this file. It also gave me hints while I found and fixed the crash. |

## Description

I set up `hw08/` with empty files and wrote `bst.h` myself, with a comment on
each function. I didn't write a design document this time, because the
assignment already spells out the functions and how they should behave.

Claude copied the Makefile and test setup from hw07 and suggested an order to
work in: one function at a time, with its tests, and a pause after each so I
could review it. In `bst.h` it added things my comments were missing, like
what happens when a NULL tree is passed in. It also corrected my `bst_insert`
comment, which said each key is compared with the root, when it's actually
compared with every node on the way down. I reworded that comment afterward.

I decided that `bst_size` should count the nodes each time it's called instead
of keeping a running count, so the size can never get out of sync with the
tree. I had Claude write `bst_destroy` early, so every test could clean up
after itself and `make memcheck` would only report real problems. I asked why
`bst_destroy` needs a separate helper function, `free_subtree`, and why it
couldn't just use a few `if` statements to free the nodes. The answer is that
a tree can be any depth, so it takes recursion (or a loop with a stack). I then
had Claude change the helper to check each child before recursing, instead of
checking for NULL at the top. I also renamed the node fields `leftChild` and
`rightChild` to `left` and `right`.

For Part 2, Claude copied `search` over with its bug still in it, and I found
the bug myself. I suspected the loop condition, and Claude asked me questions
until I saw the cause: in an empty tree, the root is NULL, and the loop reads
it before checking. I wrote the test `check_contains_empty`, and
`make memcheck` stopped with an error. Claude then cleaned up my test: it gave
it a better name, added a comment, and started `visits` at 99 instead of
leaving it unset.

I wrote the fix as well. My first try returned "not found" for an empty tree
but forgot to set `*visits`. The test caught that, because `visits` started at
99 and the test expected 0. After fixing it, I combined my new check with the
existing NULL-tree check into one, as Claude suggested, and Claude updated the
function's comment, which still said the code was copied unchanged.

I made the `data/` folder, with the original's seven keys and an empty file,
to try `treestats` and to use in the README. Claude wrote the README's
sentence on why the original never noticed the crash.

## What it got wrong

**A test that didn't test what it said.** Claude's first test for
`bst_destroy(NULL)` was `CHECK(true)`, which always passes, with a comment
claiming `make memcheck` would catch a double free. Passing NULL can't cause a
double free. Claude replaced it on its own before I reviewed it, with a plain
call in `check_null_arguments`, as in hw07.

**Small mistakes it caught itself.** A text edit put a blank line in the wrong
place in `check_contains_empty`, and the first README draft said "three more
runs" when there were only three in total. Claude noticed and fixed both
before I saw them.
