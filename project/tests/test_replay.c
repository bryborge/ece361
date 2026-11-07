/*
 * test_replay.c:
 *
 * Tests the program as it ships: runs build/garage through system(), then
 * compares what it printed and checks whether it reported failure. Runs from
 * project/, as make test does. Prints PASS or FAIL for each check; test_main.c
 * runs them and prints the summary.
 */
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "check.h"

#define OUT "build/replay.out"
#define ERR "build/replay.err"

/**
 * Runs build/garage with `args`, sending stdout to OUT and stderr to ERR.
 *
 * @return what system returns: 0 only if the program exited with status 0.
 */
static int run_garage(const char *args) {
    char command[256];
    int n = snprintf(command, sizeof command, "build/garage %s > %s 2> %s", args,
                     OUT, ERR);

    assert(n > 0 && (size_t)n < sizeof command);
    return system(command);
}

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

static bool file_holds(const char *path, const char *text) {
    FILE *f = fopen(path, "r");

    if (f == NULL) return false;

    size_t i = 0;
    int c;
    bool same = true;

    while (same && (c = fgetc(f)) != EOF) {
        same = text[i] != '\0' && c == (unsigned char)text[i];
        i++;
    }
    same = same && text[i] == '\0';
    fclose(f);
    return same;
}

static int count_lines(const char *path) {
    FILE *f = fopen(path, "r");

    assert(f != NULL);

    int lines = 0;
    int c;

    while ((c = fgetc(f)) != EOF) {
        if (c == '\n') lines++;
    }
    fclose(f);
    return lines;
}

/* Writes a scenario for a test that needs a file no scenario in scenarios/ is. */
static void write_file(const char *path, const char *text) {
    FILE *f = fopen(path, "w");

    assert(f != NULL);
    fputs(text, f);
    fclose(f);
}

/**
 * Replays scenarios/<name>.txt, and checks that it succeeds, prints nothing on
 * stderr, and prints exactly scenarios/<name>.expected.
 */
static bool replays_as_expected(const char *name) {
    char args[128];
    char expected[128];

    snprintf(args, sizeof args, "scenarios/%s.txt", name);
    snprintf(expected, sizeof expected, "scenarios/%s.expected", name);
    return run_garage(args) == 0 && file_holds(ERR, "")
        && files_match(OUT, expected);
}

/**
 * Runs garage with `args`, and checks that it reports failure, prints `message`
 * on stderr, and prints no trace.
 */
static bool fails_with(const char *args, const char *message) {
    return run_garage(args) != 0 && file_holds(ERR, message) && file_holds(OUT, "");
}

static void check_scenarios(void) {
    CHECK(replays_as_expected("1_normal"));
    CHECK(replays_as_expected("3_thresholds"));
    CHECK(replays_as_expected("4_freeze_guard"));
    CHECK(replays_as_expected("5_unexpected"));
    CHECK(replays_as_expected("7_midnight"));
}

static void check_trace_ignores_time_zone(void) {
    /* A POSIX time zone string, so this needs no time zone files. */
    int rc = system("TZ=JST-9 build/garage scenarios/7_midnight.txt > " OUT);

    CHECK(rc == 0);
    CHECK(files_match(OUT, "scenarios/7_midnight.expected"));
}

static void check_usage(void) {
    const char *usage = "usage: garage scenario.txt\n";

    CHECK(fails_with("", usage));
    CHECK(fails_with("a b", usage));
    /* Until week 9: see "Where we are" in CLAUDE.md. */
    CHECK(fails_with("scenarios/1_normal.txt --report", usage));
}

static void check_file_will_not_open(void) {
    CHECK(fails_with("scenarios/no_such_file.txt",
                     "garage: scenarios/no_such_file.txt: "
                     "No such file or directory\n"));
}

static void check_bad_scenario(void) {
    const char *path = "build/replay_bad.txt";

    write_file(path, "start 0\n# a comment\n0 15 60\n");
    CHECK(fails_with(path, "garage: build/replay_bad.txt: line 3: "
                           "invalid scenario line\n"));
    write_file(path, "");
    CHECK(fails_with(path, "garage: build/replay_bad.txt: line 1: "
                           "invalid scenario line\n"));
}

static void check_store_full(void) {
    const char *path = "build/replay_long.txt";

    write_file(path, "start 0\n0 15 60 0x0\n1100 15 60 0x0\n");
    CHECK(run_garage(path) != 0);
    /* The store holds 1024 records, ticks 0 to 1023. The trace up to there stays. */
    CHECK(file_holds(ERR, "garage: tick 1024: store full\n"));
    CHECK(count_lines(OUT) == 1024);
}

void run_replay_tests(void) {
    check_scenarios();
    check_trace_ignores_time_zone();
    check_usage();
    check_file_will_not_open();
    check_bad_scenario();
    check_store_full();
}
