/*
 * fsm.c:
 *
 * The garage door's transition table and the functions that use it.
 */
#include <assert.h>
#include <stddef.h>

#include "fsm.h"

/* For each state and event, the next state and the action to take. SPEC.md
 * gives the reason for each row. † marks an entry that can never be
 * reached; it is filled in because every entry must be. */
const transition_t fsm_table[N_STATES][N_EVENTS] = {
    [CLOSED] = {
        [E_REMOTE]                = { OPENING,    A_NONE           },
        [E_REMOTE_FROZEN]         = { CLOSED,     A_REFUSE_FROZEN  },
        [E_REMOTE_BARRIER]        = { OPENING,    A_NONE           },
        [E_REMOTE_FROZEN_BARRIER] = { CLOSED,     A_REFUSE_FROZEN  },
        [E_BARRIER]               = { CLOSED,     A_NONE           },
        [E_TIMEOUT]               = { CLOSED,     A_NONE           },  /* † */
    },
    [OPENING] = {
        [E_REMOTE]                = { CLOSING,    A_NONE           },
        [E_REMOTE_FROZEN]         = { CLOSING,    A_NONE           },
        [E_REMOTE_BARRIER]        = { OPENING,    A_REFUSE_BARRIER },
        [E_REMOTE_FROZEN_BARRIER] = { OPENING,    A_REFUSE_BARRIER },
        [E_BARRIER]               = { OPENING,    A_NONE           },
        [E_TIMEOUT]               = { OPEN,       A_NONE           },
    },
    [OPEN] = {
        [E_REMOTE]                = { CLOSING,    A_NONE           },
        [E_REMOTE_FROZEN]         = { CLOSING,    A_NONE           },
        [E_REMOTE_BARRIER]        = { OPEN,       A_REFUSE_BARRIER },
        [E_REMOTE_FROZEN_BARRIER] = { OPEN,       A_REFUSE_BARRIER },
        [E_BARRIER]               = { OPEN,       A_NONE           },
        [E_TIMEOUT]               = { OPEN,       A_NONE           },  /* † */
    },
    [CLOSING] = {
        [E_REMOTE]                = { OPENING,    A_NONE           },
        [E_REMOTE_FROZEN]         = { OPENING,    A_NONE           },
        [E_REMOTE_BARRIER]        = { OBSTRUCTED, A_OBSTRUCT       },  /* † */
        [E_REMOTE_FROZEN_BARRIER] = { OBSTRUCTED, A_OBSTRUCT       },  /* † */
        [E_BARRIER]               = { OBSTRUCTED, A_OBSTRUCT       },
        [E_TIMEOUT]               = { CLOSED,     A_NONE           },
    },
    [OBSTRUCTED] = {
        [E_REMOTE]                = { OBSTRUCTED, A_NONE           },
        [E_REMOTE_FROZEN]         = { OBSTRUCTED, A_NONE           },
        [E_REMOTE_BARRIER]        = { OBSTRUCTED, A_NONE           },
        [E_REMOTE_FROZEN_BARRIER] = { OBSTRUCTED, A_NONE           },
        [E_BARRIER]               = { OBSTRUCTED, A_NONE           },
        [E_TIMEOUT]               = { OPENING,    A_NONE           },
    },
};

static const char *const state_names[N_STATES] = {
    [CLOSED]     = "CLOSED",
    [OPENING]    = "OPENING",
    [OPEN]       = "OPEN",
    [CLOSING]    = "CLOSING",
    [OBSTRUCTED] = "OBSTRUCTED",
};

static const char *const event_names[N_EVENTS] = {
    [E_REMOTE]                = "REMOTE",
    [E_REMOTE_FROZEN]         = "REMOTE_FROZEN",
    [E_REMOTE_BARRIER]        = "REMOTE_BARRIER",
    [E_REMOTE_FROZEN_BARRIER] = "REMOTE_FROZEN_BARRIER",
    [E_BARRIER]               = "BARRIER",
    [E_TIMEOUT]               = "TIMEOUT",
};

static const char *const action_names[N_ACTIONS] = {
    [A_UNDEFINED]      = "UNDEFINED",
    [A_NONE]           = "NONE",
    [A_OBSTRUCT]       = "OBSTRUCT",
    [A_REFUSE_FROZEN]  = "REFUSE_FROZEN",
    [A_REFUSE_BARRIER] = "REFUSE_BARRIER",
};

/**
 * Takes one step of the state machine: looks up the entry for event e in
 * state s, once, with no branch on the state.
 *
 * @param s      The current state; asserted to be in range.
 * @param e      The event that happened; asserted to be in range.
 * @param action Where to store the action to take; asserted not NULL.
 * @return The next state.
 */
state_t fsm_step(state_t s, event_t e, action_t *action) {
    assert(s >= 0 && s < N_STATES);
    assert(e >= 0 && e < N_EVENTS);
    assert(action != NULL);

    const transition_t t = fsm_table[s][e];

    *action = t.action;
    return t.next;
}

/**
 * Names a state.
 *
 * @param s The state to name.
 * @return The name of `s` (e.g. `OPENING`), or `?` if `s` is out of range.
 */
const char *state_name(state_t s) {
    if (s < 0 || s >= N_STATES) return "?";

    return state_names[s];
}

/**
 * Names an event.
 *
 * @param e The event to name.
 * @return The name of `e` without the `E_` prefix (e.g. `REMOTE_FROZEN`), or
 *         `?` if `e` is out of range.
 */
const char *event_name(event_t e) {
    if (e < 0 || e >= N_EVENTS) return "?";

    return event_names[e];
}

/**
 * Names an action.
 *
 * @param a The action to name.
 * @return The name of `a` without the `A_` prefix (e.g. `OBSTRUCT`), or `?`
 *         if `a` is out of range.
 */
const char *action_name(action_t a) {
    if (a < 0 || a >= N_ACTIONS) return "?";

    return action_names[a];
}
