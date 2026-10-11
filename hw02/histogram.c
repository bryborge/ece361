/*
 * histogram.c:
 *
 * A text-based histogram of temperatures.
 */
#include <assert.h>
#include <stdio.h>

#include "histogram.h"

#define NUM_BINS  20      // 0..100 C in 5 C increments
#define BIN_WIDTH 5.0f

/**
 * Returns the bin that temp_c falls in. Values outside 0..100 C, including
 * infinities, are clamped to the first or last bin; NaN goes in the first.
 *
 * @param temp_c Temperature.
 * @return The bin index (0..NUM_BINS-1).
 */
static int bin_index(float temp_c) {
    // Clamp before converting: an out-of-range or NaN float to int conversion
    // is undefined. Written as !(>=) so that NaN, which fails every
    // comparison, lands here too.
    if (!(temp_c >= 0.0f)) return 0;
    if (temp_c >= NUM_BINS * BIN_WIDTH) return NUM_BINS - 1;

    return (int) (temp_c / BIN_WIDTH);
}

/**
 * Prints a histogram of the first n temperatures in the given array to
 * stdout: one line per non-empty bin with the bin's range, a bar of '*'
 * characters, and the count.
 *
 * @param arr Array of temperatures in degrees C (must hold at least n
 *            elements).
 * @param n   Number of valid values in arr (must be >= 0).
 */
void print_histogram(const float arr[], int n) {
    assert(n >= 0);
    int bins[NUM_BINS] = {0};

    for (int i = 0; i < n; i++)
        bins[bin_index(arr[i])]++;

    printf("histogram:\n");

    for (int b = 0; b < NUM_BINS; b++) {
        if (bins[b] == 0) continue;

        printf("  %5.1f to %5.1f C | ", b * BIN_WIDTH, (b + 1) * BIN_WIDTH);

        for (int k = 0; k < bins[b]; k++)
            putchar('*');

        printf(" %d\n", bins[b]);
    }
}
