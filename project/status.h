/*
 * status.h:
 *
 * The status word: one uint32_t that records the door's state and outputs at a
 * moment. SPEC.md gives the bit layout. Unused bits are 0.
 */
#ifndef STATUS_H
#define STATUS_H

#include <stdbool.h>
#include <stdint.h>

#include "fsm.h"

typedef struct {
    state_t state;
    bool motor_up;
    bool motor_down;
    bool freeze_active;
} status_t;

/**
 * Given a state and a temperature, returns the status they make. The motor bits
 * come from fsm_leds and the freeze bit from fsm_freeze_active, so the status
 * can't disagree with the LEDs or the rules. Assumes `s` is in range.
 */
status_t status_of(state_t s, float temp_c);

/**
 * Given a status, returns the word with every field packed in. Assumes `st` is
 * not `NULL`.
 */
uint32_t status_pack(const status_t *st);

/**
 * Given a word, returns the status packed in it. Unused bits are ignored.
 */
status_t status_unpack(uint32_t word);

#endif
