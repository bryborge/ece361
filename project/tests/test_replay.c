/*
 * test_replay.c:
 *
 * Scenario tests: runs build/garage on each scenario through system(), and
 * checks that it succeeds and prints exactly the scenario's expected output.
 * Runs from project/, as make test does. Prints PASS or FAIL for each check;
 * test_main.c runs them and prints the summary.
 */
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "check.h"

#define OUT "build/replay.out"

/**
 * Compares two open files, byte by byte.
 *
 * @return true if they hold the same bytes.
 */
static bool same_bytes(FILE *a, FILE *b) {
    int ca;
    int cb;

    do {
        ca = fgetc(a);
        cb = fgetc(b);
        if (ca != cb) return false;
    } while (ca != EOF);
    return true;
}

static bool files_match(const char *path_a, const char *path_b) {
    FILE *a = fopen(path_a, "r");
    FILE *b = fopen(path_b, "r");
    bool same = a != NULL && b != NULL && same_bytes(a, b);

    if (a != NULL) fclose(a);
    if (b != NULL) fclose(b);
    return same;
}

/**
 * Replays scenarios/<name>.txt, and checks that it succeeds and prints exactly
 * scenarios/<name>.expected.
 */
static bool replays_as_expected(const char *name) {
    char command[128];
    char expected[128];
    int n = snprintf(command, sizeof command,
                     "build/garage scenarios/%s.txt > " OUT, name);

    assert(n > 0 && (size_t)n < sizeof command);
    snprintf(expected, sizeof expected, "scenarios/%s.expected", name);
    return system(command) == 0 && files_match(OUT, expected);
}

void run_replay_tests(void) {
    CHECK(replays_as_expected("1_normal"));
    CHECK(replays_as_expected("2_growth"));
    CHECK(replays_as_expected("3_thresholds"));
    CHECK(replays_as_expected("4_freeze_guard"));
    CHECK(replays_as_expected("5_unexpected_input"));
    CHECK(replays_as_expected("7_midnight"));
}
