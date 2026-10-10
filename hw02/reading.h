/*
 * reading.h: reading sensor readings from standard input.
 */
#ifndef READING_H
#define READING_H

/* Reads "tick temperature humidity" lines from stdin into the three arrays.
 * Skips blank lines and lines whose first non-blank character is '#'.
 * A line that does not hold three numbers is counted as skipped, not stored.
 * Stops after max readings; if another non-blank, non-comment line follows,
 * prints a warning to stderr and stops reading.
 * Takes: arrays with room for max entries, max >= 0.
 * Returns: count, the number of readings stored (0 to max), and skipped,
 * the number of lines that were not blank, not comments, and not readings. */
struct reading_counts {
    int count;
    int skipped;
};
struct reading_counts reading_read(int ticks[], float temps[], float hums[], int max);

#endif
