#ifndef SENSOR_H
#define SENSOR_H

#include <stdint.h>
#include <time.h>

typedef struct {
    time_t timestamp;   /* UTC seconds since the Unix epoch */
    float temp_c;       /* rounded to 0.1 °C */
    float rh_pct;       /* rounded to 0.1 %RH */
} reading_t;

int sensor_read(uint32_t *base, time_t now, reading_t *out);

#endif
