#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

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

int32_t sign_extend(uint32_t value, int width) {
    if (width <= 0) return 0;
    if (width >= REGISTER_SIZE) return value;

    uint32_t value_mask = (1u << width) - 1;
    uint32_t clean_value = value & value_mask;

    uint32_t sign_bit = 1u << (width - 1);

    if (clean_value & sign_bit) {
        uint32_t extension_mask = ~0u << width;
        clean_value |= extension_mask;
    }

    return clean_value;
}

void print_binary(uint32_t x, int width) {
    if (width <= 0) return;
    if (width > REGISTER_SIZE) width = REGISTER_SIZE;

    for (int i = width - 1; i >= 0; i--) {
        uint32_t bit = (x >> i) & 1u;
        printf("%u", bit);

        // space-delimited nibbles, and don't add an extra space at the end
        if ((i % 4 == 0) && (i != 0)) printf(" ");
    }
    printf("\n");
}
