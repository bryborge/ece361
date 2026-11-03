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

/* Bits of the LED register (ASSIGNMENT.md section 3). */
#define LED_MOTOR_UP   (1u << 0)
#define LED_MOTOR_DOWN (1u << 1)

/* The LEDs for each state: the motor runs only while the door moves. */
static const uint32_t state_leds[N_STATES] = {
    [CLOSED]     = 0,
    [OPENING]    = LED_MOTOR_UP,
    [OPEN]       = 0,
    [CLOSING]    = LED_MOTOR_DOWN,
    [OBSTRUCTED] = 0,
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
 * Tells whether REMOTE was pressed between two readings of the switches.
 *
 * @param prev The switch register on the previous tick.
 * @param now  The switch register on this tick.
 * @return true if REMOTE's bit went from 0 to 1; other bits are ignored.
 */
bool fsm_remote_pressed(uint32_t prev, uint32_t now) {
    return !(prev & SW_REMOTE) && (now & SW_REMOTE);
}

/**
 * Gives the LED register for a state, so the LEDs can never disagree with
 * the state they show.
 *
 * @param s The state; asserted to be in range.
 * @return MOTOR_UP while OPENING, MOTOR_DOWN while CLOSING, otherwise 0.
 */
uint32_t fsm_leds(state_t s) {
    assert(s >= 0 && s < N_STATES);

    return state_leds[s];
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
