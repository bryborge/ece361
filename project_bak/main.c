/*
 * Entry point for the garage door opener, the ECE 361 final project.
 *
 * main initializes the iom361 hardware model, then replays a scenario file
 * one tick (one second) at a time: read the sensor and switches, run the
 * state machine, drive the LEDs and RGB LED, and store a record. Only this file
 * sets the "world" with iom361's test functions. Everything else reads it through
 * the registers, as on a board.
 *
 * `./garage scenario.txt` prints a trace line per tick. Still to come: the event
 * stack (week 7) and `./garage scenario.txt --report` (week 9).
 */
#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "clock.h"
#include "fsm.h"
#include "iom361_r4.h"
#include "scenario.h"
#include "sensor.h"
#include "status.h"
#include "store.h"

#define NUM_SWITCHES 2
#define NUM_LEDS 2

static bool register_failed(int tick, int rc) {
    fprintf(stderr, "garage: tick %d: register access failed (code %d)\n", tick, rc);
    return false;
}

/**
 * Writes the state's LEDs and RGB color. The state decides every LED, so a plain
 * write is right here, with no read first.
 *
 * @param leds Where to store what the LED register holds afterward.
 * @return 0, or the iom361 return code of a failed write.
 */
static int write_outputs(uint32_t *base, state_t state, uint32_t *leds) {
    int rc;

    *leds = iom361_writeReg(base, LEDS_REG, fsm_leds(state), &rc);
    if (rc != 0) return rc;

    rgb_t color = fsm_rgb(state);
    iom361_writeReg(base, RGB_LED_REG,
                    build_rgb_reg(1, color.red, color.green, color.blue), &rc);
    return rc;
}

/**
 * Runs the device for one tick: sets the world from the line in force, reads it
 * back through the registers, runs the state machine, writes the outputs, stores
 * a record, and prints a trace line.
 *
 * @param door  The door; updated by the tick.
 * @param prev  The last tick's switches; updated by the tick.
 * @return true, or false after reporting a failure on stderr.
 */
static bool run_tick(const scenario_t *s, uint32_t *base, store_t *store, door_t *door,
                     int tick, uint32_t *prev) {
    const scenario_line_t *line = scenario_at(s, tick);
    _iom361_set_sensor1(line->temp_c, line->rh_pct);
    _iom361_set_switches(line->switches);

    reading_t r;
    int rc = sensor_read(base, s->start + tick, &r);
    if (rc != 0) return register_failed(tick, rc);

    uint32_t switches = iom361_readReg(base, SWITCHES_REG, &rc);
    if (rc != 0) return register_failed(tick, rc);

    /* The tick's action goes onto the event stack in week 7. */
    fsm_tick(door, r.timestamp, *prev, switches, r.temp_c);

    uint32_t leds;
    rc = write_outputs(base, door->state, &leds);
    if (rc != 0) return register_failed(tick, rc);

    status_t status = status_of(door->state, r.temp_c);
    uint32_t word = status_pack(&status);

    record_t record = { .reading = r, .state = door->state, .status = word };
    if (store_insert(store, &record) != 0) {
        fprintf(stderr, "garage: tick %d: store full\n", tick);
        return false;
    }

    char when[CLOCK_TEXT_SIZE];
    if (clock_format(r.timestamp, when) != 0) {
        fprintf(stderr, "garage: tick %d: time out of range\n", tick);
        return false;
    }
    printf("%s state=%s temp=%.1f rh=%.1f leds=0x%x status=0x%08x\n", when,
           state_name(door->state), r.temp_c, r.rh_pct, leds, word);

    /* Saved last: a press is off in the last tick and on in this one. */
    *prev = switches;
    return true;
}

/**
 * Runs every tick from 0 to the last one.
 *
 * @return true, or false after reporting a failure on stderr.
 */
static bool replay(const scenario_t *s, uint32_t *base, store_t *store) {
    door_t door = { .state = CLOSED, .entered_at = s->start, .pos_at_entry = 0 };
    uint32_t prev = 0;
    int last = s->lines[s->count - 1].tick;

    /* The stop test is not in the loop header. A last tick of INT_MAX is accepted,
     * so tick <= last could never be false, and tick++ would overflow. */
    for (int tick = 0; ; tick++) {
        if (!run_tick(s, base, store, &door, tick, &prev)) return false;
        if (tick == last) return true;
    }
}

int main(int argc, char **argv) {
    /* An entry left out of the table reads as { CLOSED, A_UNDEFINED }. Stop
     * before the door runs, including on an entry no input would reach. */
    for (state_t st = 0; st < N_STATES; st++) {
        for (event_t e = 0; e < N_EVENTS; e++) {
            assert(fsm_table[st][e].action != A_UNDEFINED);
        }
    }

    /* --report is a usage error until week 9: see "Where we are" in CLAUDE.md. */
    if (argc != 2) {
        fprintf(stderr, "usage: garage scenario.txt\n");
        return 1;
    }
    const char *path = argv[1];

    FILE *fp = fopen(path, "r");
    if (fp == NULL) {
        fprintf(stderr, "garage: %s: %s\n", path, strerror(errno));
        return 1;
    }

    scenario_t s;
    int bad_line = scenario_load(fp, &s);
    fclose(fp);
    if (bad_line != 0) {
        fprintf(stderr, "garage: %s: line %d: invalid scenario line\n", path, bad_line);
        return 1;
    }

    int rc;
    uint32_t *base = iom361_initialize(NUM_SWITCHES, NUM_LEDS, &rc);
    if (base == NULL) {
        fprintf(stderr, "garage: iom361_initialize failed (code %d)\n", rc);
        return 1;
    }
    /* The LED display would print into the trace. */
    iom361_set_display(false);

    store_t *store = store_create();
    if (store == NULL) {
        fprintf(stderr, "garage: store_create failed\n");
        return 1;
    }

    bool ok = replay(&s, base, store);
    store_destroy(store);
    return ok ? 0 : 1;
}
