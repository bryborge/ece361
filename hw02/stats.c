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
 * @param n   Number of valid values in arr (must be > 0); arr must hold at
 *            least n elements.
 * @return The largest value in the array (0..n-1).
 */
float stats_max(const float arr[], int n) {
    assert(n > 0);
    if (n == 1) return arr[0];
    float rest = stats_max(arr, n - 1);

    return arr[n - 1] > rest ? arr[n - 1] : rest;
}

/**
 * Returns the smallest of the first n values in the given array.
 *
 * @param arr Array of values.
 * @param n   Number of valid values in arr (must be > 0); arr must hold at
 *            least n elements.
 * @return The smallest value in the array (0..n-1).
 */
float stats_min(const float arr[], int n) {
    assert(n > 0);
    float min = arr[0];

    for (int i = 1; i < n; i++)
        if (arr[i] < min) min = arr[i];

    return min;
}

/**
 * Returns the average value of the first n values in the given array.
 *
 * @param arr Array of values.
 * @param n   Number of valid values in arr (must be > 0); arr must hold at
 *            least n elements.
 * @return The average value in the array (0..n-1).
 */
float stats_mean(const float arr[], int n) {
    assert(n > 0);
    float sum = 0.0f;

    for (int i = 0; i < n; i++)
        sum += arr[i];

    return sum / n;
}
