# Homework 1

## Part 2: A bit-manipulation library

> Write `bits.h` and `bits.c` with the four functions below. Use the fixed-
> width types from `<stdint.h>`. Bits are numbered from `0`, the least-
> significant bit.

| Function | What it does |
|----------|--------------|
| `void print_binary(uint32_t x, int width)`                              | Prints the lowest `width` bits of `x`, most significant bit first, in groups of four separated by a space. `print_binary(0x2C, 8)` prints `0010 1100`. |
| `uint32_t get_field(uint32_t word, int pos, int width)`                 | Returns bits `pos` to `pos + width - 1`, shifted down to bit `0`. |
| `uint32_t set_field(uint32_t word, int pos, int width, uint32_t value)` | Returns `word` with bits `pos` to `pos + width - 1` replaced by the lowest `width` bits of `value`. All other bits are unchanged. |
| `int32_t sign_extend(uint32_t value, int width)`                        | Interprets the lowest `width` bits of `value` as a two's complement number and returns it as an `int32_t`. `sign_extend(0xF8, 8)` returns `-8`. |

### What is this?

A small library for reading and writing bit fields inside a 32-bit word, and
for sign-extending and printing them. `get_field` and `set_field` operate on
an arbitrary span of bits (`pos` through `pos + width - 1`); `sign_extend`
and `print_binary` treat `value` as a fixed field starting at bit `0`.

### Build and test

```sh
make        # builds build/tests/test_bits
make test   # builds (if needed) and runs the test binary
make clean  # removes build/
```

The test binary prints one line per check (`PASS` or `FAIL`) followed by a
pass/fail summary, and exits non-zero if any check failed.

### Input ranges and boundary behavior

`pos` and `width` are `int`, but only `pos` in `0..31` and `width >= 1` are
valid positions and widths within a 32-bit word. Each function's contract:

**`get_field(word, pos, width)`**
- Requires `pos >= 0` and `width >= 1`. If either is violated, or `pos >= 32`,
  returns `0`.
- If `pos + width` reaches past bit 31, the field is truncated to the bits
  that exist (bits `pos` through `31`) instead of failing.

**`set_field(word, pos, width, value)`**
- Requires `pos >= 0` and `width >= 1`. If either is violated, returns `word`
  unchanged.
- Requires `pos + width <= 32`. Unlike `get_field`, a field that runs off the
  top of the word is rejected rather than truncated, so a write never
  silently drops bits: `word` is returned unchanged.
- Bits of `value` above `width` are ignored; only the lowest `width` bits are
  written.

**`sign_extend(value, width)`**
- Requires `width >= 1`. If violated, returns `0`.
- `width` above `32` is clamped to `32`, since there are no more bits to
  extend.
- Bits of `value` above `width` are ignored; only the lowest `width` bits are
  interpreted as the two's complement number.

**`print_binary(x, width)`**
- Requires `width >= 1`. If violated, prints nothing, not even a newline.
- `width` above `32` is clamped to `32`, since there are no more bits to
  print.
