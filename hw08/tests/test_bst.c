/*
 * test_bst.c:
 *
 * Tests for bst. Prints PASS or FAIL for each check, then a summary; exits
 * nonzero if any check failed. Leaks are not checked here: make memcheck
 * runs these tests under AddressSanitizer, which reports them at exit.
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

static void check_create(void) {
    bst_t *t = bst_create();

    CHECK(t != NULL);
    CHECK(bst_size(t) == 0);
    CHECK(bst_height(t) == 0);
    bst_destroy(t);
}

/* The original's main inserts these, in this order. */
static const int seven_keys[] = { 27, 14, 35, 10, 19, 31, 42 };

static void check_seven_keys(void) {
    bst_t *t = bst_create();

    for (size_t i = 0; i < sizeof seven_keys / sizeof seven_keys[0]; i++) {
        bst_insert(t, seven_keys[i]);
    }
    CHECK(bst_size(t) == 7);
    CHECK(bst_height(t) == 3);

    size_t visits = 99;
    CHECK(bst_contains(t, 31, &visits));     /* 27, 35, 31 */
    CHECK(visits == 3);
    visits = 99;
    CHECK(!bst_contains(t, 15, &visits));    /* 27, 14, 19 */
    CHECK(visits == 3);
    CHECK(bst_contains(t, 31, NULL));        /* visits may be NULL */
    bst_destroy(t);
}

/* The second 5 becomes the first's child, so the tree grows a level. */
static void check_duplicate_kept(void) {
    bst_t *t = bst_create();

    CHECK(bst_insert(t, 5) == 0);
    CHECK(bst_height(t) == 1);
    CHECK(bst_insert(t, 5) == 0);
    CHECK(bst_size(t) == 2);
    CHECK(bst_height(t) == 2);
    bst_destroy(t);
}

/* Each key is larger than all before it, so every node is a right child. */
static void check_increasing_order(void) {
    bst_t *t = bst_create();

    for (int i = 1; i <= 100; i++) bst_insert(t, i);
    CHECK(bst_size(t) == 100);
    CHECK(bst_height(t) == 100);
    bst_destroy(t);
}

/* Only that it returns; that it frees every node is make memcheck's job. */
static void check_destroy_full(void) {
    bst_t *t = bst_create();
    bool inserted = true;

    for (size_t i = 0; i < sizeof seven_keys / sizeof seven_keys[0]; i++) {
        if (bst_insert(t, seven_keys[i]) != 0) inserted = false;
    }
    CHECK(inserted);
    bst_destroy(t);
}

static void check_null_arguments(void) {
    bst_destroy(NULL);
    CHECK(bst_insert(NULL, 1) == -1);
    CHECK(bst_size(NULL) == 0);
    CHECK(bst_height(NULL) == 0);

    size_t visits = 99;
    CHECK(!bst_contains(NULL, 1, &visits));
    CHECK(visits == 0);
}

/* The input that crashed the faithful port: root is NULL before the loop. */
static void check_contains_empty(void) {
    bst_t *t = bst_create();
    size_t visits = 99;

    CHECK(!bst_contains(t, 1, &visits));
    CHECK(visits == 0);
    bst_destroy(t);
}

int main(void) {
    check_create();
    check_seven_keys();
    check_duplicate_kept();
    check_increasing_order();
    check_destroy_full();
    check_null_arguments();
    check_contains_empty();

    printf("%d of %d checks passed\n", total - fails, total);

    return fails != 0;
}
