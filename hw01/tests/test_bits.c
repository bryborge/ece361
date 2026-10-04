#include <limits.h>
#include <stdint.h>
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

static void test_set_field(void)
{
    /* Slide example */
    CHECK(set_field(0xB6C5u, 4, 4, 3u) == 0xB635u);

    /* Value too wide */
    CHECK(set_field(0xFFFFFF0Fu, 4, 4, 0x1F5u) == 0xFFFFFF5Fu);

    /* Width 1, pos 31 */
    CHECK(set_field(0u, 31, 1, 1u) == 0x80000000u);
    CHECK(set_field(0xFFFFFFFFu, 31, 1, 0u) == 0x7FFFFFFFu);

    /* Width 1, value 2 */
    CHECK(set_field(0xFFFFFFFFu, 0, 1, 2u) == 0xFFFFFFFEu);

    /* Width 32 */
    CHECK(set_field(0x12345678u, 0, 32, 0xDEADBEEFu) == 0xDEADBEEFu);

    /* Round trip */
    CHECK(get_field(set_field(0u, 12, 8, 0xABu), 12, 8) == 0xABu);
    CHECK(set_field(0u, 12, 8, 0xABu) == 0x000AB000u);

    /* Reject */
    CHECK(set_field(0x12345678u, 0, 0, 0xFFFFFFFFu) == 0x12345678u);
    CHECK(set_field(0x12345678u, 0, 33, 0xFFFFFFFFu) == 0x12345678u);
    CHECK(set_field(0x12345678u, -1, 1, 0xFFFFFFFFu) == 0x12345678u);
    CHECK(set_field(0x12345678u, 32, 1, 0xFFFFFFFFu) == 0x12345678u);
    CHECK(set_field(0x12345678u, 28, 5, 0xFFFFFFFFu) == 0x12345678u);
    CHECK(set_field(0x12345678u, INT_MAX, INT_MAX, 0xFFFFFFFFu) == 0x12345678u);
}

static void test_sign_extend(void)
{
    /* Handout */
    CHECK(sign_extend(0xF8u, 8) == -8);

    /* Positive */
    CHECK(sign_extend(0x7Fu, 8) == 127);

    /* Most negative */
    CHECK(sign_extend(0x80u, 8) == -128);
    CHECK(sign_extend(0x80000000u, 32) == INT32_MIN);

    /* Others */
    CHECK(sign_extend(0xFFFFFFFFu, 32) == -1);
    CHECK(sign_extend(0x7FFFFFFFu, 32) == INT32_MAX);

    /* Width 1 */
    CHECK(sign_extend(1u, 1) == -1);
    CHECK(sign_extend(0u, 1) == 0);

    /* Value too wide */
    CHECK(sign_extend(0x1F8u, 8) == -8);
    CHECK(sign_extend(0x100u, 8) == 0);

    /* Reject */
    CHECK(sign_extend(0xFFu, 0) == 0);
    CHECK(sign_extend(0xFFu, 33) == 0);
    CHECK(sign_extend(0xFFu, -1) == 0);
}

int main(void)
{
    test_get_field();
    test_set_field();
    test_sign_extend();

    printf("%d run, %d failed\n", total, fails);
    return fails != 0;
}
