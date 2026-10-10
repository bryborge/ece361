/*
 * test_stats.c: unit tests for the stats module.
 *
 * Prints PASS or FAIL for each check and a summary, and exits with 1 if any
 * check fails, so that make test fails.
 */
#include <stdio.h>

#include "../stats.h"

#define BIG_N 10000

static int total;
static int fails;

#define CHECK(cond) do {                                               \
    total++;                                                           \
    if (cond) printf("(PASS) Test: %s\n", #cond);                      \
    else {                                                             \
        printf("(FAIL) Test: %s (line %d)\n", #cond, __LINE__);        \
        fails++;                                                       \
    }                                                                  \
} while (0)

/**
 * One value: it is the minimum, the maximum, and the mean.
 */
static void check_one_value(void) {
    float a[] = {42.0f};
    CHECK(stats_min(a, 1) == 42.0f);
    CHECK(stats_max(a, 1) == 42.0f);
    CHECK(stats_mean(a, 1) == 42.0f);
}

/**
 * The maximum is found at either end of the array.
 */
static void check_max_at_ends(void) {
    float start[] = {9.0f, 1.0f, 2.0f, 3.0f};
    float end[] = {1.0f, 2.0f, 3.0f, 9.0f};
    CHECK(stats_max(start, 4) == 9.0f);
    CHECK(stats_max(end, 4) == 9.0f);
}

/**
 * Longest run above a threshold: a value equal to the threshold ends a run,
 * the first of two equal runs wins, and no run gives length 0 and start -1.
 */
static void check_longest_run(void) {
    float equal[] = {31.0f, 31.0f, 30.0f, 31.0f};
    struct stats_run r = stats_longest_run_above(equal, 4, 30.0f);
    CHECK(r.length == 2 && r.start == 0);

    float tie[] = {31.0f, 0.0f, 31.0f};
    r = stats_longest_run_above(tie, 3, 30.0f);
    CHECK(r.length == 1 && r.start == 0);

    float below[] = {1.0f, 2.0f};
    r = stats_longest_run_above(below, 2, 30.0f);
    CHECK(r.length == 0 && r.start == -1);
}

/**
 * The recursive maximum on 10,000 values: one stack frame per value.
 */
static void check_max_big(void) {
    float a[BIG_N];
    for (int i = 0; i < BIG_N; i++)
        a[i] = (float) i;
    a[BIG_N / 2] = 2.0f * BIG_N;
    CHECK(stats_max(a, BIG_N) == 2.0f * BIG_N);
}

/**
 * Prints a summary of checks run and how many of those checks failed.
 */
static void print_summary(void) {
    printf("\nSummary:\n");
    printf("--------\n");
    printf("%d total\n", total);
    printf("%d failed\n", fails);
    printf("\n");
}

int main(void) {
    check_one_value();
    check_max_at_ends();
    check_longest_run();
    check_max_big();

    print_summary();

    return fails != 0;
}
