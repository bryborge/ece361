#include <stdio.h>
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

static void print_summary(void) {
    printf("\nSummary:\n");
    printf("--------\n");
    printf("%d total\n", total);
    printf("%d failed\n", fails);
}

int main(void) {
    check_get_field();
    check_set_field();

    print_summary();

    return fails != 0;
}
