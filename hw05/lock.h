/*
 * lock.h:
 *
 * A combination lock written as a finite state machine. The code to unlock it
 * is 3-1-4. Each right digit moves the state machine along\, and the third one
 * opens it. The transitions live in a table indexed by state and event.
 */
#ifndef LOCK_H
#define LOCK_H

typedef enum { LOCKED, GOT1, GOT2, OPEN, N_STATES } state_t;
typedef enum { E_RIGHT, E_WRONG, E_CLOSE, E_TIMEOUT, N_EVENTS } event_t;
typedef enum { A_UNDEFINED, A_NONE, A_UNLOCK, A_LOCK, A_BEEP, N_ACTIONS } action_t;

typedef struct { state_t next; action_t action; } transition_t;

extern const transition_t lock_table[N_STATES][N_EVENTS];

/**
 * Given a state `s` and an event `e`, returns the next state and stores the
 * action to take in *action. Assumes `s` and `e` are in range and action is not
 * `NULL`.
 */
state_t lock_step(state_t s, event_t e, action_t *action);

/**
 * Given a state `s` and a character `c`, returns the event `c` stands for.
 * Returns -1 for any other character, including `C` and `T`. Assumes `s` is in
 * range.
 */
int lock_event(state_t s, char c);

/**
 * Given a state `s`, returns its name (e.g. `GOT2`), or `?` if `s` is out of
 * range.
 */
const char *state_name(state_t s);

/**
 * Given an event `e`, returns its name without the `E_` prefix (e.g. `RIGHT`),
 * or `?` if `e` is out of range.
 */
const char *event_name(event_t e);

/**
 * Given an action `a`, returns its name without the `A_` prefix
 * (e.g. `UNLOCK`), or `?` if `a` is out of range.
 */
const char *action_name(action_t a);

#endif
