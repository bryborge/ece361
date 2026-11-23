/**
 * @file test_fsm.c
 * @brief Unit tests for the finite state machine module.
 */
#include <stdio.h>

#include "unity.h"

#include "fsm.h"

/* Compare by name so that a failure reads "Expected 'OPEN' Was 'CLOSED'". */
#define TEST_ASSERT_STATE(expected, actual) \
    TEST_ASSERT_EQUAL_STRING(               \
        fsm_state_name(expected),           \
        fsm_state_name(actual)              \
    )

void setUp(void) {}

void tearDown(void) {}

/**
 * A new machine starts closed; remote off.
 */
static void test_fsm_init_starts_closed_and_at_rest(void) {
    const fsm_t fsm = fsm_init();

    TEST_ASSERT_STATE(CLOSED, fsm.state);
    TEST_ASSERT_FALSE(fsm.remote_was_on);
    TEST_ASSERT_EQUAL_INT(0, fsm.position);
    TEST_ASSERT_EQUAL_INT(0, fsm.pause_ticks_left);
}

/**
 * Every cell of the transition table is filled in. A cell left out of the
 * table's initializer is zeroed, which gives it the action A_UNDEFINED.
 */
static void test_fsm_step_has_no_undefined_transition(void) {
    for (int s = 0; s < N_STATES; s++) {
        for (int e = 0; e < N_EVENTS; e++) {
            action_t action;
            fsm_step((state_t)s, (event_t)e, &action);

            char cell[48];
            snprintf(cell, sizeof cell, "state %s, event %d",
                fsm_state_name((state_t)s), e);
            TEST_ASSERT_NOT_EQUAL_INT_MESSAGE(A_UNDEFINED, action, cell);
        }
    }
}

/**
 * Normal operation: Remote press opens the initially closed garage door.
 * Once it is open, a second remote press closes it again. No logged events.
 *
 * CLOSED -> OPENING -> OPEN -> CLOSING -> CLOSED.
 */
static void test_fsm_step_normal_operation(void) {
    state_t state = CLOSED;
    action_t action;

    state = fsm_step(state, E_REMOTE_PRESSED, &action);
    TEST_ASSERT_STATE(OPENING, state);
    TEST_ASSERT_EQUAL_INT(A_NONE, action);

    state = fsm_step(state, E_TIMER_EXPIRED, &action);
    TEST_ASSERT_STATE(OPEN, state);
    TEST_ASSERT_EQUAL_INT(A_NONE, action);

    state = fsm_step(state, E_REMOTE_PRESSED, &action);
    TEST_ASSERT_STATE(CLOSING, state);
    TEST_ASSERT_EQUAL_INT(A_NONE, action);

    state = fsm_step(state, E_TIMER_EXPIRED, &action);
    TEST_ASSERT_STATE(CLOSED, state);
    TEST_ASSERT_EQUAL_INT(A_NONE, action);
}

/**
 * A blocked light barrier stops a closing door and logs the
 * obstruction. When the pause is over, the door opens again. No logged event.
 *
 * CLOSING -> OBSTRUCTED -> OPENING.
 */
static void test_fsm_step_obstruction_while_closing(void) {
    state_t state = CLOSING;
    action_t action;

    state = fsm_step(state, E_BARRIER_BLOCKED, &action);
    TEST_ASSERT_STATE(OBSTRUCTED, state);
    TEST_ASSERT_EQUAL_INT(A_LOG_OBSTRUCTION, action);

    state = fsm_step(state, E_TIMER_EXPIRED, &action);
    TEST_ASSERT_STATE(OPENING, state);
    TEST_ASSERT_EQUAL_INT(A_NONE, action);
}

/**
 * A remote press at or below 1.0 C is refused while the door is
 * closed. The door stays closed and the refusal is logged.
 *
 * CLOSED -> CLOSED.
 */
static void test_fsm_step_freezing_press_refused_when_closed(void) {
    state_t state = CLOSED;
    action_t action;

    state = fsm_step(state, E_REMOTE_PRESSED_FREEZING, &action);
    TEST_ASSERT_STATE(CLOSED, state);
    TEST_ASSERT_EQUAL_INT(A_LOG_REFUSAL, action);
}

/**
 * A remote press reverses a door that is in motion.
 *
 * OPENING -> CLOSING, and CLOSING -> OPENING.
 */
static void test_fsm_step_press_reverses_moving_door(void) {
    state_t state;
    action_t action;

    state = fsm_step(OPENING, E_REMOTE_PRESSED, &action);
    TEST_ASSERT_STATE(CLOSING, state);
    TEST_ASSERT_EQUAL_INT(A_NONE, action);

    state = fsm_step(CLOSING, E_REMOTE_PRESSED, &action);
    TEST_ASSERT_STATE(OPENING, state);
    TEST_ASSERT_EQUAL_INT(A_NONE, action);
}

/**
 * The freeze guard applies only to a closed door. In any other
 * state a press at or below 1.0 C acts like a press above it, and no refusal
 * is logged.
 *
 * OPENING -> CLOSING, OPEN -> CLOSING, and CLOSING -> OPENING.
 */
static void test_fsm_step_freezing_press_allowed_when_not_closed(void) {
    state_t state;
    action_t action;

    state = fsm_step(OPENING, E_REMOTE_PRESSED_FREEZING, &action);
    TEST_ASSERT_STATE(CLOSING, state);
    TEST_ASSERT_EQUAL_INT(A_NONE, action);

    state = fsm_step(OPEN, E_REMOTE_PRESSED_FREEZING, &action);
    TEST_ASSERT_STATE(CLOSING, state);
    TEST_ASSERT_EQUAL_INT(A_NONE, action);

    state = fsm_step(CLOSING, E_REMOTE_PRESSED_FREEZING, &action);
    TEST_ASSERT_STATE(OPENING, state);
    TEST_ASSERT_EQUAL_INT(A_NONE, action);
}

/**
 * A blocked light barrier is ignored unless the door is closing.
 * The state does not change and no obstruction is logged.
 */
static void test_fsm_step_barrier_ignored_when_not_closing(void) {
    state_t state;
    action_t action;

    state = fsm_step(CLOSED, E_BARRIER_BLOCKED, &action);
    TEST_ASSERT_STATE(CLOSED, state);
    TEST_ASSERT_EQUAL_INT(A_NONE, action);

    state = fsm_step(OPENING, E_BARRIER_BLOCKED, &action);
    TEST_ASSERT_STATE(OPENING, state);
    TEST_ASSERT_EQUAL_INT(A_NONE, action);

    state = fsm_step(OPEN, E_BARRIER_BLOCKED, &action);
    TEST_ASSERT_STATE(OPEN, state);
    TEST_ASSERT_EQUAL_INT(A_NONE, action);

    state = fsm_step(OBSTRUCTED, E_BARRIER_BLOCKED, &action);
    TEST_ASSERT_STATE(OBSTRUCTED, state);
    TEST_ASSERT_EQUAL_INT(A_NONE, action);
}

/**
 * A remote press is ignored while the door is obstructed, regardless of
 * temperature, and is not logged as a refusal.
 */
static void test_fsm_step_press_ignored_when_obstructed(void) {
    state_t state;
    action_t action;

    state = fsm_step(OBSTRUCTED, E_REMOTE_PRESSED, &action);
    TEST_ASSERT_STATE(OBSTRUCTED, state);
    TEST_ASSERT_EQUAL_INT(A_NONE, action);

    state = fsm_step(OBSTRUCTED, E_REMOTE_PRESSED_FREEZING, &action);
    TEST_ASSERT_STATE(OBSTRUCTED, state);
    TEST_ASSERT_EQUAL_INT(A_NONE, action);
}

/**
 * A remote press opens the closed garage door in 15 ticks. The door is
 * OPENING on the tick of the press and the 14 ticks after it, and OPEN on
 * the tick after those.
 */
static void test_fsm_tick_press_opens_door_in_15_ticks(void) {
    const fsm_inputs_t press = {.remote_on = true, .temp_c = 15.0f};
    const fsm_inputs_t idle = {.remote_on = false, .temp_c = 15.0f};
    fsm_t fsm = fsm_init();
    fsm_report_t report;

    fsm_tick(&fsm, &press, &report);
    TEST_ASSERT_STATE(OPENING, fsm.state);
    TEST_ASSERT_TRUE(report.remote_pressed);

    for (int i = 0; i < 14; i++) {
        fsm_tick(&fsm, &idle, &report);
        TEST_ASSERT_STATE(OPENING, fsm.state);
        TEST_ASSERT_FALSE(report.remote_pressed);
    }

    fsm_tick(&fsm, &idle, &report);
    TEST_ASSERT_STATE(OPEN, fsm.state);
}

/**
 * A remote press closes an open garage door in 15 ticks. The door is
 * CLOSING on the tick of the press and the 14 ticks after it, and CLOSED on
 * the tick after those.
 */
static void test_fsm_tick_press_closes_door_in_15_ticks(void) {
    const fsm_inputs_t press = {.remote_on = true, .temp_c = 15.0f};
    const fsm_inputs_t idle = {.remote_on = false, .temp_c = 15.0f};
    fsm_t fsm = {.state = OPEN, .position = 15};
    fsm_report_t report;

    fsm_tick(&fsm, &press, &report);
    TEST_ASSERT_STATE(CLOSING, fsm.state);

    for (int i = 0; i < 14; i++) {
        fsm_tick(&fsm, &idle, &report);
        TEST_ASSERT_STATE(CLOSING, fsm.state);
    }

    fsm_tick(&fsm, &idle, &report);
    TEST_ASSERT_STATE(CLOSED, fsm.state);
    TEST_ASSERT_EQUAL_INT(0, fsm.position);
}

/**
 * Holding the remote on counts as a single press. A second press
 * requires the remote to be released and turned on again.
 */
static void test_fsm_tick_held_remote_is_one_press(void) {
    const fsm_inputs_t held = {.remote_on = true, .temp_c = 15.0f};
    const fsm_inputs_t released = {.remote_on = false, .temp_c = 15.0f};
    fsm_t fsm = fsm_init();
    fsm_report_t report;

    fsm_tick(&fsm, &held, &report);
    TEST_ASSERT_TRUE(report.remote_pressed);
    TEST_ASSERT_STATE(OPENING, fsm.state);

    fsm_tick(&fsm, &held, &report);
    TEST_ASSERT_FALSE(report.remote_pressed);
    TEST_ASSERT_STATE(OPENING, fsm.state);

    fsm_tick(&fsm, &released, &report);
    TEST_ASSERT_FALSE(report.remote_pressed);
    TEST_ASSERT_STATE(OPENING, fsm.state);

    fsm_tick(&fsm, &held, &report);
    TEST_ASSERT_TRUE(report.remote_pressed);
    TEST_ASSERT_STATE(CLOSING, fsm.state);
}

/**
 * When the garage door reverses its travel, it goes back only as far as
 * it has already gone.
 *
 * The door still moves one step on the tick of the press, and then reverses.
 * A door at position 4 therefore reverses from position 5 and is CLOSED
 * 5 ticks later.
 */
static void test_fsm_tick_reversal_takes_distance_travelled(void) {
    const fsm_inputs_t press = {.remote_on = true, .temp_c = 15.0f};
    const fsm_inputs_t idle = {.remote_on = false, .temp_c = 15.0f};
    fsm_t fsm = {.state = OPENING, .position = 4};
    fsm_report_t report;

    fsm_tick(&fsm, &press, &report);
    TEST_ASSERT_STATE(CLOSING, fsm.state);
    TEST_ASSERT_EQUAL_INT(5, fsm.position);

    for (int i = 0; i < 4; i++) {
        fsm_tick(&fsm, &idle, &report);
        TEST_ASSERT_STATE(CLOSING, fsm.state);
    }

    fsm_tick(&fsm, &idle, &report);
    TEST_ASSERT_STATE(CLOSED, fsm.state);
    TEST_ASSERT_EQUAL_INT(0, fsm.position);
}

/**
 * The light barrier stops a closing garage door for 2 ticks when it
 * detects an object in the door's path, then the door begins to open again,
 * and the obstruction is reported.
 */
static void test_fsm_tick_obstruction_pauses_then_reopens(void) {
    const fsm_inputs_t blocked = {.barrier_blocked = true, .temp_c = 15.0f};
    const fsm_inputs_t idle = {.barrier_blocked = false, .temp_c = 15.0f};
    fsm_t fsm = {.state = CLOSING, .position = 3};
    fsm_report_t report;

    fsm_tick(&fsm, &blocked, &report);
    TEST_ASSERT_STATE(OBSTRUCTED, fsm.state);
    TEST_ASSERT_TRUE(report.obstruction_detected);
    TEST_ASSERT_EQUAL_INT(3, fsm.position);

    fsm_tick(&fsm, &blocked, &report);
    TEST_ASSERT_STATE(OBSTRUCTED, fsm.state);
    TEST_ASSERT_FALSE(report.obstruction_detected);
    TEST_ASSERT_EQUAL_INT(3, fsm.position);

    fsm_tick(&fsm, &blocked, &report);
    TEST_ASSERT_STATE(OPENING, fsm.state);
    TEST_ASSERT_EQUAL_INT(3, fsm.position);

    for (int i = 0; i < 11; i++) {
        fsm_tick(&fsm, &idle, &report);
        TEST_ASSERT_STATE(OPENING, fsm.state);
    }

    fsm_tick(&fsm, &idle, &report);
    TEST_ASSERT_STATE(OPEN, fsm.state);
}

/**
 * The light barrier is checked before completion is checked. The garage
 * door stops instead of moving on the tick it would have finished closing.
 */
static void test_fsm_tick_barrier_precedes_completion(void) {
    const fsm_inputs_t blocked = {.barrier_blocked = true, .temp_c = 15.0f};
    fsm_t fsm = {.state = CLOSING, .position = 1};
    fsm_report_t report;

    fsm_tick(&fsm, &blocked, &report);
    TEST_ASSERT_STATE(OBSTRUCTED, fsm.state);
    TEST_ASSERT_TRUE(report.obstruction_detected);
    TEST_ASSERT_EQUAL_INT(1, fsm.position);
}

/**
 * A remote press at exactly 1.0 C is refused while the door is closed,
 * and is reported as both a press and a refusal. At 1.1 C it opens the door.
 */
static void test_fsm_tick_freezing_press_refused_when_closed(void) {
    const fsm_inputs_t freezing_press = {.remote_on = true, .temp_c = 1.0f};
    const fsm_inputs_t warm_press = {.remote_on = true, .temp_c = 1.1f};
    fsm_report_t report;

    fsm_t fsm = fsm_init();
    fsm_tick(&fsm, &freezing_press, &report);
    TEST_ASSERT_STATE(CLOSED, fsm.state);
    TEST_ASSERT_TRUE(report.remote_pressed);
    TEST_ASSERT_TRUE(report.press_refused);

    fsm = fsm_init();
    fsm_tick(&fsm, &warm_press, &report);
    TEST_ASSERT_STATE(OPENING, fsm.state);
    TEST_ASSERT_TRUE(report.remote_pressed);
    TEST_ASSERT_FALSE(report.press_refused);
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
static void test_fsm_tick_freeze_guard_active_when_closed_and_cold(void) {
    const fsm_inputs_t cold = {.temp_c = 1.0f};
    const fsm_inputs_t warm = {.temp_c = 1.1f};
    fsm_report_t report;

    fsm_t fsm = fsm_init();
    fsm_tick(&fsm, &cold, &report);
    TEST_ASSERT_TRUE(report.freeze_guard_active);

    fsm_tick(&fsm, &warm, &report);
    TEST_ASSERT_FALSE(report.freeze_guard_active);

    fsm = (fsm_t){.state = OPEN, .position = 15};
    fsm_tick(&fsm, &cold, &report);
    TEST_ASSERT_FALSE(report.freeze_guard_active);

    fsm = (fsm_t){.state = CLOSING, .position = 1};
    fsm_tick(&fsm, &cold, &report);
    TEST_ASSERT_STATE(CLOSED, fsm.state);
    TEST_ASSERT_TRUE(report.freeze_guard_active);
}

/**
 * The up motor runs only while the door is opening, and the down motor
 * only while it is closing. In every other state, both motors are off.
 */
static void test_fsm_outputs_motor_runs_only_while_door_moves(void) {
    for (int s = 0; s < N_STATES; s++) {
        const fsm_outputs_t outputs = fsm_outputs((state_t)s);
        const char *name = fsm_state_name((state_t)s);

        TEST_ASSERT_EQUAL_MESSAGE(s == OPENING, outputs.motor_up, name);
        TEST_ASSERT_EQUAL_MESSAGE(s == CLOSING, outputs.motor_down, name);
    }
}

/**
 * Each state has its own color, so no two states look the same.
 */
static void test_fsm_outputs_color_is_distinct_per_state(void) {
    for (int a = 0; a < N_STATES; a++) {
        for (int b = a + 1; b < N_STATES; b++) {
            const fsm_outputs_t x = fsm_outputs((state_t)a);
            const fsm_outputs_t y = fsm_outputs((state_t)b);
            const bool same_color =
                x.red == y.red && x.green == y.green && x.blue == y.blue;

            char pair[48];
            snprintf(pair, sizeof pair, "%s and %s",
                fsm_state_name((state_t)a), fsm_state_name((state_t)b));
            TEST_ASSERT_FALSE_MESSAGE(same_color, pair);
        }
    }
}

/**
 * Packing a status word and unpacking it again gives back every field,
 * for every state and every combination of the three flags.
 */
static void test_fsm_status_unpack_returns_packed_fields(void) {
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

            TEST_ASSERT_STATE(packed.state, unpacked.state);
            TEST_ASSERT_EQUAL(packed.motor_up, unpacked.motor_up);
            TEST_ASSERT_EQUAL(packed.motor_down, unpacked.motor_down);
            TEST_ASSERT_EQUAL(
                packed.freeze_guard_active, unpacked.freeze_guard_active);
        }
    }
}

/**
 * Every bit of a status word outside its four fields is 0. The fields
 * are bits 0 to 2 and bits 4 to 6, so the used bits are 0x77.
 */
static void test_fsm_status_unused_bits_are_zero(void) {
    const uint32_t unused_bits = ~UINT32_C(0x77);

    for (int s = 0; s < N_STATES; s++) {
        const fsm_status_t all_flags_on = {
            .state               = (state_t)s,
            .motor_up            = true,
            .motor_down          = true,
            .freeze_guard_active = true,
        };

        TEST_ASSERT_BITS_LOW(unused_bits, fsm_status_pack(all_flags_on));
    }
}

/**
 * Each field has a fixed place in the status word. The state is the
 * low hex digit, and the next digit holds the up motor (bit 4), the down
 * motor (bit 5), and the freeze guard (bit 6).
 */
static void test_fsm_status_pack_places_each_field(void) {
    const fsm_status_t opening = {.state = OPENING, .motor_up = true};
    const fsm_status_t closing = {.state = CLOSING, .motor_down = true};
    const fsm_status_t guarded = {.state = CLOSED, .freeze_guard_active = true};

    TEST_ASSERT_EQUAL_HEX32(0x00000011, fsm_status_pack(opening));
    TEST_ASSERT_EQUAL_HEX32(0x00000023, fsm_status_pack(closing));
    TEST_ASSERT_EQUAL_HEX32(0x00000040, fsm_status_pack(guarded));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_fsm_init_starts_closed_and_at_rest);
    RUN_TEST(test_fsm_step_has_no_undefined_transition);
    RUN_TEST(test_fsm_step_normal_operation);
    RUN_TEST(test_fsm_step_obstruction_while_closing);
    RUN_TEST(test_fsm_step_freezing_press_refused_when_closed);
    RUN_TEST(test_fsm_step_press_reverses_moving_door);
    RUN_TEST(test_fsm_step_freezing_press_allowed_when_not_closed);
    RUN_TEST(test_fsm_step_barrier_ignored_when_not_closing);
    RUN_TEST(test_fsm_step_press_ignored_when_obstructed);
    RUN_TEST(test_fsm_tick_press_opens_door_in_15_ticks);
    RUN_TEST(test_fsm_tick_press_closes_door_in_15_ticks);
    RUN_TEST(test_fsm_tick_held_remote_is_one_press);
    RUN_TEST(test_fsm_tick_reversal_takes_distance_travelled);
    RUN_TEST(test_fsm_tick_obstruction_pauses_then_reopens);
    RUN_TEST(test_fsm_tick_barrier_precedes_completion);
    RUN_TEST(test_fsm_tick_freezing_press_refused_when_closed);
    RUN_TEST(test_fsm_tick_freeze_guard_active_when_closed_and_cold);
    RUN_TEST(test_fsm_outputs_motor_runs_only_while_door_moves);
    RUN_TEST(test_fsm_outputs_color_is_distinct_per_state);
    RUN_TEST(test_fsm_status_unpack_returns_packed_fields);
    RUN_TEST(test_fsm_status_unused_bits_are_zero);
    RUN_TEST(test_fsm_status_pack_places_each_field);

    return UNITY_END();
}
