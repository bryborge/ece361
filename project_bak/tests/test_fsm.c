/*
 * test_fsm.c:
 *
 * Tests for fsm.c. Every cell of the transition table is written out from
 * the table in SPEC.md, not read from the code. Prints PASS or FAIL for each
 * check; test_main.c runs them and prints the summary.
 */
#include <stdbool.h>
#include <stdint.h>
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

/* fsm_remote_pressed */

static void check_pressed_rising_edge(void) {
    CHECK(fsm_remote_pressed(0x0, 0x1));
}

static void check_pressed_not_an_edge(void) {
    CHECK(!fsm_remote_pressed(0x1, 0x1));   /* held: counts once, not again */
    CHECK(!fsm_remote_pressed(0x1, 0x0));   /* released */
    CHECK(!fsm_remote_pressed(0x0, 0x0));
}

static void check_pressed_ignores_barrier(void) {
    CHECK(fsm_remote_pressed(0x2, 0x3));    /* barrier on throughout */
    CHECK(!fsm_remote_pressed(0x1, 0x3));   /* barrier came on, REMOTE held */
    CHECK(!fsm_remote_pressed(0x0, 0x2));   /* barrier alone */
}

/* fsm_leds: MOTOR_UP is LED 0, MOTOR_DOWN is LED 1 (ASSIGNMENT.md section 3). */

static void check_leds(void) {
    CHECK(fsm_leds(CLOSED)     == 0x0);
    CHECK(fsm_leds(OPENING)    == 0x1);
    CHECK(fsm_leds(OPEN)       == 0x0);
    CHECK(fsm_leds(CLOSING)    == 0x2);
    CHECK(fsm_leds(OBSTRUCTED) == 0x0);
}

/* fsm_tick: sequences. A rig holds the door, its clock, and the last
 * switch reading, and advances one tick at a time through fsm_tick, as main
 * does. */

#define T0   1791273600     /* 2026-10-06 08:00:00 UTC */
#define WARM 15.0f
#define COLD 1.0f           /* exactly FREEZE_C: the freeze guard is on */
#define MILD 1.1f           /* the next reading above FREEZE_C */

typedef struct {
    door_t door;
    time_t now;
    uint32_t prev;
} rig_t;

/**
 * Starts a rig with the door closed at T0 and every switch off.
 */
static rig_t start(void) {
    rig_t r = { .door = { CLOSED, T0, 0 }, .now = T0, .prev = 0 };

    return r;
}

/**
 * Advances the rig by one tick.
 *
 * @param r      The rig.
 * @param sw     The switch register on this tick.
 * @param temp_c The temperature on this tick.
 * @return The tick's action.
 */
static action_t tick(rig_t *r, uint32_t sw, float temp_c) {
    r->now++;
    action_t a = fsm_tick(&r->door, r->now, r->prev, sw, temp_c);
    r->prev = sw;

    return a;
}

/**
 * Advances the rig n ticks with the inputs held.
 *
 * @return true if the door stayed in its state on every one of them.
 */
static bool stays(rig_t *r, uint32_t sw, float temp_c, int n) {
    state_t s = r->door.state;

    for (int i = 0; i < n; i++) {
        tick(r, sw, temp_c);
        if (r->door.state != s) return false;
    }
    return true;
}

/**
 * Advances the rig with the inputs held until the door leaves its state.
 *
 * @return The number of ticks it took, or -1 if the door was still in its
 *         state after 100 ticks.
 */
static int ticks_to_leave(rig_t *r, uint32_t sw, float temp_c) {
    state_t s = r->door.state;

    for (int n = 1; n <= 100; n++) {
        tick(r, sw, temp_c);
        if (r->door.state != s) return n;
    }
    return -1;
}

/**
 * Starts a rig with the door fully open: a press, then 15 ticks of travel.
 */
static rig_t start_open(void) {
    rig_t r = start();

    tick(&r, SW_REMOTE, WARM);
    ticks_to_leave(&r, 0, WARM);
    return r;
}

static void check_full_open_and_close(void) {
    rig_t r = start();

    CHECK(tick(&r, SW_REMOTE, WARM) == A_NONE);
    CHECK(r.door.state == OPENING);
    CHECK(ticks_to_leave(&r, 0, WARM) == 15);           /* rule 1 */
    CHECK(r.door.state == OPEN);

    CHECK(tick(&r, SW_REMOTE, WARM) == A_NONE);
    CHECK(r.door.state == CLOSING);
    CHECK(ticks_to_leave(&r, 0, WARM) == 15);           /* rule 1 */
    CHECK(r.door.state == CLOSED);
}

static void check_held_press_counts_once(void) {
    rig_t r = start();

    tick(&r, SW_REMOTE, WARM);
    CHECK(stays(&r, SW_REMOTE, WARM, 14));              /* still held */
}

static void check_freeze_guard(void) {
    rig_t r = start();

    CHECK(tick(&r, SW_REMOTE, COLD) == A_REFUSE_FROZEN);    /* rule 3 */
    CHECK(r.door.state == CLOSED);
    tick(&r, 0, COLD);
    CHECK(tick(&r, SW_REMOTE | SW_BARRIER, COLD) == A_REFUSE_FROZEN);
    CHECK(r.door.state == CLOSED);
    tick(&r, 0, MILD);
    CHECK(tick(&r, SW_REMOTE, MILD) == A_NONE);         /* the guard clears */
    CHECK(r.door.state == OPENING);
}

static void check_cold_does_not_matter_once_open(void) {
    rig_t r = start_open();

    CHECK(tick(&r, SW_REMOTE, COLD) == A_NONE);
    CHECK(r.door.state == CLOSING);
}

static void check_opens_with_barrier_on(void) {
    rig_t r = start();

    CHECK(tick(&r, SW_REMOTE | SW_BARRIER, WARM) == A_NONE);
    CHECK(r.door.state == OPENING);
}

static void check_barrier_refuses_close(void) {
    rig_t r = start_open();

    CHECK(stays(&r, SW_BARRIER, WARM, 3));              /* barrier alone */
    CHECK(tick(&r, SW_REMOTE | SW_BARRIER, WARM) == A_REFUSE_BARRIER);
    CHECK(r.door.state == OPEN);

    r = start();
    tick(&r, SW_REMOTE, WARM);
    tick(&r, 0, WARM);
    CHECK(tick(&r, SW_REMOTE | SW_BARRIER, WARM) == A_REFUSE_BARRIER);
    CHECK(r.door.state == OPENING);
}

/* A reversal travels back only as far as the door had gone. A position
 * snapshot taken after the state change, instead of before, gets these
 * lengths wrong. */
static void check_reversal_mid_travel(void) {
    rig_t r = start();

    tick(&r, SW_REMOTE, WARM);                          /* OPENING from 0 */
    CHECK(stays(&r, 0, WARM, 5));                       /* at 5 */
    tick(&r, SW_REMOTE, WARM);                          /* at 6: reverse */
    CHECK(r.door.state == CLOSING);
    CHECK(ticks_to_leave(&r, 0, WARM) == 6);
    CHECK(r.door.state == CLOSED);

    r = start_open();
    tick(&r, SW_REMOTE, WARM);                          /* CLOSING from 15 */
    CHECK(stays(&r, 0, WARM, 3));                       /* at 12 */
    tick(&r, SW_REMOTE, WARM);                          /* at 11: reverse */
    CHECK(r.door.state == OPENING);
    CHECK(ticks_to_leave(&r, 0, WARM) == 4);
    CHECK(r.door.state == OPEN);
}

static void check_obstruction(void) {
    rig_t r = start_open();

    tick(&r, SW_REMOTE, WARM);                          /* CLOSING from 15 */
    CHECK(stays(&r, 0, WARM, 5));                       /* at 10 */
    CHECK(tick(&r, SW_BARRIER, WARM) == A_OBSTRUCT);    /* at 9: rule 2 */
    CHECK(r.door.state == OBSTRUCTED);
    CHECK(ticks_to_leave(&r, SW_BARRIER, WARM) == 2);   /* barrier still on */
    CHECK(r.door.state == OPENING);
    CHECK(ticks_to_leave(&r, 0, WARM) == 6);            /* 9 to 15 */
    CHECK(r.door.state == OPEN);
}

static void check_presses_ignored_during_pause(void) {
    rig_t r = start_open();

    tick(&r, SW_REMOTE, WARM);
    tick(&r, 0, WARM);
    tick(&r, SW_BARRIER, WARM);                         /* OBSTRUCTED at 13 */
    CHECK(tick(&r, SW_REMOTE, WARM) == A_NONE);         /* during the pause */
    CHECK(r.door.state == OBSTRUCTED);
    tick(&r, 0, WARM);
    CHECK(r.door.state == OPENING);                     /* pause over */
}

/* Press before timeout: a press on a state's last tick is looked up in that
 * state. */
static void check_press_on_last_tick(void) {
    rig_t r = start();

    tick(&r, SW_REMOTE, WARM);
    CHECK(stays(&r, 0, WARM, 14));
    CHECK(tick(&r, SW_REMOTE, WARM) == A_NONE);         /* OPENING's last */
    CHECK(r.door.state == CLOSING);
    CHECK(ticks_to_leave(&r, 0, WARM) == 15);

    r = start_open();
    tick(&r, SW_REMOTE, WARM);
    CHECK(stays(&r, 0, WARM, 14));
    CHECK(tick(&r, SW_REMOTE, COLD) == A_NONE);         /* CLOSING's last */
    CHECK(r.door.state == OPENING);                     /* not refused */
    CHECK(ticks_to_leave(&r, 0, WARM) == 15);
}

static void check_press_as_pause_ends(void) {
    rig_t r = start_open();

    tick(&r, SW_REMOTE, WARM);
    tick(&r, 0, WARM);
    tick(&r, SW_BARRIER, WARM);                         /* OBSTRUCTED at 13 */
    tick(&r, 0, WARM);
    CHECK(tick(&r, SW_REMOTE, WARM) == A_NONE);         /* the pause's last */
    CHECK(r.door.state == OPENING);                     /* press ignored */
    CHECK(ticks_to_leave(&r, 0, WARM) == 2);            /* 13 to 15 */
}

/* The barrier on CLOSING's last tick stops the door at 0. Under the old
 * order (timeout before press), a press as the pause ended then entered
 * CLOSING with no time allowed, and the next tick reached position -1. */
static void check_barrier_on_last_tick(void) {
    rig_t r = start_open();

    tick(&r, SW_REMOTE, WARM);
    CHECK(stays(&r, 0, WARM, 14));
    CHECK(tick(&r, SW_BARRIER, WARM) == A_OBSTRUCT);    /* at 0 */
    CHECK(r.door.state == OBSTRUCTED);
    tick(&r, 0, WARM);
    tick(&r, SW_REMOTE, WARM);                          /* the pause's last */
    CHECK(r.door.state == OPENING);
    CHECK(ticks_to_leave(&r, 0, WARM) == 15);
    CHECK(r.door.state == OPEN);
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

/* fsm_rgb: written out from SPEC.md. */
static bool rgb_is(state_t s, uint8_t red, uint8_t green, uint8_t blue) {
    rgb_t c = fsm_rgb(s);

    return c.red == red && c.green == green && c.blue == blue;
}

static void check_rgb(void) {
    CHECK(rgb_is(CLOSED,       0, 255,   0));   /* green */
    CHECK(rgb_is(OPENING,      0, 255, 255));   /* cyan */
    CHECK(rgb_is(OPEN,         0,   0, 255));   /* blue */
    CHECK(rgb_is(CLOSING,    255, 255,   0));   /* yellow */
    CHECK(rgb_is(OBSTRUCTED, 255,   0,   0));   /* red */
}

/* fsm_freeze_active: rule 3 needs both CLOSED and the cold. */
static void check_freeze_active(void) {
    /* CLOSED, either side of FREEZE_C */
    CHECK(fsm_freeze_active(CLOSED, 1.0f));
    CHECK(fsm_freeze_active(CLOSED, 0.9f));
    CHECK(fsm_freeze_active(CLOSED, -50.0f));
    CHECK(!fsm_freeze_active(CLOSED, 1.1f));
    /* the cold does not matter in any other state */
    CHECK(!fsm_freeze_active(OPENING, 0.5f));
    CHECK(!fsm_freeze_active(OPEN, 0.5f));
    CHECK(!fsm_freeze_active(CLOSING, 0.5f));
    CHECK(!fsm_freeze_active(OBSTRUCTED, 0.5f));
}

void run_fsm_tests(void) {
    /* fsm_step */
    check_step_closed();
    check_step_opening();
    check_step_open();
    check_step_closing();
    check_step_obstructed();
    /* fsm_remote_pressed */
    check_pressed_rising_edge();
    check_pressed_not_an_edge();
    check_pressed_ignores_barrier();
    /* fsm_leds */
    check_leds();
    /* fsm_rgb */
    check_rgb();
    /* fsm_freeze_active */
    check_freeze_active();
    /* fsm_tick */
    check_full_open_and_close();
    check_held_press_counts_once();
    check_freeze_guard();
    check_cold_does_not_matter_once_open();
    check_opens_with_barrier_on();
    check_barrier_refuses_close();
    check_reversal_mid_travel();
    check_obstruction();
    check_presses_ignored_during_pause();
    check_press_on_last_tick();
    check_press_as_pause_ends();
    check_barrier_on_last_tick();
    /* names */
    check_state_name();
    check_state_name_out_of_range();
    check_event_name();
    check_event_name_out_of_range();
    check_action_name();
    check_action_name_out_of_range();
}
