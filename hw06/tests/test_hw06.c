/*
 * test_hw06.c:
 *
 * Tests for hw06. Prints PASS or FAIL for each check, then a summary; exits
 * nonzero if any check failed.
 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "iom361_r4.h"
#include "panel.h"
#include "scenario.h"
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
    /* a press, a release and a held switch at once */
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
    /* any nonzero on counts as on */
    CHECK(panel_led(base, 3, 5) == 0);
    CHECK(leds_of(base) == 0xE);
}

static void check_led_already_in_state(void) {
    uint32_t *base = board_with_leds(0x5);

    CHECK(panel_led(base, 0, 1) == 0);
    CHECK(leds_of(base) == 0x5);
    CHECK(panel_led(base, 1, 0) == 0);
    CHECK(leds_of(base) == 0x5);
}

static void check_led_top_bit(void) {
    uint32_t *base = board_with_leds(0x0);

    CHECK(panel_led(base, 31, 1) == 0);
    CHECK(leds_of(base) == 0x80000000u);
}

static void check_led_out_of_range(void) {
    uint32_t *base = board_with_leds(0x5);

    CHECK(panel_led(base, -1, 1) == -1);
    CHECK(panel_led(base, 32, 1) == -1);
    CHECK(leds_of(base) == 0x5);
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

static void check_sensor_rounds_to_tenths(void) {
    uint32_t *base = iom361_initialize(4, 4, NULL);
    reading_t r;

    /* the register holds 90.99998: unrounded, this would sit below 91.0 */
    _iom361_set_sensor1(91.0f, 40.0f);
    CHECK(sensor_read(base, 1000, &r) == 0);
    CHECK(r.t == 1000);
    CHECK(r.temp_c == 91.0f);
    CHECK(r.rh_pct == 40.0f);

    /* below zero, where a cast to int would round the wrong way */
    _iom361_set_sensor1(-12.3f, 40.0f);
    CHECK(sensor_read(base, 1000, &r) == 0);
    CHECK(r.temp_c == -12.3f);
}

static void check_sensor_keeps_bits_19_to_0(void) {
    uint32_t *base = iom361_initialize(4, 4, NULL);
    reading_t r;

    _iom361_set_sensor1(91.0f, 40.0f);
    /* Directly, because _iom361_set_sensor1 can't set these bits and a write
       to a read-only register is ignored. */
    base[TEMP_REG / sizeof *base] |= 0xFFF00000u;
    base[HUMID_REG / sizeof *base] |= 0xFFF00000u;
    CHECK(sensor_read(base, 1000, &r) == 0);
    CHECK(r.temp_c == 91.0f);
    CHECK(r.rh_pct == 40.0f);
}

static void check_sensor_failed_read(void) {
    uint32_t *base = iom361_initialize(4, 4, NULL);
    reading_t r = {.t = 1000, .temp_c = 1.5f, .rh_pct = 2.5f};

    /* not the address iom361_initialize returned, so every read fails */
    CHECK(sensor_read(base + 1, 2000, &r) == 1);
    CHECK(r.t == 1000);
    CHECK(r.temp_c == 1.5f);
    CHECK(r.rh_pct == 2.5f);
}

/**
 * Loads text as if it were a scenario file.
 *
 * @return what scenario_load returns, or -1 if the temporary file won't open.
 */
static int load(const char *text, scenario_t *s) {
    FILE *fp = tmpfile();
    if (fp == NULL) return -1;

    fputs(text, fp);
    rewind(fp);
    int rc = scenario_load(fp, s);
    fclose(fp);
    return rc;
}

static void check_load_comments_and_blanks(void) {
    scenario_t s;
    int rc = load("# a comment\n"
                  "\n"
                  "start 1000   # trailing\n"
                  "   # indented\n"
                  "0 21.5 40 0x0\n"
                  "\n"
                  "1 -12.5 40.5 0x6  # trailing\n"
                  "2 22 41 a\n", &s);

    CHECK(rc == 0);
    CHECK(s.start == 1000);
    CHECK(s.count == 3);
    CHECK(s.lines[0].tick == 0);
    CHECK(s.lines[0].temp_c == 21.5f);
    CHECK(s.lines[0].rh_pct == 40.0f);
    CHECK(s.lines[0].switches == 0x0);
    CHECK(s.lines[1].tick == 1);
    CHECK(s.lines[1].temp_c == -12.5f);
    CHECK(s.lines[1].rh_pct == 40.5f);
    CHECK(s.lines[1].switches == 0x6);
    /* hex without the 0x */
    CHECK(s.lines[2].switches == 0xA);
}

static void check_load_no_start_line(void) {
    scenario_t s;

    CHECK(load("0 21.5 40 0x0\n", &s) == 1);
    /* the number counts every line in the file, comments too */
    CHECK(load("# a comment\n\n0 21.5 40 0x0\n", &s) == 3);
    /* the start line comes too late */
    CHECK(load("0 21.5 40 0x0\nstart 1000\n", &s) == 1);
}

static void check_load_start_range(void) {
    scenario_t s;

    /* both ends are good */
    CHECK(load("start 0\n0 21.5 40 0x0\n", &s) == 0);
    CHECK(load("start 253402300799\n0 21.5 40 0x0\n", &s) == 0);
    CHECK(s.start == 253402300799LL);
    /* one past each end */
    CHECK(load("start -1\n0 21.5 40 0x0\n", &s) == 1);
    CHECK(load("start 253402300800\n0 21.5 40 0x0\n", &s) == 1);
    /* these would make gmtime return NULL */
    CHECK(load("start 99999999999999999\n0 21.5 40 0x0\n", &s) == 1);
    CHECK(load("start -99999999999999999\n0 21.5 40 0x0\n", &s) == 1);
    /* the line number counts every line */
    CHECK(load("# a comment\nstart 253402300800\n0 21.5 40 0x0\n", &s) == 2);
}

/**
 * Loads a good start line and data line, then a comment of length characters,
 * then eol and after. The comment is line 3.
 */
static int load_with_comment(int length, const char *eol, const char *after,
                             scenario_t *s) {
    char comment[400];
    char text[600];

    memset(comment, 'x', sizeof comment);
    comment[0] = '#';
    comment[length] = '\0';
    snprintf(text, sizeof text, "start 1000\n0 21.5 40 0x0\n%s%s%s", comment, eol, after);
    return load(text, s);
}

static void check_load_long_lines(void) {
    scenario_t s;
    char text[600];

    /* the longest line, with each line ending and with none */
    CHECK(load_with_comment(255, "\n", "", &s) == 0);
    CHECK(load_with_comment(255, "\r\n", "", &s) == 0);
    CHECK(load_with_comment(255, "", "", &s) == 0);
    /* it stays one line, so the next line's number is right */
    CHECK(load_with_comment(255, "\n", "hello\n", &s) == 4);
    CHECK(load_with_comment(255, "\r\n", "hello\n", &s) == 4);
    /* one character too many, with each line ending and with none */
    CHECK(load_with_comment(256, "\n", "", &s) == 3);
    CHECK(load_with_comment(256, "\r\n", "", &s) == 3);
    CHECK(load_with_comment(256, "", "", &s) == 3);
    /* far too long: its tail is not a line of its own */
    CHECK(load_with_comment(300, "\n", "", &s) == 3);
    /* a tail that would read as data */
    CHECK(load_with_comment(257, " 7 99 99 0x1\n", "", &s) == 3);
    /* a data line too */
    snprintf(text, sizeof text, "start 1000\n%-300s\n", "0 21.5 40 0x0");
    CHECK(load(text, &s) == 2);
}

/**
 * Loads a start line, then count data lines at ticks 0, 1, 2 and so on, then
 * after.
 *
 * @return what scenario_load returns, or -1 if the temporary file won't open.
 */
static int load_data_lines(int count, const char *after, scenario_t *s) {
    FILE *fp = tmpfile();
    if (fp == NULL) return -1;

    fputs("start 1000\n", fp);
    for (int tick = 0; tick < count; tick++) {
        fprintf(fp, "%d 21.5 40 0x0\n", tick);
    }
    fputs(after, fp);
    rewind(fp);
    int rc = scenario_load(fp, s);
    fclose(fp);
    return rc;
}

static void check_load_capacity(void) {
    scenario_t s;

    /* exactly full, and the last line is really there */
    CHECK(load_data_lines(SCENARIO_MAX_LINES, "", &s) == 0);
    CHECK(s.count == SCENARIO_MAX_LINES);
    CHECK(s.lines[SCENARIO_MAX_LINES - 1].tick == SCENARIO_MAX_LINES - 1);
    /* comments and blank lines take no room */
    CHECK(load_data_lines(SCENARIO_MAX_LINES, "# a comment\n\n", &s) == 0);
    /* one too many: the start line is line 1, so this is line 1002 */
    CHECK(load_data_lines(SCENARIO_MAX_LINES + 1, "", &s) == 1002);
}

static void check_load_wrong_fields(void) {
    scenario_t s;

    CHECK(load("start 1000\n0 21.5 40\n", &s) == 2);
    CHECK(load("start 1000\n0 21.5 40 0x0 POWER\n", &s) == 2);
    CHECK(load("start 1000\nhello\n", &s) == 2);
    /* the first wrong line, not the last */
    CHECK(load("start 1000\n0 21.5 40 0x0\n1 21.5\n2 21.5\n", &s) == 3);
}

static void check_load_ticks_must_increase(void) {
    scenario_t s;

    /* the same tick twice */
    CHECK(load("start 1000\n0 21.5 40 0x0\n0 21.5 40 0x0\n", &s) == 3);
    /* going backward */
    CHECK(load("start 1000\n0 21.5 40 0x0\n5 21.5 40 0x0\n3 21.5 40 0x0\n", &s) == 4);
    /* a gap is fine */
    CHECK(load("start 1000\n0 21.5 40 0x0\n5 21.5 40 0x0\n", &s) == 0);
}

static void check_load_first_tick_is_zero(void) {
    scenario_t s;

    CHECK(load("start 1000\n5 21.5 40 0x0\n", &s) == 2);
    CHECK(load("start 1000\n-1 21.5 40 0x0\n", &s) == 2);
}

static void check_load_no_data(void) {
    scenario_t s;

    CHECK(load("", &s) == 1);
    CHECK(load("# only a comment\n", &s) == 2);
    CHECK(load("start 1000\n", &s) == 2);
}

static void check_values_hold_between_lines(void) {
    scenario_t s;

    CHECK(load("start 1000\n"
               "0 21.5 40 0x0\n"
               "5 22.5 41 0x1\n"
               "9 23.5 42 0x2\n", &s) == 0);
    CHECK(scenario_at(&s, 0)->tick == 0);
    CHECK(scenario_at(&s, 4)->tick == 0);
    CHECK(scenario_at(&s, 4)->temp_c == 21.5f);
    CHECK(scenario_at(&s, 5)->tick == 5);
    CHECK(scenario_at(&s, 8)->tick == 5);
    CHECK(scenario_at(&s, 8)->switches == 0x1);
    CHECK(scenario_at(&s, 9)->tick == 9);
}

int main(void) {
    iom361_set_display(false);

    check_pressed_and_released();
    check_led_keeps_other_leds();
    check_led_already_in_state();
    check_led_top_bit();
    check_led_out_of_range();
    check_toggle_keeps_other_leds();
    check_toggle_top_bit();
    check_toggle_out_of_range();
    check_sensor_rounds_to_tenths();
    check_sensor_keeps_bits_19_to_0();
    check_sensor_failed_read();
    check_load_comments_and_blanks();
    check_load_no_start_line();
    check_load_start_range();
    check_load_long_lines();
    check_load_capacity();
    check_load_wrong_fields();
    check_load_ticks_must_increase();
    check_load_first_tick_is_zero();
    check_load_no_data();
    check_values_hold_between_lines();

    printf("%d of %d checks passed\n", total - fails, total);

    return fails != 0;
}
