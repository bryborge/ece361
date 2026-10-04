#include <stdbool.h>
#include <stdint.h>

#include "bits.h"

#define REGISTER_SIZE 32

static bool invalid_bounds(int pos, int width) {
    return pos < 0 || width <= 0;
}

static bool starts_outside_register(int pos) {
    return pos >= REGISTER_SIZE;
}

static bool runs_past_register(int pos, int width) {
    return width > REGISTER_SIZE - pos;
}

static uint32_t low_mask(int width) {
    return (width >= REGISTER_SIZE) ? ~0u : (1u << width) - 1;
}

uint32_t get_field(uint32_t word, int pos, int width) {
    if (invalid_bounds(pos, width)) return 0;
    if (starts_outside_register(pos)) return 0;
    if (runs_past_register(pos, width)) width = REGISTER_SIZE - pos;

    uint32_t shifted_word = word >> pos;

    return shifted_word & low_mask(width);
}

uint32_t set_field(uint32_t word, int pos, int width, uint32_t value) {
    if (invalid_bounds(pos, width)) return word;
    if (runs_past_register(pos, width)) return word;

    uint32_t mask = low_mask(width);
    uint32_t clean_value = value & mask;
    uint32_t shifted_mask = mask << pos;
    uint32_t shifted_value = clean_value << pos;

    return (word & ~shifted_mask) | shifted_value;
}
