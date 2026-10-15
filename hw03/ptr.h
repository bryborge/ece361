/*
 * ptr.h:
 *
 * A pointer library: functions that change the caller's variables through
 * pointers and walk arrays with pointer arithmetic.
 */
#ifndef PTR_H
#define PTR_H

// const float *find_above(const float *a, int n, float threshold);
// void reverse(float *a, int n);

/**
 * Given the addresses of two floats, exchanges the values stored there: after
 * the call, *a holds the old *b and *b holds the old *a. Returns nothing.
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

#endif
