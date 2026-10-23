/*
 * test_tally.c:
 *
 * Tests for tally.c. Prints PASS or FAIL for each check, then a summary;
 * exits nonzero if any check failed.
 */

/* fmemopen is POSIX, hidden by -std=c11 without this. */
#define _POSIX_C_SOURCE 200809L

#include <stdbool.h>
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

/**
 * Captures what tally_print(t, out) writes and checks it matches expected
 * exactly.
 *
 * @param t        The tally to print.
 * @param expected Exact expected output, including each line's newline.
 * @return true if the captured output matches expected.
 */
static bool prints(const tally_t *t, const char *expected) {
    /* glibc's fmemopen only NUL-terminates after a write, so an empty
     * output would otherwise leave buf as garbage. */
    char buf[512] = {0};
    FILE *mem = fmemopen(buf, sizeof buf, "w");
    if (mem == NULL) return false;

    tally_print(t, mem);
    fflush(mem);

    bool matches = strcmp(buf, expected) == 0;
    fclose(mem);
    return matches;
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

static void check_reused_tally_is_empty(void) {
    tally_t *t[TALLY_MAX];

    create_all(t);
    tally_add(t[1], "OPEN");
    tally_destroy(t[1]);
    t[1] = tally_create();  /* the only free slot */

    CHECK(t[1] != NULL);
    CHECK(tally_size(t[1]) == 0);
    CHECK(tally_count(t[1], "OPEN") == 0);
    CHECK(prints(t[1], ""));

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

    tally_add(t, "OPEN");
    CHECK(tally_add(t, "") == -1);
    CHECK(tally_size(t) == 1);

    tally_destroy(t);
}

static void check_add_name_length_limit(void) {
    tally_t *t = tally_create();
    char longest[TALLY_LEN + 1];    /* TALLY_LEN letters and the '\0' */
    char too_long[TALLY_LEN + 2];   /* one letter more */

    memset(longest, 'A', TALLY_LEN);
    longest[TALLY_LEN] = '\0';
    memset(too_long, 'A', TALLY_LEN + 1);  /* longest plus one more 'A' */
    too_long[TALLY_LEN + 1] = '\0';

    CHECK(tally_add(t, longest) == 1);
    CHECK(tally_add(t, longest) == 2);
    CHECK(tally_add(t, too_long) == -1);
    CHECK(tally_count(t, longest) == 2);  /* not cut down to longest */
    CHECK(tally_size(t) == 1);

    tally_destroy(t);
}

static void check_add_null(void) {
    tally_t *t = tally_create();

    CHECK(tally_add(NULL, "OPEN") == -1);
    CHECK(tally_add(t, NULL) == -1);
    CHECK(tally_size(t) == 0);

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
    CHECK(tally_count(t, "NEW") == 0);
    CHECK(tally_size(t) == TALLY_NAMES);
    CHECK(tally_add(t, "N00") == 2);   /* an existing one still counts */
    CHECK(tally_add(t, "N15") == 2);

    tally_destroy(t);
}

/* =========================== tally_count tests ============================ */
static void check_count_after_adds(void) {
    tally_t *t = tally_create();

    tally_add(t, "OPEN");
    tally_add(t, "CLOSE");
    tally_add(t, "OPEN");
    tally_add(t, "ALARM");
    tally_add(t, "OPEN");

    CHECK(tally_count(t, "ALARM") == 1);
    CHECK(tally_count(t, "CLOSE") == 1);
    CHECK(tally_count(t, "OPEN") == 3);

    tally_destroy(t);
}

static void check_count_never_added(void) {
    tally_t *t = tally_create();

    CHECK(tally_count(t, "OPEN") == 0);   /* empty tally */
    tally_add(t, "OPEN");
    CHECK(tally_count(t, "LOCK") == 0);   /* would go before OPEN */
    CHECK(tally_count(t, "ZONE") == 0);   /* would go after OPEN */
    CHECK(tally_count(t, "OPE") == 0);    /* a prefix is not the name */
    CHECK(tally_count(t, "") == 0);

    tally_destroy(t);
}

static void check_count_null(void) {
    tally_t *t = tally_create();

    tally_add(t, "OPEN");
    CHECK(tally_count(NULL, "OPEN") == 0);
    CHECK(tally_count(t, NULL) == 0);

    tally_destroy(t);
}

static void check_count_two_tallies_independent(void) {
    tally_t *a = tally_create();
    tally_t *b = tally_create();

    tally_add(a, "OPEN");
    tally_add(a, "OPEN");
    tally_add(b, "OPEN");
    tally_add(b, "CLOSE");

    CHECK(tally_count(a, "OPEN") == 2);
    CHECK(tally_count(b, "OPEN") == 1);
    CHECK(tally_count(a, "CLOSE") == 0);
    CHECK(tally_count(b, "CLOSE") == 1);

    tally_destroy(a);
    tally_destroy(b);
}

/* ============================ tally_size tests ============================ */
static void check_size_counts_names_not_adds(void) {
    tally_t *t = tally_create();

    CHECK(tally_size(t) == 0);
    tally_add(t, "OPEN");
    CHECK(tally_size(t) == 1);
    tally_add(t, "OPEN");
    CHECK(tally_size(t) == 1);  /* same name again: no new entry */
    tally_add(t, "CLOSE");
    CHECK(tally_size(t) == 2);

    tally_destroy(t);
}

static void check_size_null(void) {
    CHECK(tally_size(NULL) == 0);
}

/* =========================== tally_print tests ============================ */
static void check_print_order(void) {
    tally_t *t = tally_create();

    tally_add(t, "OPEN");
    tally_add(t, "CLOSE");
    tally_add(t, "OPEN");
    tally_add(t, "LOCK");
    tally_add(t, "ALARM");
    tally_add(t, "CLOSE");
    tally_add(t, "OPEN");
    tally_add(t, "CLOSE");

    CHECK(prints(t, "ALARM 1\n"
                    "CLOSE 3\n"
                    "LOCK 1\n"
                    "OPEN 3\n"));

    tally_destroy(t);
}

static void check_print_strcmp_order(void) {
    tally_t *t = tally_create();

    tally_add(t, "open");
    tally_add(t, "OPENED");
    tally_add(t, "OPEN");

    /* a prefix comes first, and every uppercase letter before lowercase */
    CHECK(prints(t, "OPEN 1\n"
                    "OPENED 1\n"
                    "open 1\n"));

    tally_destroy(t);
}

static void check_print_empty(void) {
    tally_t *t = tally_create();

    CHECK(prints(t, ""));

    tally_destroy(t);
}

static void check_print_null(void) {
    tally_t *t = tally_create();

    tally_add(t, "OPEN");
    CHECK(prints(NULL, ""));
    tally_print(t, NULL);  /* must return without writing anywhere */
    CHECK(prints(t, "OPEN 1\n"));

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
    /* tally_create */
    check_create_max_then_one_more();
    /* tally_destroy */
    check_create_after_destroy();
    check_reused_tally_is_empty();
    check_destroy_null();
    /* tally_add */
    check_add_counts_up();
    check_add_empty_name();
    check_add_name_length_limit();
    check_add_null();
    check_add_full_tally();
    /* tally_count */
    check_count_after_adds();
    check_count_never_added();
    check_count_null();
    check_count_two_tallies_independent();
    /* tally_size */
    check_size_counts_names_not_adds();
    check_size_null();
    /* tally_print */
    check_print_order();
    check_print_strcmp_order();
    check_print_empty();
    check_print_null();

    print_summary();

    return fails != 0;
}
