// Tests for tally.c.

#include <stdio.h>

#include "tally.h"

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

/* =========================== tally_create tests =========================== */
static void check_create_max_then_one_more(void) {
    tally_t *t[TALLY_MAX];
    int created = 0;

    for (int i = 0; i < TALLY_MAX; i++) {
        t[i] = tally_create();
        if (t[i] != NULL) created++;
    }
    CHECK(created == TALLY_MAX);
    for (int i = 1; i < TALLY_MAX; i++) {
        CHECK(t[i] != t[i - 1]);
    }
    CHECK(tally_create() == NULL);
}

static void print_summary(void) {
    printf("\nSummary:\n");
    printf("--------\n");
    printf("%d total\n", total);
    printf("%d failed\n", fails);
    printf("\n");
}

int main(void) {
    // tally_create
    check_create_max_then_one_more();

    print_summary();

    return fails != 0;
}
