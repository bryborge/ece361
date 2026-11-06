#include <math.h>

#include "iom361_r4.h"
#include "sensor.h"

/* The value is in bits 19..0. On real hardware the upper bits can be status
 * bits, and the formulas would turn them into nonsense. */
#define RAW_MASK 0xFFFFFu
#define FULL_SCALE 1048576.0    /* 2^20 */

/**
 * Rounds to the nearest 0.1. lround gives an integer, which has no negative
 * zero, so -0.04 becomes 0.0 and a trace never prints -0.0.
 */
static float round_to_tenth(double value) {
    return (float)(lround(value * 10.0) / 10.0);
}

int sensor_read(uint32_t *base, time_t now, reading_t *r) {
    int rc;

    uint32_t st = iom361_readReg(base, TEMP_REG, &rc) & RAW_MASK;
    if (rc != 0) return rc;

    uint32_t srh = iom361_readReg(base, HUMID_REG, &rc) & RAW_MASK;
    if (rc != 0) return rc;

    /* Stored only now, so a failed second read leaves *r unchanged. */
    r->t = now;
    r->temp_c = round_to_tenth(st / FULL_SCALE * 200.0 - 50.0);
    r->rh_pct = round_to_tenth(srh / FULL_SCALE * 100.0);
    return 0;
}
