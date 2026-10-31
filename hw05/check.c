/*
 * check.c:
 *
 * Checks a transition table for entries left out, next states out of range,
 * states that cannot be reached, and states with no way out.
 */
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

#include "check.h"
#include "lock.h"

/**
 * Tells whether an entry is a transition the lock could really take.
 *
 * @param t The entry to look at.
 * @return true if `t` has an action and a next state in range.
 */
static bool is_transition(transition_t t) {
    return t.action != A_UNDEFINED && t.next >= 0 && t.next < N_STATES;
}

/**
 * Marks every state reachable from `start`, searching breadth first.
 *
 * @param table   The table to search.
 * @param start   The state to search from.
 * @param reached Set to true for each state that can be reached, `start`
 *                included; left alone for the rest.
 */
static void find_reachable(const transition_t table[N_STATES][N_EVENTS],
                           state_t start, bool reached[N_STATES]) {
    state_t queue[N_STATES];  /* each state is queued (once at most) */
    int head = 0;
    int tail = 0;

    reached[start] = true;
    queue[tail++] = start;

    while (head < tail) {
        state_t s = queue[head++];

        for (event_t e = 0; e < N_EVENTS; e++) {
            transition_t t = table[s][e];

            if (is_transition(t) && !reached[t.next]) {
                reached[t.next] = true;
                queue[tail++] = t.next;
            }
        }
    }
}

/**
 * Checks `table` and reports each problem on its own line: for each state in
 * order, its entries left out or out of range and then whether it has a way
 * out; after that, the states that cannot be reached.
 *
 * @param table The table to check.
 * @param start The state the machine starts in; asserted to be in range.
 * @param out   Where to write the problems; asserted not NULL.
 * @return How many problems were found; 0 for a sound table.
 */
int fsm_check(const transition_t table[N_STATES][N_EVENTS], state_t start,
              FILE *out) {
    assert(start >= 0 && start < N_STATES);
    assert(out != NULL);

    int problems = 0;

    for (state_t s = 0; s < N_STATES; s++) {
        bool way_out = false;

        for (event_t e = 0; e < N_EVENTS; e++) {
            transition_t t = table[s][e];

            if (is_transition(t) && t.next != s) way_out = true;

            if (t.action == A_UNDEFINED) {
                fprintf(out, "%s %s: entry left out (A_UNDEFINED)\n",
                        state_name(s), event_name(e));
                problems++;
            } else if (t.next < 0 || t.next >= N_STATES) {
                fprintf(out, "%s %s: next state out of range\n",
                        state_name(s), event_name(e));
                problems++;
            }
        }

        if (!way_out) {
            fprintf(out, "%s: no way out\n", state_name(s));
            problems++;
        }
    }

    bool reached[N_STATES] = { false };
    find_reachable(table, start, reached);

    for (state_t s = 0; s < N_STATES; s++) {
        if (!reached[s]) {
            fprintf(out, "%s: not reachable from %s\n", state_name(s),
                    state_name(start));
            problems++;
        }
    }

    return problems;
}
