/*
 * main.c:
 *
 * Replays a scenario file into a trace, one line per tick. The device sees a
 * press of switch k and it toggles LED k. Only this file sets the "world" with
 * iom361's test functions. Everything else reads it through the registers, as
 * it would be on actual hardware.
 */
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
    /* Invoked w/ correct number of arguments? If not, err and return. */
    if (argc != 2) {
        fprintf(stderr, "usage: replay scenario.txt\n");
        return 1;
    }

    /* Successfully open the scenario file? If not, err and return. */
    FILE *fp = fopen(argv[1], "r");
    if (fp == NULL) {
        fprintf(stderr, "replay: %s: %s\n", argv[1], strerror(errno));
        return 1;
    }

    /* Successfully load the scenario file? If not, err and return. */
    scenario_t scenario;
    int bad_line = scenario_load(fp, &scenario);
    fclose(fp);
    if (bad_line != 0) {
        fprintf(stderr, "replay: %s:%d: bad line\n", argv[1], bad_line);
        return 1;
    }

    /* Did iom361 initialize? If not, err and return. */
    int rc;
    uint32_t *base = iom361_initialize(NUM_SWITCHES, NUM_LEDS, &rc);
    if (base == NULL) {
        fprintf(stderr, "replay: iom361_initialize failed (rc %d)\n", rc);
        return 1;
    }

    /* Turn LED display off so it doesn't interfere with printed output. */
    iom361_set_display(false);

    /**
     * -------------------------THE MAIN LOOP ---------------------------------
     */
    uint32_t before = 0;
    const int last_tick = scenario.lines[scenario.count - 1].tick;

    /* The stop test isn't in the loop header: scenario_load accepts a last tick
       of INT_MAX, where tick <= last_tick is never false and tick++ overflows. */
    for (int tick = 0; ; tick++) {
        /* Set the world from the line in force, and the clock to this tick. */
        const scenario_line_t *line = scenario_at(&scenario, tick);
        _iom361_set_switches(line->switches);
        _iom361_set_sensor1(line->temp_c, line->rh_pct);
        time_t t = scenario.start + tick;

        /* Did the sensor read succeed? If not, err and return. */
        reading_t reading;
        rc = sensor_read(base, t, &reading);
        if (rc != 0) {
            fprintf(stderr, "replay: tick %d: sensor_read failed (rc %d)\n", tick, rc);
            return 1;
        }

        /* Toggle LED k for every switch k just pressed. */
        uint32_t now = iom361_readReg(base, SWITCHES_REG, NULL);
        uint32_t pressed = panel_pressed(now, before);
        for (int k = 0; k < NUM_SWITCHES; k++) {
            /* read - modify - write. */
            if (pressed & (1u << k)) panel_toggle(base, k);
        }

        /* Print the trace line (UTC). */
        uint32_t leds = iom361_readReg(base, LEDS_REG, NULL);
        char when[32];
        strftime(when, sizeof when, "%Y-%m-%d %H:%M:%S", gmtime(&reading.t));

        printf("%s temp=%.1f rh=%.1f switches=0x%" PRIx32 " pressed=0x%" PRIx32
               " leds=0x%" PRIx32 "\n", when, reading.temp_c, reading.rh_pct, now,
               pressed, leds);

        /* Keep this tick's switches to detect presses on the next one. */
        before = now;

        if (tick == last_tick) break;
    }

    return 0;
}
