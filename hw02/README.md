# Homework 2

## Splitting `readings` into modules

> The program works. It is also one file, with global variables, repeated
> code, and a main() that does everything. HW2 asks you to split it into
> modules without changing what it prints.

### What is this?

`readings` reads sensor readings from standard input and prints a summary: the
number of readings, the minimum, maximum, and mean temperature and humidity,
the longest run of readings above a temperature threshold, and a histogram of
the temperatures.

The starter was a single `readings.c`. It is now split into four modules, with
no global variables: `main` owns the arrays and passes them, with their count,
to each module.

| File | Responsibility |
|------|----------------|
| `main.c`              | Parses the threshold argument, calls the other modules, and prints the summary. |
| `reading.h`, `reading.c`     | Parses readings from stdin into arrays of ticks, temperatures, and humidities. |
| `stats.h`, `stats.c`         | Minimum, maximum, mean, and longest run above a threshold, over an array of floats. |
| `histogram.h`, `histogram.c` | Prints a histogram of temperatures in 5 C bins. |

| Function | What it does |
|----------|--------------|
| `struct reading_counts reading_read(int ticks[], float temps[], float hums[], int max)` | Reads up to `max` readings from stdin and returns how many were stored and how many lines were skipped. |
| `float stats_max(const float arr[], int n)`  | Returns the largest of `arr[0..n-1]`. Recursive. |
| `float stats_min(const float arr[], int n)`  | Returns the smallest of `arr[0..n-1]`. |
| `float stats_mean(const float arr[], int n)` | Returns the mean of `arr[0..n-1]`. |
| `struct stats_run stats_longest_run_above(const float arr[], int n, float th)` | Returns the length and start index of the longest run of values strictly greater than `th`. |
| `void print_histogram(const float arr[], int n)` | Prints a histogram of `arr[0..n-1]` to stdout, one line per non-empty bin. |

### Usage

```sh
build/readings [threshold] < data/normal.txt
```

`threshold` is in degrees C and defaults to `30.0`. Input is one reading per
line, `tick temperature humidity`, for example `12 23.5 41.0`. For
`data/edge.txt` with a threshold of 25:

```
readings: 9
skipped:  2
temperature: min   -3.5  max  104.2  mean  45.97 C
humidity:    min   40.0  max   50.0  mean  45.61 %RH
above 25.0 C: longest run 6 readings, from tick 180 to tick 540
histogram:
    0.0 to   5.0 C | ** 2
   20.0 to  25.0 C | * 1
   30.0 to  35.0 C | *** 3
   95.0 to 100.0 C | *** 3
```

### Build and test

```sh
make        # builds build/readings
make test   # builds (if needed) and runs the unit tests, then regress.sh
make clean  # removes build/
```

`make test` runs one unit test binary per module (`build/test_stats`,
`build/test_histogram`, `build/test_reading`). Each prints one line per check
(`PASS` or `FAIL`) followed by a summary, and exits non-zero if any check
failed. `regress.sh` then runs `build/readings` on every file in `data/` and
compares its output with the file of the same name in `expected/`, plus
`data/edge.txt` with a threshold of 25.

Everything is built with `-fsanitize=address`, so an out-of-bounds access in
any test aborts with a report instead of passing silently.

### Input ranges and boundary behavior

Every function that takes an array and a count requires the array to hold at
least that many elements. C cannot check this inside the function, since an
array parameter is only a pointer; AddressSanitizer catches violations in the
test builds. The `assert`s below are active because the build does not define
`NDEBUG`.

**`reading_read(ticks, temps, hums, max)`**
- Requires `max >= 0` (asserted).
- Blank lines, and lines whose first non-blank character is `#`, are ignored
  and not counted.
- A line that does not start with three numbers is counted in `skipped`. A
  line with extra fields after three numbers is accepted.
- If another data line appears once `max` readings are stored, prints
  `warning: more than <max> readings, the rest are ignored` to stderr and
  stops. Exactly `max` readings produce no warning.
- `nan` and `inf` are accepted as numbers.
- Lines longer than 127 characters are split by `fgets`, and the rest is read
  as a new line.

**`stats_max`, `stats_min`, `stats_mean(arr, n)`**
- Require `n > 0` (asserted).
- `stats_max` is recursive, one stack frame per element, so `n` must not be
  large enough to overflow the stack. `main` caps it at 1000.

**`stats_longest_run_above(arr, n, th)`**
- Requires `n >= 0` (asserted). `n == 0` is valid and finds no run.
- Values equal to `th` end a run.
- If several runs share the longest length, the first one is returned.
- If no value is greater than `th`, returns `{0, -1}`. `start` is `-1` so it
  can never be mistaken for a real index; check `length` before using it.

**`print_histogram(arr, n)`**
- Requires `n >= 0` (asserted). `n == 0` prints only the `histogram:` heading.
- Bins are 5 C wide, from 0 to 100 C, and include their lower edge: `25.0`
  goes in `25.0 to 30.0`.
- Values below 0 C, including `-inf`, go in the first bin. Values at or above
  100 C, including `inf`, go in the last. NaN goes in the first bin.
- Empty bins are not printed.

**`threshold` argument**
- Parsed with `strtof`; anything left over after the number is an error, and
  the program exits with status 1.
- An empty argument (`""`) is read as `0`, and `nan` or `inf` are accepted. A
  `nan` threshold always prints `never`.
