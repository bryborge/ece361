/**
 * @file sensor.c
 * @brief The temperature and humidity sensor.
 */
#include <math.h>
#include <string.h>

#include "iom361_r4.h"
#include "sensor.h"

static float round_to_tenth(double val) {
    return (float)(lround(val * 10.0) / 10.0);
}

int sensor_read(uint32_t *base, time_t now, reading_t *reading) {
    int rc;

    uint32_t st = iom361_readReg(base, TEMP_REG, &rc);
    if (rc != 0) return rc;

    uint32_t srh = iom361_readReg(base, HUMID_REG, &rc);
    if (rc != 0) return rc;

    reading->timestamp = now;
    /* See: docs/AHT20.md for details on these values. */
    reading->temp_c = round_to_tenth(st / 1048576.0 * 200.0 - 50.0);
    reading->rh_pct = round_to_tenth(srh / 1048576.0 * 100.0);

    return 0;
}

int sensor_format_timestamp(time_t timestamp, char text[20]) {
    const struct tm *utc = gmtime(&timestamp);
    if (utc == NULL) return 1;

    /* Format into a local buffer so that text is untouched on failure. */
    char formatted[20];
    if (strftime(formatted, sizeof formatted, "%Y-%m-%d %H:%M:%S", utc) == 0) {
        return 1;
    }

    memcpy(text, formatted, sizeof formatted);
    return 0;
}
