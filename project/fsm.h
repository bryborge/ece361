/*
 * fsm.h:
 *
 * The garage door's state machine. The transitions live in a table indexed
 * by state and event; SPEC.md gives the table and the reason for each row.
 */
#ifndef FSM_H
#define FSM_H

#include <stdbool.h>
#include <stdint.h>

/* Bits of the switch register (ASSIGNMENT.md section 3). */
#define SW_REMOTE  (1u << 0)    /* a press: each change from 0 to 1 counts */
#define SW_BARRIER (1u << 1)    /* a level: on while something is in the way */

/* CLOSED is the initial state. */
typedef enum { CLOSED, OPENING, OPEN, CLOSING, OBSTRUCTED, N_STATES } state_t;

typedef enum {
    E_REMOTE,                   /* press, above FREEZE_C, barrier clear */
    E_REMOTE_FROZEN,            /* press, at or below FREEZE_C, barrier clear */
    E_REMOTE_BARRIER,           /* press, above FREEZE_C, barrier on */
    E_REMOTE_FROZEN_BARRIER,    /* press, at or below FREEZE_C, barrier on */
    E_BARRIER,                  /* the barrier is on */
    E_TIMEOUT,                  /* the current state's time is up */
    N_EVENTS
} event_t;

typedef enum {
    A_UNDEFINED,                /* 0: an entry nobody filled in */
    A_NONE,
    A_OBSTRUCT,
    A_REFUSE_FROZEN,
    A_REFUSE_BARRIER,
    N_ACTIONS
} action_t;

typedef struct { state_t next; action_t action; } transition_t;

extern const transition_t fsm_table[N_STATES][N_EVENTS];

/**
 * Given a state `s` and an event `e`, returns the next state and stores the
 * action to take in *action. Assumes `s` and `e` are in range and action is
 * not `NULL`.
 */
state_t fsm_step(state_t s, event_t e, action_t *action);

/**
 * Given the switch register on the previous tick and on this one, returns
 * whether REMOTE was pressed: its bit went from 0 to 1. Other bits are
 * ignored.
 */
bool fsm_remote_pressed(uint32_t prev, uint32_t now);

/**
 * Given a state `s`, returns the LED register for it: MOTOR_UP (bit 0) while
 * OPENING, MOTOR_DOWN (bit 1) while CLOSING, otherwise 0. Assumes `s` is in
 * range.
 */
uint32_t fsm_leds(state_t s);

/**
 * Given a state `s`, returns its name (e.g. `OPENING`), or `?` if `s` is out
 * of range.
 */
const char *state_name(state_t s);

/**
 * Given an event `e`, returns its name without the `E_` prefix (e.g.
 * `REMOTE_FROZEN`), or `?` if `e` is out of range.
 */
const char *event_name(event_t e);

/**
 * Given an action `a`, returns its name without the `A_` prefix (e.g.
 * `OBSTRUCT`), or `?` if `a` is out of range.
 */
const char *action_name(action_t a);

#endif
