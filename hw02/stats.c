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
 * @param arr Array of values (must hold at least n elements).
 * @param n   Number of valid values in arr (must be > 0).
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
 * @param arr Array of values (must hold at least n elements).
 * @param n   Number of valid values in arr (must be > 0).
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
 * @param arr Array of values (must hold at least n elements).
 * @param n   Number of valid values in the array (must be > 0).
 * @return The average value in the array (0..n-1).
 */
float stats_mean(const float arr[], int n) {
    assert(n > 0);
    float sum = 0.0f;

    for (int i = 0; i < n; i++)
        sum += arr[i];

    return sum / n;
}

/**
 * Returns the longest run of consecutive values in the given array that are
 * strictly greater than the given threshold. If several runs share the longest
 * length, the first one in the array is returned. If no value is greater than
 * the threshold, it returns {0, -1}.
 *
 * @param arr Array of values (must hold at least n elements).
 * @param n   Number of valid values in the array (must be >= 0).
 * @param th  The threshold.
 * @return The length and start index of the longest run of values greater
 *         than the threshold.
 */
struct stats_run stats_longest_run_above(const float arr[], int n, float th) {
    assert(n >= 0);
    struct stats_run best = {0, -1};
    int run = 0, start = 0;

    for (int i = 0; i < n; i++) {
        // A value greater than the threshold extends the current run, and
        // starts a new one if no run is in progress; a longer run replaces the
        // best.
        if (arr[i] > th) {
            if (run == 0) start = i;
            run++;

            if (run > best.length) {
                best.length = run;
                best.start = start;
            }
        } else {
            run = 0;
        }
    }

    return best;
}
