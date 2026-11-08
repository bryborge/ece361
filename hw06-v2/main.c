#include <errno.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "iom361_r4.h"
#include "panel.h"
#include "scenario.h"
#include "sensor.h"

#define NUM_SWITCHES 4
#define NUM_LEDS 4

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "usage: replay scenario.txt\n");
        return 1;
    }

    FILE *fp = fopen(argv[1], "r");

    if (fp == NULL) {
        fprintf(stderr, "replay: %s: %s\n", argv[1], strerror(errno));
        return 1;
    }

    scenario_t scenario;
    int bad_line = scenario_load(fp, &scenario);
    fclose(fp);

    if (bad_line != 0) {
        fprintf(stderr, "replay: %s:%d: bad line\n", argv[1], bad_line);
        return 1;
    }

    int rc;
    uint32_t *base = iom361_initialize(NUM_SWITCHES, NUM_LEDS, &rc);

    if (base == NULL) {
        fprintf(stderr, "replay: iom361_initialize failed (rc %d)\n", rc);
        return 1;
    }

    iom361_set_display(false);  /* Turn LED display off so it doesn't interfere with printed output. */

    uint32_t before = 0;

    const int last_tick = scenario.lines[scenario.count - 1].tick;

    for (int tick = 0; tick <= last_tick; tick++) {
        const scenario_line_t *line = scenario_at(&scenario, tick);

        _iom361_set_switches(line->switches);
        _iom361_set_sensor1(line->temp_c, line->rh_pct);

        /* one second per tick */
        time_t t = scenario.start + tick;

        reading_t reading;
        sensor_read(base, t, &reading);

        uint32_t now = iom361_readReg(base, SWITCHES_REG, NULL);

        uint32_t pressed = panel_pressed(now, before);

        for (int k = 0; k < NUM_SWITCHES; k++) {
            if (pressed & (1u << k)) panel_toggle(base, k);
        }

        uint32_t leds = iom361_readReg(base, LEDS_REG, NULL);

        /* UTC, so the trace is the same in every time zone */
        char when[32];
        strftime(when, sizeof when, "%Y-%m-%d %H:%M:%S", gmtime(&reading.t));

        printf("%s temp=%.1f rh=%.1f switches=0x%" PRIx32 " pressed=0x%" PRIx32
               " leds=0x%" PRIx32 "\n", when, reading.temp_c, reading.rh_pct, now,
               pressed, leds);

        before = now;
    }

    return 0;
}
