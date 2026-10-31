/*
 * check.h:
 *
 * A checker for a transition table. It finds the mistakes a table can hold
 * before the table is ever run.
 */
#ifndef CHECK_H
#define CHECK_H

#include <stdio.h>

#include "lock.h"

/**
 * Given a transition table, a start state and an output stream, writes one
 * line to `out` per problem in `table` and returns how many it found. The
 * problems are:
 *   - an entry left out (`A_UNDEFINED`),
 *   - a next state out of range,
 *   - a state that cannot be reached from `start`, and
 *   - a state with no way out.
 * Entries that are left out or out of range are not followed when looking
 * for reachable states or ways out. Assumes `start` is in range and `out` is
 * not `NULL`.
 */
int fsm_check(const transition_t table[N_STATES][N_EVENTS], state_t start,
              FILE *out);

#endif
