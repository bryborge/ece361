#include <stdio.h>

#include "../stats.h"

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


static void check_one_value(void) {
    float a[] = {42.0f};
    CHECK(stats_max(a, 1) == 42.0f);
    CHECK(stats_min(a, 1) == 42.0f);
    CHECK(stats_mean(a, 1) == 42.0f);
}

static void check_max_at_ends(void) {
    float start[] = {9.0f, 1.0f, 2.0f, 3.0f};
    CHECK(stats_max(start, 4) == 9.0f);

    float end[] = {1.0f, 2.0f, 3.0f, 9.0f};
    CHECK(stats_max(end, 4) == 9.0f);
}

static void check_min_at_ends(void) {
    float start[] = {-9.0f, 1.0f, 2.0f, 3.0f};
    CHECK(stats_min(start, 4) == -9.0f);

    float end[] = {1.0f, 2.0f, 3.0f, -9.0f};
    CHECK(stats_min(end, 4) == -9.0f);
}

static void check_mean(void) {
    float a[] = {1.0f, 2.0f, 3.0f, 4.0f};
    CHECK(stats_mean(a, 4) == 2.5f);

    float mixed[] = {-3.0f, 3.0f};
    CHECK(stats_mean(mixed, 2) == 0.0f);
}

static void check_ignores_values_past_n(void) {
    float a[] = {5.0f, 1.0f, 3.0f, 100.0f, -100.0f};
    CHECK(stats_max(a, 3) == 5.0f);
    CHECK(stats_min(a, 3) == 1.0f);
    CHECK(stats_mean(a, 3) == 3.0f);
}

static int run_is(struct stats_run r, int length, int start) {
    return r.length == length && r.start == start;
}

static void check_run_none(void) {
    float below[] = {1.0f, 2.0f, 3.0f};
    CHECK(run_is(stats_longest_run_above(below, 3, 5.0f), 0, -1));

    float equal[] = {5.0f, 5.0f};
    CHECK(run_is(stats_longest_run_above(equal, 2, 5.0f), 0, -1));

    CHECK(run_is(stats_longest_run_above(below, 0, 0.0f), 0, -1));
}

static void check_run_at_ends(void) {
    float start[] = {9.0f, 9.0f, 1.0f, 9.0f};
    CHECK(run_is(stats_longest_run_above(start, 4, 5.0f), 2, 0));

    float end[] = {9.0f, 1.0f, 9.0f, 9.0f};
    CHECK(run_is(stats_longest_run_above(end, 4, 5.0f), 2, 2));

    float all[] = {9.0f, 9.0f, 9.0f};
    CHECK(run_is(stats_longest_run_above(all, 3, 5.0f), 3, 0));
}

static void check_run_tie_keeps_first(void) {
    float a[] = {9.0f, 9.0f, 1.0f, 9.0f, 9.0f};
    CHECK(run_is(stats_longest_run_above(a, 5, 5.0f), 2, 0));
}

static void check_run_ignores_values_past_n(void) {
    float a[] = {9.0f, 1.0f, 9.0f, 9.0f, 9.0f};
    CHECK(run_is(stats_longest_run_above(a, 2, 5.0f), 1, 0));
}

static void print_summary(void) {
    printf("\nSummary:\n");
    printf("--------\n");
    printf("%d total\n", total);
    printf("%d failed\n", fails);
    printf("\n");
}

int main(void) {
    // statistics
    check_one_value();
    check_max_at_ends();
    check_min_at_ends();
    check_mean();
    check_ignores_values_past_n();
    // longest run
    check_run_none();
    check_run_at_ends();
    check_run_tie_keeps_first();
    check_run_ignores_values_past_n();

    print_summary();

    return fails != 0;
}
