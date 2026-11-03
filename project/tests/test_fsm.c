/*
 * test_fsm.c:
 *
 * Tests for fsm.c. Every cell of the transition table is written out from
 * the table in SPEC.md, not read from the code. Prints PASS or FAIL for each
 * check; test_main.c runs them and prints the summary.
 */
#include <stdbool.h>
#include <string.h>

#include "check.h"
#include "fsm.h"

/**
 * Checks that event e in state s leads to state next with action action.
 *
 * @return true if fsm_step gives both the expected state and action.
 */
static bool steps_to(state_t s, event_t e, state_t next, action_t action) {
    /* Starts as A_UNDEFINED, so an fsm_step that never writes *action fails
     * the check instead of reading an uninitialized value. */
    action_t got_action = A_UNDEFINED;
    state_t got_next = fsm_step(s, e, &got_action);

    return got_next == next && got_action == action;
}

/* fsm_step: one function per row of the table. † marks a cell that can
 * never be reached; it is tested because it must still be filled in. */

static void check_step_closed(void) {
    CHECK(steps_to(CLOSED, E_REMOTE,                OPENING, A_NONE));
    CHECK(steps_to(CLOSED, E_REMOTE_FROZEN,         CLOSED,  A_REFUSE_FROZEN));
    CHECK(steps_to(CLOSED, E_REMOTE_BARRIER,        OPENING, A_NONE));
    CHECK(steps_to(CLOSED, E_REMOTE_FROZEN_BARRIER, CLOSED,  A_REFUSE_FROZEN));
    CHECK(steps_to(CLOSED, E_BARRIER,               CLOSED,  A_NONE));
    CHECK(steps_to(CLOSED, E_TIMEOUT,               CLOSED,  A_NONE));  /* † */
}

static void check_step_opening(void) {
    CHECK(steps_to(OPENING, E_REMOTE,                CLOSING, A_NONE));
    CHECK(steps_to(OPENING, E_REMOTE_FROZEN,         CLOSING, A_NONE));
    CHECK(steps_to(OPENING, E_REMOTE_BARRIER,        OPENING, A_REFUSE_BARRIER));
    CHECK(steps_to(OPENING, E_REMOTE_FROZEN_BARRIER, OPENING, A_REFUSE_BARRIER));
    CHECK(steps_to(OPENING, E_BARRIER,               OPENING, A_NONE));
    CHECK(steps_to(OPENING, E_TIMEOUT,               OPEN,    A_NONE));
}

static void check_step_open(void) {
    CHECK(steps_to(OPEN, E_REMOTE,                CLOSING, A_NONE));
    CHECK(steps_to(OPEN, E_REMOTE_FROZEN,         CLOSING, A_NONE));
    CHECK(steps_to(OPEN, E_REMOTE_BARRIER,        OPEN,    A_REFUSE_BARRIER));
    CHECK(steps_to(OPEN, E_REMOTE_FROZEN_BARRIER, OPEN,    A_REFUSE_BARRIER));
    CHECK(steps_to(OPEN, E_BARRIER,               OPEN,    A_NONE));
    CHECK(steps_to(OPEN, E_TIMEOUT,               OPEN,    A_NONE));  /* † */
}

static void check_step_closing(void) {
    CHECK(steps_to(CLOSING, E_REMOTE,                OPENING,    A_NONE));
    CHECK(steps_to(CLOSING, E_REMOTE_FROZEN,         OPENING,    A_NONE));
    CHECK(steps_to(CLOSING, E_REMOTE_BARRIER,        OBSTRUCTED, A_OBSTRUCT));  /* † */
    CHECK(steps_to(CLOSING, E_REMOTE_FROZEN_BARRIER, OBSTRUCTED, A_OBSTRUCT));  /* † */
    CHECK(steps_to(CLOSING, E_BARRIER,               OBSTRUCTED, A_OBSTRUCT));
    CHECK(steps_to(CLOSING, E_TIMEOUT,               CLOSED,     A_NONE));
}

static void check_step_obstructed(void) {
    CHECK(steps_to(OBSTRUCTED, E_REMOTE,                OBSTRUCTED, A_NONE));
    CHECK(steps_to(OBSTRUCTED, E_REMOTE_FROZEN,         OBSTRUCTED, A_NONE));
    CHECK(steps_to(OBSTRUCTED, E_REMOTE_BARRIER,        OBSTRUCTED, A_NONE));
    CHECK(steps_to(OBSTRUCTED, E_REMOTE_FROZEN_BARRIER, OBSTRUCTED, A_NONE));
    CHECK(steps_to(OBSTRUCTED, E_BARRIER,               OBSTRUCTED, A_NONE));
    CHECK(steps_to(OBSTRUCTED, E_TIMEOUT,               OPENING,    A_NONE));
}

/* state_name, event_name, action_name */

static void check_state_name(void) {
    CHECK(strcmp(state_name(CLOSED),     "CLOSED")     == 0);
    CHECK(strcmp(state_name(OPENING),    "OPENING")    == 0);
    CHECK(strcmp(state_name(OPEN),       "OPEN")       == 0);
    CHECK(strcmp(state_name(CLOSING),    "CLOSING")    == 0);
    CHECK(strcmp(state_name(OBSTRUCTED), "OBSTRUCTED") == 0);
}

static void check_state_name_out_of_range(void) {
    CHECK(strcmp(state_name(N_STATES),     "?") == 0);  /* one past the end */
    CHECK(strcmp(state_name((state_t)-1), "?") == 0);
}

static void check_event_name(void) {
    CHECK(strcmp(event_name(E_REMOTE),                "REMOTE")                == 0);
    CHECK(strcmp(event_name(E_REMOTE_FROZEN),         "REMOTE_FROZEN")         == 0);
    CHECK(strcmp(event_name(E_REMOTE_BARRIER),        "REMOTE_BARRIER")        == 0);
    CHECK(strcmp(event_name(E_REMOTE_FROZEN_BARRIER), "REMOTE_FROZEN_BARRIER") == 0);
    CHECK(strcmp(event_name(E_BARRIER),               "BARRIER")               == 0);
    CHECK(strcmp(event_name(E_TIMEOUT),               "TIMEOUT")               == 0);
}

static void check_event_name_out_of_range(void) {
    CHECK(strcmp(event_name(N_EVENTS),     "?") == 0);  /* one past the end */
    CHECK(strcmp(event_name((event_t)-1), "?") == 0);
}

static void check_action_name(void) {
    CHECK(strcmp(action_name(A_UNDEFINED),      "UNDEFINED")      == 0);
    CHECK(strcmp(action_name(A_NONE),           "NONE")           == 0);
    CHECK(strcmp(action_name(A_OBSTRUCT),       "OBSTRUCT")       == 0);
    CHECK(strcmp(action_name(A_REFUSE_FROZEN),  "REFUSE_FROZEN")  == 0);
    CHECK(strcmp(action_name(A_REFUSE_BARRIER), "REFUSE_BARRIER") == 0);
}

static void check_action_name_out_of_range(void) {
    CHECK(strcmp(action_name(N_ACTIONS),     "?") == 0);  /* one past the end */
    CHECK(strcmp(action_name((action_t)-1), "?") == 0);
}

void run_fsm_tests(void) {
    /* fsm_step */
    check_step_closed();
    check_step_opening();
    check_step_open();
    check_step_closing();
    check_step_obstructed();
    /* names */
    check_state_name();
    check_state_name_out_of_range();
    check_event_name();
    check_event_name_out_of_range();
    check_action_name();
    check_action_name_out_of_range();
}
