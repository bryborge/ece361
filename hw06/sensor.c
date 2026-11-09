/*
 * sensor.c:
 *
 * Converts the AHT20's raw 20-bit values, ST and SRH, to rounded readings.
 */
#include <math.h>

#include "iom361_r4.h"
#include "sensor.h"

#define RAW_MASK 0xFFFFFu    /* Keep bits 19..0; Ignore "status bits" */
#define RAW_RANGE 1048576.0  /* 2^20 */

/**
 * Rounds to the nearest 0.1. lround returns a long, which has no negative
 * zero.
 */
static float round_to_tenth(double value) {
    return (float)(lround(value * 10.0) / 10.0);
}

/**
 * Both reads come before any write to *r, so that a failure of either one
 * leaves *r unchanged.
 */
int sensor_read(uint32_t *base, time_t now, reading_t *r) {
    int rc;

    uint32_t st = iom361_readReg(base, TEMP_REG, &rc) & RAW_MASK;
    if (rc != 0) return rc;

    uint32_t srh = iom361_readReg(base, HUMID_REG, &rc) & RAW_MASK;
    if (rc != 0) return rc;

    r->t = now;
    r->temp_c = round_to_tenth(st / RAW_RANGE * 200.0 - 50.0);
    r->rh_pct = round_to_tenth(srh / RAW_RANGE * 100.0);

    return 0;
}
