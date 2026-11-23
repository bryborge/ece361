/*
 * status.c:
 *
 * Packs and unpacks the status word with hw01's get_field and set_field.
 */
#include <assert.h>
#include <stddef.h>

#include "bits.h"
#include "status.h"

/* The bit layout (SPEC.md, Status word bit layout). */
#define STATE_POS   0
#define STATE_WIDTH 3
#define UP_POS      3
#define DOWN_POS    4
#define FREEZE_POS  5
#define FLAG_WIDTH  1

/**
 * Gives the status a state and a temperature make.
 *
 * @param s      The state; asserted to be in range by fsm_leds.
 * @param temp_c The temperature, rounded to 0.1 °C.
 * @return The status.
 */
status_t status_of(state_t s, float temp_c) {
    uint32_t leds = fsm_leds(s);
    status_t st = {
        .state = s,
        .motor_up = (leds & LED_MOTOR_UP) != 0,
        .motor_down = (leds & LED_MOTOR_DOWN) != 0,
        .freeze_active = fsm_freeze_active(s, temp_c),
    };

    return st;
}

/**
 * Packs a status into a word. Starting from 0 is what keeps the unused bits 0.
 *
 * @param st The status; asserted not NULL.
 * @return The word.
 */
uint32_t status_pack(const status_t *st) {
    assert(st != NULL);

    uint32_t word = 0;

    word = set_field(word, STATE_POS, STATE_WIDTH, (uint32_t)st->state);
    word = set_field(word, UP_POS, FLAG_WIDTH, st->motor_up);
    word = set_field(word, DOWN_POS, FLAG_WIDTH, st->motor_down);
    word = set_field(word, FREEZE_POS, FLAG_WIDTH, st->freeze_active);
    return word;
}

/**
 * Unpacks a word into a status.
 *
 * @param word The word; its unused bits are ignored.
 * @return The status.
 */
status_t status_unpack(uint32_t word) {
    status_t st = {
        .state = (state_t)get_field(word, STATE_POS, STATE_WIDTH),
        .motor_up = get_field(word, UP_POS, FLAG_WIDTH),
        .motor_down = get_field(word, DOWN_POS, FLAG_WIDTH),
        .freeze_active = get_field(word, FREEZE_POS, FLAG_WIDTH),
    };

    return st;
}
