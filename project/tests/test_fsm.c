/*
 * test_fsm.c:
 *
 * Tests for fsm.c. Prints PASS or FAIL for each check; test_main.c runs them
 * and prints the summary. A check that covers every state or every cell
 * prints the first case that fails before its FAIL line.
 */
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "check.h"
#include "fsm.h"

/**
 * Runs n ticks with the same inputs.
 *
 * @return true if the machine was in state s after every one of them.
 */
static bool stays(fsm_t *fsm, const fsm_inputs_t *inputs, state_t s, int n) {
    fsm_report_t report;
    bool stayed = true;

    for (int i = 0; i < n; i++) {
        fsm_tick(fsm, inputs, &report);
        stayed = fsm->state == s && stayed;
    }
    return stayed;
}

/**
 * Checks every cell of the transition table.
 *
 * @return true if no cell has the action A_UNDEFINED.
 */
static bool no_undefined_transition(void) {
    for (int s = 0; s < N_STATES; s++) {
        for (int e = 0; e < N_EVENTS; e++) {
            action_t action;
            fsm_step((state_t)s, (event_t)e, &action);

            if (action == A_UNDEFINED) {
                printf("  undefined: state %s, event %d\n",
                    fsm_state_name((state_t)s), e);
                return false;
            }
        }
    }
    return true;
}

/**
 * Checks the motors of every state.
 *
 * @return true if the up motor runs only while OPENING and the down motor
 *         only while CLOSING.
 */
static bool motors_run_only_while_moving(void) {
    for (int s = 0; s < N_STATES; s++) {
        const fsm_outputs_t outputs = fsm_outputs((state_t)s);

        if (outputs.motor_up != (s == OPENING)
            || outputs.motor_down != (s == CLOSING)) {
            printf("  wrong motors: %s\n", fsm_state_name((state_t)s));
            return false;
        }
    }
    return true;
}

/**
 * Compares the color of every pair of states.
 *
 * @return true if no two states have the same color.
 */
static bool colors_distinct(void) {
    for (int a = 0; a < N_STATES; a++) {
        for (int b = a + 1; b < N_STATES; b++) {
            const fsm_outputs_t x = fsm_outputs((state_t)a);
            const fsm_outputs_t y = fsm_outputs((state_t)b);

            if (x.red == y.red && x.green == y.green && x.blue == y.blue) {
                printf("  same color: %s and %s\n",
                    fsm_state_name((state_t)a), fsm_state_name((state_t)b));
                return false;
            }
        }
    }
    return true;
}

/**
 * Packs and unpacks every state with every combination of the three flags.
 *
 * @return true if every field comes back as it was packed.
 */
static bool unpack_returns_packed_fields(void) {
    for (int s = 0; s < N_STATES; s++) {
        for (int flags = 0; flags < 8; flags++) {
            const fsm_status_t packed = {
                .state               = (state_t)s,
                .motor_up            = (flags & 1) != 0,
                .motor_down          = (flags & 2) != 0,
                .freeze_guard_active = (flags & 4) != 0,
            };
            const fsm_status_t unpacked =
                fsm_status_unpack(fsm_status_pack(packed));

            if (unpacked.state != packed.state
                || unpacked.motor_up != packed.motor_up
                || unpacked.motor_down != packed.motor_down
                || unpacked.freeze_guard_active != packed.freeze_guard_active) {
                printf("  round trip: state %s, flags %d\n",
                    fsm_state_name((state_t)s), flags);
                return false;
            }
        }
    }
    return true;
}

/**
 * Packs every state with all three flags on.
 *
 * @return true if every bit outside the fields (0x77) is 0.
 */
static bool unused_bits_zero(void) {
    const uint32_t unused_bits = ~UINT32_C(0x77);

    for (int s = 0; s < N_STATES; s++) {
        const fsm_status_t all_flags_on = {
            .state               = (state_t)s,
            .motor_up            = true,
            .motor_down          = true,
            .freeze_guard_active = true,
        };

        if ((fsm_status_pack(all_flags_on) & unused_bits) != 0) {
            printf("  unused bits set: %s\n", fsm_state_name((state_t)s));
            return false;
        }
    }
    return true;
}

/**
 * A new machine starts closed; remote off.
 */
static void check_fsm_init_starts_closed_and_at_rest(void) {
    const fsm_t fsm = fsm_init();

    CHECK(fsm.state == CLOSED);
    CHECK(!fsm.remote_was_on);
    CHECK(fsm.position == 0);
    CHECK(fsm.pause_ticks_left == 0);
}

/**
 * Every cell of the transition table is filled in. A cell left out of the
 * table's initializer is zeroed, which gives it the action A_UNDEFINED.
 */
static void check_fsm_step_has_no_undefined_transition(void) {
    CHECK(no_undefined_transition());
}

/**
 * Normal operation: Remote press opens the initially closed garage door.
 * Once it is open, a second remote press closes it again. No logged events.
 *
 * CLOSED -> OPENING -> OPEN -> CLOSING -> CLOSED.
 */
static void check_fsm_step_normal_operation(void) {
    state_t state = CLOSED;
    action_t action;

    state = fsm_step(state, E_REMOTE_PRESSED, &action);
    CHECK(state == OPENING);
    CHECK(action == A_NONE);

    state = fsm_step(state, E_TIMER_EXPIRED, &action);
    CHECK(state == OPEN);
    CHECK(action == A_NONE);

    state = fsm_step(state, E_REMOTE_PRESSED, &action);
    CHECK(state == CLOSING);
    CHECK(action == A_NONE);

    state = fsm_step(state, E_TIMER_EXPIRED, &action);
    CHECK(state == CLOSED);
    CHECK(action == A_NONE);
}

/**
 * A blocked light barrier stops a closing door and logs the
 * obstruction. When the pause is over, the door opens again. No logged event.
 *
 * CLOSING -> OBSTRUCTED -> OPENING.
 */
static void check_fsm_step_obstruction_while_closing(void) {
    state_t state = CLOSING;
    action_t action;

    state = fsm_step(state, E_BARRIER_BLOCKED, &action);
    CHECK(state == OBSTRUCTED);
    CHECK(action == A_LOG_OBSTRUCTION);

    state = fsm_step(state, E_TIMER_EXPIRED, &action);
    CHECK(state == OPENING);
    CHECK(action == A_NONE);
}

/**
 * A remote press at or below 1.0 C is refused while the door is
 * closed. The door stays closed and the refusal is logged.
 *
 * CLOSED -> CLOSED.
 */
static void check_fsm_step_freezing_press_refused_when_closed(void) {
    state_t state = CLOSED;
    action_t action;

    state = fsm_step(state, E_REMOTE_PRESSED_FREEZING, &action);
    CHECK(state == CLOSED);
    CHECK(action == A_LOG_REFUSAL);
}

/**
 * A remote press reverses a door that is in motion.
 *
 * OPENING -> CLOSING, and CLOSING -> OPENING.
 */
static void check_fsm_step_press_reverses_moving_door(void) {
    state_t state;
    action_t action;

    state = fsm_step(OPENING, E_REMOTE_PRESSED, &action);
    CHECK(state == CLOSING);
    CHECK(action == A_NONE);

    state = fsm_step(CLOSING, E_REMOTE_PRESSED, &action);
    CHECK(state == OPENING);
    CHECK(action == A_NONE);
}

/**
 * The freeze guard applies only to a closed door. In any other
 * state a press at or below 1.0 C acts like a press above it, and no refusal
 * is logged.
 *
 * OPENING -> CLOSING, OPEN -> CLOSING, and CLOSING -> OPENING.
 */
static void check_fsm_step_freezing_press_allowed_when_not_closed(void) {
    state_t state;
    action_t action;

    state = fsm_step(OPENING, E_REMOTE_PRESSED_FREEZING, &action);
    CHECK(state == CLOSING);
    CHECK(action == A_NONE);

    state = fsm_step(OPEN, E_REMOTE_PRESSED_FREEZING, &action);
    CHECK(state == CLOSING);
    CHECK(action == A_NONE);

    state = fsm_step(CLOSING, E_REMOTE_PRESSED_FREEZING, &action);
    CHECK(state == OPENING);
    CHECK(action == A_NONE);
}

/**
 * A blocked light barrier is ignored unless the door is closing.
 * The state does not change and no obstruction is logged.
 */
static void check_fsm_step_barrier_ignored_when_not_closing(void) {
    state_t state;
    action_t action;

    state = fsm_step(CLOSED, E_BARRIER_BLOCKED, &action);
    CHECK(state == CLOSED);
    CHECK(action == A_NONE);

    state = fsm_step(OPENING, E_BARRIER_BLOCKED, &action);
    CHECK(state == OPENING);
    CHECK(action == A_NONE);

    state = fsm_step(OPEN, E_BARRIER_BLOCKED, &action);
    CHECK(state == OPEN);
    CHECK(action == A_NONE);

    state = fsm_step(OBSTRUCTED, E_BARRIER_BLOCKED, &action);
    CHECK(state == OBSTRUCTED);
    CHECK(action == A_NONE);
}

/**
 * A remote press is ignored while the door is obstructed, regardless of
 * temperature, and is not logged as a refusal.
 */
static void check_fsm_step_press_ignored_when_obstructed(void) {
    state_t state;
    action_t action;

    state = fsm_step(OBSTRUCTED, E_REMOTE_PRESSED, &action);
    CHECK(state == OBSTRUCTED);
    CHECK(action == A_NONE);

    state = fsm_step(OBSTRUCTED, E_REMOTE_PRESSED_FREEZING, &action);
    CHECK(state == OBSTRUCTED);
    CHECK(action == A_NONE);
}

/**
 * A remote press opens the closed garage door in 15 ticks. The door is
 * OPENING on the tick of the press and the 14 ticks after it, and OPEN on
 * the tick after those.
 */
static void check_fsm_tick_press_opens_door_in_15_ticks(void) {
    const fsm_inputs_t press = {.remote_on = true, .temp_c = 15.0f};
    const fsm_inputs_t idle = {.remote_on = false, .temp_c = 15.0f};
    fsm_t fsm = fsm_init();
    fsm_report_t report;

    fsm_tick(&fsm, &press, &report);
    CHECK(fsm.state == OPENING);
    CHECK(report.remote_pressed);

    bool stayed_opening = true;
    bool pressed_again = false;

    for (int i = 0; i < 14; i++) {
        fsm_tick(&fsm, &idle, &report);
        stayed_opening = fsm.state == OPENING && stayed_opening;
        pressed_again = report.remote_pressed || pressed_again;
    }
    CHECK(stayed_opening);
    CHECK(!pressed_again);

    fsm_tick(&fsm, &idle, &report);
    CHECK(fsm.state == OPEN);
}

/**
 * A remote press closes an open garage door in 15 ticks. The door is
 * CLOSING on the tick of the press and the 14 ticks after it, and CLOSED on
 * the tick after those.
 */
static void check_fsm_tick_press_closes_door_in_15_ticks(void) {
    const fsm_inputs_t press = {.remote_on = true, .temp_c = 15.0f};
    const fsm_inputs_t idle = {.remote_on = false, .temp_c = 15.0f};
    fsm_t fsm = {.state = OPEN, .position = 15};
    fsm_report_t report;

    fsm_tick(&fsm, &press, &report);
    CHECK(fsm.state == CLOSING);

    CHECK(stays(&fsm, &idle, CLOSING, 14));

    fsm_tick(&fsm, &idle, &report);
    CHECK(fsm.state == CLOSED);
    CHECK(fsm.position == 0);
}

/**
 * Holding the remote on counts as a single press. A second press
 * requires the remote to be released and turned on again.
 */
static void check_fsm_tick_held_remote_is_one_press(void) {
    const fsm_inputs_t held = {.remote_on = true, .temp_c = 15.0f};
    const fsm_inputs_t released = {.remote_on = false, .temp_c = 15.0f};
    fsm_t fsm = fsm_init();
    fsm_report_t report;

    fsm_tick(&fsm, &held, &report);
    CHECK(report.remote_pressed);
    CHECK(fsm.state == OPENING);

    fsm_tick(&fsm, &held, &report);
    CHECK(!report.remote_pressed);
    CHECK(fsm.state == OPENING);

    fsm_tick(&fsm, &released, &report);
    CHECK(!report.remote_pressed);
    CHECK(fsm.state == OPENING);

    fsm_tick(&fsm, &held, &report);
    CHECK(report.remote_pressed);
    CHECK(fsm.state == CLOSING);
}

/**
 * When the garage door reverses its travel, it goes back only as far as
 * it has already gone.
 *
 * The door still moves one step on the tick of the press, and then reverses.
 * A door at position 4 therefore reverses from position 5 and is CLOSED
 * 5 ticks later.
 */
static void check_fsm_tick_reversal_takes_distance_travelled(void) {
    const fsm_inputs_t press = {.remote_on = true, .temp_c = 15.0f};
    const fsm_inputs_t idle = {.remote_on = false, .temp_c = 15.0f};
    fsm_t fsm = {.state = OPENING, .position = 4};
    fsm_report_t report;

    fsm_tick(&fsm, &press, &report);
    CHECK(fsm.state == CLOSING);
    CHECK(fsm.position == 5);

    CHECK(stays(&fsm, &idle, CLOSING, 4));

    fsm_tick(&fsm, &idle, &report);
    CHECK(fsm.state == CLOSED);
    CHECK(fsm.position == 0);
}

/**
 * The light barrier stops a closing garage door for 2 ticks when it
 * detects an object in the door's path, then the door begins to open again,
 * and the obstruction is reported.
 */
static void check_fsm_tick_obstruction_pauses_then_reopens(void) {
    const fsm_inputs_t blocked = {.barrier_blocked = true, .temp_c = 15.0f};
    const fsm_inputs_t idle = {.barrier_blocked = false, .temp_c = 15.0f};
    fsm_t fsm = {.state = CLOSING, .position = 3};
    fsm_report_t report;

    fsm_tick(&fsm, &blocked, &report);
    CHECK(fsm.state == OBSTRUCTED);
    CHECK(report.obstruction_detected);
    CHECK(fsm.position == 3);

    fsm_tick(&fsm, &blocked, &report);
    CHECK(fsm.state == OBSTRUCTED);
    CHECK(!report.obstruction_detected);
    CHECK(fsm.position == 3);

    fsm_tick(&fsm, &blocked, &report);
    CHECK(fsm.state == OPENING);
    CHECK(fsm.position == 3);

    CHECK(stays(&fsm, &idle, OPENING, 11));

    fsm_tick(&fsm, &idle, &report);
    CHECK(fsm.state == OPEN);
}

/**
 * The light barrier is checked before completion is checked. The garage
 * door stops instead of moving on the tick it would have finished closing.
 */
static void check_fsm_tick_barrier_precedes_completion(void) {
    const fsm_inputs_t blocked = {.barrier_blocked = true, .temp_c = 15.0f};
    fsm_t fsm = {.state = CLOSING, .position = 1};
    fsm_report_t report;

    fsm_tick(&fsm, &blocked, &report);
    CHECK(fsm.state == OBSTRUCTED);
    CHECK(report.obstruction_detected);
    CHECK(fsm.position == 1);
}

/**
 * A remote press at exactly 1.0 C is refused while the door is closed,
 * and is reported as both a press and a refusal. At 1.1 C it opens the door.
 */
static void check_fsm_tick_freezing_press_refused_when_closed(void) {
    const fsm_inputs_t freezing_press = {.remote_on = true, .temp_c = 1.0f};
    const fsm_inputs_t warm_press = {.remote_on = true, .temp_c = 1.1f};
    fsm_report_t report;

    fsm_t fsm = fsm_init();
    fsm_tick(&fsm, &freezing_press, &report);
    CHECK(fsm.state == CLOSED);
    CHECK(report.remote_pressed);
    CHECK(report.press_refused);

    fsm = fsm_init();
    fsm_tick(&fsm, &warm_press, &report);
    CHECK(fsm.state == OPENING);
    CHECK(report.remote_pressed);
    CHECK(!report.press_refused);
}

/**
 * The freeze guard is active when:
 *
 *      1) the garage door is closed, and
 *      2) it is at or below 1.0 C.
 *
 * Both are judged at the end of the tick. When the garage door finishes
 * closing, the freeze guard becomes active on that same tick.
 */
static void check_fsm_tick_freeze_guard_active_when_closed_and_cold(void) {
    const fsm_inputs_t cold = {.temp_c = 1.0f};
    const fsm_inputs_t warm = {.temp_c = 1.1f};
    fsm_report_t report;

    fsm_t fsm = fsm_init();
    fsm_tick(&fsm, &cold, &report);
    CHECK(report.freeze_guard_active);

    fsm_tick(&fsm, &warm, &report);
    CHECK(!report.freeze_guard_active);

    fsm = (fsm_t){.state = OPEN, .position = 15};
    fsm_tick(&fsm, &cold, &report);
    CHECK(!report.freeze_guard_active);

    fsm = (fsm_t){.state = CLOSING, .position = 1};
    fsm_tick(&fsm, &cold, &report);
    CHECK(fsm.state == CLOSED);
    CHECK(report.freeze_guard_active);
}

/**
 * The up motor runs only while the door is opening, and the down motor
 * only while it is closing. In every other state, both motors are off.
 */
static void check_fsm_outputs_motor_runs_only_while_door_moves(void) {
    CHECK(motors_run_only_while_moving());
}

/**
 * Each state has its own color, so no two states look the same.
 */
static void check_fsm_outputs_color_is_distinct_per_state(void) {
    CHECK(colors_distinct());
}

/**
 * Packing a status word and unpacking it again gives back every field,
 * for every state and every combination of the three flags.
 */
static void check_fsm_status_unpack_returns_packed_fields(void) {
    CHECK(unpack_returns_packed_fields());
}

/**
 * Every bit of a status word outside its four fields is 0. The fields
 * are bits 0 to 2 and bits 4 to 6, so the used bits are 0x77.
 */
static void check_fsm_status_unused_bits_are_zero(void) {
    CHECK(unused_bits_zero());
}

/**
 * Each field has a fixed place in the status word. The state is the
 * low hex digit, and the next digit holds the up motor (bit 4), the down
 * motor (bit 5), and the freeze guard (bit 6).
 */
static void check_fsm_status_pack_places_each_field(void) {
    const fsm_status_t opening = {.state = OPENING, .motor_up = true};
    const fsm_status_t closing = {.state = CLOSING, .motor_down = true};
    const fsm_status_t guarded = {.state = CLOSED, .freeze_guard_active = true};

    CHECK(fsm_status_pack(opening) == 0x00000011);
    CHECK(fsm_status_pack(closing) == 0x00000023);
    CHECK(fsm_status_pack(guarded) == 0x00000040);
}

void run_fsm_tests(void) {
    check_fsm_init_starts_closed_and_at_rest();
    check_fsm_step_has_no_undefined_transition();
    check_fsm_step_normal_operation();
    check_fsm_step_obstruction_while_closing();
    check_fsm_step_freezing_press_refused_when_closed();
    check_fsm_step_press_reverses_moving_door();
    check_fsm_step_freezing_press_allowed_when_not_closed();
    check_fsm_step_barrier_ignored_when_not_closing();
    check_fsm_step_press_ignored_when_obstructed();
    check_fsm_tick_press_opens_door_in_15_ticks();
    check_fsm_tick_press_closes_door_in_15_ticks();
    check_fsm_tick_held_remote_is_one_press();
    check_fsm_tick_reversal_takes_distance_travelled();
    check_fsm_tick_obstruction_pauses_then_reopens();
    check_fsm_tick_barrier_precedes_completion();
    check_fsm_tick_freezing_press_refused_when_closed();
    check_fsm_tick_freeze_guard_active_when_closed_and_cold();
    check_fsm_outputs_motor_runs_only_while_door_moves();
    check_fsm_outputs_color_is_distinct_per_state();
    check_fsm_status_unpack_returns_packed_fields();
    check_fsm_status_unused_bits_are_zero();
    check_fsm_status_pack_places_each_field();
}
