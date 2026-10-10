# Homework 2

## What is this?

`readings` reads sensor readings from standard input, one `tick temperature
humidity` line each, and prints a summary: how many readings there were and
how many lines were skipped, the minimum, maximum, and mean temperature and
humidity, the longest run of readings above a temperature threshold, and a
histogram of the temperatures.

It started as the one-file starter `readings.c`, with global variables and a
`main` that did everything. It is now four modules plus `main.c`, and it
prints exactly what the starter printed, byte for byte.

## Build and test

```sh
make        # builds build/readings
make test   # builds (if needed), runs the unit tests, then the regression test
make clean  # removes build/
```

The program is `build/readings`, not `./readings`: every build product goes
in `build/`, and `regress.sh` runs `build/readings`.

```sh
build/readings < data/normal.txt        # threshold 30.0 C (the default)
build/readings 25 < data/edge.txt       # threshold 25.0 C
```

`make test` runs two things, and stops at the first failure:

- `build/test_stats`, the unit tests for the stats module in
  `tests/test_stats.c`. It prints one `(PASS)` or `(FAIL)` line per check and
  a summary, and exits non-zero if any check failed.
- `sh regress.sh`, which runs `build/readings` on every file in `data/` (and
  `edge.txt` again with threshold 25) and compares the output with
  `expected/` using `diff`. Any difference is a failure.

## Modules

```
          +--------+
          |  main  |
          +--------+
         /    |     \
        v     v      v
 +---------+ +-------+ +-----------+
 | reading | | stats | | histogram |
 +---------+ +-------+ +-----------+
```

An arrow means "calls". Only `main` calls other modules; `reading`, `stats`,
and `histogram` do not know about each other.

| Module | Files | Public interface | Responsible for |
|--------|-------|------------------|-----------------|
| reading   | `reading.h`, `reading.c`     | `struct reading_counts reading_read(int ticks[], float temps[], float hums[], int max)` | reading and parsing the input lines, skipping comments, blank lines, and bad lines |
| stats     | `stats.h`, `stats.c`         | `stats_min`, `stats_max` (recursive), `stats_mean`, `struct stats_run stats_longest_run_above(...)` | minimum, maximum, and mean of an array, and the longest run above a threshold |
| histogram | `histogram.h`, `histogram.c` | `void histogram_print(const float a[], int n)` | printing the histogram |
| main      | `main.c`                     |  | the command line, calling the modules, and printing the results |

Each header's comments state what every function takes, returns, and
assumes. Helpers used in one file only are `static`: `is_blank_or_comment` in
`reading.c` and `bin_index` in `histogram.c`.

### Design choices

- **No global variables.** The readings live in `main` as local arrays and
  are passed to the functions that need them.
- **Two results come back in a struct.** `reading_read` returns the count and
  the number of skipped lines in `struct reading_counts`;
  `stats_longest_run_above` returns the run's length and start index in
  `struct stats_run`. This avoids pointer out-parameters before pointers are
  covered in HW3.
- **`stats` knows nothing about ticks.** It works on plain `float` arrays and
  returns an index; `main` turns the index into ticks.
- **One recursive maximum.** `stats_max(a, n)` compares `a[n - 1]` with the
  maximum of the first `n - 1` values, and is called for both temperature and
  humidity. It uses one stack frame per value, so a very large array
  overflows the stack.
- **The mean is summed in `float`, in index order,** exactly as the starter
  did. Summing in `double` can change the last printed digit.

### Header dependencies

The Makefile lists, for each `.o`, its `.c` and every header it includes, so
`make` rebuilds exactly what a change affects:

| You change | `make` rebuilds |
|------------|-----------------|
| `stats.h`     | `main.o`, `stats.o`, and `build/readings`; `make test` also rebuilds `tests/test_stats.o` and `build/test_stats` |
| `histogram.c` | `histogram.o` and `build/readings` |
