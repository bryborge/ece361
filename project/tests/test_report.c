/*
 * test_report.c:
 *
 * Tests for report.c. Prints PASS or FAIL for each check; test_main.c runs
 * them and prints the summary.
 */
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "check.h"
#include "report.h"
#include "store.h"

/* A window small enough to check the averages by hand. */
static const size_t window = 3;

/* The first record's timestamp: 2026-10-06 08:00:00 UTC. */
static const time_t start_time = 1791273600;

/*
 * The records' temperatures, in time order. They are not in temperature
 * order, so a report that skipped the sort would average the wrong records.
 */
static const float temps[] = {15.6f, 15.0f, 15.3f, 15.9f};

/**
 * Creates a store holding the first count records: record i is at temps[i]
 * °C, stamped start_time + i.
 *
 * @return The store, for the check to destroy.
 */
static store_t *store_with(size_t count) {
    assert(count <= sizeof temps / sizeof temps[0]);

    store_t *store = store_create();
    assert(store != NULL);

    for (size_t i = 0; i < count; i++) {
        const record_t record = {
            .reading = {
                .timestamp = start_time + (time_t)i,
                .temp_c = temps[i],
            },
        };
        const int rc = store_insert(store, &record);
        assert(rc == 0);
    }
    return store;
}

/**
 * Runs the report over store with the test window, into a temporary file, and
 * reads what it printed back into text as a string. Asserts that the
 * temporary file works and that everything printed fits in text.
 *
 * @return What report_print returned.
 */
static int report_into(const store_t *store, char *text, size_t size) {
    FILE *out = tmpfile();
    assert(out != NULL);

    const int rc = report_print(store, window, out);

    rewind(out);
    const size_t length = fread(text, 1, size - 1, out);
    const int next = fgetc(out);
    assert(!ferror(out));
    assert(next == EOF);
    text[length] = '\0';

    fclose(out);
    return rc;
}

/**
 * With fewer records than the window, the report prints nothing and succeeds.
 */
static void check_report_fewer_than_window_prints_nothing(void) {
    store_t *store = store_with(2);
    char text[256];

    CHECK(report_into(store, text, sizeof text) == 0);
    CHECK(strcmp(text, "") == 0);
    store_destroy(store);
}

/**
 * With exactly as many records as the window, the report prints one line:
 * the last record's timestamp and the average of all of them.
 */
static void check_report_exactly_window_prints_one_line(void) {
    store_t *store = store_with(3);
    char text[256];

    // (15.6 + 15.0 + 15.3) / 3 = 15.3
    CHECK(report_into(store, text, sizeof text) == 0);
    CHECK(strcmp(text, "2026-10-06 08:00:02 avg=15.3\n") == 0);
    store_destroy(store);
}

/**
 * One record past the window, the window slides by one: the second line drops
 * the oldest record and adds the newest.
 */
static void check_report_window_slides_by_one(void) {
    store_t *store = store_with(4);
    char text[256];

    // (15.6 + 15.0 + 15.3) / 3 = 15.3, then (15.0 + 15.3 + 15.9) / 3 = 15.4
    CHECK(report_into(store, text, sizeof text) == 0);
    CHECK(strcmp(text,
                 "2026-10-06 08:00:02 avg=15.3\n"
                 "2026-10-06 08:00:03 avg=15.4\n") == 0);
    store_destroy(store);
}

void run_report_tests(void) {
    check_report_fewer_than_window_prints_nothing();
    check_report_exactly_window_prints_one_line();
    check_report_window_slides_by_one();
}
