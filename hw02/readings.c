/*
 * readings.c: ECE 361, Fall 2026, HW2 starter
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
 * Usage: ./readings [threshold] < data/normal.txt
 *        threshold is in degrees C, default 30.0
 *
 * The program works. It is also one file, with global variables, repeated
 * code, and a main() that does everything. HW2 asks you to split it into
 * modules without changing what it prints.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "stats.h"

#define MAX_READINGS 1000
#define LINE_LEN     128
#define NUM_BINS     20          /* histogram: 0 to 100 C in 5 C bins */
#define BIN_WIDTH    5.0f

int   ticks[MAX_READINGS];
float temps[MAX_READINGS];
float hums[MAX_READINGS];
int   count = 0;
int   skipped = 0;
float threshold = 30.0f;

int main(int argc, char *argv[]) {
    char line[LINE_LEN];

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

    while (fgets(line, sizeof line, stdin) != NULL) {
        int i = 0;
        while (line[i] == ' ' || line[i] == '\t')
            i++;
        if (line[i] == '\n' || line[i] == '\0' || line[i] == '#')
            continue;
        if (count == MAX_READINGS) {
            fprintf(stderr, "warning: more than %d readings, the rest are ignored\n", MAX_READINGS);
            break;
        }
        if (sscanf(line, "%d %f %f", &ticks[count], &temps[count], &hums[count]) == 3)
            count++;
        else
            skipped++;
    }

    printf("readings: %d\n", count);
    printf("skipped:  %d\n", skipped);
    if (count == 0) {
        printf("no readings, no summary\n");
        return 0;
    }

    printf("temperature: min %6.1f  max %6.1f  mean %6.2f C\n",
           stats_min(temps, count), stats_max(temps, count), stats_mean(temps, count));
    printf("humidity:    min %6.1f  max %6.1f  mean %6.2f %%RH\n",
           stats_min(hums, count), stats_max(hums, count), stats_mean(hums, count));

    struct stats_run r = stats_longest_run_above(temps, count, threshold);
    if (r.length == 0)
        printf("above %.1f C: never\n", threshold);
    else
        printf("above %.1f C: longest run %d readings, from tick %d to tick %d\n",
               threshold, r.length, ticks[r.start], ticks[r.start + r.length - 1]);

    /* histogram of temperatures, 5 C bins; values outside 0..100 go to the end bins */
    int bins[NUM_BINS] = {0};
    for (int i = 0; i < count; i++) {
        int b = (int) (temps[i] / BIN_WIDTH);
        if (temps[i] < 0.0f)
            b = 0;
        if (b >= NUM_BINS)
            b = NUM_BINS - 1;
        bins[b]++;
    }
    printf("histogram:\n");
    for (int b = 0; b < NUM_BINS; b++) {
        if (bins[b] == 0)
            continue;
        printf("  %5.1f to %5.1f C | ", b * BIN_WIDTH, (b + 1) * BIN_WIDTH);
        for (int k = 0; k < bins[b]; k++)
            putchar('*');
        printf(" %d\n", bins[b]);
    }
    return 0;
}
