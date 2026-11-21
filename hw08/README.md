# A Binary Search Tree, Ported

> A binary search tree from TutorialsPoint, rewritten as a reusable library,
> with a test that found its crash, the fix, and a program that shows how the
> order of insertion changes the tree's height.

### What is this?

For this homework I took a binary search tree from TutorialsPoint
(`BinarySearchTree.c`) and rewrote it as a library other code can use. The
original works for the one example in its `main`, but it can't be reused:
there is only one tree, stored in a global variable; `search` prints to the
screen; nothing is ever freed; and searching an empty tree crashes it.

My version, `bst`, lets a program create as many trees as it needs, and keeps
the tree's structs hidden inside `bst.c`. `treestats` is a small program that
uses it: it reads numbers, adds them to a tree, and prints how many there are
and how tall the tree is.

| File | What it's for |
|------|---------------|
| `BinarySearchTree.c` | The original code, from Canvas, unchanged. |
| `bst.h`, `bst.c` | The tree library. A comment on each function says which function in the original it came from. |
| `treestats.c` | Reads numbers and prints the size and height of the tree they make. |
| `tests/test_bst.c` | Unit tests for the library, including the test that found the crash. |
| `data/` | Sample input for `treestats`: the original's seven keys, and an empty file. |
| `AI_USAGE.md` | How I used AI on this homework. |

### Build and test

Run these from `hw08/`:

```sh
make           # builds build/treestats
make test      # builds if needed, then runs the unit tests
make memcheck  # runs the unit tests again, built with the sanitizers
make clean     # deletes build/
```

The code compiles with `gcc -std=c11 -Wall -Wextra` and no warnings.

`make memcheck` builds with AddressSanitizer and UndefinedBehaviorSanitizer
(`-fsanitize=address,undefined -fno-sanitize-recover=undefined`). With these,
a memory error, a leak, or undefined behavior stops the tests with an error
instead of going unnoticed. The sanitized build goes in its own folder,
`build/memcheck/`, so it never gets mixed up with the normal build.

The unit tests print `PASS` or `FAIL` for each check and a total at the end.
If any check fails, the exit status is nonzero.

### Usage

```sh
build/treestats < data/sample.txt
build/treestats < data/empty.txt
seq 1 1023 | build/treestats
```

`treestats` reads numbers separated by spaces or newlines until the input
ends, adds them to a tree in the order they came, and prints one line:

| Output | Exit status |
|--------|-------------|
| `n 7, height 3` | 0 |
| `empty tree` | 0 |
| `not a number after 4 values` | 1 |

Only whole numbers that fit in an `int` count, so `12abc` and `99999999999`
are rejected. If `treestats` runs out of memory or can't read its input, it
prints one of these to stderr and exits with status 1:

```
treestats: out of memory
treestats: error reading input
```

### Where this differs from the assignment

- **The program is `build/treestats`, not `./treestats`.** Everything the
  build makes goes in `build/`, including the test program,
  `build/tests/test_bst`.

### Part 2: The crash

I first copied `search` into `bst_contains` as closely as I could, bug and
all. The test `check_contains_empty` in `tests/test_bst.c` searches a new,
empty tree, and `make memcheck` stopped with this as the first line of its
report:

```
bst.c:139:19: runtime error: member access within null pointer of type 'const struct node'
```

The loop starts with `while (current->data != key)`, and `current` starts as
the tree's root. In an empty tree the root is NULL, so the very first check
reads through a NULL pointer. The loop does check for NULL, but only after
moving to a child, which is too late for the root.

The original never noticed because its `main` always inserts its seven keys
before it searches, so the root is never NULL when `search` runs.

The fix checks for an empty tree before the loop starts and returns "not
found" with 0 visits. The test now passes.

### Part 3: What order does to a tree

```
$ seq 1 1023 | build/treestats
n 1023, height 1023
$ seq 1 1023 | sort -R | build/treestats
n 1023, height 24
```

The shuffled height is different every run, because `sort -R` shuffles the
numbers differently each time. Other runs gave 21, 25 and 22.

When the keys go in in increasing order, each new key is bigger than all the
others and goes to the far right, so the tree becomes one long chain 1023
nodes tall, while shuffled keys give a tree only about 20 to 25 tall, not far
from the smallest possible height of 10. A search looks at one node per level,
so finding a key can take up to 1023 visits in the first tree but only about
20 to 25 in the second.

### Known gaps

**The out-of-memory code isn't tested.** If `malloc` fails in `bst_create` or
`bst_insert`, the function returns an error and leaves the tree unchanged.
Testing that would require a way to make `malloc` fail on purpose, which I
didn't build. The same goes for `treestats`' out-of-memory and read-error
messages.

**`treestats` has no automated tests.** I checked its outputs by hand,
including inputs like `12abc`, a number too big for an `int`, and the
smallest and largest `int` values.
