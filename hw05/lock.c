/*
 * lock.c:
 *
 * The combination lock's transition table and the functions that use it.
 */
#include <assert.h>
#include <stddef.h>

#include "lock.h"

/* For each state and event, the next state and the action to take. */
const transition_t lock_table[N_STATES][N_EVENTS] = {
    [LOCKED] = {
        [E_RIGHT]   = { GOT1,   A_NONE },
        [E_WRONG]   = { LOCKED, A_BEEP },
        [E_CLOSE]   = { LOCKED, A_NONE },
        [E_TIMEOUT] = { LOCKED, A_NONE },
    },
    [GOT1] = {
        [E_RIGHT]   = { GOT2,   A_NONE },
        [E_WRONG]   = { LOCKED, A_BEEP },
        [E_CLOSE]   = { GOT1,   A_NONE },
        [E_TIMEOUT] = { LOCKED, A_NONE },
    },
    [GOT2] = {
        [E_RIGHT]   = { OPEN,   A_UNLOCK },
        [E_WRONG]   = { LOCKED, A_BEEP   },
        [E_CLOSE]   = { GOT2,   A_NONE   },
        [E_TIMEOUT] = { LOCKED, A_NONE   },
    },
    [OPEN] = {
        [E_RIGHT]   = { OPEN,   A_NONE },
        [E_WRONG]   = { OPEN,   A_NONE },
        [E_CLOSE]   = { LOCKED, A_LOCK },
        [E_TIMEOUT] = { LOCKED, A_LOCK },
    },
};

static const char next_digit[N_STATES] = {
    [LOCKED] = '3',
    [GOT1]   = '1',
    [GOT2]   = '4',
    [OPEN]   = '\0',  // not strictly needed; defined for completeness.
};

static const char *const state_names[N_STATES] = {
    [LOCKED] = "LOCKED",
    [GOT1]   = "GOT1",
    [GOT2]   = "GOT2",
    [OPEN]   = "OPEN",
};

static const char *const event_names[N_EVENTS] = {
    [E_RIGHT]   = "RIGHT",
    [E_WRONG]   = "WRONG",
    [E_CLOSE]   = "CLOSE",
    [E_TIMEOUT] = "TIMEOUT",
};

static const char *const action_names[N_ACTIONS] = {
    [A_UNDEFINED] = "UNDEFINED",
    [A_NONE]      = "NONE",
    [A_UNLOCK]    = "UNLOCK",
    [A_LOCK]      = "LOCK",
    [A_BEEP]      = "BEEP",
};

/**
 * Takes one step of the lock: looks up the entry for event e in state s,
 * once, with no branch on the state.
 *
 * @param s      The current state; asserted to be in range.
 * @param e      The event that happened; asserted to be in range.
 * @param action Where to store the action to take; asserted not NULL.
 * @return The next state.
 */
state_t lock_step(state_t s, event_t e, action_t *action) {
    assert(s >= 0 && s < N_STATES);
    assert(e >= 0 && e < N_EVENTS);
    assert(action != NULL);

    const transition_t t = lock_table[s][e];

    *action = t.action;
    return t.next;
}

/**
 * Turns a character into an event for state s, with no branch on the state:
 * only which entry of next_digit is read depends on it.
 *
 * @param s The current state; asserted to be in range.
 * @param c The character typed: a digit, 'c' (the door closing) or 't' (the
 *          timer running out).
 * @return E_RIGHT if c is the next digit of the code in state s, E_WRONG for
 *         any other digit (every digit while OPEN), E_CLOSE for 'c',
 *         E_TIMEOUT for 't', or -1 for any other character.
 */
int lock_event(state_t s, char c) {
    assert(s >= 0 && s < N_STATES);

    if (c == 'c') return E_CLOSE;
    if (c == 't') return E_TIMEOUT;
    if (c < '0' || c > '9') return -1;

    return c == next_digit[s] ? E_RIGHT : E_WRONG;
}

/**
 * Names a state.
 *
 * @param s The state to name.
 * @return The name of `s` (e.g. `GOT2`), or `?` if `s` is out of range.
 */
const char *state_name(state_t s) {
    if (s < 0 || s >= N_STATES) return "?";

    return state_names[s];
}

/**
 * Names an event.
 *
 * @param e The event to name.
 * @return The name of `e` without the `E_` prefix (e.g. `RIGHT`), or `?` if `e`
 *         is out of range.
 */
const char *event_name(event_t e) {
    if (e < 0 || e >= N_EVENTS) return "?";

    return event_names[e];
}

/**
 * Names an action.
 *
 * @param a The action to name.
 * @return The name of `a` without the `A_` prefix (e.g. `UNLOCK`), or `?` if
 *         `a` is out of range.
 */
const char *action_name(action_t a) {
    if (a < 0 || a >= N_ACTIONS) return "?";

    return action_names[a];
}
