/*
 * test_hw06.c:
 *
 * Tests for hw06. Prints PASS or FAIL for each check, then a summary; exits
 * nonzero if any check failed.
 */
#include <stdint.h>
#include <stdio.h>

#include "iom361_r4.h"
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

/**
 * Resets iom361 and sets the LED register directly, so that a panel test
 * starts from a known pattern without going through the code under test.
 *
 * @return the base address of the registers.
 */
static uint32_t *board_with_leds(uint32_t leds) {
    uint32_t *base = iom361_initialize(4, 4, NULL);

    iom361_writeReg(base, LEDS_REG, leds, NULL);
    return base;
}

static uint32_t leds_of(uint32_t *base) {
    return iom361_readReg(base, LEDS_REG, NULL);
}

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

static void check_toggle_keeps_other_leds(void) {
    uint32_t *base = board_with_leds(0x5);

    CHECK(panel_toggle(base, 1) == 0);
    CHECK(leds_of(base) == 0x7);
    CHECK(panel_toggle(base, 0) == 0);
    CHECK(leds_of(base) == 0x6);
}

static void check_toggle_top_bit(void) {
    uint32_t *base = board_with_leds(0x0);

    CHECK(panel_toggle(base, 31) == 0);
    CHECK(leds_of(base) == 0x80000000u);
    CHECK(panel_toggle(base, 31) == 0);
    CHECK(leds_of(base) == 0x0);
}

static void check_toggle_out_of_range(void) {
    uint32_t *base = board_with_leds(0x5);

    CHECK(panel_toggle(base, -1) == -1);
    CHECK(panel_toggle(base, 32) == -1);
    CHECK(leds_of(base) == 0x5);
}

int main(void) {
    iom361_set_display(false);

    check_pressed_and_released();
    check_toggle_keeps_other_leds();
    check_toggle_top_bit();
    check_toggle_out_of_range();

    printf("%d of %d checks passed\n", total - fails, total);

    return fails != 0;
}
