/*
 * test_lock.c:
 *
 * Tests for lock.c. Prints PASS or FAIL for each check, then a summary;
 * exits nonzero if any check failed.
 */
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

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
 * Checks that event e in state s leads to state next with action action.
 *
 * @return true if lock_step gives both the expected state and action.
 */
static bool steps_to(state_t s, event_t e, state_t next, action_t action) {
    /* Starts as A_UNDEFINED, so a lock_step that never writes *action fails
     * the check instead of reading an uninitialized value. */
    action_t got_action = A_UNDEFINED;
    state_t got_next = lock_step(s, e, &got_action);

    return got_next == next && got_action == action;
}

/* ============================ lock_step tests ============================= */
/*
 * One check per table entry, written from the spec (docs/PRD.md section 3.2)
 * rather than read from lock_table, so a wrong entry fails.
 */
static void check_step_locked(void) {
    CHECK(steps_to(LOCKED, E_RIGHT,   GOT1,   A_NONE));
    CHECK(steps_to(LOCKED, E_WRONG,   LOCKED, A_BEEP));
    CHECK(steps_to(LOCKED, E_CLOSE,   LOCKED, A_NONE));
    CHECK(steps_to(LOCKED, E_TIMEOUT, LOCKED, A_NONE));  /* no timer runs */
}

static void check_step_got1(void) {
    CHECK(steps_to(GOT1, E_RIGHT,   GOT2,   A_NONE));
    CHECK(steps_to(GOT1, E_WRONG,   LOCKED, A_BEEP));
    CHECK(steps_to(GOT1, E_CLOSE,   GOT1,   A_NONE));
    CHECK(steps_to(GOT1, E_TIMEOUT, LOCKED, A_NONE));    /* silently */
}

static void check_step_got2(void) {
    CHECK(steps_to(GOT2, E_RIGHT,   OPEN,   A_UNLOCK));
    CHECK(steps_to(GOT2, E_WRONG,   LOCKED, A_BEEP));
    CHECK(steps_to(GOT2, E_CLOSE,   GOT2,   A_NONE));
    CHECK(steps_to(GOT2, E_TIMEOUT, LOCKED, A_NONE));    /* silently */
}

static void check_step_open(void) {
    CHECK(steps_to(OPEN, E_RIGHT,   OPEN,   A_NONE));    /* lock_event never gives it */
    CHECK(steps_to(OPEN, E_WRONG,   OPEN,   A_NONE));
    CHECK(steps_to(OPEN, E_CLOSE,   LOCKED, A_LOCK));
    CHECK(steps_to(OPEN, E_TIMEOUT, LOCKED, A_LOCK));
}

/* ============================ lock_event tests ============================ */
static void check_event_right_digit(void) {
    CHECK(lock_event(LOCKED, '3') == E_RIGHT);
    CHECK(lock_event(GOT1,   '1') == E_RIGHT);
    CHECK(lock_event(GOT2,   '4') == E_RIGHT);
}

/* Each wrong digit is the right one in another state, so a lookup in the
 * wrong state's entry fails. */
static void check_event_wrong_digit(void) {
    CHECK(lock_event(LOCKED, '1') == E_WRONG);
    CHECK(lock_event(GOT1,   '3') == E_WRONG);  /* why 3 3 1 4 does not open */
    CHECK(lock_event(GOT2,   '1') == E_WRONG);
    CHECK(lock_event(LOCKED, '0') == E_WRONG);  /* the ends of the digits */
    CHECK(lock_event(LOCKED, '9') == E_WRONG);
}

static void check_event_open(void) {
    CHECK(lock_event(OPEN, '3') == E_WRONG);    /* no next digit while open */
    CHECK(lock_event(OPEN, '1') == E_WRONG);
    CHECK(lock_event(OPEN, '4') == E_WRONG);
}

static void check_event_close_and_timeout(void) {
    for (state_t s = LOCKED; s < N_STATES; s++) {
        CHECK(lock_event(s, 'c') == E_CLOSE);
        CHECK(lock_event(s, 't') == E_TIMEOUT);
    }
}

static void check_event_other(void) {
    CHECK(lock_event(LOCKED, 'C') == -1);       /* only lowercase */
    CHECK(lock_event(LOCKED, 'T') == -1);
    CHECK(lock_event(LOCKED, 'x') == -1);
    CHECK(lock_event(LOCKED, ' ') == -1);
    CHECK(lock_event(LOCKED, '\n') == -1);
    CHECK(lock_event(LOCKED, '/') == -1);       /* just below '0' */
    CHECK(lock_event(LOCKED, ':') == -1);       /* just above '9' */
    CHECK(lock_event(LOCKED, (char)0xE9) == -1);  /* negative if char is signed */
    CHECK(lock_event(OPEN, '\0') == -1);        /* OPEN's "no digit" value */
}

/* ============================ state_name tests ============================ */
static void check_state_name(void) {
    CHECK(strcmp(state_name(LOCKED), "LOCKED") == 0);
    CHECK(strcmp(state_name(GOT1),   "GOT1")   == 0);
    CHECK(strcmp(state_name(GOT2),   "GOT2")   == 0);
    CHECK(strcmp(state_name(OPEN),   "OPEN")   == 0);
}

static void check_state_name_out_of_range(void) {
    CHECK(strcmp(state_name(N_STATES),     "?") == 0);  /* one past the end */
    CHECK(strcmp(state_name((state_t)-1), "?") == 0);
}

/* ============================ event_name tests ============================ */
static void check_event_name(void) {
    CHECK(strcmp(event_name(E_RIGHT),   "RIGHT")   == 0);
    CHECK(strcmp(event_name(E_WRONG),   "WRONG")   == 0);
    CHECK(strcmp(event_name(E_CLOSE),   "CLOSE")   == 0);
    CHECK(strcmp(event_name(E_TIMEOUT), "TIMEOUT") == 0);
}

static void check_event_name_out_of_range(void) {
    CHECK(strcmp(event_name(N_EVENTS),     "?") == 0);  /* one past the end */
    CHECK(strcmp(event_name((event_t)-1), "?") == 0);
}

/* =========================== action_name tests ============================ */
static void check_action_name(void) {
    CHECK(strcmp(action_name(A_UNDEFINED), "UNDEFINED") == 0);
    CHECK(strcmp(action_name(A_NONE),      "NONE")      == 0);
    CHECK(strcmp(action_name(A_UNLOCK),    "UNLOCK")    == 0);
    CHECK(strcmp(action_name(A_LOCK),      "LOCK")      == 0);
    CHECK(strcmp(action_name(A_BEEP),      "BEEP")      == 0);
}

static void check_action_name_out_of_range(void) {
    CHECK(strcmp(action_name(N_ACTIONS),     "?") == 0);  /* one past the end */
    CHECK(strcmp(action_name((action_t)-1), "?") == 0);
}

static void print_summary(void) {
    printf("\nSummary:\n");
    printf("--------\n");
    printf("%d total\n", total);
    printf("%d failed\n", fails);
    printf("\n");
}

int main(void) {
    /* lock_step */
    check_step_locked();
    check_step_got1();
    check_step_got2();
    check_step_open();
    /* lock_event */
    check_event_right_digit();
    check_event_wrong_digit();
    check_event_open();
    check_event_close_and_timeout();
    check_event_other();
    /* state_name */
    check_state_name();
    check_state_name_out_of_range();
    /* event_name */
    check_event_name();
    check_event_name_out_of_range();
    /* action_name */
    check_action_name();
    check_action_name_out_of_range();

    print_summary();

    return fails != 0;
}
