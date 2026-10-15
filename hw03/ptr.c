/*
 * A pointer library.
 */
#include <stddef.h>

#include "ptr.h"

/**
 * Returns a pointer to the first value greater than threshold. Returns NULL if
 * a is NULL, n is 0 or less, or if no value greater than threshold was found.
 *
 * @param *a The array to search (read only).
 * @param n  The number of values in the array to consider.
 * @param th The threshold; a value equal to it does not count.
 * @return A pointer to the first floating point value greater than threshold.
 */
const float *find_above(const float *a, int n, float th) {
    if (a == NULL || n < 1) return NULL;

    for (const float *p = a; p < a + n; p++) {
        if (*p > th) return p;
    }

    return NULL;
}

/**
 * Stores the smallest and largest of a[0..n-1] in *min and *max and returns 0;
 * If n < 1 or any pointer is NULL, it changes nothing and returns -1.
 *
 * @param *a   The array to scan (read only).
 * @param n    The number of values in the array to consider.
 * @param *min Where to store the smallest value of a[0..n-1].
 * @param *max Where to store the largest value of a[0..n-1].
 * @return 0 on success, or -1 if n < 1 or any pointer is NULL.
 */
int minmax(const float *a, int n, float *min, float *max) {
    if (a == NULL || min == NULL || max == NULL || n < 1) return -1;

    float lo = *a, hi = *a;
    for (const float *p = a + 1; p < a + n; p++) {
        if (*p < lo) lo = *p;
        if (*p > hi) hi = *p;
    }

    *min = lo;
    *max = hi;

    return 0;
}

/**
 * Reverses a[0..n-1] in place. It does nothing if a is NULL or n < 2.
 *
 * @param *a The array to reverse; modified in place.
 * @param n  The number of values in a to reverse.
 */
void reverse(float *a, int n) {
    if (a == NULL || n < 2) return;

    for (float *lo = a, *hi = a + n - 1; lo < hi; lo++, hi--) {
        swap_float(lo, hi);
    }
}

/**
 * Swaps *a and *b.
 *
 * @param *a The first float; receives the old value of *b.
 * @param *b The second float; receives the old value of *a.
 */
void swap_float(float *a, float *b) {
    float x = *a;
    *a = *b;
    *b = x;
}
