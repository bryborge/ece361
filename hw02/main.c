/*
 * main.c: ECE 361, Fall 2026, HW2
 *
 * Reads sensor readings from standard input and prints a summary: the number
 * of readings, the minimum, maximum, and mean temperature and humidity, the
 * longest run of readings above a temperature threshold, and a histogram of
 * the temperatures.
 *
 * Input: one reading per line, "tick temperature humidity", for example
 *     12  23.5  41.0
 * Blank lines and lines starting with '#' are ignored. A line that does not
 * hold three numbers is counted as skipped.
 *
 * Usage: build/readings [threshold] < data/normal.txt
 *        threshold is in degrees C, default 30.0
 *
 * main handles the command line and printing; reading, stats, and histogram
 * do the work. The readings live here and are passed to the modules.
 */
#include <stdio.h>
#include <stdlib.h>

#include "histogram.h"
#include "reading.h"
#include "stats.h"

#define MAX_READINGS 1000

int main(int argc, char *argv[]) {
    int   ticks[MAX_READINGS];
    float temps[MAX_READINGS];
    float hums[MAX_READINGS];
    float threshold = 30.0f;

    if (argc > 2) {
        fprintf(stderr, "usage: %s [threshold] < readings.txt\n", argv[0]);
        return 1;
    }
    if (argc == 2) {
        char *end;
        threshold = strtof(argv[1], &end);
        if (*end != '\0') {
            fprintf(stderr, "error: threshold '%s' is not a number\n", argv[1]);
            return 1;
        }
    }

    struct reading_counts c = reading_read(ticks, temps, hums, MAX_READINGS);

    printf("readings: %d\n", c.count);
    printf("skipped:  %d\n", c.skipped);
    if (c.count == 0) {
        printf("no readings, no summary\n");
        return 0;
    }

    printf("temperature: min %6.1f  max %6.1f  mean %6.2f C\n",
           stats_min(temps, c.count), stats_max(temps, c.count), stats_mean(temps, c.count));
    printf("humidity:    min %6.1f  max %6.1f  mean %6.2f %%RH\n",
           stats_min(hums, c.count), stats_max(hums, c.count), stats_mean(hums, c.count));

    struct stats_run r = stats_longest_run_above(temps, c.count, threshold);
    if (r.length == 0)
        printf("above %.1f C: never\n", threshold);
    else
        printf("above %.1f C: longest run %d readings, from tick %d to tick %d\n",
               threshold, r.length, ticks[r.start], ticks[r.start + r.length - 1]);

    histogram_print(temps, c.count);
    return 0;
}
