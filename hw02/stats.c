/*
 * stats.c: statistics over an array of floats.
 */
#include "stats.h"

float stats_min(const float a[], int n) {
    float min = a[0];
    for (int i = 1; i < n; i++)
        if (a[i] < min)
            min = a[i];
    return min;
}

/* the largest of the first n-1 values, compared with the last one */
float stats_max(const float a[], int n) {
    if (n == 1) return a[0];
    float rest = stats_max(a, n - 1);

    return a[n - 1] > rest ? a[n - 1] : rest;
}

/* the sum stays a float, added in index order, so the printed mean matches
 * the starter to the last digit */
float stats_mean(const float a[], int n) {
    float sum = 0.0f;
    for (int i = 0; i < n; i++)
        sum += a[i];
    return sum / n;
}
