/*
 * main.c:
 *
 * Reads sensor readings from standard input and prints a summary: the number
 * of readings, the minimum, maximum, and mean temperature and humidity, the
 * longest run of readings above a temperature threshold, and a histogram of
 * the temperatures.
 *
 * Input: one reading per line, "tick temperature humidity" (e.g. 12 23.5 41.0)
 *
 * Blank lines and lines whose first non-blank character is '#' are ignored. A
 * line that does not hold three numbers is counted as skipped.
 *
 * Usage: build/readings [threshold] < data/normal.txt
 *        threshold is in degrees C, default 30.0
 */
#include <stdio.h>
#include <stdlib.h>

#include "histogram.h"
#include "reading.h"
#include "stats.h"

#define MAX_READINGS 1000

/**
 * Parses the optional threshold argument, reads readings from stdin, and
 * prints the summary described at the top of this file.
 *
 * @param argc Number of command-line arguments (at most 2).
 * @param argv Command-line arguments; argv[1], if given, is the threshold in
 *             degrees C.
 * @return 0 on success, including when there are no readings; 1 if the
 *         arguments are invalid, after printing a message to stderr.
 */
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

    struct reading_counts counts =
        reading_read(ticks, temps, hums, MAX_READINGS);

    printf("readings: %d\n", counts.count);
    printf("skipped:  %d\n", counts.skipped);
    if (counts.count == 0) {
        printf("no readings, no summary\n");
        return 0;
    }

    printf("temperature: min %6.1f  max %6.1f  mean %6.2f C\n",
           stats_min(temps, counts.count),
           stats_max(temps, counts.count),
           stats_mean(temps, counts.count));
    printf("humidity:    min %6.1f  max %6.1f  mean %6.2f %%RH\n",
           stats_min(hums, counts.count),
           stats_max(hums, counts.count),
           stats_mean(hums, counts.count));

    struct stats_run run = stats_longest_run_above(
        temps, counts.count, threshold);

    if (run.length == 0)
        printf("above %.1f C: never\n", threshold);
    else
        printf("above %.1f C: longest run %d readings, "
               "from tick %d to tick %d\n",
               threshold,
               run.length,
               ticks[run.start],
               ticks[run.start + run.length - 1]);

    print_histogram(temps, counts.count);
    return 0;
}
