#include <assert.h>
#include <stdbool.h>
#include <string.h>

#include "scenario.h"

/* The last second of the year 9999. Adding any int tick still gives a time that
 * fits a time_t and that gmtime can split, so start + tick cannot overflow. */
#define START_MAX 253402300799LL

static bool is_blank(const char *line) {
    char word[2];

    return sscanf(line, "%1s", word) == EOF;
}

/**
 * Reads a line of exactly "start" and a timestamp. The count is the check: %1s
 * tries to read one word more, so anything extra gives 3. A literal "start" in
 * the format would accept "start5", so the word is read and compared instead.
 *
 * @return true if the line is good.
 */
static bool parse_start(const char *line, time_t *start) {
    char word[7];
    long long timestamp;
    char extra[2];

    if (sscanf(line, "%6s %lld %1s", word, &timestamp, extra) != 2) return false;
    if (strcmp(word, "start") != 0) return false;
    if (timestamp < 0 || timestamp > START_MAX) return false;

    *start = (time_t)timestamp;
    return true;
}

/**
 * Reads a line of exactly tick, temperature, humidity and switches. iom361 would
 * clamp a value outside the sensor's range without a word, which would hide a
 * typo in the file. Written this way round, the checks also reject nan.
 *
 * @return true if the line is good.
 */
static bool parse_data(const char *line, scenario_line_t *data) {
    unsigned switches;
    char extra[2];

    if (sscanf(line, "%d %f %f %x %1s", &data->tick, &data->temp_c, &data->rh_pct,
               &switches, extra) != 4) return false;
    if (!(data->temp_c >= -50.0f && data->temp_c <= 150.0f)) return false;
    if (!(data->rh_pct >= 0.0f && data->rh_pct <= 100.0f)) return false;

    data->switches = switches;
    return true;
}

/**
 * @return true if `tick` can be the next line's tick. Nothing is in force before
 *         the first line, so it has to be 0, or the replay's first ticks would
 *         have no values. This also rejects negative ticks.
 */
static bool is_next_tick(const scenario_t *s, int tick) {
    if (s->count == 0) return tick == 0;
    return tick > s->lines[s->count - 1].tick;
}

int scenario_load(FILE *fp, scenario_t *s) {
    /* Room for the longest line, a CRLF and the '\0'. */
    char line[SCENARIO_MAX_LINE_LEN + 3];
    int line_number = 0;
    bool seen_start = false;

    s->count = 0;
    while (fgets(line, sizeof line, fp) != NULL) {
        line_number++;
        if (strcspn(line, "\r\n") > SCENARIO_MAX_LINE_LEN) return line_number;

        /* A '\0' at the # ends the string, so sscanf never sees the comment. */
        char *hash = strchr(line, '#');
        if (hash != NULL) *hash = '\0';
        if (is_blank(line)) continue;

        if (!seen_start) {
            if (!parse_start(line, &s->start)) return line_number;
            seen_start = true;
            continue;
        }

        scenario_line_t data;
        if (!parse_data(line, &data)) return line_number;
        if (!is_next_tick(s, data.tick)) return line_number;
        if (s->count == SCENARIO_MAX_LINES) return line_number;
        s->lines[s->count++] = data;
    }

    /* fgets also stops on a read error, which must not pass for the end of a
     * good file. There is no wrong line to name, so it is treated like missing
     * data. */
    if (s->count == 0 || ferror(fp)) return line_number + 1;
    return 0;
}

const scenario_line_t *scenario_at(const scenario_t *s, int tick) {
    assert(tick >= 0);
    assert(s->count > 0);

    /* A linear scan, not a binary search: at most 1000 comparisons per tick, and
     * no measurement or requirement calls for less. */
    const scenario_line_t *found = &s->lines[0];
    for (int i = 1; i < s->count && s->lines[i].tick <= tick; i++) {
        found = &s->lines[i];
    }
    return found;
}
