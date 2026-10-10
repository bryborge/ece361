/*
 * histogram.h: a text histogram of temperatures.
 */
#ifndef HISTOGRAM_H
#define HISTOGRAM_H

/* Prints the histogram of a[0..n-1] to stdout: "histogram:" then one line per
 * non-empty 5 C bin from 0 to 100 C. Values below 0 go to the first bin,
 * values at or above 100 go to the last. Assumes n >= 0. */
void histogram_print(const float a[], int n);

#endif
