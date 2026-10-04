#include <limits.h>
#include <stdio.h>

#include "bits.h"

static int total;
static int fails;

#define CHECK(cond) do { \
    total++; \
    if (cond) { \
        printf("PASS line %d: %s\n", __LINE__, #cond); \
    } else { \
        printf("FAIL line %d: %s\n", __LINE__, #cond); \
        fails++; \
    } \
} while (0)

static void test_get_field(void)
{
    /* Slide example */
    CHECK(get_field(0xB6C5u, 4, 4) == 0xCu);

    /* Width 1 */
    CHECK(get_field(0x1u, 0, 1) == 1u);
    CHECK(get_field(0x80000000u, 31, 1) == 1u);
    CHECK(get_field(0x7FFFFFFFu, 31, 1) == 0u);

    /* Width 32 */
    CHECK(get_field(0xDEADBEEFu, 0, 32) == 0xDEADBEEFu);

    /* Edges */
    CHECK(get_field(0xF0000000u, 28, 4) == 0xFu);
    CHECK(get_field(0xABCD1234u, 16, 16) == 0xABCDu);

    /* Reject */
    CHECK(get_field(0xFFFFFFFFu, 0, 0) == 0u);
    CHECK(get_field(0xFFFFFFFFu, 0, 33) == 0u);
    CHECK(get_field(0xFFFFFFFFu, 0, -1) == 0u);
    CHECK(get_field(0xFFFFFFFFu, -1, 1) == 0u);
    CHECK(get_field(0xFFFFFFFFu, 32, 1) == 0u);
    CHECK(get_field(0xFFFFFFFFu, 28, 5) == 0u);
    CHECK(get_field(0xFFFFFFFFu, 31, 2) == 0u);

    /* Overflow guard */
    CHECK(get_field(0xFFFFFFFFu, INT_MAX, INT_MAX) == 0u);
    CHECK(get_field(0xFFFFFFFFu, INT_MIN, 1) == 0u);
    CHECK(get_field(0xFFFFFFFFu, 0, INT_MIN) == 0u);
}

int main(void)
{
    test_get_field();

    printf("%d run, %d failed\n", total, fails);
    return fails != 0;
}
