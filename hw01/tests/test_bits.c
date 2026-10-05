// Tests for bits.c (Part 2) and status.c (Part 3).

// fmemopen is POSIX, hidden by -std=c11 without this.
#define _POSIX_C_SOURCE 200809L

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

#include "bits.h"
#include "status.h"

static int total;
static int fails;

#define CHECK(cond) do {                                               \
    total++;                                                           \
    if (cond) printf("(PASS) Test: %s\n", #cond);                      \
    else {                                                             \
        printf("(FAIL) Test: %s (line %d)\n", #cond, __LINE__);        \
        fails++;                                                       \
    }                                                                  \
} while (0)

/**
 * Exercises get_field() across single-bit, multi-bit, and full-word fields,
 * plus truncation and invalid-bounds cases.
 */
static void check_get_field(void) {
    // Single bit at each end of the word.
    CHECK(get_field(0x1u, 0, 1) == 1u);
    CHECK(get_field(0x80000000u, 31, 1) == 1u);
    CHECK(get_field(0x7FFFFFFFu, 31, 1) == 0u);

    // Multi-bit fields, up to full word.
    CHECK(get_field(0xABCD1234u, 16, 16) == 0xABCDu);
    CHECK(get_field(0xDEADBEEFu, 0, 32) == 0xDEADBEEFu);
    CHECK(get_field(0xABCD1234u, 4, 8) == 0x23u);

    // Fields that run past bit 31 are truncated there.
    CHECK(get_field(0xFFFFFFFFu, 28, 5) == 0xFu);
    CHECK(get_field(0xFFFFFFFFu, 0, 33) == 0xFFFFFFFFu);
    CHECK(get_field(0xFFFFFFFFu, 1, INT_MAX) == 0x7FFFFFFFu);

    // Width below 1 selects nothing.
    CHECK(get_field(0xFFFFFFFFu, 0, 0) == 0u);
    CHECK(get_field(0xFFFFFFFFu, 0, -1) == 0u);

    // Position outside 0..31 selects nothing.
    CHECK(get_field(0xFFFFFFFFu, -1, 1) == 0u);
    CHECK(get_field(0xFFFFFFFFu, 32, 1) == 0u);
}

/**
 * Exercises set_field() across single-bit, multi-bit, and full-word writes,
 * plus value truncation and invalid-bounds cases that leave the word
 * unchanged.
 */
static void check_set_field(void) {
    // Single bit at each end of the word.
    CHECK(set_field(0u, 0, 1, 1u) == 0x1u);
    CHECK(set_field(0u, 31, 1, 1u) == 0x80000000u);
    CHECK(set_field(0xFFFFFFFFu, 31, 1, 0u) == 0x7FFFFFFFu);

    // Multi-bit fields, up to full word. Bits outside the field are unchanged.
    CHECK(set_field(0xB6C5u, 4, 4, 3u) == 0xB635u);
    CHECK(set_field(0x12345678u, 0, 32, 0xDEADBEEFu) == 0xDEADBEEFu);

    // Value bits above the width are dropped.
    CHECK(set_field(0u, 4, 4, 0x1F5u) == 0x50u);

    // Fields that run past bit 31 leave the word unchanged.
    CHECK(set_field(0x12345678u, 28, 5, 0xFFFFFFFFu) == 0x12345678u);
    CHECK(set_field(0x12345678u, 1, INT_MAX, 0xFFFFFFFFu) == 0x12345678u);

    // Width below 1 leaves the word unchanged.
    CHECK(set_field(0x12345678u, 0, 0, 0xFFFFFFFFu) == 0x12345678u);
    CHECK(set_field(0x12345678u, 0, -1, 0xFFFFFFFFu) == 0x12345678u);

    // Position outside 0..31 leaves the word unchanged.
    CHECK(set_field(0x12345678u, -1, 1, 0xFFFFFFFFu) == 0x12345678u);
    CHECK(set_field(0x12345678u, 32, 1, 0xFFFFFFFFu) == 0x12345678u);
}

/**
 * Exercises sign_extend() across positive and negative values at various
 * widths, plus value truncation, width clamping, and invalid-bounds cases.
 */
static void check_sign_extend(void) {
    // Width 1: the only bit is the sign bit.
    CHECK(sign_extend(0u, 1) == 0);
    CHECK(sign_extend(1u, 1) == -1);

    // Multi-bit fields, up to full word. Sign bit clear is positive, set is
    // negative.
    CHECK(sign_extend(0x7Fu, 8) == 127);
    CHECK(sign_extend(0xF8u, 8) == -8);
    CHECK(sign_extend(0x80u, 8) == -128);
    CHECK(sign_extend(0x800u, 12) == -2048);
    CHECK(sign_extend(0x40000000u, 31) == -0x40000000);
    CHECK(sign_extend(0x7FFFFFFFu, 32) == INT32_MAX);
    CHECK(sign_extend(0x80000000u, 32) == INT32_MIN);

    // Value bits above the width are dropped.
    CHECK(sign_extend(0x100u, 8) == 0);

    // Width above 32 is treated as 32.
    CHECK(sign_extend(0x80000000u, 33) == INT32_MIN);
    CHECK(sign_extend(0x80000000u, INT_MAX) == INT32_MIN);

    // Width below 1 returns 0.
    CHECK(sign_extend(0xFFu, 0) == 0);
    CHECK(sign_extend(0xFFu, -1) == 0);
}

/**
 * Helper function for `check_print_binary`.
 *
 * Captures print_binary(x, width)'s stdout output and checks it matches
 * `expected` exactly.
 *
 * @param x        Value to print.
 * @param width    Number of low bits to print.
 * @param expected Exact expected output, including the trailing newline.
 * @return `true` if the captured output matches `expected`.
 */
static bool prints_binary(uint32_t x, int width, const char *expected) {
    // glibc's fmemopen only NUL-terminates after a write, so an empty output
    // would otherwise leave buf as garbage.
    char buf[64] = {0};
    FILE *mem = fmemopen(buf, sizeof buf, "w");
    if (mem == NULL) return false;

    FILE *saved_stdout = stdout;
    stdout = mem;
    print_binary(x, width);
    fflush(mem);
    stdout = saved_stdout;

    bool matches = strcmp(buf, expected) == 0;
    fclose(mem);
    return matches;
}

/**
 * Exercises print_binary() across nibble grouping, value truncation, width
 * clamping, and invalid-width cases.
 */
static void check_print_binary(void) {
    // Single nibble, MSB first.
    CHECK(prints_binary(0x5u, 4, "0101\n"));

    // Nibbles are space-separated, with no trailing space.
    CHECK(prints_binary(0xB6C5u, 16, "1011 0110 1100 0101\n"));
    CHECK(prints_binary(0xDEADBEEFu, 32,
                        "1101 1110 1010 1101 1011 1110 1110 1111\n"));

    // Nibbles are grouped from the LSB, so a partial group leads.
    CHECK(prints_binary(0x1Fu, 5, "1 1111\n"));

    // Value bits above the width are dropped.
    CHECK(prints_binary(0xF5u, 4, "0101\n"));

    // Width above 32 is treated as 32.
    CHECK(prints_binary(0x80000000u, 33,
                        "1000 0000 0000 0000 0000 0000 0000 0000\n"));

    // Width below 1 prints nothing.
    CHECK(prints_binary(0xFFu, 0, ""));
    CHECK(prints_binary(0xFFu, -1, ""));
}

/**
 * Exercises status_unpack() on the spec's worked example, each invalid mode,
 * setpoint boundaries, all-zero and all-one words, a word with the reserved
 * bit set, and each flag set alone.
 */
static void check_status_unpack(void) {
    // Spec's worked example: AUTO, heater on, set point 22.
    CHECK(status_unpack(0x1631u).heat == true);
    CHECK(status_unpack(0x1631u).cool == false);
    CHECK(status_unpack(0x1631u).fan == false);
    CHECK(status_unpack(0x1631u).fault == false);
    CHECK(status_unpack(0x1631u).mode == MODE_AUTO);
    CHECK(status_unpack(0x1631u).setpoint == 22);

    // Invalid modes 5 to 7 come back as their raw values.
    CHECK(status_unpack(0x0050u).heat == false);
    CHECK(status_unpack(0x0050u).cool == false);
    CHECK(status_unpack(0x0050u).fan == false);
    CHECK(status_unpack(0x0050u).fault == false);
    CHECK(status_unpack(0x0050u).mode == 5);
    CHECK(status_unpack(0x0050u).setpoint == 0);

    CHECK(status_unpack(0x0060u).heat == false);
    CHECK(status_unpack(0x0060u).cool == false);
    CHECK(status_unpack(0x0060u).fan == false);
    CHECK(status_unpack(0x0060u).fault == false);
    CHECK(status_unpack(0x0060u).mode == 6);
    CHECK(status_unpack(0x0060u).setpoint == 0);

    CHECK(status_unpack(0x0070u).heat == false);
    CHECK(status_unpack(0x0070u).cool == false);
    CHECK(status_unpack(0x0070u).fan == false);
    CHECK(status_unpack(0x0070u).fault == false);
    CHECK(status_unpack(0x0070u).mode == 7);
    CHECK(status_unpack(0x0070u).setpoint == 0);

    // All four flags set, without leaking into mode or set point.
    CHECK(status_unpack(0x000Fu).heat == true);
    CHECK(status_unpack(0x000Fu).cool == true);
    CHECK(status_unpack(0x000Fu).fan == true);
    CHECK(status_unpack(0x000Fu).fault == true);
    CHECK(status_unpack(0x000Fu).mode == MODE_OFF);
    CHECK(status_unpack(0x000Fu).setpoint == 0);

    // All-zero word.
    CHECK(status_unpack(0x0000u).heat == false);
    CHECK(status_unpack(0x0000u).cool == false);
    CHECK(status_unpack(0x0000u).fan == false);
    CHECK(status_unpack(0x0000u).fault == false);
    CHECK(status_unpack(0x0000u).mode == MODE_OFF);
    CHECK(status_unpack(0x0000u).setpoint == 0);

    // All-one word: invalid mode 7, set point -1, reserved bit ignored.
    CHECK(status_unpack(0xFFFFu).heat == true);
    CHECK(status_unpack(0xFFFFu).cool == true);
    CHECK(status_unpack(0xFFFFu).fan == true);
    CHECK(status_unpack(0xFFFFu).fault == true);
    CHECK(status_unpack(0xFFFFu).mode == 7);
    CHECK(status_unpack(0xFFFFu).setpoint == -1);

    // Set point at each end of the 8-bit two's complement range.
    CHECK(status_unpack(0x7F00u).heat == false);
    CHECK(status_unpack(0x7F00u).cool == false);
    CHECK(status_unpack(0x7F00u).fan == false);
    CHECK(status_unpack(0x7F00u).fault == false);
    CHECK(status_unpack(0x7F00u).mode == MODE_OFF);
    CHECK(status_unpack(0x7F00u).setpoint == 127);

    CHECK(status_unpack(0x8000u).heat == false);
    CHECK(status_unpack(0x8000u).cool == false);
    CHECK(status_unpack(0x8000u).fan == false);
    CHECK(status_unpack(0x8000u).fault == false);
    CHECK(status_unpack(0x8000u).mode == MODE_OFF);
    CHECK(status_unpack(0x8000u).setpoint == -128);

    // The worked example with reserved bit 7 set decodes identically.
    CHECK(status_unpack(0x16B1u).heat == true);
    CHECK(status_unpack(0x16B1u).cool == false);
    CHECK(status_unpack(0x16B1u).fan == false);
    CHECK(status_unpack(0x16B1u).fault == false);
    CHECK(status_unpack(0x16B1u).mode == MODE_AUTO);
    CHECK(status_unpack(0x16B1u).setpoint == 22);

    // Each flag alone, so a flag that reads a neighbor's bit is caught.
    CHECK(status_unpack(0x0001u).heat == true);
    CHECK(status_unpack(0x0001u).cool == false);
    CHECK(status_unpack(0x0001u).fan == false);
    CHECK(status_unpack(0x0001u).fault == false);

    CHECK(status_unpack(0x0002u).heat == false);
    CHECK(status_unpack(0x0002u).cool == true);
    CHECK(status_unpack(0x0002u).fan == false);
    CHECK(status_unpack(0x0002u).fault == false);

    CHECK(status_unpack(0x0004u).heat == false);
    CHECK(status_unpack(0x0004u).cool == false);
    CHECK(status_unpack(0x0004u).fan == true);
    CHECK(status_unpack(0x0004u).fault == false);

    CHECK(status_unpack(0x0008u).heat == false);
    CHECK(status_unpack(0x0008u).cool == false);
    CHECK(status_unpack(0x0008u).fan == false);
    CHECK(status_unpack(0x0008u).fault == true);
}

/**
 * Prints a summary of checks run and how many of those checks failed.
 */
static void print_summary(void) {
    printf("\nSummary:\n");
    printf("--------\n");
    printf("%d total\n", total);
    printf("%d failed\n", fails);
}

/**
 * The main test loop. Runs all checks and prints a summary.
 *
 * @return `0` if every check passed, `1` if any failed.
 */
int main(void) {
    check_get_field();
    check_set_field();
    check_sign_extend();
    check_print_binary();
    check_status_unpack();

    print_summary();

    return fails != 0;
}
