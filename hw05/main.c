/*
 * main.c:
 *
 * Runs the combination lock on standard input: starts in LOCKED, turns each
 * character into an event, and prints one line per event, the state, the
 * character, the event, the next state and the action, e.g.
 * "GOT2 4 RIGHT -> OPEN UNLOCK". After the input, prints "final: STATE".
 *
 * Whitespace (newlines, '\r', spaces) is skipped silently. Any other character
 * that is not an event is skipped with a line on stderr, so a typo is noticed
 * but stdout keeps the format above.
 *
 * Usage: build/lock < data/codes.txt
 */
#include <ctype.h>
#include <stdio.h>

#include "lock.h"

/**
 * Runs the lock on stdin and prints its steps. Characters skipped gets sent to
 * stderr.
 *
 * @return 0.
 */
int main(void) {
    state_t s = LOCKED;
    int c;

    while ((c = getchar()) != EOF) {
        /* Line endings/spaces are layout, not typos. */
        if (isspace(c)) continue;

        int e = lock_event(s, c);
        if (e == -1) {
            if (isprint(c)) {
                fprintf(stderr, "lock: ignoring '%c'\n", c);
            } else {
                fprintf(stderr, "lock: ignoring byte 0x%02X\n", c);
            }
            continue;
        }

        action_t action;
        state_t next = lock_step(s, e, &action);

        printf("%s %c %s -> %s %s\n", state_name(s), c, event_name(e),
               state_name(next), action_name(action));
        s = next;
    }

    printf("final: %s\n", state_name(s));

    return 0;
}
