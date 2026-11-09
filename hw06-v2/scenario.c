/*
 * scenario.c:
 *
 * Parses a scenario file, one line at a time.
 */
#include <assert.h>
#include <inttypes.h>
#include <stdbool.h>
#include <string.h>

#include "scenario.h"

/* The last second of the year 9999. Adding any int tick still gives a time that
   gmtime can split, so start + tick can't overflow or crash the trace. */
#define START_MAX 253402300799LL

/**
 * Translates and checks each line as it reads it. sscanf's match count says
 * whether a line is well formed; seen_start and the last stored tick carry what
 * the checks need from one line to the next.
 */
int scenario_load(FILE *fp, scenario_t *s) {
    char line[SCENARIO_MAX_LINE_LEN + 3];   /* the line, a CRLF and the '\0' */
    int line_number = 0;
    bool seen_start = false;

    s->count = 0;

    while (fgets(line, sizeof line, fp) != NULL) {
        line_number++;

        /* Is this line too long? If so, return. */
        if (strcspn(line, "\r\n") > SCENARIO_MAX_LINE_LEN) return line_number;

        /* Is this a comment? If so, drop it. */
        char *hash = strchr(line, '#');
        if (hash != NULL) *hash = '\0';

        /* Is this a blank line? If so, skip it. */
        char word[2];
        if (sscanf(line, "%1s", word) != 1) continue;

        /* Is this the start line? If so, mark it and proceed. */
        long long start;
        if (sscanf(line, " start %lld %1s", &start, word) == 1) {
            if (start < 0 || start > START_MAX) return line_number;
            s->start = start;
            seen_start = true;
            continue;
        }

        /* Is this data before the start line? If so, return. */
        if (!seen_start) return line_number;

        /* Have we reached the max number of lines for this scenario? Return. */
        if (s->count == SCENARIO_MAX_LINES) return line_number;

        /* Is this a data line, with exactly four fields? If not, return. */
        scenario_line_t *data = &s->lines[s->count];
        if (sscanf(line, "%d %f %f %" SCNx32 " %1s", &data->tick, &data->temp_c,
                   &data->rh_pct, &data->switches, word) != 4) {
            return line_number;
        }

        /* Is this the first data line, but not at tick 0? If so, return: the
           ticks before it would have no values. */
        if (s->count == 0 && data->tick != 0) return line_number;

        /* Does this tick fail to increase on the last line? If so, return. */
        if (s->count > 0 && data->tick <= s->lines[s->count - 1].tick) {
            return line_number;
        }

        s->count++;
    }

    /* No data at all, or did fgets stop on a read error instead of the end of
       the file? There is no wrong line, so point past the last one. */
    if (s->count == 0 || ferror(fp)) return line_number + 1;

    return 0;
}

/**
 * Searches back from the last line. The first line is at tick 0, so a tick that
 * is not negative always stops the search.
 */
const scenario_line_t *scenario_at(const scenario_t *s, int tick) {
    assert(s->count > 0);
    assert(tick >= 0);

    int i = s->count - 1;

    while (s->lines[i].tick > tick) i--;

    return &s->lines[i];
}
