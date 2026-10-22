// Tests for tally.c.

#include <stdio.h>
#include <string.h>

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

/* ============================ tally_add tests ============================= */
static void check_add_counts_up(void) {
    tally_t *t = tally_create();

    CHECK(tally_add(t, "OPEN") == 1);
    CHECK(tally_add(t, "OPEN") == 2);
    CHECK(tally_add(t, "CLOSE") == 1);  /* goes in before OPEN: OPEN moves */
    CHECK(tally_add(t, "ALARM") == 1);  /* goes in first: CLOSE, OPEN move */
    CHECK(tally_add(t, "OPEN") == 3);   /* still found, count kept */
    CHECK(tally_add(t, "CLOSE") == 2);

    tally_destroy(t);
}

static void check_add_empty_name(void) {
    tally_t *t = tally_create();

    CHECK(tally_add(t, "") == -1);

    tally_destroy(t);
}

static void check_add_name_length_limit(void) {
    tally_t *t = tally_create();
    char longest[TALLY_LEN + 1];    /* TALLY_LEN letters and the '\0' */
    char too_long[TALLY_LEN + 2];   /* one letter more */

    memset(longest, 'A', TALLY_LEN);
    longest[TALLY_LEN] = '\0';
    memset(too_long, 'B', TALLY_LEN + 1);
    too_long[TALLY_LEN + 1] = '\0';

    CHECK(tally_add(t, longest) == 1);
    CHECK(tally_add(t, longest) == 2);
    CHECK(tally_add(t, too_long) == -1);

    tally_destroy(t);
}

static void check_add_null(void) {
    tally_t *t = tally_create();

    CHECK(tally_add(NULL, "OPEN") == -1);
    CHECK(tally_add(t, NULL) == -1);

    tally_destroy(t);
}

static void check_add_full_tally(void) {
    tally_t *t = tally_create();
    char name[8];

    for (int i = 0; i < TALLY_NAMES; i++) {
        snprintf(name, sizeof name, "N%02d", i);
        tally_add(t, name);
    }

    CHECK(tally_add(t, "NEW") == -1);  /* a new name is refused */
    CHECK(tally_add(t, "N00") == 2);   /* an existing one still counts */
    CHECK(tally_add(t, "N15") == 2);

    tally_destroy(t);
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
    // tally_add
    check_add_counts_up();
    check_add_empty_name();
    check_add_name_length_limit();
    check_add_null();
    check_add_full_tally();

    print_summary();

    return fails != 0;
}
