// fmemopen is POSIX, hidden by -std=c11 without this.
#define _POSIX_C_SOURCE 200809L

#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "../histogram.h"

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
 * Captures print_histogram(arr, n)'s stdout output and checks it matches
 * `expected` exactly.
 *
 * @param arr      Array of temperatures in degrees C.
 * @param n        Number of valid values in arr.
 * @param expected Exact expected output, including the trailing newline.
 * @return `true` if the captured output matches `expected`.
 */
static bool prints_histogram(const float arr[], int n, const char *expected) {
    // glibc's fmemopen only NUL-terminates after a write, so if a regression
    // printed nothing, buf would otherwise be garbage.
    char buf[256] = {0};
    FILE *mem = fmemopen(buf, sizeof buf, "w");
    if (mem == NULL) return false;

    FILE *saved_stdout = stdout;
    stdout = mem;
    print_histogram(arr, n);
    fflush(mem);
    stdout = saved_stdout;

    bool matches = strcmp(buf, expected) == 0;
    fclose(mem);
    return matches;
}

static void check_no_values(void) {
    float a[] = {23.5f};
    CHECK(prints_histogram(a, 0, "histogram:\n"));
}

static void check_one_value(void) {
    float a[] = {23.5f};
    CHECK(prints_histogram(a, 1,
                           "histogram:\n"
                           "   20.0 to  25.0 C | * 1\n"));
}

static void check_bin_lower_edge_inclusive(void) {
    float a[] = {25.0f};
    CHECK(prints_histogram(a, 1,
                           "histogram:\n"
                           "   25.0 to  30.0 C | * 1\n"));
}

static void check_clamps_to_end_bins(void) {
    float below[] = {-12.0f};
    CHECK(prints_histogram(below, 1,
                           "histogram:\n"
                           "    0.0 to   5.0 C | * 1\n"));

    float above[] = {100.0f, 150.0f};
    CHECK(prints_histogram(above, 2,
                           "histogram:\n"
                           "   95.0 to 100.0 C | ** 2\n"));
}

static void check_clamps_non_finite_and_huge(void) {
    float low[] = {NAN, -INFINITY, -1e30f};
    CHECK(prints_histogram(low, 3,
                           "histogram:\n"
                           "    0.0 to   5.0 C | *** 3\n"));

    float high[] = {INFINITY, 1e30f};
    CHECK(prints_histogram(high, 2,
                           "histogram:\n"
                           "   95.0 to 100.0 C | ** 2\n"));
}

static void check_skips_empty_bins(void) {
    float a[] = {41.0f, 12.0f, 13.0f};
    CHECK(prints_histogram(a, 3,
                           "histogram:\n"
                           "   10.0 to  15.0 C | ** 2\n"
                           "   40.0 to  45.0 C | * 1\n"));
}

static void check_ignores_values_past_n(void) {
    float a[] = {12.0f, 41.0f};
    CHECK(prints_histogram(a, 1,
                           "histogram:\n"
                           "   10.0 to  15.0 C | * 1\n"));
}

static void print_summary(void) {
    printf("\nSummary:\n");
    printf("--------\n");
    printf("%d total\n", total);
    printf("%d failed\n", fails);
    printf("\n");
}

int main(void) {
    check_no_values();
    check_one_value();
    check_bin_lower_edge_inclusive();
    check_clamps_to_end_bins();
    check_clamps_non_finite_and_huge();
    check_skips_empty_bins();
    check_ignores_values_past_n();

    print_summary();

    return fails != 0;
}
