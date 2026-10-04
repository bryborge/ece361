// fmemopen is POSIX, hidden by -std=c11 without this.
#define _POSIX_C_SOURCE 200809L

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

#include "bits.h"

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

// Returns whether print_binary(x, width) writes exactly `expected` to stdout.
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

static void print_summary(void) {
    printf("\nSummary:\n");
    printf("--------\n");
    printf("%d total\n", total);
    printf("%d failed\n", fails);
}

int main(void) {
    check_get_field();
    check_set_field();
    check_sign_extend();
    check_print_binary();

    print_summary();

    return fails != 0;
}
