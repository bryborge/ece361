/*
 * test_sensor.c:
 *
 * Tests for sensor.c, run against the iom361 model. Prints PASS or FAIL for
 * each check; test_main.c runs them and prints the summary.
 */
#include <math.h>
#include <stdbool.h>
#include <stdint.h>

#include "check.h"
#include "iom361_r4.h"
#include "sensor.h"

/**
 * Sets the sensor to temp and rh, and reads them back through sensor_read.
 *
 * @return true if the read succeeds and gives the two expected values.
 */
static bool reads_back(float temp, float rh, float want_temp, float want_rh) {
    uint32_t *base = iom361_initialize(2, 2, NULL);
    reading_t r;

    _iom361_set_sensor1(temp, rh);
    if (sensor_read(base, 0, &r) != 0) return false;
    return r.temp_c == want_temp && r.rh_pct == want_rh;
}

static void check_sensor_conversion(void) {
    CHECK(reads_back(0.0f, 60.0f, 0.0f, 60.0f));
    CHECK(reads_back(100.0f, 60.0f, 100.0f, 60.0f));
    /* FREEZE_C, the only threshold in the rules, and the readings either side
     * of it. The register alone would not land on it exactly. */
    CHECK(reads_back(1.0f, 60.0f, 1.0f, 60.0f));
    CHECK(reads_back(0.9f, 60.0f, 0.9f, 60.0f));
    CHECK(reads_back(1.1f, 60.0f, 1.1f, 60.0f));
}

static void check_sensor_rounds_to_tenths(void) {
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
    uint32_t *base = iom361_initialize(2, 2, NULL);
    reading_t r;

    CHECK(sensor_read(base, 0, &r) == 0);
    CHECK(r.temp_c == 23.5f);
    CHECK(r.rh_pct == 75.0f);
}

static void check_sensor_timestamp(void) {
    uint32_t *base = iom361_initialize(2, 2, NULL);
    reading_t r;

    CHECK(sensor_read(base, 1791273602, &r) == 0);
    CHECK(r.timestamp == 1791273602);
}

static void check_sensor_no_negative_zero(void) {
    uint32_t *base = iom361_initialize(2, 2, NULL);
    reading_t r;

    _iom361_set_sensor1(-0.04f, 50.0f);
    CHECK(sensor_read(base, 0, &r) == 0);
    /* -0.0 == 0.0 is true, so only the sign bit tells them apart. It matters
     * because a trace would print -0.0. */
    CHECK(r.temp_c == 0.0f);
    CHECK(!signbit(r.temp_c));
}

static void check_sensor_keeps_bits_19_to_0(void) {
    uint32_t *base = iom361_initialize(2, 2, NULL);
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
    uint32_t *base = iom361_initialize(2, 2, NULL);
    reading_t r = { .timestamp = 7, .temp_c = 1.5f, .rh_pct = 2.5f };

    CHECK(sensor_read(base + 1, 1791273602, &r) == 1);
    CHECK(r.timestamp == 7);
    CHECK(r.temp_c == 1.5f);
    CHECK(r.rh_pct == 2.5f);
}

void run_sensor_tests(void) {
    check_sensor_conversion();
    check_sensor_rounds_to_tenths();
    check_sensor_range_ends();
    check_sensor_after_initialize();
    check_sensor_timestamp();
    check_sensor_no_negative_zero();
    check_sensor_keeps_bits_19_to_0();
    check_sensor_failed_read();
}
