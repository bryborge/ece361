/**
 * @file fsm.c
 * @brief The garage door state machine.
 *
 * The machine is built in two layers. The transition table maps a state and
 * an event to the next state and an action, and fsm_step looks it up. On top
 * of that, fsm_tick turns one tick's inputs into events, feeds them to the
 * table in a fixed order, and keeps the door's position and pause countdown.
 */
#include <assert.h>
#include <stddef.h>

#include "fsm.h"

/* Rule values from the assignment. */
static const int travel_ticks         = 15;
static const int pause_ticks          = 2;
static const float freeze_threshold_c = 1.0f;

/* Transitions -------------------------------------------------------------- */

typedef struct {
    state_t next;
    action_t action;
} transition_t;

/*
 * The transition table, indexed by state and then by event. Each cell holds
 * the next state and the action to take. A cell left out of the initializer
 * is zeroed, which gives it the action A_UNDEFINED.
 */
static const transition_t transitions[N_STATES][N_EVENTS] = {
    [CLOSED] = {
        [E_REMOTE_PRESSED]          = { OPENING, A_NONE },
        [E_REMOTE_PRESSED_FREEZING] = { CLOSED, A_LOG_REFUSAL },
        [E_BARRIER_BLOCKED]         = { CLOSED, A_NONE },
        [E_TIMER_EXPIRED]           = { CLOSED, A_NONE }
    },
    [OPENING] = {
        [E_REMOTE_PRESSED]          = { CLOSING, A_NONE },
        [E_REMOTE_PRESSED_FREEZING] = { CLOSING, A_NONE },
        [E_BARRIER_BLOCKED]         = { OPENING, A_NONE },
        [E_TIMER_EXPIRED]           = { OPEN, A_NONE }
    },
    [OPEN] = {
        [E_REMOTE_PRESSED]          = { CLOSING, A_NONE },
        [E_REMOTE_PRESSED_FREEZING] = { CLOSING, A_NONE },
        [E_BARRIER_BLOCKED]         = { OPEN, A_NONE },
        [E_TIMER_EXPIRED]           = { OPEN, A_NONE }
    },
    [CLOSING] = {
        [E_REMOTE_PRESSED]          = { OPENING, A_NONE },
        [E_REMOTE_PRESSED_FREEZING] = { OPENING, A_NONE },
        [E_BARRIER_BLOCKED]         = { OBSTRUCTED, A_LOG_OBSTRUCTION },
        [E_TIMER_EXPIRED]           = { CLOSED, A_NONE }
    },
    [OBSTRUCTED] = {
        [E_REMOTE_PRESSED]          = { OBSTRUCTED, A_NONE },
        [E_REMOTE_PRESSED_FREEZING] = { OBSTRUCTED, A_NONE },
        [E_BARRIER_BLOCKED]         = { OBSTRUCTED, A_NONE },
        [E_TIMER_EXPIRED]           = { OPENING, A_NONE }
    }
};

state_t fsm_step(state_t state, event_t event, action_t *action) {
    assert(state >= 0 && state < N_STATES);
    assert(event >= 0 && event < N_EVENTS);
    assert(action != NULL);

    const transition_t t = transitions[state][event];

    *action = t.action;
    return t.next;
}

/* Machine ------------------------------------------------------------------ */

fsm_t fsm_init(void) {
    return (fsm_t){
        .state            = CLOSED,
        .remote_was_on    = false,
        .position         = 0,
        .pause_ticks_left = 0,
    };
}

/**
 * Feed one event to the transition table, then update the machine and the
 * report from the result.
 */
static void dispatch(fsm_t *fsm, event_t event, fsm_report_t *report) {
    action_t action;
    const state_t next = fsm_step(fsm->state, event, &action);

    switch (action) {
        case A_NONE:
            break;
        case A_LOG_REFUSAL:
            report->press_refused = true;
            break;
        case A_LOG_OBSTRUCTION:
            report->obstruction_detected = true;
            break;
        default:
            assert(false && "transition has no action");
            break;
    }

    /* The pause starts when the door becomes obstructed. */
    if (next == OBSTRUCTED && fsm->state != OBSTRUCTED) {
        fsm->pause_ticks_left = pause_ticks;
    }

    fsm->state = next;
}

void fsm_tick(fsm_t *fsm, const fsm_inputs_t *inputs, fsm_report_t *report) {
    assert(fsm != NULL);
    assert(inputs != NULL);
    assert(report != NULL);

    *report = (fsm_report_t){0};

    const bool freezing = inputs->temp_c <= freeze_threshold_c;
    const state_t starting_state = fsm->state;

    /* 1. Barrier check. */
    if (inputs->barrier_blocked) {
        dispatch(fsm, E_BARRIER_BLOCKED, report);
    }

    /*
     * 2. Completion check. Skipped if the barrier just changed the state: a
     * state entered during this tick does not move or count down until the
     * next one.
     */
    if (fsm->state == starting_state) {
        bool complete = false;

        switch (fsm->state) {
            case OPENING:
                if (fsm->position < travel_ticks) fsm->position++;
                complete = fsm->position == travel_ticks;
                break;
            case CLOSING:
                if (fsm->position > 0) fsm->position--;
                complete = fsm->position == 0;
                break;
            case OBSTRUCTED:
                if (fsm->pause_ticks_left > 0) fsm->pause_ticks_left--;
                complete = fsm->pause_ticks_left == 0;
                break;
            default:
                break;
        }

        if (complete) dispatch(fsm, E_TIMER_EXPIRED, report);
    }

    /* 3. Remote check. on now, off before. */
    const bool pressed = inputs->remote_on && !fsm->remote_was_on;
    fsm->remote_was_on = inputs->remote_on;

    if (pressed) {
        report->remote_pressed = true;
        dispatch(fsm,
            freezing ? E_REMOTE_PRESSED_FREEZING : E_REMOTE_PRESSED,
            report);
    }

    report->freeze_guard_active = freezing && fsm->state == CLOSED;
}

/* Outputs ------------------------------------------------------------------ */

static const fsm_outputs_t outputs[] = {
    /*               up     down   red  green  blue */
    [CLOSED]     = { false, false,   0,  255,    0 },  // Green
    [OPENING]    = { true,  false, 255,  255,    0 },  // Yellow
    [OPEN]       = { false, false,   0,    0,  255 },  // Blue
    [CLOSING]    = { false, true,  255,    0,  255 },  // Magenta
    [OBSTRUCTED] = { false, false, 255,    0,    0 },  // Red
};

/* Fails the build if a state is added without an entry in outputs. */
_Static_assert(
    sizeof outputs / sizeof outputs[0] == N_STATES,
    "outputs are out of step with state_t"
);

fsm_outputs_t fsm_outputs(state_t state) {
    assert(state >= 0 && state < N_STATES);

    return outputs[state];
}

/* Status word -------------------------------------------------------------- */

/* The layout: the lowest bit of each field, and the state's width. */
static const unsigned status_state_lsb        = 0;
static const unsigned status_state_width      = 3;
static const unsigned status_motor_up_lsb     = 4;
static const unsigned status_motor_down_lsb   = 5;
static const unsigned status_freeze_guard_lsb = 6;

/**
 * Get the field of a word that is width bits wide and starts at bit lsb.
 */
static uint32_t get_field(uint32_t word, unsigned lsb, unsigned width) {
    assert(width > 0 && width < 32 && lsb + width <= 32);

    const uint32_t mask = (UINT32_C(1) << width) - 1;

    return (word >> lsb) & mask;
}

/**
 * Get a copy of a word with one field replaced by value. The field is width
 * bits wide and starts at bit lsb. Bits of value that do not fit are dropped.
 */
static uint32_t set_field(
    uint32_t word, unsigned lsb, unsigned width, uint32_t value
) {
    assert(width > 0 && width < 32 && lsb + width <= 32);

    const uint32_t mask = ((UINT32_C(1) << width) - 1) << lsb;

    return (word & ~mask) | ((value << lsb) & mask);
}

uint32_t fsm_status_pack(fsm_status_t status) {
    assert(status.state >= 0 && status.state < N_STATES);

    uint32_t word = 0;

    word = set_field(word, status_state_lsb, status_state_width, status.state);
    word = set_field(word, status_motor_up_lsb, 1, status.motor_up);
    word = set_field(word, status_motor_down_lsb, 1, status.motor_down);
    word = set_field(word, status_freeze_guard_lsb, 1,
        status.freeze_guard_active);

    return word;
}

fsm_status_t fsm_status_unpack(uint32_t word) {
    const uint32_t state =
        get_field(word, status_state_lsb, status_state_width);

    assert(state < N_STATES);

    return (fsm_status_t){
        .state               = (state_t)state,
        .motor_up            = get_field(word, status_motor_up_lsb, 1),
        .motor_down          = get_field(word, status_motor_down_lsb, 1),
        .freeze_guard_active = get_field(word, status_freeze_guard_lsb, 1),
    };
}

/* State names -------------------------------------------------------------- */

static const char *const state_names[] = {
    [CLOSED]     = "CLOSED",
    [OPENING]    = "OPENING",
    [OPEN]       = "OPEN",
    [CLOSING]    = "CLOSING",
    [OBSTRUCTED] = "OBSTRUCTED",
};

/* Fails the build if a state is added without an entry in state_names. */
_Static_assert(
    sizeof state_names / sizeof state_names[0] == N_STATES,
    "state_names are out of step with state_t"
);

const char *fsm_state_name(state_t state) {
    assert(state >= 0 && state < N_STATES);

    return state_names[state];
}
