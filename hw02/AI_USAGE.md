# AI Usage

## Tools

| Tool | Model | Used for |
|------|-------|----------|
| Claude Code | Opus 5.5 | Reviewing my refactor for accuracy and memory safety, doc comments, unit tests, README |

## Description

I did the refactor by hand: splitting `readings.c` into the `stats`, `histogram`, and `reading` modules, then turning what was left into `main.c`. After each step I asked Claude Code to review it, checking that the doc comments were accurate and consistent across modules and pointing out memory and stack safety problems. I decided which findings to act on, and how. Some I fixed myself, some I had Claude apply, and some I turned down.

Claude wrote the unit tests for all three modules (`tests/test_stats.c`, `tests/test_histogram.c`, `tests/test_reading.c`), filled in the doc comments for `histogram`, `reading`, and `main`, and wrote `README.md`. For each new test, it ran the tests against deliberately broken copies of the code to show that they catch the bug they target. I kept all git operations to myself.

The one behavior change came out of the review: `bin_index` in `histogram.c` turned a temperature into an array index before clamping it, so `nan`, `inf`, or a huge value in the input crashed the program with an out-of-bounds write. Claude laid out the options, and I chose to clamp before converting. Output is unchanged for every input that did not already crash.

`stats_max` stays recursive on purpose, since recursion is this week's topic.

## What it got wrong

**Reviewed the wrong scope.** I asked for a review of my changes, and the first one covered all of `hw02`, including starter code I had not touched yet. I had to say I only wanted the branch's changes.

**Ignored the pattern hw01 already used.** For the histogram tests, Claude captured stdout with `dup2` and a temp file. `hw01/tests/test_bits.c` already does this with `fmemopen`, and I had to point Claude at it before it matched the existing approach.

**Comments that were terse or put facts in the wrong place.** It described tie-breaking in `stats_longest_run_above` as "the earliest wins a tie", which I had to ask about before it gave a clear wording. It also wrote the `{0, -1}` no-run result into the comment on `struct stats_run`, as if it were a property of the type, when it is how one function uses the type. I caught that, and the fact now lives only on the function.

**Reported tests passing against a stale binary.** After I renamed the Makefile rule to `build/main`, nothing could rebuild `build/readings` any more, but `make test` kept running the regression tests against the old binary on disk. Claude reported a reformatted `printf` as passing regression before it noticed. It caught the mismatch later, while writing the usage line in `main.c`, and rebuilt from scratch to confirm.

**Caught its own mistakes, but only after making them.** A test meant to check clamping below 0 C used `-3.0`, which lands in the first bin even without the clamp, so the test could never fail. Claude found this with a broken copy of `histogram.c` and changed it to `-12.0`. It also added a guard to the `fmemopen` capture that turned out to be unneeded on glibc, and introduced a local variable that only shortened a line, which is a no-op. It removed both.
