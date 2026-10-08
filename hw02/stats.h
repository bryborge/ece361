/*
 * stats.h: statistics over an array of floats.
 */
#ifndef STATS_H
#define STATS_H

/* Largest value in a[0..n-1], computed recursively: one stack frame per
 * element, so a very large n overflows the stack. Assumes n >= 1. */
float stats_max(const float a[], int n);

#endif
