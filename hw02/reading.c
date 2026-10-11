/*
 * reading.c:
 *
 * Parses sensor readings from standard input.
 */
#include <assert.h>
#include <stdio.h>

#include "reading.h"

#define LINE_LEN 128

/**
 * Returns whether line holds only spaces and tabs, or has '#' as its first
 * other character.
 *
 * @param line A NUL-terminated line, as read by fgets.
 * @return 1 if the line is blank or a comment, 0 otherwise.
 */
static int is_blank_or_comment(const char line[]) {
    int i = 0;

    while (line[i] == ' ' || line[i] == '\t')
        i++;

    return line[i] == '\n' || line[i] == '\0' || line[i] == '#';
}

/**
 * Reads readings from stdin into the given arrays, one per line in the form
 * "tick temperature humidity". Blank and comment lines are ignored, and a line
 * that does not hold three numbers is counted as skipped. If a line remains
 * once max readings are stored, prints a warning to stderr and stops.
 *
 * @param ticks Array for the ticks (must hold at least max elements).
 * @param temps Array for the temperatures in degrees C (must hold at least
 *              max elements).
 * @param hums  Array for the humidities in %RH (must hold at least max
 *              elements).
 * @param max   Number of readings the arrays can hold (must be >= 0).
 * @return The number of readings stored and the number of lines skipped.
 */
struct reading_counts reading_read(
    int ticks[], float temps[], float hums[], int max) {
    assert(max >= 0);
    struct reading_counts counts = {0, 0};
    char line[LINE_LEN];

    while (fgets(line, sizeof line, stdin) != NULL) {
        if (is_blank_or_comment(line)) continue;

        if (counts.count == max) {
            fprintf(stderr,
                    "warning: more than %d readings, the rest are ignored\n",
                    max);
            break;
        }

        if (sscanf(line, "%d %f %f", &ticks[counts.count],
                   &temps[counts.count], &hums[counts.count]) == 3)
            counts.count++;
        else
            counts.skipped++;
    }

    return counts;
}
