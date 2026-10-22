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

/**
 * Fills t with TALLY_MAX calls to tally_create, so the whole pool is in use.
 *
 * @param t Where to store each tally (NULL if a create failed).
 * @return How many of the creates returned a tally.
 */
static int create_all(tally_t *t[TALLY_MAX]) {
    int created = 0;

    for (int i = 0; i < TALLY_MAX; i++) {
        t[i] = tally_create();
        if (t[i] != NULL) created++;
    }

    return created;
}

/**
 * Gives back every tally in t, so the next test starts with an empty pool.
 *
 * @param t The tallies from create_all.
 */
static void destroy_all(tally_t *t[TALLY_MAX]) {
    for (int i = 0; i < TALLY_MAX; i++) {
        tally_destroy(t[i]);
    }
}

/* =========================== tally_create tests =========================== */
static void check_create_max_then_one_more(void) {
    tally_t *t[TALLY_MAX];

    CHECK(create_all(t) == TALLY_MAX);
    for (int i = 1; i < TALLY_MAX; i++) {
        CHECK(t[i] != t[i - 1]);
    }
    CHECK(tally_create() == NULL);

    destroy_all(t);
}

/* ========================== tally_destroy tests =========================== */
static void check_create_after_destroy(void) {
    tally_t *t[TALLY_MAX];

    create_all(t);
    tally_destroy(t[2]);
    tally_t *again = tally_create();
    CHECK(again == t[2]);  /* the only free slot */
    CHECK(tally_create() == NULL);

    destroy_all(t);
}

static void check_destroy_null(void) {
    tally_t *t[TALLY_MAX];

    create_all(t);
    tally_destroy(NULL);
    CHECK(tally_create() == NULL);  /* NULL freed no slot */

    destroy_all(t);
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
    // tally_destroy
    check_create_after_destroy();
    check_destroy_null();

    print_summary();

    return fails != 0;
}
