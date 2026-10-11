/*
 * stats.c:
 *
 * Summary statistics over arrays of float readings.
 */
#include <assert.h>

#include "stats.h"

/**
 * Returns the largest of the first n values in the given array.
 *
 * @param arr Array of values.
 * @param n   Number of elements in the array (must be >= 1).
 * @return The largest value in the array (0..n-1).
 */
float stats_max(const float arr[], int n) {
    assert(n >= 1);
    if (n == 1) return arr[0];
    float rest = stats_max(arr, n - 1);

    return arr[n - 1] > rest ? arr[n - 1] : rest;
}
