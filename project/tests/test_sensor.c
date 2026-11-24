/*
 * test_sensor.c:
 *
 * Tests for sensor.c. Prints PASS or FAIL for each check; test_main.c runs
 * them and prints the summary.
 */
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#include "check.h"
#include "iom361_r4.h"
#include "sensor.h"

/**
 * Starts each check with a fresh iom361.
 *
 * @return The base address of its register block.
 */
static uint32_t *start(void) {
    int rc;
    uint32_t *base = iom361_initialize(2, 2, &rc);

    assert(base != NULL);
    return base;
}

/**
 * sensor_read() fills every field of type reading.
 */
static void check_sensor_read_fills_reading_at_0_c(void) {
    uint32_t *base = start();
    const time_t now = 1791273600;  // 2026-10-06 08:00:00 UTC

    _iom361_set_sensor1(0.0f, 50.0f);

    reading_t reading;
    int rc = sensor_read(base, now, &reading);

    CHECK(rc == 0);
    CHECK(reading.timestamp == now);
    CHECK(reading.temp_c == 0.0f);
    CHECK(reading.rh_pct == 50.0f);
}

/**
 * sensor_read() rounds to 0.1, so a reading set to exactly the freeze-guard
 * threshold (1.0 C) compares equal to it.
 */
static void check_sensor_read_rounds_at_freeze_threshold(void) {
    uint32_t *base = start();

    _iom361_set_sensor1(1.0f, 60.0f);

    reading_t reading;
    int rc = sensor_read(base, 0, &reading);

    CHECK(rc == 0);
    CHECK(reading.temp_c == 1.0f);
    CHECK(reading.rh_pct == 60.0f);
}

/**
 * sensor_read() converts correctly at 100 C, far enough from 0 C that an
 * error in the scale factor is not hidden by the rounding.
 */
static void check_sensor_read_converts_at_100_c(void) {
    uint32_t *base = start();

    _iom361_set_sensor1(100.0f, 25.0f);

    reading_t reading;
    int rc = sensor_read(base, 0, &reading);

    CHECK(rc == 0);
    CHECK(reading.temp_c == 100.0f);
    CHECK(reading.rh_pct == 25.0f);
}

/**
 * sensor_read() returns the iom361 code of a failed register read and leaves
 * the reading untouched.
 */
static void check_sensor_read_fails_on_wrong_base(void) {
    start();

    uint32_t not_iom361 = 0;
    reading_t reading = {.timestamp = 1, .temp_c = -1.0f, .rh_pct = -1.0f};

    int rc = sensor_read(&not_iom361, 2, &reading);

    CHECK(rc == 1);  // iom361: wrong base address
    CHECK(reading.timestamp == 1);
    CHECK(reading.temp_c == -1.0f);
    CHECK(reading.rh_pct == -1.0f);
}

/**
 * sensor_format_timestamp() prints the start time as given.
 */
static void check_sensor_format_timestamp_start_time(void) {
    const time_t start = 1791273600;  // 2026-10-06 08:00:00 UTC

    char text[20];
    int rc = sensor_format_timestamp(start, text);

    CHECK(rc == 0);
    CHECK(strcmp(text, "2026-10-06 08:00:00") == 0);
}

/**
 * sensor_format_timestamp() counts seconds: 59 seconds after the start time
 * is still within the same minute.
 */
static void check_sensor_format_timestamp_59_seconds_later(void) {
    const time_t start = 1791273600;  // 2026-10-06 08:00:00 UTC

    char text[20];
    int rc = sensor_format_timestamp(start + 59, text);

    CHECK(rc == 0);
    CHECK(strcmp(text, "2026-10-06 08:00:59") == 0);
}

/**
 * sensor_format_timestamp() changes the date at midnight: one second after
 * 23:59:59 is 00:00:00 on the next day.
 */
static void check_sensor_format_timestamp_rolls_over_at_midnight(void) {
    const time_t last_second = 1791331199;  // 2026-10-06 23:59:59 UTC

    char text[20];

    CHECK(sensor_format_timestamp(last_second, text) == 0);
    CHECK(strcmp(text, "2026-10-06 23:59:59") == 0);

    CHECK(sensor_format_timestamp(last_second + 1, text) == 0);
    CHECK(strcmp(text, "2026-10-07 00:00:00") == 0);
}

/**
 * sensor_format_timestamp() knows that 2027 is not a leap year: one second
 * after the end of 28 February is 1 March.
 */
static void check_sensor_format_timestamp_rolls_over_to_march(void) {
    const time_t last_second = 1803859199;  // 2027-02-28 23:59:59 UTC

    char text[20];

    CHECK(sensor_format_timestamp(last_second, text) == 0);
    CHECK(strcmp(text, "2027-02-28 23:59:59") == 0);

    CHECK(sensor_format_timestamp(last_second + 1, text) == 0);
    CHECK(strcmp(text, "2027-03-01 00:00:00") == 0);
}

void run_sensor_tests(void) {
    check_sensor_read_fills_reading_at_0_c();
    check_sensor_read_rounds_at_freeze_threshold();
    check_sensor_read_converts_at_100_c();
    check_sensor_read_fails_on_wrong_base();
    check_sensor_format_timestamp_start_time();
    check_sensor_format_timestamp_59_seconds_later();
    check_sensor_format_timestamp_rolls_over_at_midnight();
    check_sensor_format_timestamp_rolls_over_to_march();
}
