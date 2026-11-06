/*
 * test_hw06.c:
 *
 * Tests for panel.c, sensor.c and scenario.c. Prints PASS or FAIL for each
 * check, then a summary; exits nonzero if any check failed.
 */
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#include "iom361_r4.h"
#include "panel.h"
// #include "scenario.h"
#include "sensor.h"

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
    /* one press, one release and one held switch at once */
    CHECK(panel_pressed(0x6, 0x3) == 0x4);
    CHECK(panel_released(0x6, 0x3) == 0x1);
    /* the top bit */
    CHECK(panel_pressed(0x80000000u, 0x0) == 0x80000000u);
    CHECK(panel_released(0x0, 0x80000000u) == 0x80000000u);
}

static void check_led_keeps_other_leds(void) {
    uint32_t *base = board_with_leds(0x5);

    CHECK(panel_led(base, 1, 1) == 0);
    CHECK(leds_of(base) == 0x7);
    CHECK(panel_led(base, 0, 0) == 0);
    CHECK(leds_of(base) == 0x6);
}

static void check_led_already_in_state(void) {
    uint32_t *base = board_with_leds(0x5);

    CHECK(panel_led(base, 0, 1) == 0);
    CHECK(leds_of(base) == 0x5);
    CHECK(panel_led(base, 1, 0) == 0);
    CHECK(leds_of(base) == 0x5);
}

static void check_led_any_nonzero_is_on(void) {
    uint32_t *base = board_with_leds(0x0);

    CHECK(panel_led(base, 2, 2) == 0);
    CHECK(leds_of(base) == 0x4);
}

static void check_led_range(void) {
    uint32_t *base = board_with_leds(0x5);

    CHECK(panel_led(base, -1, 1) == -1);
    CHECK(panel_led(base, 32, 1) == -1);
    CHECK(leds_of(base) == 0x5);
    /* The ends of the range. UBSan stops a signed shift at 31. */
    CHECK(panel_led(base, 31, 1) == 0);
    CHECK(leds_of(base) == 0x80000005u);
    CHECK(panel_led(base, 31, 0) == 0);
    CHECK(leds_of(base) == 0x5);
    CHECK(panel_led(base, 0, 0) == 0);
    CHECK(leds_of(base) == 0x4);
}

static void check_led_failed_access(void) {
    uint32_t *base = board_with_leds(0x5);

    /* The range check comes first, so a bad n wins over a bad base. */
    CHECK(panel_led(base + 1, 32, 1) == -1);
    CHECK(panel_led(base + 1, 1, 1) == 1);
    CHECK(leds_of(base) == 0x5);
}

static void check_toggle_keeps_other_leds(void) {
    uint32_t *base = board_with_leds(0x5);

    CHECK(panel_toggle(base, 1) == 0);
    CHECK(leds_of(base) == 0x7);
    CHECK(panel_toggle(base, 1) == 0);
    CHECK(leds_of(base) == 0x5);
    CHECK(panel_toggle(base, 0) == 0);
    CHECK(leds_of(base) == 0x4);
}

static void check_toggle_range(void) {
    uint32_t *base = board_with_leds(0x5);

    CHECK(panel_toggle(base, -1) == -1);
    CHECK(panel_toggle(base, 32) == -1);
    CHECK(leds_of(base) == 0x5);
    CHECK(panel_toggle(base, 31) == 0);
    CHECK(leds_of(base) == 0x80000005u);
}

static void check_toggle_failed_access(void) {
    uint32_t *base = board_with_leds(0x5);

    CHECK(panel_toggle(base + 1, 32) == -1);
    CHECK(panel_toggle(base + 1, 1) == 1);
    CHECK(leds_of(base) == 0x5);
}

/**
 * Sets the sensor to temp and rh, and reads them back through sensor_read.
 *
 * @return true if the read succeeds and gives the two expected values.
 */
static bool reads_back(float temp, float rh, float want_temp, float want_rh) {
    uint32_t *base = iom361_initialize(4, 4, NULL);
    reading_t r;

    _iom361_set_sensor1(temp, rh);
    if (sensor_read(base, 0, &r) != 0) return false;
    return r.temp_c == want_temp && r.rh_pct == want_rh;
}

static void check_sensor_rounds_to_tenths(void) {
    /* The register gives back 90.99998, which is below a 91.0 threshold. */
    CHECK(reads_back(91.0f, 60.0f, 91.0f, 60.0f));
    CHECK(reads_back(21.5f, 40.0f, 21.5f, 40.0f));
    CHECK(reads_back(21.54f, 40.06f, 21.5f, 40.1f));
    CHECK(reads_back(-12.3f, 0.3f, -12.3f, 0.3f));
    CHECK(reads_back(-12.36f, 0.0f, -12.4f, 0.0f));
}

static void check_sensor_range_ends(void) {
    CHECK(reads_back(-50.0f, 0.0f, -50.0f, 0.0f));
    /* The registers top out just under these, and rounding brings them back. */
    CHECK(reads_back(150.0f, 100.0f, 150.0f, 100.0f));
}

static void check_sensor_after_initialize(void) {
    uint32_t *base = iom361_initialize(4, 4, NULL);
    reading_t r;

    CHECK(sensor_read(base, 0, &r) == 0);
    CHECK(r.temp_c == 23.5f);
    CHECK(r.rh_pct == 75.0f);
}

static void check_sensor_timestamp(void) {
    uint32_t *base = iom361_initialize(4, 4, NULL);
    reading_t r;

    CHECK(sensor_read(base, 1791273602, &r) == 0);
    CHECK(r.t == 1791273602);
}

static void check_sensor_no_negative_zero(void) {
    uint32_t *base = iom361_initialize(4, 4, NULL);
    reading_t r;

    _iom361_set_sensor1(-0.04f, 50.0f);
    CHECK(sensor_read(base, 0, &r) == 0);
    /* -0.0 == 0.0 is true, so only the sign bit tells them apart. It matters
     * because a trace would print -0.0. */
    CHECK(r.temp_c == 0.0f);
    CHECK(!signbit(r.temp_c));
}

static void check_sensor_keeps_bits_19_to_0(void) {
    uint32_t *base = iom361_initialize(4, 4, NULL);
    reading_t r;

    _iom361_set_sensor1(91.0f, 60.0f);
    /* Directly, because _iom361_set_sensor1 can't set these bits and a write
     * to a read-only register is ignored. */
    base[TEMP_REG / sizeof *base] |= 0xFFF00000u;
    base[HUMID_REG / sizeof *base] |= 0xFFF00000u;
    CHECK(sensor_read(base, 0, &r) == 0);
    CHECK(r.temp_c == 91.0f);
    CHECK(r.rh_pct == 60.0f);
}

static void check_sensor_failed_read(void) {
    uint32_t *base = iom361_initialize(4, 4, NULL);
    reading_t r = { .t = 7, .temp_c = 1.5f, .rh_pct = 2.5f };

    CHECK(sensor_read(base + 1, 1791273602, &r) == 1);
    CHECK(r.t == 7);
    CHECK(r.temp_c == 1.5f);
    CHECK(r.rh_pct == 2.5f);
}

int main(void) {
    /* The LED display would print over the PASS and FAIL lines. */
    iom361_set_display(false);

    /* panel */
    check_pressed_and_released();
    check_led_keeps_other_leds();
    check_led_already_in_state();
    check_led_any_nonzero_is_on();
    check_led_range();
    check_led_failed_access();
    check_toggle_keeps_other_leds();
    check_toggle_range();
    check_toggle_failed_access();

    /* sensor */
    check_sensor_rounds_to_tenths();
    check_sensor_range_ends();
    check_sensor_after_initialize();
    check_sensor_timestamp();
    check_sensor_no_negative_zero();
    check_sensor_keeps_bits_19_to_0();
    check_sensor_failed_read();

    printf("%d of %d checks passed\n", total - fails, total);

    return fails != 0;
}
