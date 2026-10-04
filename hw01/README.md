# ECE 361 HW1

## Build and test

From `hw01/`:

```sh
make          # build bits.o
make test     # build and run tests/test_bits
make clean    # remove object files and the test program
```

`make test` prints one `PASS` or `FAIL` line per check (with its source line and condition), then a summary line `N run, M failed`. The test program exits nonzero if any check fails, so `make test` fails too.

Just before the summary, the test program prints one demo line, `demo: print_binary(0x2C, 8) -> 0010 1100`. It is not counted as a check: the program cannot read its own stdout, so the line is there to be checked by eye. The text it prints comes from `format_binary`, which the checks do cover.

The Makefile compiles with `-I.`, so `tests/test_bits.c` can write `#include "bits.h"`. Compiling the test by hand needs the flag too:

```sh
gcc -std=c11 -Wall -Wextra -I. -c tests/test_bits.c -o tests/test_bits.o
```

## Part 2: bit-manipulation library

`bits.h` and `bits.c` provide the four Part 2 functions (`get_field`, `set_field`, `sign_extend`, `print_binary`) plus one extra public helper, `format_binary`.

### Contract

Bits are numbered from 0, the least significant bit. Constants: `WORD_BITS` = 32.

| Function | Valid input | On invalid input |
|----------|-------------|------------------|
| `uint32_t get_field(uint32_t word, int pos, int width)` | width 1..32, pos 0..31, pos + width <= 32 | returns 0 |
| `uint32_t set_field(uint32_t word, int pos, int width, uint32_t value)` | same as `get_field` | returns `word` unchanged |
| `int32_t sign_extend(uint32_t value, int width)` | width 1..32 | returns 0 |
| `void format_binary(char *buf, int size, uint32_t x, int width)` | width 1..32 and size >= width + (width - 1) / 4 + 1 | if size >= 1, `buf` is `""`; if size < 1, nothing is written |
| `void print_binary(uint32_t x, int width)` | width 1..32 | prints nothing |

Rules shared by all functions:

- Bits of `x`, `value` or `word` outside the selected width or field are ignored (or, for `set_field`, left unchanged in `word`).
- Rejection is silent. The caller cannot tell a rejected `get_field` from a legitimate 0; this is accepted and documented.
- Any `int` is allowed as an argument, including negative values and `INT_MAX`. No input causes undefined behavior.
- `print_binary` prints no newline. Groups of four from the right, separated by a space, no padding: `print_binary(0x2C, 8)` prints `0010 1100`; width 6 prints `10 1100`.
- `sign_extend` converts a `uint32_t` above `INT32_MAX` to `int32_t`. In C that conversion is implementation-defined, not undefined; gcc wraps modulo 2^32. This code assumes that behavior.
- `format_binary` is not required by the handout. It exists so the string logic can be tested, because the test program cannot read its own stdout. `print_binary` is a thin wrapper that prints its result. The four required functions still behave as the handout specifies.

### Boundary behavior

- Width 32 is valid: `get_field(w, 0, 32)` returns all of `w`, and `set_field(w, 0, 32, v)` returns `v`. Internally the width-32 mask is a separate case, because `1u << 32` is undefined behavior.
- The top bit is reachable: pos 31 with width 1 is valid. Pos 31 with width 2 runs past bit 31 and is rejected.
- `sign_extend` at width 1 gives -1 for 1 and 0 for 0. At width 32, `0x80000000` gives `INT32_MIN` and `0xFFFFFFFF` gives -1.
- `format_binary` succeeds when `size` is exactly the size needed (10 for width 8) and writes nothing past it; one byte less gives `""`. `BIN_BUF_SIZE` (40) fits any width.
- `INT_MAX` and `INT_MIN` as pos or width are rejected without overflow: the range checks bound pos and width before adding them.
