/*
 * histogram.h:
 *
 * A text-based histogram of temperatures.
 */
#ifndef HISTOGRAM_H
#define HISTOGRAM_H

/**
 * Given an array of temperatures in degrees C and n, the number of valid values
 * in it, prints a histogram of the array (0..n-1) to stdout in 5 C bins from
 * 0 to 100 C. Values below 0 C are counted in the first bin and values at or
 * above 100 C in the last; empty bins are not printed. The array must hold
 * at least n elements. Assumes n >= 0 and that every value is a number (not
 * NaN) within the range of an int.
 */
void print_histogram(const float arr[], int n);

#endif
