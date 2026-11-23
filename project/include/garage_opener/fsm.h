/**
 * @file fsm.h
 * @brief The garage door state machine.
 */
#ifndef FSM_H
#define FSM_H

#include <stdbool.h>
#include <stdint.h>

/* States */
typedef enum {
    CLOSED,                     // The door is closed (Initial state).
    OPENING,                    // The door is opening.
    OPEN,                       // The door is open.
    CLOSING,                    // The door is closing.
    OBSTRUCTED,                 // The door is obstructed by something.
    N_STATES
} state_t;

/* Events */
typedef enum {
    E_REMOTE_PRESSED,           // The remote was pressed, temp > 1.0 C.
    E_REMOTE_PRESSED_FREEZING,  // The remote was pressed, temp <= 1.0 C.
    E_BARRIER_BLOCKED,          // The light barrier is triggered.
    E_TIMER_EXPIRED,            // The door's travel or its pause is complete.
    N_EVENTS
} event_t;

/* Actions */
typedef enum {
    A_UNDEFINED,                // Undefined transition.
    A_NONE,                     // Do nothing.
    A_LOG_REFUSAL,              // Record a refused remote press.
    A_LOG_OBSTRUCTION,          // Record an obstruction.
    N_ACTIONS
} action_t;

typedef struct {
    bool remote_on;
    bool barrier_blocked;
    float temp_c;
} fsm_inputs_t;

typedef struct {
    bool motor_up;
    bool motor_down;
    uint8_t red;
    uint8_t green;
    uint8_t blue;
} fsm_outputs_t;

typedef struct {
    state_t state;
    bool remote_was_on;
    int position;
    int pause_ticks_left;
} fsm_t;

typedef struct {
    bool remote_pressed;
    bool press_refused;
    bool obstruction_detected;
    bool freeze_guard_active;
} fsm_report_t;

/*
 * The fields of the status word: one 32-bit word that summarizes the machine.
 * Each hex digit holds one kind of information, so the word can be read at a
 * glance:
 *
 *   Bits 0 to 2   The state.
 *   Bit  4        The up motor is on.
 *   Bit  5        The down motor is on.
 *   Bit  6        The freeze guard is active.
 *
 * Every other bit is 0.
 */
typedef struct {
    state_t state;
    bool motor_up;
    bool motor_down;
    bool freeze_guard_active;
} fsm_status_t;

/**
 * Initialize a machine in its starting condition. The garage door is closed,
 * and the remote is off before the first tick.
 *
 * @return The machine, for the caller to keep and pass to fsm_tick.
 */
fsm_t fsm_init(void);

/**
 * Run the machine for one tick. Events are derived from the inputs and
 * applied in a fixed order:
 *
 *   1. Barrier: the light barrier is blocked.
 *   2. Completion: an opening or closing door moves one step, and an
 *      obstructed door counts down its pause. Reaching the end of travel or
 *      of the pause completes the state.
 *   3. Press: the remote went from off to on since the previous tick.
 *
 * A state entered during a tick does not move or count down until the next
 * tick. Asserts that no argument is NULL.
 *
 * @param fsm    The machine to advance.
 * @param inputs What the machine sees on this tick.
 * @param report Where to store what happened on this tick. Every field is
 *               written.
 */
void fsm_tick(fsm_t *fsm, const fsm_inputs_t *inputs, fsm_report_t *report);

/**
 * Look up the transition for an event occurring in a state. The machine keeps
 * no state of its own: the caller holds the current state and stores the one
 * returned. Asserts that state and event are in range and action is not NULL.
 *
 * @param state  The current state of the machine.
 * @param event  The event that occurred.
 * @param action Where to store the action to take.
 * @return The next state.
 */
state_t fsm_step(state_t state, event_t event, action_t *action);

/**
 * Look up the outputs for a state: which motor runs, and the color of the
 * status light as red, green, and blue levels (0 to 255). The outputs hold
 * for as long as the machine is in the state. Asserts that state is in range.
 *
 * @param state The state to look up.
 * @return The outputs for the state.
 */
fsm_outputs_t fsm_outputs(state_t state);

/**
 * Pack the fields into a status word. Asserts that the state is in range.
 *
 * @param status The fields to pack.
 * @return The status word, with every unused bit 0.
 */
uint32_t fsm_status_pack(fsm_status_t status);

/**
 * Unpack a status word into its fields. Unused bits are ignored. Asserts
 * that the word holds a valid state.
 *
 * @param word The status word.
 * @return The fields held in the word.
 */
fsm_status_t fsm_status_unpack(uint32_t word);

/**
 * Get the name of a state. Asserts that state is in range.
 *
 * @param state The state to name.
 * @return A string that is valid for the life of the program.
 */
const char *fsm_state_name(state_t state);

#endif
