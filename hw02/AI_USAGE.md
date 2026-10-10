# AI usage, HW2

## Tools

- Claude Code (Claude Opus 5.5), in VS Code.

## What I used it for

- Reading the assignment and `readings.c` and writing a requirements and plan document, `docs/REQUIREMENTS.md`: what to refactor, the module interfaces, and the order of work.
- Implementing the refactor one step at a time, in the order the plan gives: the stats module, the histogram module, the reading module, moving the globals into `main`, the Makefile header dependencies, and the unit tests. After each step I reviewed the code, asked questions about it, and committed it myself.
- Explaining choices I had to make: returning two results in a struct or through a pointer, and whether to keep `-fsanitize=address` in the build.
- Drafting this file and `README.md`.
- I chose the test style myself: `tests/test_stats.c` uses the same `CHECK` macro and `static` counters as my hw01 tests. The model first wrote a different style without looking at hw01.

## What it got wrong

### Requirements it added that the assignment does not have

The first version of `docs/REQUIREMENTS.md` listed two requirements more strictly than the PDF states them. It said "no global variables" also bans `static` file-scope variables, and it gave `float stats_max(const float a[], int n)` as the required signature. The PDF only says "No global variables", and it introduces that signature with "for example".

How I found it: I asked the model where the Rules came from and whether it had made them up. Going back to the PDF, it found both overstatements, corrected them, and tagged every requirement with the part of the PDF it comes from, so the doc now shows which lines are the assignment and which are the model's own analysis.

### A search and replace that broke the build

When moving the globals into `main` (step 6), the model replaced every `count` with `c.count` across `main.c` in one text substitution. The one place that already said `c.count` became `c.c.count`, and the file no longer compiled.

How it was found: `make` failed with `'struct reading_counts' has no member named 'c'` at that line. The model fixed the line, rebuilt, and reran `regress.sh` (4 of 4 pass). The lesson is that a blind search and replace is not a refactoring tool; the compiler caught this one, but a substitution that still compiles would only be caught by the tests.
