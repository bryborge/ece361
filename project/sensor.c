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
 *
 * @param value The value to round.
 * @return The rounded value.
 */
static float round_to_tenth(double value) {
    return (float)(lround(value * 10.0) / 10.0);
}

/**
 * Reads TEMP_REG and HUMID_REG and converts them to °C and %RH, each
 * rounded to 0.1, stamped with the time now.
 *
 * Returns 0 on success, or the iom361 return code of the first read that
 * fails. On failure, *out is left unchanged.
 */
int sensor_read(uint32_t *base, time_t now, reading_t *out) {
    int rc;

    /* A failed read still returns a value (0xDEADBEEF), not an error, so
     * rc is the only sign of failure. */
    uint32_t st = iom361_readReg(base, TEMP_REG, &rc) & RAW_MASK;
    if (rc != 0) return rc;

    uint32_t srh = iom361_readReg(base, HUMID_REG, &rc) & RAW_MASK;
    if (rc != 0) return rc;

    /* Stored only now, so a failed second read leaves *out unchanged. */
    out->timestamp = now;
    out->temp_c = round_to_tenth(st / FULL_SCALE * 200.0 - 50.0);   /* AHT20 */
    out->rh_pct = round_to_tenth(srh / FULL_SCALE * 100.0);         /* AHT20 */
    return 0;
}
