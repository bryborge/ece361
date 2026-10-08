/*
 * stats.c: statistics over an array of floats.
 */
#include "stats.h"

/* the largest of the first n-1 values, compared with the last one */
float stats_max(const float a[], int n) {
    if (n == 1) return a[0];
    float rest = stats_max(a, n - 1);

    return a[n - 1] > rest ? a[n - 1] : rest;
}
