/*
 * test_store.c:
 *
 * Tests for store.c. Prints PASS or FAIL for each check, then one line with
 * the number passed and failed; exits nonzero if any check failed.
 */

#include <stdbool.h>
#include <stdio.h>

#include "store.h"

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
 * Builds a record with the given temperature and timestamp. The other
 * fields are fixed, since the store orders by temperature only.
 *
 * @param temp_c The temperature in °C.
 * @param t      The timestamp, used to tell duplicates apart.
 * @return The record.
 */
static record_t rec(float temp_c, time_t t) {
    record_t r = {
        .reading = { .timestamp = t, .temp_c = temp_c, .rh_pct = 50.0f },
        .state = CLOSED,
        .status = 0,
    };

    return r;
}

/* record_compare */

static void check_compare_same_step(void) {
    record_t a = rec(21.2f, 0), b = rec(21.4f, 0);

    CHECK(record_compare(&a, &b) == 0);
}

static void check_compare_neighboring_steps(void) {
    record_t a = rec(21.4f, 0), b = rec(21.6f, 0);

    CHECK(record_compare(&a, &b) < 0);
    CHECK(record_compare(&b, &a) > 0);
}

static void check_compare_on_boundary(void) {
    /* 21.5 starts the step 21.5 to 21.9; it does not end 21.0 to 21.4. */
    record_t lo = rec(21.4f, 0), edge = rec(21.5f, 0), hi = rec(21.9f, 0);

    CHECK(record_compare(&edge, &lo) > 0);
    CHECK(record_compare(&edge, &hi) == 0);
}

static void check_compare_negative(void) {
    /* floor puts -0.2 in the step -0.5 to -0.1, below 0.0. */
    record_t a = rec(-0.4f, 0), b = rec(-0.2f, 0), zero = rec(0.0f, 0);

    CHECK(record_compare(&a, &b) == 0);
    CHECK(record_compare(&b, &zero) < 0);
}

/* store_create, store_destroy */

static void check_create_while_in_use(void) {
    store_t *s = store_create();

    CHECK(s != NULL);
    CHECK(store_create() == NULL);

    store_destroy(s);
}

static void check_create_after_destroy(void) {
    store_t *s = store_create();
    record_t r = rec(20.0f, 0);
    store_insert(s, &r);
    store_destroy(s);

    s = store_create();
    CHECK(s != NULL);
    CHECK(store_count(s) == 0);   /* handed out again, emptied */
    CHECK(store_height(s) == 0);

    store_destroy(s);
}

static void check_destroy_empty(void) {
    store_destroy(store_create());

    store_t *s = store_create();
    CHECK(s != NULL);  /* the empty store was given back */

    store_destroy(s);
}

static void check_destroy_null(void) {
    store_destroy(NULL);

    CHECK(true);  /* reaching this line is the test */
}

/* store_insert, store_count, store_height */

static void check_insert_into_empty(void) {
    store_t *s = store_create();
    record_t r = rec(20.0f, 0);

    CHECK(store_insert(s, &r) == 0);
    CHECK(store_count(s) == 1);
    CHECK(store_height(s) == 1);

    store_destroy(s);
}

static void check_count_height_two(void) {
    store_t *s = store_create();
    record_t a = rec(20.0f, 0), b = rec(20.5f, 1);
    store_insert(s, &a);
    store_insert(s, &b);

    CHECK(store_count(s) == 2);
    CHECK(store_height(s) == 2);

    store_destroy(s);
}

static void check_count_height_hundred(void) {
    store_t *s = store_create();

    /* Ascending keys, one per insert: a plain tree would be a chain. */
    for (int i = 0; i < 100; i++) {
        record_t r = rec(i * 0.5f, i);
        store_insert(s, &r);
    }
    CHECK(store_count(s) == 100);
    CHECK(store_height(s) == 100);

    store_destroy(s);
}

static void check_count_height_hundred_duplicates(void) {
    store_t *s = store_create();

    /* One key, 100 records: a list per key keeps the height at 1. */
    for (int i = 0; i < 100; i++) {
        record_t r = rec(21.0f + (i % 5) * 0.1f, i);
        store_insert(s, &r);
    }
    CHECK(store_count(s) == 100);
    CHECK(store_height(s) == 1);

    store_destroy(s);
}

static void check_insert_full(void) {
    store_t *s = store_create();
    record_t r = rec(20.0f, 0);

    /* The cap is private to store.c, so fill until the store refuses. The
     * bound only stops a broken store from looping forever. */
    int inserted = 0;
    while (inserted < 100000 && store_insert(s, &r) == 0) inserted++;

    CHECK(inserted < 100000);
    CHECK(store_count(s) == inserted);
    CHECK(store_insert(s, &r) == -1);
    CHECK(store_count(s) == inserted);

    store_destroy(s);
}

/* store_find */

static void check_find_duplicates(void) {
    store_t *s = store_create();
    record_t in[] = { rec(21.0f, 0), rec(25.0f, 1), rec(21.2f, 2), rec(20.9f, 3),
                      rec(21.4f, 4) };
    for (int i = 0; i < 5; i++) store_insert(s, &in[i]);

    record_t got[5];
    CHECK(store_find(s, 21.3f, got, 5) == 3);
    /* Every one of them, in insertion order. */
    CHECK(got[0].reading.timestamp == 0);
    CHECK(got[1].reading.timestamp == 2);
    CHECK(got[2].reading.timestamp == 4);

    store_destroy(s);
}

static void check_find_missing(void) {
    store_t *s = store_create();
    record_t a = rec(20.0f, 0), b = rec(22.0f, 1);
    store_insert(s, &a);
    store_insert(s, &b);

    record_t got[2];
    CHECK(store_find(s, 21.0f, got, 2) == 0);

    store_destroy(s);
}

static void check_find_out_too_small(void) {
    store_t *s = store_create();
    for (int i = 0; i < 3; i++) {
        record_t r = rec(21.0f, i);
        store_insert(s, &r);
    }

    /* Room for 2 of 3 matches: the count says 3, and got[2] is untouched. */
    record_t got[3] = { [2] = rec(99.0f, 99) };
    CHECK(store_find(s, 21.0f, got, 2) == 3);
    CHECK(got[1].reading.timestamp == 1);
    CHECK(got[2].reading.timestamp == 99);

    store_destroy(s);
}

/* store_walk */

static void check_walk_key_order(void) {
    store_t *s = store_create();
    record_t in[] = { rec(25.0f, 0), rec(18.0f, 1), rec(21.2f, 2), rec(30.0f, 3),
                      rec(21.4f, 4), rec(18.3f, 5) };
    for (int i = 0; i < 6; i++) store_insert(s, &in[i]);

    record_t got[6];
    CHECK(store_walk(s, got, 6) == 6);

    time_t want[] = { 1, 5, 2, 4, 0, 3 };  /* by key, then insertion order */
    bool in_order = true;
    for (int i = 0; i < 6; i++) {
        if (got[i].reading.timestamp != want[i]) in_order = false;
    }
    CHECK(in_order);

    store_destroy(s);
}

static void check_walk_out_too_small(void) {
    store_t *s = store_create();
    for (int i = 0; i < 3; i++) {
        record_t r = rec(20.0f + i, i);
        store_insert(s, &r);
    }

    record_t got[3] = { [2] = rec(99.0f, 99) };
    CHECK(store_walk(s, got, 2) == 3);
    CHECK(got[1].reading.timestamp == 1);
    CHECK(got[2].reading.timestamp == 99);

    store_destroy(s);
}

int main(void) {
    /* record_compare */
    check_compare_same_step();
    check_compare_neighboring_steps();
    check_compare_on_boundary();
    check_compare_negative();
    /* store_create, store_destroy */
    check_create_while_in_use();
    check_create_after_destroy();
    check_destroy_empty();
    check_destroy_null();
    /* store_insert, store_count, store_height */
    check_insert_into_empty();
    check_count_height_two();
    check_count_height_hundred();
    check_count_height_hundred_duplicates();
    check_insert_full();
    /* store_find */
    check_find_duplicates();
    check_find_missing();
    check_find_out_too_small();
    /* store_walk */
    check_walk_key_order();
    check_walk_out_too_small();

    printf("\n%d passed, %d failed\n", total - fails, fails);

    return fails != 0;
}
