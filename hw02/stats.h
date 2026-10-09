/*
 * stats.h: statistics over an array of floats.
 */
#ifndef STATS_H
#define STATS_H

/* Smallest value in a[0..n-1]. Assumes n >= 1. */
float stats_min(const float a[], int n);

/* Largest value in a[0..n-1], computed recursively: one stack frame per
 * element, so a very large n overflows the stack. Assumes n >= 1. */
float stats_max(const float a[], int n);

/* Mean of a[0..n-1], summed in float from index 0 upward. Assumes n >= 1. */
float stats_mean(const float a[], int n);

/* The longest run of consecutive values in a[0..n-1] strictly greater than
 * threshold. A value equal to threshold ends a run. On a tie, the first run
 * wins. Returns length, the run length (0 if no value is above threshold),
 * and start, the index of the run's first value (-1 if length is 0).
 * Assumes n >= 0. */
struct stats_run {
    int length;
    int start;
};
struct stats_run stats_longest_run_above(const float a[], int n, float threshold);

#endif
