#include <stddef.h>
#include <stdio.h>

#include "ptr.h"

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


static void check_swap_float(void) {
    float a = 3.3f, b = 9.2f;
    swap_float(&a, &b);
    CHECK(a == 9.2f);
    CHECK(b == 3.3f);
}

static void check_swap_float_twice_restores(void) {
    float a = -1.5f, b = 0.0f;
    swap_float(&a, &b);
    swap_float(&a, &b);
    CHECK(a == -1.5f);
    CHECK(b == 0.0f);
}

static void check_minmax_several(void) {
    float a[] = {4.0f, -2.5f, 7.0f, 1.0f};
    float min = 0.0f, max = 0.0f;
    CHECK(minmax(a, 4, &min, &max) == 0);
    CHECK(min == -2.5f);
    CHECK(max == 7.0f);
}

static void check_minmax_at_ends(void) {
    float a[] = {-8.0f, 2.0f, 3.0f, 9.0f};
    float min = 0.0f, max = 0.0f;
    CHECK(minmax(a, 4, &min, &max) == 0);
    CHECK(min == -8.0f);
    CHECK(max == 9.0f);
}

static void check_minmax_one_value(void) {
    float a[] = {42.0f};
    float min = 0.0f, max = 0.0f;
    CHECK(minmax(a, 1, &min, &max) == 0);
    CHECK(min == 42.0f);
    CHECK(max == 42.0f);
}

static void check_minmax_ignores_values_past_n(void) {
    float a[] = {5.0f, 1.0f, 3.0f, 100.0f, -100.0f};
    float min = 0.0f, max = 0.0f;
    CHECK(minmax(a, 3, &min, &max) == 0);
    CHECK(min == 1.0f);
    CHECK(max == 5.0f);
}

static void check_minmax_n_zero(void) {
    float a[] = {1.0f, 2.0f};
    float min = 111.0f, max = 222.0f;
    CHECK(minmax(a, 0, &min, &max) == -1);
    CHECK(min == 111.0f);
    CHECK(max == 222.0f);
}

static void check_minmax_null(void) {
    float a[] = {1.0f, 2.0f};
    float min = 111.0f, max = 222.0f;

    CHECK(minmax(NULL, 2, &min, &max) == -1);
    CHECK(min == 111.0f);
    CHECK(max == 222.0f);

    CHECK(minmax(a, 2, NULL, &max) == -1);
    CHECK(max == 222.0f);

    CHECK(minmax(a, 2, &min, NULL) == -1);
    CHECK(min == 111.0f);
}

static void print_summary(void) {
    printf("\nSummary:\n");
    printf("--------\n");
    printf("%d total\n", total);
    printf("%d failed\n", fails);
    printf("\n");
}

int main(void) {
    // swap_float
    check_swap_float();
    check_swap_float_twice_restores();
    // minmax
    check_minmax_several();
    check_minmax_at_ends();
    check_minmax_one_value();
    check_minmax_ignores_values_past_n();
    check_minmax_n_zero();
    check_minmax_null();

    print_summary();

    return fails != 0;
}
