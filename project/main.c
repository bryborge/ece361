/*
 * Entry point for the garage door opener, the ECE 361 final project.
 *
 * main initializes the iom361 hardware model, then replays a scenario file
 * one tick (one second) at a time: read the sensor and switches, run the
 * state machine, drive the LEDs and RGB LED, store a record, and update the
 * event stack.
 *
 * `./garage scenario.txt` prints a trace line per tick;
 * `./garage scenario.txt --report` prints only the moving-average report.
 */
#include <stdio.h>

#include "iom361_r4.h"
#include "sensor.h"

int main(void) {
    int rc;
    uint32_t *base = iom361_initialize(2, 2, &rc);

    if (base == NULL) {
        fprintf(stderr, "iom361_initialize failed: %d\n", rc);
        return 1;
    }

    _iom361_set_sensor1(15.0f, 60.0f);  /* Garage: 15.0 C, 60 %RH */

    reading_t r;
    rc = sensor_read(base, 0, &r);      /* no clock until week 6 */
    if (rc != 0) {
        fprintf(stderr, "sensor_read failed: %d\n", rc);
        return 1;
    }

    printf("Garage door opener: %.1f C, %.1f %%RH\n", r.temp_c, r.rh_pct);
    return 0;
}
