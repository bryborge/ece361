/*
 * stats.h:
 *
 * Statistics over an array of floating point numbers.
 */
#ifndef STATS_H
#define STATS_H

/**
 * Given an array of values and n, the number of valid values in it, returns the
 * largest value in the array (0..n-1). arr must hold at least n elements.
 * Assumes n > 0 and that n is not so large that it will result in a stack
 * overflow.
 */
float stats_max(const float arr[], int n);

/**
 * Given an array of values and n, the number of valid values in it, returns the
 * smallest value in the array (0..n-1). arr must hold at least n elements.
 * Assumes n > 0.
 */
float stats_min(const float arr[], int n);

/**
 * Given an array of values and n, the number of valid values in it, returns the
 * average value in the array (0..n-1). arr must hold at least n elements.
 * Assumes n > 0.
 */
float stats_mean(const float arr[], int n);

/**
 * A run of consecutive values in an array: its length and the index of its
 * first value.
 */
struct stats_run {
    int length;
    int start;
};

/**
 * Given an array of values, the number of valid values in it, and a
 * threshold, returns the longest run of consecutive values in the array
 * (0..n-1) strictly greater than the threshold. The array must hold at least n
 * elements. Assumes n >= 0.
 */
struct stats_run stats_longest_run_above(const float arr[], int n, float th);

#endif
