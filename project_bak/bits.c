#include <stdbool.h>
#include <stdint.h>

#include "bits.h"

#define WORD_WIDTH 32

/**
 * Checks whether a field's position or width is out of bounds.
 *
 * @param pos   Index of the field's lowest bit.
 * @param width Number of bits in the field.
 * @return `true` if `pos` is negative or `width` is not positive.
 */
static bool invalid_bounds(int pos, int width) {
    return pos < 0 || width <= 0;
}

/**
 * Checks whether a field's lowest bit sits beyond the top of the word.
 *
 * @param pos Index of the field's lowest bit.
 * @return `true` if `pos` is at or past `WORD_WIDTH`.
 */
static bool starts_outside_word(int pos) {
    return pos >= WORD_WIDTH;
}

/**
 * Checks whether a field's highest bit sits beyond `WORD_WIDTH - 1`.
 *
 * @param pos   Index of the field's lowest bit.
 * @param width Number of bits in the field.
 * @return `true` if `pos + width` exceeds `WORD_WIDTH`.
 */
static bool runs_past_word(int pos, int width) {
    return width > WORD_WIDTH - pos;
}

/**
 * Builds a mask of `width` ones in the low bits.
 *
 * When `width >= WORD_WIDTH`, returns all ones instead of computing
 * the shift, since shifting a 32-bit value by 32 or more bits is
 * undefined behavior.
 *
 * @param width Number of low bits to set. Must be positive.
 * @return A mask with bits `0` to `width - 1` set.
 */
static uint32_t low_mask(int width) {
    return (width >= WORD_WIDTH) ? ~0u : (1u << width) - 1;
}

/**
 * Returns bits `pos` to `pos + width - 1` of `word`, shifted down to bit `0`.
 *
 * If the field runs past the top of the word, it is truncated to the bits
 * that exist.
 *
 * @param word  Word to extract from.
 * @param pos   Index of the field's lowest bit.
 * @param width Number of bits in the field.
 * @return The extracted field, or `0` for invalid bounds.
 */
uint32_t get_field(uint32_t word, int pos, int width) {
    if (invalid_bounds(pos, width)) return 0;
    if (starts_outside_word(pos)) return 0;
    if (runs_past_word(pos, width)) width = WORD_WIDTH - pos;

    uint32_t shifted_word = word >> pos;

    return shifted_word & low_mask(width);
}

/**
 * Returns `word` with bits `pos` to `pos + width - 1` replaced by the lowest
 * `width` bits of `value`. All other bits are unchanged.
 *
 * Unlike get_field(), a field that runs past the top of the word is
 * rejected rather than truncated.
 *
 * @param word  Word to modify.
 * @param pos   Index of the field's lowest bit.
 * @param width Number of bits in the field.
 * @param value New field contents. Bits above `width` are ignored.
 * @return The modified word, or `word` unchanged for invalid bounds.
 */
uint32_t set_field(uint32_t word, int pos, int width, uint32_t value) {
    if (invalid_bounds(pos, width)) return word;
    if (runs_past_word(pos, width)) return word;

    uint32_t mask = low_mask(width);
    uint32_t clean_value = value & mask;
    uint32_t shifted_mask = mask << pos;
    uint32_t shifted_value = clean_value << pos;

    return (word & ~shifted_mask) | shifted_value;
}
