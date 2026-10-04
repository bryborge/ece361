#include <stdint.h>

#include "bits.h"

#define REGISTER_SIZE 32

uint32_t get_field(uint32_t word, int pos, int width) {
    // Guard: reject negative positions and negative or zero widths.
    if (pos < 0 || width <= 0) return 0;

    // Guard: Reject positions outside the register size.
    if (pos >= REGISTER_SIZE) return 0;

    // Guard: Clamp the width to prevent reading past the register.
    if (pos + width >= REGISTER_SIZE) width = REGISTER_SIZE - pos;

    // Extraction logic
    uint32_t shifted_word = word >> pos;
    uint32_t mask = (width >= REGISTER_SIZE) ? ~0u : (1u << width) - 1;

    return shifted_word & mask;
}
