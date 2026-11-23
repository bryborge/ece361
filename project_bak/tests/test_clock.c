/*
 * test_clock.c:
 *
 * Tests for clock.c. Prints PASS or FAIL for each check; test_main.c runs them
 * and prints the summary.
 */
#include <limits.h>
#include <stdbool.h>
#include <string.h>

#include "check.h"
#include "clock.h"

static bool formats_as(time_t t, const char *want) {
    char text[CLOCK_TEXT_SIZE];

    return clock_format(t, text) == 0 && strcmp(text, want) == 0;
}

static void check_clock_start_prints_as_given(void) {
    CHECK(formats_as(1791273600, "2026-10-06 08:00:00"));
}

static void check_clock_59_seconds_later(void) {
    CHECK(formats_as(1791273600 + 59, "2026-10-06 08:00:59"));
}

static void check_clock_midnight(void) {
    CHECK(formats_as(1791331199, "2026-10-06 23:59:59"));
    CHECK(formats_as(1791331199 + 1, "2026-10-07 00:00:00"));
}

static void check_clock_end_of_february(void) {
    CHECK(formats_as(1803859199, "2027-02-28 23:59:59"));
    CHECK(formats_as(1803859199 + 1, "2027-03-01 00:00:00"));
    /* 2028 is a leap year */
    CHECK(formats_as(1835395199, "2028-02-28 23:59:59"));
    CHECK(formats_as(1835395199 + 1, "2028-02-29 00:00:00"));
}

static void check_clock_year_after_9999(void) {
    /* scenario_load caps start at the end of 9999, and a tick can pass it. */
    CHECK(formats_as(253402300799, "9999-12-31 23:59:59"));
    CHECK(formats_as(253402300799 + 1, "10000-01-01 00:00:00"));
}

static void check_clock_too_far_ahead(void) {
    char text[CLOCK_TEXT_SIZE] = "unchanged";

    /* The year would not fit gmtime's int, so gmtime gives NULL. */
    CHECK(clock_format(LLONG_MAX, text) == -1);
    CHECK(strcmp(text, "unchanged") == 0);
}

void run_clock_tests(void) {
    check_clock_start_prints_as_given();
    check_clock_59_seconds_later();
    check_clock_midnight();
    check_clock_end_of_february();
    check_clock_year_after_9999();
    check_clock_too_far_ahead();
}
