/*
 * scenario.h:
 *
 * A scenario file: the world a replay runs in, as a start time and the sensor
 * and switch values at each tick where they change. The values hold between
 * lines.
 */
#ifndef SCENARIO_H
#define SCENARIO_H

#include <stdint.h>
#include <stdio.h>
#include <time.h>

#define SCENARIO_MAX_LINES 1000     /* data lines; comments and blanks are free */
#define SCENARIO_MAX_LINE_LEN 255   /* characters, not counting the line ending */

typedef struct {
    int tick;
    float temp_c;
    float rh_pct;
    uint32_t switches;
} scenario_line_t;

typedef struct {
    time_t start;
    int count;
    scenario_line_t lines[SCENARIO_MAX_LINES];
} scenario_t;

/**
 * Loads the scenario in fp into *s.
 *
 * @return 0, or the number of the first wrong line, counting every line in the
 *         file. A file with no data lines, or a read error, returns one past the
 *         last line read. After a nonzero return, *s must not be used.
 */
int scenario_load(FILE *fp, scenario_t *s);

/**
 * Gives the line in force at tick: the last line at or before it. Assumes *s
 * was loaded successfully and tick is not negative.
 */
const scenario_line_t *scenario_at(const scenario_t *s, int tick);

#endif
