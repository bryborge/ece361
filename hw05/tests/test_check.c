/*
 * test_check.c:
 *
 * Tests for check.c. Each test breaks a copy of lock_table in one way and
 * checks both the count fsm_check returns and the lines it writes. Prints
 * PASS or FAIL for each check, then a summary; exits nonzero if any check
 * failed.
 */
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "check.h"
#include "lock.h"

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
 * Runs fsm_check on table from LOCKED and checks what it finds.
 *
 * @param table    The table to check.
 * @param problems The count fsm_check should return.
 * @param expected Exactly what fsm_check should write.
 * @return true if fsm_check returns problems and writes expected.
 */
static bool finds(const transition_t table[N_STATES][N_EVENTS], int problems,
                  const char *expected) {
    char got[512] = "";
    FILE *out = tmpfile();
    if (out == NULL) return false;

    int count = fsm_check(table, LOCKED, out);
    rewind(out);
    size_t n = fread(got, 1, sizeof got - 1, out);
    got[n] = '\0';
    fclose(out);

    return count == problems && strcmp(got, expected) == 0;
}

/**
 * Copies lock_table into copy, so a test can break the copy.
 *
 * @param copy The table to fill.
 */
static void copy_lock_table(transition_t copy[N_STATES][N_EVENTS]) {
    memcpy(copy, lock_table, sizeof lock_table);
}

/* ============================ fsm_check tests ============================= */
static void check_lock_table(void) {
    CHECK(finds(lock_table, 0, ""));
}

static void check_entry_left_out(void) {
    transition_t t[N_STATES][N_EVENTS];
    copy_lock_table(t);
    t[GOT2][E_TIMEOUT] = (transition_t){ 0 };

    CHECK(finds(t, 1, "GOT2 TIMEOUT: entry left out (A_UNDEFINED)\n"));
}

static void check_next_out_of_range(void) {
    transition_t t[N_STATES][N_EVENTS];
    copy_lock_table(t);
    t[GOT1][E_CLOSE].next = N_STATES;

    CHECK(finds(t, 1, "GOT1 CLOSE: next state out of range\n"));
}

static void check_unreachable(void) {
    transition_t t[N_STATES][N_EVENTS];
    copy_lock_table(t);
    t[GOT1][E_RIGHT].next = LOCKED;     /* the only way into GOT2 */

    CHECK(finds(t, 2, "GOT2: not reachable from LOCKED\n"
                      "OPEN: not reachable from LOCKED\n"));
}

static void check_no_way_out(void) {
    transition_t t[N_STATES][N_EVENTS];
    copy_lock_table(t);
    t[OPEN][E_CLOSE]   = (transition_t){ OPEN, A_NONE };
    t[OPEN][E_TIMEOUT] = (transition_t){ OPEN, A_NONE };

    CHECK(finds(t, 1, "OPEN: no way out\n"));
}

/* A left-out entry reads as { LOCKED, A_UNDEFINED }; it must not count as a
 * way back to LOCKED. */
static void check_left_out_is_not_a_way_out(void) {
    transition_t t[N_STATES][N_EVENTS];
    copy_lock_table(t);
    t[OPEN][E_CLOSE]   = (transition_t){ 0 };
    t[OPEN][E_TIMEOUT] = (transition_t){ 0 };

    CHECK(finds(t, 3, "OPEN CLOSE: entry left out (A_UNDEFINED)\n"
                      "OPEN TIMEOUT: entry left out (A_UNDEFINED)\n"
                      "OPEN: no way out\n"));
}

/* An out-of-range next state is not followed, so what lies only beyond it
 * cannot be reached. */
static void check_out_of_range_is_not_followed(void) {
    transition_t t[N_STATES][N_EVENTS];
    copy_lock_table(t);
    t[GOT2][E_RIGHT].next = N_STATES;   /* the only way into OPEN */

    CHECK(finds(t, 2, "GOT2 RIGHT: next state out of range\n"
                      "OPEN: not reachable from LOCKED\n"));
}

static void print_summary(void) {
    printf("\nSummary:\n");
    printf("--------\n");
    printf("%d total\n", total);
    printf("%d failed\n", fails);
    printf("\n");
}

int main(void) {
    check_lock_table();
    check_entry_left_out();
    check_next_out_of_range();
    check_unreachable();
    check_no_way_out();
    check_left_out_is_not_a_way_out();
    check_out_of_range_is_not_followed();

    print_summary();

    return fails != 0;
}
