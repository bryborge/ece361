/*
 * test_hw06.c:
 *
 * Tests for panel.c, sensor.c and scenario.c. Prints PASS or FAIL for each
 * check, then a summary; exits nonzero if any check failed.
 */
#include <assert.h>
#include <limits.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

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

/**
 * Loads `text` as if it were a scenario file.
 *
 * @return what scenario_load returns.
 */
static int load(const char *text, scenario_t *s) {
    FILE *fp = tmpfile();

    assert(fp != NULL);
    fputs(text, fp);
    rewind(fp);
    int result = scenario_load(fp, s);
    fclose(fp);
    return result;
}

static bool line_is(const scenario_line_t *line, int tick, float temp, float rh,
                    uint32_t switches) {
    return line->tick == tick && line->temp_c == temp && line->rh_pct == rh
        && line->switches == switches;
}

static void check_load_good_file(void) {
    scenario_t s;

    CHECK(load("start 1791273600\n"
               "0 15.0 60 0x0\n"
               "5 16.5 61.5 0xA\n", &s) == 0);
    CHECK(s.start == 1791273600);
    CHECK(s.count == 2);
    CHECK(line_is(&s.lines[0], 0, 15.0f, 60.0f, 0x0));
    CHECK(line_is(&s.lines[1], 5, 16.5f, 61.5f, 0xA));
}

static void check_load_comments_and_blanks(void) {
    scenario_t s;

    CHECK(load("# a comment before start\n"
               "\n"
               "start 1791273600    # with a trailing comment\n"
               "   # an indented comment\n"
               "   \t \n"
               "# tick temp rh switches\n"
               "0 15.0 60 0x0       # with a trailing comment\n"
               "\n"
               "5 16.0 60 0x1\n"
               "# a comment at the end\n", &s) == 0);
    CHECK(s.start == 1791273600);
    CHECK(s.count == 2);
    CHECK(line_is(&s.lines[0], 0, 15.0f, 60.0f, 0x0));
    CHECK(line_is(&s.lines[1], 5, 16.0f, 60.0f, 0x1));
}

static void check_load_accepted_forms(void) {
    scenario_t s;

    /* CRLF line endings */
    CHECK(load("start 0\r\n0 15 60 0x1\r\n", &s) == 0);
    CHECK(line_is(&s.lines[0], 0, 15.0f, 60.0f, 0x1));
    /* hex without 0x, in either case */
    CHECK(load("start 0\n0 15 60 ff\n1 15 60 FF\n", &s) == 0);
    CHECK(s.lines[0].switches == 0xFF);
    CHECK(s.lines[1].switches == 0xFF);
    /* no line ending on the last line */
    CHECK(load("start 0\n0 15 60 0x1", &s) == 0);
    CHECK(s.count == 1);
    /* bits above the board's switches: only iom361 knows the switch count */
    CHECK(load("start 0\n0 15 60 0xFFFFFFFF\n", &s) == 0);
    CHECK(s.lines[0].switches == 0xFFFFFFFFu);
}

static void check_load_no_start_line(void) {
    scenario_t s;

    CHECK(load("0 15 60 0x0\n", &s) == 1);
    CHECK(load("# a comment\n\n0 15 60 0x0\n", &s) == 3);
}

static void check_load_bad_start_line(void) {
    scenario_t s;

    CHECK(load("start\n0 15 60 0x0\n", &s) == 1);
    CHECK(load("start abc\n0 15 60 0x0\n", &s) == 1);
    CHECK(load("start 5 6\n0 15 60 0x0\n", &s) == 1);
    CHECK(load("start 5abc\n0 15 60 0x0\n", &s) == 1);
    CHECK(load("start5\n0 15 60 0x0\n", &s) == 1);
    CHECK(load("starts 5\n0 15 60 0x0\n", &s) == 1);
    CHECK(load("START 5\n0 15 60 0x0\n", &s) == 1);
    /* a second start */
    CHECK(load("start 5\n0 15 60 0x0\nstart 6\n", &s) == 3);
    CHECK(load("start 5\nstart 6\n0 15 60 0x0\n", &s) == 2);
}

static void check_load_start_range(void) {
    scenario_t s;

    CHECK(load("start 0\n0 15 60 0x0\n", &s) == 0);
    CHECK(s.start == 0);
    CHECK(load("start -1\n0 15 60 0x0\n", &s) == 1);
    /* 9999-12-31 23:59:59 UTC, the last one */
    CHECK(load("start 253402300799\n0 15 60 0x0\n", &s) == 0);
    CHECK(s.start == 253402300799);
    CHECK(load("start 253402300800\n0 15 60 0x0\n", &s) == 1);
}

static void check_load_wrong_fields(void) {
    scenario_t s;

    /* a missing field, on the line after a comment to pin the line number */
    CHECK(load("start 0\n# a comment\n0 15 60\n", &s) == 3);
    CHECK(load("start 0\n0 15 60 0x0\n1 15\n", &s) == 3);
    /* an extra field */
    CHECK(load("start 0\n0 15 60 0x0 POWER\n", &s) == 2);
    CHECK(load("start 0\n0 15 60 0x0 1\n", &s) == 2);
    /* text that does not parse */
    CHECK(load("start 0\nabc 15 60 0x0\n", &s) == 2);
    CHECK(load("start 0\n0abc 15 60 0x0\n", &s) == 2);
    CHECK(load("start 0\n0 15.0abc 60 0x0\n", &s) == 2);
    CHECK(load("start 0\n0 15 60abc 0x0\n", &s) == 2);
    CHECK(load("start 0\n0 15 60 0xZ\n", &s) == 2);
    CHECK(load("start 0\n0 15 60 0x1z\n", &s) == 2);
}

static void check_load_ticks(void) {
    scenario_t s;

    /* the first tick must be 0 */
    CHECK(load("start 0\n5 15 60 0x0\n", &s) == 2);
    CHECK(load("start 0\n-1 15 60 0x0\n", &s) == 2);
    /* increasing */
    CHECK(load("start 0\n0 15 60 0x0\n0 15 60 0x0\n", &s) == 3);
    CHECK(load("start 0\n0 15 60 0x0\n5 15 60 0x0\n5 15 60 0x0\n", &s) == 4);
    CHECK(load("start 0\n0 15 60 0x0\n5 15 60 0x0\n4 15 60 0x0\n", &s) == 4);
    CHECK(load("start 0\n0 15 60 0x0\n-1 15 60 0x0\n", &s) == 3);
    /* the largest int is accepted; the replay's loop has to stop safely at it */
    CHECK(load("start 0\n0 15 60 0x0\n2147483647 15 60 0x0\n", &s) == 0);
    CHECK(s.lines[1].tick == INT_MAX);
}

static void check_load_value_ranges(void) {
    scenario_t s;

    CHECK(load("start 0\n0 -50 0 0x0\n1 150 100 0x0\n", &s) == 0);
    CHECK(line_is(&s.lines[0], 0, -50.0f, 0.0f, 0x0));
    CHECK(line_is(&s.lines[1], 1, 150.0f, 100.0f, 0x0));
    CHECK(load("start 0\n0 -50.1 60 0x0\n", &s) == 2);
    CHECK(load("start 0\n0 150.1 60 0x0\n", &s) == 2);
    CHECK(load("start 0\n0 15 -0.1 0x0\n", &s) == 2);
    CHECK(load("start 0\n0 15 100.1 0x0\n", &s) == 2);
    CHECK(load("start 0\n0 nan 60 0x0\n", &s) == 2);
    CHECK(load("start 0\n0 inf 60 0x0\n", &s) == 2);
    CHECK(load("start 0\n0 15 nan 0x0\n", &s) == 2);
    CHECK(load("start 0\n0 15 inf 0x0\n", &s) == 2);
}

static void check_load_nothing_to_point_at(void) {
    scenario_t s;

    CHECK(load("", &s) == 1);
    CHECK(load("\n", &s) == 2);
    CHECK(load("# a comment\n# another\n", &s) == 3);
    CHECK(load("# no line ending", &s) == 2);
    CHECK(load("start 0\n", &s) == 2);
    CHECK(load("start 0\n# a comment\n", &s) == 3);
}

/**
 * Loads a file of a start line and `count` data lines, with ticks 0, 1, 2...
 *
 * @return what scenario_load returns.
 */
static int load_data_lines(int count, scenario_t *s) {
    FILE *fp = tmpfile();

    assert(fp != NULL);
    fputs("start 0\n", fp);
    for (int i = 0; i < count; i++) {
        fprintf(fp, "%d 15 60 0x0\n", i);
    }
    rewind(fp);
    int result = scenario_load(fp, s);
    fclose(fp);
    return result;
}

static void check_load_capacity(void) {
    scenario_t s;

    CHECK(load_data_lines(SCENARIO_MAX_LINES, &s) == 0);
    CHECK(s.count == SCENARIO_MAX_LINES);
    CHECK(s.lines[SCENARIO_MAX_LINES - 1].tick == SCENARIO_MAX_LINES - 1);
    /* Data line 1001 is line 1002 of the file, after the start line. */
    CHECK(load_data_lines(SCENARIO_MAX_LINES + 1, &s) == SCENARIO_MAX_LINES + 2);
}

/**
 * Loads a good scenario with a comment line of exactly `len` characters as its
 * second line, followed by `ending`, and then by `rest`.
 *
 * @return what scenario_load returns.
 */
static int load_long_line(int len, const char *ending, const char *rest,
                          scenario_t *s) {
    FILE *fp = tmpfile();

    assert(fp != NULL);
    fputs("start 0\n", fp);
    for (int i = 0; i < len; i++) {
        fputc('#', fp);
    }
    fputs(ending, fp);
    fputs(rest, fp);
    rewind(fp);
    int result = scenario_load(fp, s);
    fclose(fp);
    return result;
}

static void check_load_long_lines(void) {
    const char *data = "0 15 60 0x0\n";
    scenario_t s;

    CHECK(load_long_line(SCENARIO_MAX_LINE_LEN, "\n", data, &s) == 0);
    CHECK(s.count == 1);
    CHECK(load_long_line(SCENARIO_MAX_LINE_LEN, "\r\n", data, &s) == 0);
    /* The line after the longest line keeps its number. A buffer too short for
     * the CRLF would split the long line in two, and this would give 4. */
    CHECK(load_long_line(SCENARIO_MAX_LINE_LEN, "\n", "abc\n", &s) == 3);
    CHECK(load_long_line(SCENARIO_MAX_LINE_LEN, "\r\n", "abc\n", &s) == 3);
    CHECK(load_long_line(SCENARIO_MAX_LINE_LEN + 1, "\n", data, &s) == 2);
    CHECK(load_long_line(1000, "\n", data, &s) == 2);
    /* At the very end of the file there is no line ending, and no data. */
    CHECK(load_long_line(SCENARIO_MAX_LINE_LEN, "", "", &s) == 3);
    CHECK(load_long_line(SCENARIO_MAX_LINE_LEN + 1, "", "", &s) == 2);
}

static void check_line_in_force(void) {
    scenario_t s;

    CHECK(load("start 0\n"
               "0 15 60 0x0\n"
               "5 16 61 0x1\n"
               "6 17 62 0x0\n"
               "60 18 63 0x2\n", &s) == 0);
    /* on a line */
    CHECK(scenario_at(&s, 0)->tick == 0);
    CHECK(scenario_at(&s, 5)->tick == 5);
    CHECK(scenario_at(&s, 6)->tick == 6);
    CHECK(scenario_at(&s, 60)->tick == 60);
    /* between two lines: the values hold */
    CHECK(line_is(scenario_at(&s, 1), 0, 15.0f, 60.0f, 0x0));
    CHECK(line_is(scenario_at(&s, 4), 0, 15.0f, 60.0f, 0x0));
    CHECK(line_is(scenario_at(&s, 7), 6, 17.0f, 62.0f, 0x0));
    CHECK(line_is(scenario_at(&s, 59), 6, 17.0f, 62.0f, 0x0));
    /* past the last line */
    CHECK(line_is(scenario_at(&s, 61), 60, 18.0f, 63.0f, 0x2));
    CHECK(line_is(scenario_at(&s, INT_MAX), 60, 18.0f, 63.0f, 0x2));
}

static void check_line_in_force_one_line(void) {
    scenario_t s;

    CHECK(load("start 0\n0 15 60 0x1\n", &s) == 0);
    CHECK(scenario_at(&s, 0)->tick == 0);
    CHECK(scenario_at(&s, 100)->tick == 0);
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

    /* scenario */
    check_load_good_file();
    check_load_comments_and_blanks();
    check_load_accepted_forms();
    check_load_no_start_line();
    check_load_bad_start_line();
    check_load_start_range();
    check_load_wrong_fields();
    check_load_ticks();
    check_load_value_ranges();
    check_load_nothing_to_point_at();
    check_load_capacity();
    check_load_long_lines();
    check_line_in_force();
    check_line_in_force_one_line();

    printf("%d of %d checks passed\n", total - fails, total);

    return fails != 0;
}
