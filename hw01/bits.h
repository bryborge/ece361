#ifndef BITS_H
#define BITS_H

#include <stdint.h>

/**
 * Prints the lowest `width` bits of `x`, MSB first, in groups of four separated
 * by a space.
 */
void print_binary(uint32_t x, int width);

/**
 * Returns bits `pos` to `pos + width - 1` of `word`, shifted down to bit `0`.
 */
uint32_t get_field(uint32_t word, int pos, int width);

/**
 * Returns `word` with bits `pos` to `pos + width - 1` replaced by the lowest
 * `width` bits of `value`. All other bits are unchanged.
 */
uint32_t set_field(uint32_t word, int pos, int width, uint32_t value);

/**
 * Interprets the lowest `width` bits of `value` as a two's complement number
 * and returns it as an `int32_t`.
 */
int32_t sign_extend(uint32_t value, int width);

#endif
