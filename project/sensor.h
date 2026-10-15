#ifndef SENSOR_H
#define SENSOR_H

#include <stdint.h>

int sensor_read(uint32_t *base, float *temp_c, float *rh_pct);

#endif
