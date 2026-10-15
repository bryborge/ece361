#include <math.h>
#include "iom361_r4.h"
#include "sensor.h"

/**
 * Reads TEMP_REG and HUMID_REG and converts them to
 * °C and %RH, each rounded to 0.1.
 *
 * Returns 0 on success, or the iom361 return code of the first read that
 * fails. On failure, *temp_c and *rh_pct are left unchanged.
 */
int sensor_read(uint32_t *base, float *temp_c, float *rh_pct) {
    int rc;

    // Read temperature signal; return early if the read failed.
    uint32_t st = iom361_readReg(base, TEMP_REG, &rc);
    if (rc != 0) return rc;

    // Read relative humidity signal; return early if the read failed.
    uint32_t srh = iom361_readReg(base, HUMID_REG, &rc);
    if (rc != 0) return rc;

    double t = st / 1048576.0 * 200.0 - 50.0;   /* AHT20 formula */
    double rh = srh / 1048576.0 * 100.0;        /* AHT20 formula */
    *temp_c = (float)(round(t * 10.0) / 10.0);
    *rh_pct = (float)(round(rh * 10.0) / 10.0);

    return 0;
}
