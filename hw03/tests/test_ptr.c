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

    print_summary();

    return fails != 0;
}
