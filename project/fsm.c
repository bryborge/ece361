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

/* At or below this temperature, REMOTE in CLOSED is refused (rule 3). */
#define FREEZE_C 1.0f

#define TRAVEL   15     /* ticks for a full open or close (rule 1) */
#define PAUSE    2      /* ticks OBSTRUCTED holds the door (rule 2) */
/* CLOSED and OPEN never time out. A time allowed of 0 would behave the same
 * today, because their timeout entries (†) loop back with A_NONE, but it
 * would look them up on every tick: an edit to either entry would then act
 * every tick instead of never. */
#define NO_TIMER (-1)

/* The press event for each combination of the freeze guard and the barrier,
 * indexed [frozen][barrier]. */
static const event_t press_events[2][2] = {
    [false][false] = E_REMOTE,
    [false][true]  = E_REMOTE_BARRIER,
    [true][false]  = E_REMOTE_FROZEN,
    [true][true]   = E_REMOTE_FROZEN_BARRIER,
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
 * Gives the seconds the door has spent in its current state.
 *
 * @param d   The door.
 * @param now The current time; asserted not before d->entered_at.
 * @return now - d->entered_at.
 */
static int elapsed(const door_t *d, time_t now) {
    assert(now >= d->entered_at);

    /* Plain subtraction assumes time_t counts whole seconds. POSIX
     * guarantees that and C11 does not; gmtime and the 2038 question in
     * ASSIGNMENT.md already depend on it. */
    return (int)(now - d->entered_at);
}

/**
 * Gives the door's position, derived from the clock.
 *
 * @param d   The door.
 * @param now The current time.
 * @return The position, 0 (down) to TRAVEL (up); asserted to be in range.
 */
static int position(const door_t *d, time_t now) {
    int pos;

    switch (d->state) {
    case OPENING: pos = d->pos_at_entry + elapsed(d, now); break;
    case CLOSING: pos = d->pos_at_entry - elapsed(d, now); break;
    default:      pos = d->pos_at_entry;                   break;
    }
    assert(pos >= 0 && pos <= TRAVEL);
    return pos;
}

/**
 * Gives the time the door may spend in its current state, fixed when the
 * state was entered.
 *
 * @param d The door.
 * @return The ticks left to travel while OPENING or CLOSING, PAUSE while
 *         OBSTRUCTED, NO_TIMER otherwise.
 */
static int time_allowed(const door_t *d) {
    switch (d->state) {
    case OPENING:    return TRAVEL - d->pos_at_entry;
    case CLOSING:    return d->pos_at_entry;
    case OBSTRUCTED: return PAUSE;
    default:         return NO_TIMER;
    }
}

/**
 * Tells whether the door's time in its current state is up.
 */
static bool timed_out(const door_t *d, time_t now) {
    int allowed = time_allowed(d);

    return allowed != NO_TIMER && elapsed(d, now) >= allowed;
}

/**
 * Looks up event e for the door in the table. If the state changes, takes a
 * snapshot of the position first (position reads the old state), then
 * restarts the clock.
 *
 * @return The action the table gives.
 */
static action_t apply(door_t *d, event_t e, time_t now) {
    action_t action;
    state_t next = fsm_step(d->state, e, &action);

    if (next != d->state) {
        d->pos_at_entry = position(d, now);
        d->state = next;
        d->entered_at = now;
    }
    return action;
}

/**
 * Keeps a tick's action. A tick has at most one: the only action the
 * barrier gives leaves the door in OBSTRUCTED, where no press is refused,
 * and the timeout gives none.
 *
 * @param kept The tick's action so far.
 * @param a    The action from the latest lookup.
 */
static void keep(action_t *kept, action_t a) {
    if (a == A_NONE) return;

    assert(*kept == A_NONE);
    *kept = a;
}

/**
 * Runs one tick of the door: the barrier, a press, then the timeout, each
 * looked up from the state the one before left (SPEC.md, Order).
 *
 * @param d             The door; asserted not NULL.
 * @param now           The current time; asserted not before d->entered_at.
 * @param prev_switches The switch register on the previous tick.
 * @param switches      The switch register on this tick.
 * @param temp_c        The temperature, rounded to 0.1 °C.
 * @return The tick's action, or A_NONE.
 */
action_t fsm_tick(door_t *d, time_t now, uint32_t prev_switches,
                  uint32_t switches, float temp_c) {
    assert(d != NULL);

    bool barrier = (switches & SW_BARRIER) != 0;
    bool frozen = temp_c <= FREEZE_C;
    action_t action = A_NONE;

    if (barrier) keep(&action, apply(d, E_BARRIER, now));
    if (fsm_remote_pressed(prev_switches, switches)) {
        keep(&action, apply(d, press_events[frozen][barrier], now));
    }
    if (timed_out(d, now)) keep(&action, apply(d, E_TIMEOUT, now));

    return action;
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
