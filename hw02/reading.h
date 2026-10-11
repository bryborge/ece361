/*
 * reading.h:
 *
 * Parses sensor readings from standard input.
 */
#ifndef READING_H
#define READING_H

/**
 * The outcome of reading input: the number of readings stored and the number
 * of lines skipped because they did not hold three numbers.
 */
struct reading_counts {
    int count;
    int skipped;
};

/**
 * Given arrays for ticks, temperatures, and humidities, and max, the number of
 * readings they can hold, reads "tick temperature humidity" lines from stdin
 * into the arrays (0..count-1) and returns the counts. Blank lines and lines
 * whose first non-blank character is '#' are ignored. If a line remains once
 * max readings are stored, prints a warning to stderr and stops. Each array
 * must hold at least max elements. Assumes max >= 0.
 */
struct reading_counts reading_read(
    int ticks[], float temps[], float hums[], int max);

#endif
