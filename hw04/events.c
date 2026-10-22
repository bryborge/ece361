/*
 * events.c:
 *
 * Counts the events in a log: reads standard input line by line, adds every
 * line to one tally, then prints the tally and one last line,
 * "names: N, lines skipped: S", where S counts the lines tally_add refused.
 *
 * Blank lines and lines that start with '#' are ignored and not counted. A
 * line too long for the buffer is read to its end and counts as one line,
 * which tally_add refuses.
 *
 * Usage: build/events < data/door.txt
 */
#include <stdio.h>
#include <string.h>

#include "tally.h"

/**
 * Reads the event log from stdin, tallies it, and prints the result as
 * described at the top of this file.
 *
 * @return 0 on success; 1 if no tally was available, after printing a message
 *         to stderr.
 */
int main(void) {
    char line[64];  /* must hold more than TALLY_LEN chars: see below */
    int skipped = 0;

    tally_t *t = tally_create();
    if (t == NULL) {
        fprintf(stderr, "error: no tally available\n");
        return 1;
    }

    while (fgets(line, sizeof line, stdin) != NULL) {
        size_t len = strlen(line);

        if (len > 0 && line[len - 1] == '\n') {
            line[--len] = '\0';  /* "OPEN\n" becomes "OPEN" */
        } else if (len == sizeof line - 1) {
            /* The line did not fit: read past the rest of it, so the next
             * fgets starts on the next line. The piece kept in line is
             * longer than TALLY_LEN, so tally_add refuses it below. */
            int c;
            while ((c = getchar()) != '\n' && c != EOF) {}
        }

        if (len == 0 || line[0] == '#') continue;
        if (tally_add(t, line) == -1) skipped++;
    }

    tally_print(t, stdout);
    printf("names: %d, lines skipped: %d\n", tally_size(t), skipped);

    tally_destroy(t);

    return 0;
}
