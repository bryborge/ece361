/*
 * test_hw06.c:
 *
 * Tests for hw06. Prints PASS or FAIL for each check, then a summary; exits
 * nonzero if any check failed.
 */
#include <stdint.h>
#include <stdio.h>

#include "panel.h"

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

static void check_pressed_and_released(void) {
    /* a press */
    CHECK(panel_pressed(0x1, 0x0) == 0x1);
    CHECK(panel_released(0x1, 0x0) == 0x0);
    /* a release */
    CHECK(panel_pressed(0x0, 0x1) == 0x0);
    CHECK(panel_released(0x0, 0x1) == 0x1);
    /* a held switch */
    CHECK(panel_pressed(0x1, 0x1) == 0x0);
    CHECK(panel_released(0x1, 0x1) == 0x0);
    /* a press, a release and a held switch at once */
    CHECK(panel_pressed(0x6, 0x3) == 0x4);
    CHECK(panel_released(0x6, 0x3) == 0x1);
    /* the top bit */
    CHECK(panel_pressed(0x80000000u, 0x0) == 0x80000000u);
    CHECK(panel_released(0x0, 0x80000000u) == 0x80000000u);
}

int main(void) {
    check_pressed_and_released();

    printf("%d of %d checks passed\n", total - fails, total);

    return fails != 0;
}
