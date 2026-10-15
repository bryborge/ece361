# Homework 3

## A pointer library

> Homework 3 is about pointers: passing addresses so that a function can
> change the caller's variables or return more than one result, and walking
> arrays with pointer arithmetic.

### What is this?

`ptr` is a small library of four functions over floats and arrays of floats.
Each one takes pointers instead of values, either so it can change the
caller's variables (`swap_float`, `reverse`), return more than one result
(`minmax`), or hand back a location inside the caller's array (`find_above`).

Inside `ptr.c`, arrays are walked with pointers (`p++`, `p < a + n`, `*p`),
never with `a[i]`. Parameters a function only reads are `const`.

| File | Responsibility |
|------|----------------|
| `ptr.h`, `ptr.c`    | The pointer library. |
| `tests/test_ptr.c`  | Unit tests for every function in `ptr`. |

| Function | What it does |
|----------|--------------|
| `void swap_float(float *a, float *b)` | Exchanges `*a` and `*b`. |
| `int minmax(const float *a, int n, float *min, float *max)` | Stores the smallest and largest of `a[0..n-1]` in `*min` and `*max` and returns `0`. |
| `const float *find_above(const float *a, int n, float threshold)` | Returns a pointer to the first value in `a[0..n-1]` greater than `threshold`, or `NULL`. |
| `void reverse(float *a, int n)` | Reverses `a[0..n-1]` in place, using two pointers that move toward each other and `swap_float`. |

### Build and test

```sh
make        # builds build/ptr.o and build/tests/test_ptr
make test   # builds (if needed) and runs build/tests/test_ptr
make clean  # removes build/
```

Everything is built with `gcc -std=c11 -Wall -Wextra` and compiles with no
warnings. `build/tests/test_ptr` prints one line per check (`PASS` or `FAIL`)
followed by a summary, and exits non-zero if any check failed.

Everything is also built with `-fsanitize=address`, so an out-of-bounds access
or a `NULL` dereference in any test aborts with a report instead of passing
silently.

### Input ranges and boundary behavior

Every function that takes an array and a count requires the array to hold at
least that many elements. C cannot check this inside the function, since the
parameter is only a pointer; AddressSanitizer catches violations in the test
build.

**`swap_float(a, b)`**
- Requires `a` and `b` to be non-`NULL`.

**`minmax(a, n, min, max)`**
- Returns `-1` if `n < 1` or `a`, `min`, or `max` is `NULL`. In that case
  `*min` and `*max` are left unchanged.
- On success, `*min` and `*max` are written only once the scan is done, so
  they are never left half-updated.
- With `n == 1`, `*min` and `*max` both get `a[0]`.

**`find_above(a, n, threshold)`**
- Returns `NULL` if `a` is `NULL`, if `n < 1`, or if no value is greater than
  `threshold`.
- A value equal to `threshold` does not count.
- If several values are greater than `threshold`, returns a pointer to the
  first one.
- The returned pointer points into `a`, so it is `const` like `a`.

**`reverse(a, n)`**
- Does nothing if `a` is `NULL` or `n < 2`.
- With an odd `n`, the middle value stays where it is.
- Values past `a[n-1]` are untouched.
