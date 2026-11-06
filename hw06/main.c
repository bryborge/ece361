/*
 * main.c:
 *
 * Replays a scenario file into a trace, one line per tick. The device is simple:
 * a press of switch k toggles LED k. Only this file sets the "world" with iom361's
 * test functions. Everything else reads it through the registers, as on a board.
 */
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "iom361_r4.h"
#include "panel.h"
#include "scenario.h"
#include "sensor.h"

#define NUM_SWITCHES 4
#define NUM_LEDS 4

static bool register_failed(int tick, int rc) {
    fprintf(stderr, "replay: tick %d: register access failed (code %d)\n", tick, rc);
    return false;
}

/**
 * Prints one trace line, with the time in UTC so that a trace is the same on
 * every computer.
 *
 * @return true, or false after reporting a time that gmtime can't split.
 */
static bool print_trace_line(int tick, const reading_t *r, uint32_t switches,
                             uint32_t pressed, uint32_t leds) {
    /* The start cap in scenario.c keeps this from failing on a 64-bit time_t,
     * but the C standard allows NULL, and strftime would crash on it. */
    const struct tm *utc = gmtime(&r->t);
    if (utc == NULL) {
        fprintf(stderr, "replay: tick %d: time out of range\n", tick);
        return false;
    }

    char when[32];      /* Room for the 5-digit years the start cap allows. */
    strftime(when, sizeof when, "%Y-%m-%d %H:%M:%S", utc);
    printf("%s temp=%.1f rh=%.1f switches=0x%x pressed=0x%x leds=0x%x\n", when,
           r->temp_c, r->rh_pct, switches, pressed, leds);
    return true;
}

/**
 * Runs the device for one tick: sets the world from the line in force, reads it
 * back through the registers, toggles LED k for every switch k just pressed, and
 * prints a trace line. *before holds the last tick's switches.
 *
 * @return true, or false after reporting a failure.
 */
static bool run_tick(const scenario_t *s, uint32_t *base, int tick, uint32_t *before) {
    const scenario_line_t *line = scenario_at(s, tick);
    _iom361_set_sensor1(line->temp_c, line->rh_pct);
    _iom361_set_switches(line->switches);

    reading_t r;
    int rc = sensor_read(base, s->start + tick, &r);
    if (rc != 0) return register_failed(tick, rc);

    uint32_t switches = iom361_readReg(base, SWITCHES_REG, &rc);
    if (rc != 0) return register_failed(tick, rc);

    uint32_t pressed = panel_pressed(switches, *before);
    for (int k = 0; k < NUM_SWITCHES; k++) {
        if (pressed & (1u << k)) {
            rc = panel_toggle(base, k);
            if (rc != 0) return register_failed(tick, rc);
        }
    }

    uint32_t leds = iom361_readReg(base, LEDS_REG, &rc);
    if (rc != 0) return register_failed(tick, rc);

    if (!print_trace_line(tick, &r, switches, pressed, leds)) return false;

    /* Saved last: a press is off in the last tick and on in this one. */
    *before = switches;
    return true;
}

/**
 * Runs every tick from 0 to the last one.
 *
 * @return true, or false after reporting a failure.
 */
static bool replay(const scenario_t *s, uint32_t *base) {
    uint32_t before = 0;
    int last = s->lines[s->count - 1].tick;

    /* The stop test is not in the loop header. A last tick of INT_MAX is accepted,
     * so tick <= last could never be false, and tick++ would overflow. */
    for (int tick = 0; ; tick++) {
        if (!run_tick(s, base, tick, &before)) return false;
        if (tick == last) return true;
    }
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "usage: replay scenario.txt\n");
        return 1;
    }
    const char *path = argv[1];

    FILE *fp = fopen(path, "r");
    if (fp == NULL) {
        fprintf(stderr, "replay: %s: %s\n", path, strerror(errno));
        return 1;
    }

    scenario_t s;
    int bad_line = scenario_load(fp, &s);
    fclose(fp);
    if (bad_line != 0) {
        fprintf(stderr, "replay: %s: line %d: invalid scenario line\n", path, bad_line);
        return 1;
    }

    int rc;
    uint32_t *base = iom361_initialize(NUM_SWITCHES, NUM_LEDS, &rc);
    if (base == NULL) {
        fprintf(stderr, "replay: iom361_initialize failed (code %d)\n", rc);
        return 1;
    }
    /* The LED display would print into the trace. */
    iom361_set_display(false);

    return replay(&s, base) ? 0 : 1;
}
