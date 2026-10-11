// fmemopen is POSIX, hidden by -std=c11 without this.
#define _POSIX_C_SOURCE 200809L

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "../reading.h"

static int total;
static int fails;

#define CHECK(cond) do {                                               \
    total++;                                                           \
    if (cond) printf("(PASS) Test: %s\n", #cond);                      \
    else {                                                             \
        printf("(FAIL) Test: %s (line %d)\n", #cond, __LINE__);        \
        fails++;                                                       \
    }                                                                  \
} while (0)

// What the last read_from() call printed to stderr.
static char err[128];

/**
 * Runs reading_read(ticks, temps, hums, max) with stdin read from `input` and
 * stderr captured in `err`.
 *
 * @param input Text to read, as it would appear on stdin.
 * @param ticks Array for the ticks (must hold at least max elements).
 * @param temps Array for the temperatures (must hold at least max elements).
 * @param hums  Array for the humidities (must hold at least max elements).
 * @param max   Number of readings the arrays can hold.
 * @return What reading_read() returned, or {-1, -1} if a stream could not be
 *         opened.
 */
static struct reading_counts read_from(char *input, int ticks[],
                                       float temps[], float hums[], int max) {
    struct reading_counts counts = {-1, -1};

    // glibc's fmemopen only NUL-terminates after a write, so when nothing is
    // printed to stderr, err would otherwise keep the previous call's output.
    memset(err, 0, sizeof err);
    FILE *in = fmemopen(input, strlen(input), "r");
    FILE *errs = fmemopen(err, sizeof err, "w");

    if (in != NULL && errs != NULL) {
        FILE *saved_stdin = stdin;
        FILE *saved_stderr = stderr;
        stdin = in;
        stderr = errs;
        counts = reading_read(ticks, temps, hums, max);
        fflush(errs);
        stdin = saved_stdin;
        stderr = saved_stderr;
    }

    if (in != NULL) fclose(in);
    if (errs != NULL) fclose(errs);
    return counts;
}

static bool counts_are(struct reading_counts c, int count, int skipped) {
    return c.count == count && c.skipped == skipped;
}

static void check_empty_input(void) {
    int ticks[1];
    float temps[1], hums[1];
    CHECK(counts_are(read_from("", ticks, temps, hums, 1), 0, 0));
    CHECK(strcmp(err, "") == 0);
}

static void check_one_reading(void) {
    int ticks[1];
    float temps[1], hums[1];
    CHECK(counts_are(read_from("12  23.5  41.0\n", ticks, temps, hums, 1),
                     1, 0));
    CHECK(ticks[0] == 12);
    CHECK(temps[0] == 23.5f);
    CHECK(hums[0] == 41.0f);
}

static void check_last_line_without_newline(void) {
    int ticks[2];
    float temps[2], hums[2];
    CHECK(counts_are(read_from("1 20 40\n2 21 41", ticks, temps, hums, 2),
                     2, 0));
    CHECK(ticks[1] == 2);
}

static void check_ignores_blank_and_comment_lines(void) {
    int ticks[1];
    float temps[1], hums[1];
    CHECK(counts_are(read_from("\n   \n\t\n# header\n  # indented\n1 20 40\n",
                               ticks, temps, hums, 1),
                     1, 0));
    CHECK(ticks[0] == 1);
}

static void check_skips_malformed_lines(void) {
    int ticks[1];
    float temps[1], hums[1];
    CHECK(counts_are(read_from("1 20\nabc\n2 21 41\n", ticks, temps, hums, 1),
                     1, 2));
    // The reading after the skipped lines is stored in the first slot.
    CHECK(ticks[0] == 2);
    CHECK(temps[0] == 21.0f);
    CHECK(hums[0] == 41.0f);
}

static void check_stops_at_max(void) {
    int ticks[2];
    float temps[2], hums[2];
    CHECK(counts_are(read_from("1 20 40\n2 21 41\n3 22 42\n",
                               ticks, temps, hums, 2),
                     2, 0));
    CHECK(strcmp(err, "warning: more than 2 readings, the rest are ignored\n")
          == 0);
}

static void check_no_warning_at_exactly_max(void) {
    int ticks[2];
    float temps[2], hums[2];
    CHECK(counts_are(read_from("1 20 40\n2 21 41\n\n# done\n",
                               ticks, temps, hums, 2),
                     2, 0));
    CHECK(strcmp(err, "") == 0);
}

static void check_max_zero(void) {
    int ticks[1];
    float temps[1], hums[1];
    CHECK(counts_are(read_from("1 20 40\n", ticks, temps, hums, 0), 0, 0));
    CHECK(strcmp(err, "warning: more than 0 readings, the rest are ignored\n")
          == 0);
}

static void print_summary(void) {
    printf("\nSummary:\n");
    printf("--------\n");
    printf("%d total\n", total);
    printf("%d failed\n", fails);
    printf("\n");
}

int main(void) {
    check_empty_input();
    check_one_reading();
    check_last_line_without_newline();
    check_ignores_blank_and_comment_lines();
    check_skips_malformed_lines();
    check_stops_at_max();
    check_no_warning_at_exactly_max();
    check_max_zero();

    print_summary();

    return fails != 0;
}
