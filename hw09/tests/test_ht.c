/*
 * test_ht.c:
 *
 * Tests for ht. Prints PASS or FAIL for each check, then a summary; exits
 * nonzero if any check failed. Leaks are not checked here: make memcheck
 * runs these tests under AddressSanitizer, which reports them at exit.
 */
#include <stdbool.h>
#include <stdio.h>

#include "ht.h"

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

/*
 * A new table with nbuckets buckets, holding first, first + step, ..., up to
 * last. Returns NULL if the table can't be created or any add fails.
 */
static ht_t *table_of(size_t nbuckets, int first, int step, int last) {
    ht_t *t = ht_create(nbuckets);
    if (t == NULL) return NULL;

    for (int key = first; key <= last; key += step) {
        if (ht_add(t, key) != 0) {
            ht_destroy(t);
            return NULL;
        }
    }

    return t;
}

/* Without this check, key % 0 would be a division by zero later. */
static void check_zero_buckets(void) {
    CHECK(ht_create(0) == NULL);
}

static void check_create(void) {
    ht_t *t = ht_create(10);

    CHECK(t != NULL);
    ht_destroy(t);
}

static void check_empty_table(void) {
    ht_t *t = ht_create(10);

    CHECK(ht_size(t) == 0);
    CHECK(ht_count(t, 5) == 0);
    CHECK(ht_longest_chain(t) == 0);
    ht_destroy(t);
}

/* A second node for the same key would make the size 2. */
static void check_same_key_three_times(void) {
    ht_t *t = ht_create(10);

    CHECK(ht_add(t, 5) == 0);
    CHECK(ht_add(t, 5) == 0);
    CHECK(ht_add(t, 5) == 0);
    CHECK(ht_count(t, 5) == 3);
    CHECK(ht_size(t) == 1);
    ht_destroy(t);
}

/*
 * 5 % 10 == 15 % 10 == 25 % 10 == 5: a search that stopped at the bucket
 * instead of the key would mix them up.
 */
static void check_same_bucket(void) {
    ht_t *t = ht_create(10);

    ht_add(t, 5);
    ht_add(t, 15);
    CHECK(ht_size(t) == 2);
    CHECK(ht_count(t, 5) == 1);
    CHECK(ht_count(t, 15) == 1);
    CHECK(ht_count(t, 25) == 0);
    ht_destroy(t);
}

static void check_one_to_thousand(void) {
    ht_t *t = table_of(1000, 1, 1, 1000);

    CHECK(t != NULL);
    CHECK(ht_size(t) == 1000);

    bool each_once = true;
    for (int key = 1; key <= 1000; key++) {
        if (ht_count(t, key) != 1) each_once = false;
    }
    CHECK(each_once);
    ht_destroy(t);
}

/*
 * Only that every add succeeds and destroy returns; that it frees every node
 * is make memcheck's job. 10 buckets make each chain 100 nodes long.
 */
static void check_destroy_full(void) {
    ht_t *t = table_of(10, 1, 1, 1000);

    CHECK(t != NULL);
    ht_destroy(t);
}

/*
 * The longest chain in table_of(nbuckets, first, step, last), which it then
 * destroys. If table_of fails, this gives 0, which fails every row below.
 */
static size_t longest_of(size_t nbuckets, int first, int step, int last) {
    ht_t *t = table_of(nbuckets, first, step, last);
    size_t longest = ht_longest_chain(t);

    ht_destroy(t);
    return longest;
}

/* The four rows of the assignment's Table 2. */
static void check_table_2(void) {
    CHECK(longest_of(10, 1, 1, 1000) == 100);
    CHECK(longest_of(1000, 1, 1, 1000) == 1);
    CHECK(longest_of(1000, 10, 10, 10000) == 10);
    CHECK(longest_of(1009, 10, 10, 10000) == 1);
}

static void check_null_arguments(void) {
    ht_destroy(NULL);
    CHECK(ht_add(NULL, 1) == -1);
    CHECK(ht_count(NULL, 1) == 0);
    CHECK(ht_size(NULL) == 0);
    CHECK(ht_longest_chain(NULL) == 0);
}

int main(void) {
    check_zero_buckets();
    check_create();
    check_empty_table();
    check_same_key_three_times();
    check_same_bucket();
    check_one_to_thousand();
    check_destroy_full();
    check_table_2();
    check_null_arguments();

    printf("%d of %d checks passed\n", total - fails, total);

    return fails != 0;
}
