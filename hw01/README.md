# ECE 361 HW1

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
- `format_binary` is not required by the handout. It exists so the string logic can be tested, because the test program cannot read its own stdout.
