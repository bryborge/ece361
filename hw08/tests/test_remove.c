/*
 * test_remove.c:
 *
 * Tests for bst_remove. Prints PASS or FAIL for each check, then a summary;
 * exits nonzero if any check failed. make bonus runs these tests under
 * AddressSanitizer, which reports a leak if a removal frees nothing, and an
 * error if it frees a node twice.
 */
#include <stdbool.h>
#include <stdio.h>

#include "bst.h"

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

/* The original's main inserts these, in this order. */
static const int seven_keys[] = { 27, 14, 35, 10, 19, 31, 42 };

static bst_t *seven_key_tree(void) {
    bst_t *t = bst_create();

    for (size_t i = 0; i < sizeof seven_keys / sizeof seven_keys[0]; i++) {
        bst_insert(t, seven_keys[i]);
    }

    return t;
}

/*
 * The BONUS.md sequence. Visit counts show where each key ends up:
 *
 *         27              27              27              31
 *       /    \          /    \          /    \          /    \
 *     14      35  ->  14      35  ->  19      35  ->  19      35
 *    /  \    /  \       \    /  \            /  \                \
 *   10  19  31  42      19  31  42          31  42                42
 */
static void check_bonus_sequence(void) {
    bst_t *t = seven_key_tree();
    size_t visits = 99;

    /* 10 has no children. */
    CHECK(bst_remove(t, 10) == 0);
    CHECK(bst_size(t) == 6);
    CHECK(!bst_contains(t, 10, NULL));
    CHECK(bst_contains(t, 19, &visits) && visits == 3);  /* 27, 14, 19 */

    /* 14 has one child, 19, which takes its place. */
    CHECK(bst_remove(t, 14) == 0);
    CHECK(bst_size(t) == 5);
    CHECK(!bst_contains(t, 14, NULL));
    CHECK(bst_contains(t, 19, &visits) && visits == 2);  /* 27, 19 */

    /* 27 has two children: 31, the smallest key on its right, replaces it. */
    CHECK(bst_remove(t, 27) == 0);
    CHECK(bst_size(t) == 4);
    CHECK(bst_height(t) == 3);
    CHECK(!bst_contains(t, 27, NULL));
    CHECK(bst_contains(t, 31, &visits) && visits == 1);  /* 31 */
    CHECK(bst_contains(t, 19, &visits) && visits == 2);  /* 31, 19 */
    CHECK(bst_contains(t, 35, &visits) && visits == 2);  /* 31, 35 */
    CHECK(bst_contains(t, 42, &visits) && visits == 3);  /* 31, 35, 42 */
    bst_destroy(t);
}

static void check_not_found(void) {
    bst_t *t = seven_key_tree();
    bst_t *e = bst_create();

    CHECK(bst_remove(t, 15) == -1);
    CHECK(bst_size(t) == 7);
    CHECK(bst_height(t) == 3);
    CHECK(bst_remove(e, 1) == -1);
    CHECK(bst_size(e) == 0);
    CHECK(bst_remove(NULL, 1) == -1);
    bst_destroy(t);
    bst_destroy(e);
}

/*
 * 50 30 70 60 60 80: the second 60 is the right child of the first. Each
 * removal of 60 takes out exactly one copy.
 */
static void check_duplicates_one_at_a_time(void) {
    bst_t *t = bst_create();
    int keys[] = { 50, 30, 70, 60, 60, 80 };

    for (size_t i = 0; i < sizeof keys / sizeof keys[0]; i++) {
        bst_insert(t, keys[i]);
    }
    CHECK(bst_remove(t, 60) == 0);
    CHECK(bst_size(t) == 5);
    CHECK(bst_contains(t, 60, NULL));
    CHECK(bst_remove(t, 60) == 0);
    CHECK(bst_size(t) == 4);
    CHECK(!bst_contains(t, 60, NULL));
    CHECK(bst_remove(t, 60) == -1);
    CHECK(bst_size(t) == 4);
    bst_destroy(t);
}

/* A two-child root whose replacement, 60, has a duplicate below it. */
static void check_two_children_duplicate_min(void) {
    bst_t *t = bst_create();
    int keys[] = { 50, 30, 70, 60, 60, 80 };
    size_t visits = 99;

    for (size_t i = 0; i < sizeof keys / sizeof keys[0]; i++) {
        bst_insert(t, keys[i]);
    }
    CHECK(bst_remove(t, 50) == 0);
    CHECK(bst_size(t) == 5);
    CHECK(!bst_contains(t, 50, NULL));
    CHECK(bst_contains(t, 60, &visits) && visits == 1);  /* the new root */
    CHECK(bst_contains(t, 80, &visits) && visits == 3);  /* 60, 70, 80 */
    bst_destroy(t);
}

/* Removing every key, in insertion order, leaves an empty tree. */
static void check_remove_all(void) {
    bst_t *t = seven_key_tree();
    bool removed = true;

    for (size_t i = 0; i < sizeof seven_keys / sizeof seven_keys[0]; i++) {
        if (bst_remove(t, seven_keys[i]) != 0) removed = false;
    }
    CHECK(removed);
    CHECK(bst_size(t) == 0);
    CHECK(bst_height(t) == 0);
    CHECK(bst_insert(t, 1) == 0);        /* still usable */
    CHECK(bst_size(t) == 1);
    bst_destroy(t);
}

int main(void) {
    check_bonus_sequence();
    check_not_found();
    check_duplicates_one_at_a_time();
    check_two_children_duplicate_min();
    check_remove_all();

    printf("%d of %d checks passed\n", total - fails, total);

    return fails != 0;
}
