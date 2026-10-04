#include <stdbool.h>

#include "bits.h"

enum { WORD_BITS = 32 };

/* Not an enum constant: 0xFFFFFFFF does not fit in an int. */
static const uint32_t ALL_ONES = 0xFFFFFFFFu;

static bool width_ok(int width)
{
    return width >= 1 && width <= WORD_BITS;
}

/*
 * Order matters: width and pos are bounded before they are added, so the
 * sum is at most 31 + 32 and cannot overflow.
 */
static bool field_ok(int pos, int width)
{
    return width_ok(width) && pos >= 0 && pos <= WORD_BITS - 1
        && pos + width <= WORD_BITS;
}

/*
 * Mask of the low width bits. Precondition: width is 1..WORD_BITS.
 * Width 32 is a separate branch because 1u << 32 is undefined behavior.
 */
static uint32_t low_mask(int width)
{
    return (width == WORD_BITS) ? ALL_ONES : (1u << width) - 1u;
}

uint32_t get_field(uint32_t word, int pos, int width)
{
    if (!field_ok(pos, width)) {
        return 0;
    }
    return (word >> pos) & low_mask(width);
}

uint32_t set_field(uint32_t word, int pos, int width, uint32_t value)
{
    if (!field_ok(pos, width)) {
        return word;
    }
    uint32_t m = low_mask(width);
    return (word & ~(m << pos)) | ((value & m) << pos);
}

int32_t sign_extend(uint32_t value, int width)
{
    if (!width_ok(width)) {
        return 0;
    }
    uint32_t m = low_mask(width);
    uint32_t v = value & m;
    if (v & (1u << (width - 1))) {
        v |= ~m;
    }
    /* Implementation-defined above INT32_MAX; gcc wraps modulo 2^32. */
    return v;
}

void format_binary(char *buf, int size, uint32_t x, int width)
{
    if (size < 1) {
        return;
    }
    buf[0] = '\0';
    if (!width_ok(width)) {
        return;
    }
    int needed = width + (width - 1) / 4 + 1;
    if (size < needed) {
        return;
    }
    int n = 0;
    for (int i = width - 1; i >= 0; i--) {
        buf[n++] = '0' + ((x >> i) & 1u);
        if (i > 0 && i % 4 == 0) {
            buf[n++] = ' ';
        }
    }
    buf[n] = '\0';
}
