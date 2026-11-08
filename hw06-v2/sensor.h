/*
 * sensor.h:
 *
 * Timestamped readings from the temperature and humidity sensor.
 */
#ifndef SENSOR_H
#define SENSOR_H

#include <stdint.h>
#include <time.h>

typedef struct {
    time_t t;
    float temp_c;
    float rh_pct;
} reading_t;

/**
 * Reads the sensor registers into *r, with each value rounded to 0.1 and
 * timestamped with now.
 *
 * @return 0, or the iom361 return code of a failed read; then *r is unchanged.
 */
int sensor_read(uint32_t *base, time_t now, reading_t *r);

#endif
