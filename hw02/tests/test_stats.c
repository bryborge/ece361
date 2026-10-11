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
}

static void check_max_at_ends(void) {
    float start[] = {9.0f, 1.0f, 2.0f, 3.0f};
    CHECK(stats_max(start, 4) == 9.0f);

    float end[] = {1.0f, 2.0f, 3.0f, 9.0f};
    CHECK(stats_max(end, 4) == 9.0f);
}

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

    print_summary();

    return fails != 0;
}
