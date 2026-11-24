/*
 * test_store.c:
 *
 * Tests for store.c. Prints PASS or FAIL for each check; test_main.c runs
 * them and prints the summary. Leaks are not checked here: the tests are
 * built with AddressSanitizer, whose LeakSanitizer reports them at exit.
 */
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <time.h>

#include "check.h"
#include "store.h"

/**
 * Starts each check with a new, empty store.
 *
 * @return The store, for the check to destroy.
 */
static store_t *start(void) {
    store_t *store = store_create();

    assert(store != NULL);
    return store;
}

/**
 * Inserts record i for each i from first up to, but not including, last.
 * Record i is at 15.0 + (i % 10) * 0.5 °C: 15.0, 15.5, ..., 19.5 °C, then
 * around again.
 *
 * @return true if every insert succeeded.
 */
static bool insert_cycle(store_t *store, int first, int last) {
    bool all_inserted = true;

    for (int i = first; i < last; i++) {
        const record_t record = {
            .reading = { .temp_c = 15.0f + (float)(i % 10) * 0.5f },
        };
        all_inserted = store_insert(store, &record) == 0 && all_inserted;
    }
    return all_inserted;
}

/**
 * Finds the records in the step that holds temp_c.
 *
 * @return true if there are 3 of them, with timestamps first, first + 3, and
 *         first + 6, in that order.
 */
static bool finds_three_in_order(const store_t *store, float temp_c, time_t first) {
    const record_t key = { .reading = { .temp_c = temp_c } };
    const record_t *records;
    size_t count;

    if (store_find(store, &key, &records, &count) != 0) return false;
    if (count != 3) return false;
    for (size_t j = 0; j < 3; j++) {
        if (records[j].reading.timestamp != first + 3 * (time_t)j) return false;
    }
    return true;
}

/**
 * Two temperatures in the same 0.5 °C step have the same key.
 */
static void check_record_compare_same_step_is_equal(void) {
    const record_t a = { .reading = { .temp_c = 21.2f } };
    const record_t b = { .reading = { .temp_c = 21.4f } };

    CHECK(record_compare(&a, &b) == 0);
    CHECK(record_compare(&b, &a) == 0);
}

/**
 * Two temperatures in neighboring steps are ordered by step, even when they
 * are only 0.1 °C apart.
 */
static void check_record_compare_neighboring_steps_are_ordered(void) {
    const record_t lower = { .reading = { .temp_c = 21.4f } };
    const record_t upper = { .reading = { .temp_c = 21.5f } };

    CHECK(record_compare(&lower, &upper) < 0);
    CHECK(record_compare(&upper, &lower) > 0);
}

/**
 * A temperature exactly on a step boundary starts the step above it. The
 * boundary at 0.0 °C also checks that negative temperatures round down:
 * -0.1 °C is in the step from -0.5 °C, not the step from 0.0 °C.
 */
static void check_record_compare_boundary_starts_the_step_above(void) {
    const record_t below = { .reading = { .temp_c = -0.1f } };
    const record_t boundary = { .reading = { .temp_c = 0.0f } };
    const record_t inside = { .reading = { .temp_c = 0.4f } };

    CHECK(record_compare(&below, &boundary) < 0);
    CHECK(record_compare(&boundary, &inside) == 0);
}

/**
 * A new store holds no records and has a height of 0.
 */
static void check_store_create_starts_empty(void) {
    store_t *store = start();

    CHECK(store_count(store) == 0);
    CHECK(store_height(store) == 0);
    store_destroy(store);
}

/**
 * Destroying an empty store frees it. LeakSanitizer fails the run at exit if
 * anything is left.
 */
static void check_store_destroy_empty_store(void) {
    store_destroy(start());
}

/**
 * Inserting into an empty store adds one record.
 */
static void check_store_insert_into_empty_store(void) {
    store_t *store = start();
    const record_t record = { .reading = { .temp_c = 21.2f } };

    CHECK(store_insert(store, &record) == 0);
    CHECK(store_count(store) == 1);
    store_destroy(store);
}

/**
 * The count includes every record, whether its key is new or shared, while
 * the height counts only nodes, one per key. The 100 records span 10 keys, 10
 * records each, so every node's array grows past its starting room. The first
 * 10 arrive in increasing order, so the 10 nodes form a chain: height 10.
 */
static void check_store_count_and_height_after_1_2_and_100_inserts(void) {
    store_t *store = start();

    CHECK(insert_cycle(store, 0, 1));
    CHECK(store_count(store) == 1);
    CHECK(store_height(store) == 1);

    CHECK(insert_cycle(store, 1, 2));
    CHECK(store_count(store) == 2);
    CHECK(store_height(store) == 2);

    CHECK(insert_cycle(store, 2, 100));
    CHECK(store_count(store) == 100);
    CHECK(store_height(store) == 10);
    store_destroy(store);
}

/**
 * A long run at one temperature, the usual case in a replay, fills one node
 * and leaves the tree's height at 1.
 */
static void check_store_one_temperature_stays_one_node_tall(void) {
    store_t *store = start();
    const record_t record = { .reading = { .temp_c = 15.0f } };
    bool all_inserted = true;

    for (int i = 0; i < 100; i++) {
        all_inserted = store_insert(store, &record) == 0 && all_inserted;
    }

    CHECK(all_inserted);
    CHECK(store_count(store) == 100);
    CHECK(store_height(store) == 1);
    store_destroy(store);
}

/**
 * Every record with a key is found, in the order it was inserted. The 9
 * records fall in 3 steps, 3 records each, inserted in turn: 20.0, 21.0,
 * 22.0, then 20.2, 21.2, 22.2, then 20.4, 21.4, 22.4 °C. Each record's
 * timestamp is its place in that order, 0 to 8.
 */
static void check_store_find_gets_every_duplicate_in_order(void) {
    store_t *store = start();
    const float steps[] = { 20.0f, 21.0f, 22.0f };
    bool all_inserted = true;

    for (int i = 0; i < 9; i++) {
        const record_t record = {
            .reading = {
                .timestamp = i,
                .temp_c = steps[i % 3] + (float)(i / 3) * 0.2f,
            },
        };
        all_inserted = store_insert(store, &record) == 0 && all_inserted;
    }
    CHECK(all_inserted);

    /* Any temperature in the step finds its records. */
    CHECK(finds_three_in_order(store, 20.3f, 0));
    CHECK(finds_three_in_order(store, 21.3f, 1));
    CHECK(finds_three_in_order(store, 22.3f, 2));
    store_destroy(store);
}

/**
 * Finding a key that is not there fails and leaves the out-parameters as
 * they were: in an empty store, and on either side of a store's only key.
 */
static void check_store_find_missing_key_fails(void) {
    store_t *store = start();
    const record_t below = { .reading = { .temp_c = 19.0f } };
    const record_t stored = { .reading = { .temp_c = 21.0f } };
    const record_t above = { .reading = { .temp_c = 23.0f } };

    const record_t untouched = { .reading = { .temp_c = -1.0f } };
    const record_t *records = &untouched;
    size_t count = 42;

    CHECK(store_find(store, &stored, &records, &count) == 1);

    CHECK(store_insert(store, &stored) == 0);
    CHECK(store_find(store, &below, &records, &count) == 1);
    CHECK(store_find(store, &above, &records, &count) == 1);

    CHECK(records == &untouched);
    CHECK(count == 42);
    store_destroy(store);
}

/* The records a walk visited, in the order it visited them. */
typedef struct {
    record_t records[16];
    size_t count;
} visited_t;

/*
 * A store_walk callback that copies each record into a visited_t. Records
 * past the 16th are counted but not copied, so the count shows the overflow.
 */
static void collect(const record_t *record, void *context) {
    visited_t *visited = context;

    if (visited->count < 16) visited->records[visited->count] = *record;
    visited->count++;
}

/**
 * The walk visits records in key order, and records with the same key in the
 * order they were inserted. The records go in out of order; each record's
 * timestamp is its place in the insertion order.
 */
static void check_store_walk_visits_records_in_key_order(void) {
    store_t *store = start();
    const float temps[] = { 22.0f, 20.0f, 21.0f, 20.2f, 23.0f, 21.4f, 20.4f };
    bool all_inserted = true;

    for (int i = 0; i < 7; i++) {
        const record_t record = {
            .reading = { .timestamp = i, .temp_c = temps[i] },
        };
        all_inserted = store_insert(store, &record) == 0 && all_inserted;
    }
    CHECK(all_inserted);

    visited_t visited = { .count = 0 };
    store_walk(store, collect, &visited);

    /* 20.0, 20.2, 20.4, then 21.0, 21.4, then 22.0, then 23.0 °C. */
    const time_t expected[] = { 1, 3, 6, 2, 5, 0, 4 };
    bool in_key_order = visited.count == 7;

    for (size_t i = 0; in_key_order && i < 7; i++) {
        in_key_order = visited.records[i].reading.timestamp == expected[i];
    }

    CHECK(visited.count == 7);
    CHECK(in_key_order);
    store_destroy(store);
}

void run_store_tests(void) {
    check_record_compare_same_step_is_equal();
    check_record_compare_neighboring_steps_are_ordered();
    check_record_compare_boundary_starts_the_step_above();
    check_store_create_starts_empty();
    check_store_destroy_empty_store();
    check_store_insert_into_empty_store();
    check_store_count_and_height_after_1_2_and_100_inserts();
    check_store_one_temperature_stays_one_node_tall();
    check_store_find_gets_every_duplicate_in_order();
    check_store_find_missing_key_fails();
    check_store_walk_visits_records_in_key_order();
}
