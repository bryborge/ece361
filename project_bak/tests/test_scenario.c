/*
 * test_scenario.c:
 *
 * Tests for scenario.c: loading a file, and the line in force at a tick. Prints
 * PASS or FAIL for each check; test_main.c runs them and prints the summary.
 */
#include <assert.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "check.h"
#include "scenario.h"

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

void run_scenario_tests(void) {
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
}
