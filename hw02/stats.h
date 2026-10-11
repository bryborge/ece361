/*
 * stats.h:
 *
 * Statistics over an array of floating point numbers.
 */
#ifndef STATS_H
#define STATS_H

/**
 * Given an array of values and the length of the array, returns the largest
 * value in the array (0..n-1). Assumes n >= 1 and that n is not so large that
 * it will result in a stack overflow.
 */
float stats_max(const float arr[], int n);

#endif
