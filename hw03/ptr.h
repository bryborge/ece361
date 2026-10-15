/*
 * ptr.h:
 *
 * A pointer library: functions that change the caller's variables through
 * pointers and walk arrays with pointer arithmetic.
 */
#ifndef PTR_H
#define PTR_H

/**
 * Given the addresses of two floats, exchanges the values stored there. After
 * the call, *a holds the old *b and *b holds the old *a. Nothing is returned.
 * Assumes a and b are both non-NULL and point to valid floats.
 */
void swap_float(float *a, float *b);

/**
 * Given an array a, the number n of valid values in it, and the addresses min
 * and max, stores the smallest of a[0..n-1] in *min and the largest in *max,
 * and returns 0. If n < 1 or any pointer is NULL, returns -1 and leaves *min
 * and *max unchanged. a is const because minmax only reads it. Assumes a holds
 * at least n elements.
 */
int minmax(const float *a, int n, float *min, float *max);

/**
 * Given an array a, the number n of valid values in it, and a threshold,
 * returns a pointer to the first value in a[0..n-1] strictly greater than
 * threshold, or NULL if there is none (including when n < 1) or a is NULL.
 * The pointer points into a, so it is const like a. Assumes a holds at least n
 * elements.
 */
const float *find_above(const float *a, int n, float threshold);

/**
 * Given an array a and the number n of valid values in it, reverses a[0..n-1]
 * in place, so a[0] and a[n-1] trade places, then a[1] and a[n-2], and so on.
 * Values past a[n-1] are untouched. Returns nothing. Does nothing if a is NULL
 * or n < 2. Assumes a holds at least n elements.
 */
void reverse(float *a, int n);

#endif
