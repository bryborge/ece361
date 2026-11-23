/*
 * test_status.c:
 *
 * Tests for status.c. The expected words are written out from the bit layout
 * in SPEC.md: state in bits 2..0, MOTOR_UP in bit 3, MOTOR_DOWN in bit 4, freeze
 * guard active in bit 5. Prints PASS or FAIL for each check; test_main.c runs
 * them and prints the summary.
 */
#include <stdbool.h>
#include <stdint.h>

#include "check.h"
#include "status.h"

static bool word_is(state_t s, bool up, bool down, bool freeze, uint32_t word) {
    status_t st = { .state = s, .motor_up = up, .motor_down = down,
                    .freeze_active = freeze };

    return status_pack(&st) == word;
}

static bool round_trips(state_t s, bool up, bool down, bool freeze) {
    status_t st = { .state = s, .motor_up = up, .motor_down = down,
                    .freeze_active = freeze };
    status_t back = status_unpack(status_pack(&st));

    return back.state == s && back.motor_up == up && back.motor_down == down
        && back.freeze_active == freeze;
}

static void check_pack_each_field(void) {
    CHECK(word_is(CLOSED,     false, false, false, 0x00));
    CHECK(word_is(OPENING,    false, false, false, 0x01));
    CHECK(word_is(OPEN,       false, false, false, 0x02));
    CHECK(word_is(CLOSING,    false, false, false, 0x03));
    CHECK(word_is(OBSTRUCTED, false, false, false, 0x04));
    CHECK(word_is(CLOSED,     true,  false, false, 0x08));
    CHECK(word_is(CLOSED,     false, true,  false, 0x10));
    CHECK(word_is(CLOSED,     false, false, true,  0x20));
}

static void check_unused_bits_are_0(void) {
    /* Every field at its largest value, so nothing can spill past bit 5. */
    CHECK(word_is(OBSTRUCTED, true, true, true, 0x3C));
}

static void check_round_trip(void) {
    for (state_t s = 0; s < N_STATES; s++) {
        CHECK(round_trips(s, false, false, false));
        CHECK(round_trips(s, true,  false, false));
        CHECK(round_trips(s, false, true,  false));
        CHECK(round_trips(s, false, false, true));
        CHECK(round_trips(s, true,  true,  true));
    }
}

static void check_unpack_ignores_unused_bits(void) {
    status_t st = status_unpack(0xFFFFFFC1u);

    CHECK(st.state == OPENING);
    CHECK(!st.motor_up);
    CHECK(!st.motor_down);
    CHECK(!st.freeze_active);
}

static uint32_t word_of(state_t s, float temp_c) {
    status_t st = status_of(s, temp_c);

    return status_pack(&st);
}

static void check_status_of(void) {
    /* warm */
    CHECK(word_of(CLOSED,     15.0f) == 0x00);
    CHECK(word_of(OPENING,    15.0f) == 0x09);
    CHECK(word_of(OPEN,       15.0f) == 0x02);
    CHECK(word_of(CLOSING,    15.0f) == 0x13);
    CHECK(word_of(OBSTRUCTED, 15.0f) == 0x04);
    /* cold: the freeze bit is on only in CLOSED */
    CHECK(word_of(CLOSED,     1.0f) == 0x20);
    CHECK(word_of(OPENING,    1.0f) == 0x09);
    CHECK(word_of(OPEN,       1.0f) == 0x02);
    CHECK(word_of(CLOSING,    1.0f) == 0x13);
    CHECK(word_of(OBSTRUCTED, 1.0f) == 0x04);
    /* just above the guard */
    CHECK(word_of(CLOSED,     1.1f) == 0x00);
}

void run_status_tests(void) {
    check_pack_each_field();
    check_unused_bits_are_0();
    check_round_trip();
    check_unpack_ignores_unused_bits();
    check_status_of();
}
