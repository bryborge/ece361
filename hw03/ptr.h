/*
 * ptr.h:
 *
 * A pointer library: functions that change the caller's variables through
 * pointers and walk arrays with pointer arithmetic.
 */
#ifndef PTR_H
#define PTR_H

// const float *find_above(const float *a, int n, float threshold);
// int minmax(const float *a, int n, float *min, float *max);
// void reverse(float *a, int n);

/**
 * Given the addresses of two floats, exchanges the values stored there: after
 * the call, *a holds the old *b and *b holds the old *a. Returns nothing.
 * Assumes a and b are both non-NULL and point to valid floats.
 */
void swap_float(float *a, float *b);

#endif
