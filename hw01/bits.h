#ifndef BITS_H
#define BITS_H

#include <stdint.h>

/*
 * Bits are numbered from 0, the least significant bit. A word is 32 bits.
 * Out-of-range arguments are rejected silently through the return value.
 * Any int is allowed as an argument; no input causes undefined behavior.
 */

/* Fits any format_binary result: 32 digits, 7 separating spaces, 1 terminator. */
enum { BIN_BUF_SIZE = 32 + 7 + 1 };

/*
 * Returns the width-bit field of word that starts at bit pos.
 * Valid: width 1..32, pos 0..31, pos + width <= 32.
 * Invalid: returns 0, which the caller cannot tell from a field that is 0.
 */
uint32_t get_field(uint32_t word, int pos, int width);

/*
 * Returns word with the width-bit field at bit pos replaced by the low
 * width bits of value. Higher bits of value are ignored; bits of word
 * outside the field are left unchanged.
 * Valid: same as get_field.
 * Invalid: returns word unchanged.
 */
uint32_t set_field(uint32_t word, int pos, int width, uint32_t value);

/*
 * Treats the low width bits of value as a two's-complement number and
 * returns it sign-extended to 32 bits. Higher bits of value are ignored.
 * Valid: width 1..32.
 * Invalid: returns 0.
 * Assumes gcc's conversion of uint32_t values above INT32_MAX to int32_t,
 * which wraps modulo 2^32 (implementation-defined in C).
 */
int32_t sign_extend(uint32_t value, int width);

/*
 * Writes the low width bits of x into buf as '0'/'1' digits, most
 * significant first, in groups of four from the right separated by one
 * space, with no padding: (0x2C, 8) gives "0010 1100", (0x2C, 6) gives
 * "10 1100". Not required by the handout; it exists so the tests can
 * check the text that print_binary prints.
 * Valid: width 1..32 and size >= width + (width - 1) / 4 + 1.
 * Invalid: if size >= 1, buf is ""; if size < 1, nothing is written.
 */
void format_binary(char *buf, int size, uint32_t x, int width);

/*
 * Prints the low width bits of x to stdout in format_binary's format,
 * with no trailing newline.
 * Valid: width 1..32.
 * Invalid: prints nothing.
 */
void print_binary(uint32_t x, int width);

#endif
