/*
 * test_istack.c:
 *
 * Tests for istack. Prints PASS or FAIL for each check, then a summary; exits
 * nonzero if any check failed. Leaks are not checked here: make memcheck
 * runs these tests under AddressSanitizer, which reports them at exit.
 */
#include <stdio.h>
#include <stdlib.h>

#include "istack.h"

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
    istack_t *s = istack_create();

    CHECK(s != NULL);
    CHECK(istack_count(s) == 0);
    CHECK(istack_is_empty(s));
    CHECK(istack_capacity(s) == ISTACK_START);
    istack_destroy(s);
}

static void check_empty_leaves_v_alone(void) {
    istack_t *s = istack_create();
    int v = 42;

    CHECK(istack_pop(s, &v) == -1);
    CHECK(v == 42);
    CHECK(istack_peek(s, &v) == -1);
    CHECK(v == 42);
    /* emptied by pops, not only fresh */
    CHECK(istack_push(s, 7) == 0);
    CHECK(istack_pop(s, NULL) == 0);
    CHECK(istack_pop(s, &v) == -1);
    CHECK(v == 42);
    istack_destroy(s);
}

static void check_last_in_first_out(void) {
    istack_t *s = istack_create();
    int v = 0;

    CHECK(istack_push(s, 1) == 0);
    CHECK(istack_push(s, 2) == 0);
    CHECK(istack_push(s, 3) == 0);
    CHECK(istack_count(s) == 3);
    CHECK(istack_peek(s, &v) == 0 && v == 3);
    CHECK(istack_count(s) == 3);
    CHECK(istack_pop(s, &v) == 0 && v == 3);
    CHECK(istack_pop(s, &v) == 0 && v == 2);
    CHECK(istack_pop(s, &v) == 0 && v == 1);
    CHECK(istack_is_empty(s));
    istack_destroy(s);
}

static void check_ninth_push_doubles(void) {
    istack_t *s = istack_create();
    int v = 0;
    int kept = 1;

    for (int i = 0; i < 8; i++) CHECK(istack_push(s, i * 10) == 0);
    CHECK(istack_count(s) == 8);
    CHECK(istack_capacity(s) == 8);
    CHECK(istack_push(s, 80) == 0);
    CHECK(istack_count(s) == 9);
    CHECK(istack_capacity(s) == 16);
    for (int i = 8; i >= 0; i--) {
        if (istack_pop(s, &v) != 0 || v != i * 10) kept = 0;
    }
    CHECK(kept);
    CHECK(istack_is_empty(s));
    istack_destroy(s);
}

static void check_1000_pushes_and_pops(void) {
    istack_t *s = istack_create();
    int v = 0;
    int pushed = 1;
    int popped = 1;

    for (int i = 0; i < 1000; i++) {
        if (istack_push(s, i) != 0) pushed = 0;
    }
    CHECK(pushed);
    CHECK(istack_count(s) == 1000);
    CHECK(istack_capacity(s) == 1024);    /* 8, 16, ..., 1024 */
    for (int i = 999; i >= 0; i--) {
        if (istack_pop(s, &v) != 0 || v != i) popped = 0;
    }
    CHECK(popped);
    CHECK(istack_is_empty(s));
    CHECK(istack_capacity(s) == 1024);    /* the room never shrinks */
    istack_destroy(s);
}

static void check_to_array(void) {
    istack_t *s = istack_create();
    size_t n = 99;
    int v = 0;

    for (int i = 0; i < 10; i++) istack_push(s, i + 1);
    int *a = istack_to_array(s, &n);
    CHECK(a != NULL);
    CHECK(n == 10);
    if (a != NULL && n == 10) {
        int bottom_first = 1;
        for (int i = 0; i < 10; i++) {
            if (a[i] != i + 1) bottom_first = 0;
        }
        CHECK(bottom_first);

        /* a copy: changing one leaves the other alone */
        a[9] = -1;
        CHECK(istack_peek(s, &v) == 0 && v == 10);
        istack_pop(s, NULL);
        istack_push(s, 500);
        CHECK(a[9] == -1);
    }
    free(a);

    /* empty: NULL and *n = 0 */
    istack_t *e = istack_create();
    n = 99;
    CHECK(istack_to_array(e, &n) == NULL);
    CHECK(n == 0);
    istack_destroy(e);
    istack_destroy(s);
}

static void check_null_arguments(void) {
    int v = 42;
    size_t n = 99;

    istack_destroy(NULL);               /* must not crash */
    CHECK(istack_push(NULL, 1) == -1);
    CHECK(istack_pop(NULL, &v) == -1);
    CHECK(v == 42);
    CHECK(istack_peek(NULL, &v) == -1);
    CHECK(v == 42);
    CHECK(istack_count(NULL) == 0);
    CHECK(istack_is_empty(NULL));
    CHECK(istack_capacity(NULL) == 0);
    CHECK(istack_to_array(NULL, &n) == NULL);
    CHECK(n == 0);

    /* v may be NULL; n may not */
    istack_t *s = istack_create();
    istack_push(s, 1);
    istack_push(s, 2);
    CHECK(istack_peek(s, NULL) == 0);
    CHECK(istack_count(s) == 2);
    CHECK(istack_pop(s, NULL) == 0);
    CHECK(istack_count(s) == 1);
    CHECK(istack_to_array(s, NULL) == NULL);
    istack_destroy(s);
}

static void check_two_stacks_independent(void) {
    istack_t *a = istack_create();
    istack_t *b = istack_create();
    int v = 0;

    istack_push(a, 1);
    istack_push(b, 100);
    for (int i = 0; i < 20; i++) istack_push(a, i);   /* a grows; b doesn't */
    CHECK(istack_count(a) == 21);
    CHECK(istack_count(b) == 1);
    CHECK(istack_capacity(a) == 32);
    CHECK(istack_capacity(b) == 8);
    CHECK(istack_pop(b, &v) == 0 && v == 100);
    CHECK(istack_is_empty(b));
    CHECK(istack_peek(a, &v) == 0 && v == 19);
    istack_destroy(a);
    istack_destroy(b);
}

/* Only that it returns; that it frees everything is make memcheck's job. */
static void check_destroy_not_empty(void) {
    istack_t *s = istack_create();

    for (int i = 0; i < 20; i++) istack_push(s, i);   /* past one growth */
    CHECK(istack_count(s) == 20);
    istack_destroy(s);
}

int main(void) {
    check_create();
    check_empty_leaves_v_alone();
    check_last_in_first_out();
    check_ninth_push_doubles();
    check_1000_pushes_and_pops();
    check_to_array();
    check_null_arguments();
    check_two_stacks_independent();
    check_destroy_not_empty();

    printf("%d of %d checks passed\n", total - fails, total);

    return fails != 0;
}
