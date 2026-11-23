/**
 * @file test_sensor.c
 * @brief Unit tests for the sensor module.
 */
#include <stdint.h>
#include <time.h>

#include "iom361_r4.h"
#include "unity.h"

#include "sensor.h"

static uint32_t *base;

void setUp(void) {
    int rc;
    base = iom361_initialize(2, 2, &rc);

    TEST_ASSERT_NOT_NULL(base);
}

void tearDown(void) {}

/**
 * sensor_read() fills every field of type reading.
 */
static void test_sensor_read_fills_reading_at_0_c(void) {
    const time_t now = 1791273600;  // 2026-10-06 08:00:00 UTC

    _iom361_set_sensor1(0.0f, 50.0f);

    reading_t reading;
    int rc = sensor_read(base, now, &reading);

    TEST_ASSERT_EQUAL_INT(0, rc);
    TEST_ASSERT_EQUAL_INT64(now, reading.timestamp);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, reading.temp_c);
    TEST_ASSERT_EQUAL_FLOAT(50.0f, reading.rh_pct);
}

/**
 * sensor_read() rounds to 0.1, so a reading set to exactly the freeze-guard
 * threshold (1.0 C) compares equal to it.
 */
static void test_sensor_read_rounds_at_freeze_threshold(void) {
    _iom361_set_sensor1(1.0f, 60.0f);

    reading_t reading;
    int rc = sensor_read(base, 0, &reading);

    TEST_ASSERT_EQUAL_INT(0, rc);
    TEST_ASSERT_FLOAT_WITHIN(0.0f, 1.0f, reading.temp_c);
    TEST_ASSERT_FLOAT_WITHIN(0.0f, 60.0f, reading.rh_pct);
}

/**
 * sensor_read() converts correctly at 100 C, far enough from 0 C that an
 * error in the scale factor is not hidden by the rounding.
 */
static void test_sensor_read_converts_at_100_c(void) {
    _iom361_set_sensor1(100.0f, 25.0f);

    reading_t reading;
    int rc = sensor_read(base, 0, &reading);

    TEST_ASSERT_EQUAL_INT(0, rc);
    TEST_ASSERT_EQUAL_FLOAT(100.0f, reading.temp_c);
    TEST_ASSERT_EQUAL_FLOAT(25.0f, reading.rh_pct);
}

/**
 * sensor_read() returns the iom361 code of a failed register read and leaves
 * the reading untouched.
 */
static void test_sensor_read_fails_on_wrong_base(void) {
    uint32_t not_iom361 = 0;
    reading_t reading = {.timestamp = 1, .temp_c = -1.0f, .rh_pct = -1.0f};

    int rc = sensor_read(&not_iom361, 2, &reading);

    TEST_ASSERT_EQUAL_INT(1, rc);  // iom361: wrong base address
    TEST_ASSERT_EQUAL_INT64(1, reading.timestamp);
    TEST_ASSERT_EQUAL_FLOAT(-1.0f, reading.temp_c);
    TEST_ASSERT_EQUAL_FLOAT(-1.0f, reading.rh_pct);
}

/**
 * sensor_format_timestamp() prints the start time as given.
 */
static void test_sensor_format_timestamp_start_time(void) {
    const time_t start = 1791273600;  // 2026-10-06 08:00:00 UTC

    char text[20];
    int rc = sensor_format_timestamp(start, text);

    TEST_ASSERT_EQUAL_INT(0, rc);
    TEST_ASSERT_EQUAL_STRING("2026-10-06 08:00:00", text);
}

/**
 * sensor_format_timestamp() counts seconds: 59 seconds after the start time
 * is still within the same minute.
 */
static void test_sensor_format_timestamp_59_seconds_later(void) {
    const time_t start = 1791273600;  // 2026-10-06 08:00:00 UTC

    char text[20];
    int rc = sensor_format_timestamp(start + 59, text);

    TEST_ASSERT_EQUAL_INT(0, rc);
    TEST_ASSERT_EQUAL_STRING("2026-10-06 08:00:59", text);
}

/**
 * sensor_format_timestamp() changes the date at midnight: one second after
 * 23:59:59 is 00:00:00 on the next day.
 */
static void test_sensor_format_timestamp_rolls_over_at_midnight(void) {
    const time_t last_second = 1791331199;  // 2026-10-06 23:59:59 UTC

    char text[20];

    TEST_ASSERT_EQUAL_INT(0, sensor_format_timestamp(last_second, text));
    TEST_ASSERT_EQUAL_STRING("2026-10-06 23:59:59", text);

    TEST_ASSERT_EQUAL_INT(0, sensor_format_timestamp(last_second + 1, text));
    TEST_ASSERT_EQUAL_STRING("2026-10-07 00:00:00", text);
}

/**
 * sensor_format_timestamp() knows that 2027 is not a leap year: one second
 * after the end of 28 February is 1 March.
 */
static void test_sensor_format_timestamp_rolls_over_to_march(void) {
    const time_t last_second = 1803859199;  // 2027-02-28 23:59:59 UTC

    char text[20];

    TEST_ASSERT_EQUAL_INT(0, sensor_format_timestamp(last_second, text));
    TEST_ASSERT_EQUAL_STRING("2027-02-28 23:59:59", text);

    TEST_ASSERT_EQUAL_INT(0, sensor_format_timestamp(last_second + 1, text));
    TEST_ASSERT_EQUAL_STRING("2027-03-01 00:00:00", text);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_sensor_read_fills_reading_at_0_c);
    RUN_TEST(test_sensor_read_rounds_at_freeze_threshold);
    RUN_TEST(test_sensor_read_converts_at_100_c);
    RUN_TEST(test_sensor_read_fails_on_wrong_base);
    RUN_TEST(test_sensor_format_timestamp_start_time);
    RUN_TEST(test_sensor_format_timestamp_59_seconds_later);
    RUN_TEST(test_sensor_format_timestamp_rolls_over_at_midnight);
    RUN_TEST(test_sensor_format_timestamp_rolls_over_to_march);

    return UNITY_END();
}
