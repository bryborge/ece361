/*
 * sensor.h:
 *
 * The temperature and humidity sensor, read through iom361's registers.
 */
#ifndef SENSOR_H
#define SENSOR_H

#include <stdint.h>
#include <time.h>

typedef struct {
    time_t t;           /* UTC seconds since the Unix epoch */
    float temp_c;       /* rounded to 0.1 degrees C */
    float rh_pct;       /* rounded to 0.1 %RH */
} reading_t;

/**
 * Reads TEMP_REG and HUMID_REG, and stores the converted values in *r with the
 * timestamp `now`.
 *
 * @return 0, or the iom361 return code of a failed read, which leaves *r
 *         unchanged.
 */
int sensor_read(uint32_t *base, time_t now, reading_t *r);

#endif
