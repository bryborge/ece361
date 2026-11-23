/**
 * @file test_store.c
 * @brief Unit tests for the record store module.
 */
#include "unity.h"

#include "store.h"

/*
 * Each test gets a new, empty store. tearDown destroys it, and Unity runs
 * tearDown even after a failed assertion, so a failing test does not leak.
 */
static store_t *store;

void setUp(void) {
    store = store_create();
    TEST_ASSERT_NOT_NULL(store);
}

void tearDown(void) {
    store_destroy(store);
    store = NULL;
}

/**
 * Two temperatures in the same 0.5 °C step have the same key.
 */
static void test_record_compare_same_step_is_equal(void) {
    const record_t a = { .reading = { .temp_c = 21.2f } };
    const record_t b = { .reading = { .temp_c = 21.4f } };

    TEST_ASSERT_EQUAL_INT(0, record_compare(&a, &b));
    TEST_ASSERT_EQUAL_INT(0, record_compare(&b, &a));
}

/**
 * Two temperatures in neighboring steps are ordered by step, even when they
 * are only 0.1 °C apart.
 */
static void test_record_compare_neighboring_steps_are_ordered(void) {
    const record_t lower = { .reading = { .temp_c = 21.4f } };
    const record_t upper = { .reading = { .temp_c = 21.5f } };

    TEST_ASSERT_LESS_THAN_INT(0, record_compare(&lower, &upper));
    TEST_ASSERT_GREATER_THAN_INT(0, record_compare(&upper, &lower));
}

/**
 * A temperature exactly on a step boundary starts the step above it. The
 * boundary at 0.0 °C also checks that negative temperatures round down:
 * -0.1 °C is in the step from -0.5 °C, not the step from 0.0 °C.
 */
static void test_record_compare_boundary_starts_the_step_above(void) {
    const record_t below = { .reading = { .temp_c = -0.1f } };
    const record_t boundary = { .reading = { .temp_c = 0.0f } };
    const record_t inside = { .reading = { .temp_c = 0.4f } };

    TEST_ASSERT_LESS_THAN_INT(0, record_compare(&below, &boundary));
    TEST_ASSERT_EQUAL_INT(0, record_compare(&boundary, &inside));
}

/**
 * A new store holds no records and has a height of 0.
 */
static void test_store_create_starts_empty(void) {
    TEST_ASSERT_EQUAL_size_t(0, store_count(store));
    TEST_ASSERT_EQUAL_size_t(0, store_height(store));
}

/**
 * Destroying an empty store frees it. LeakSanitizer fails the test at exit if
 * anything is left.
 */
static void test_store_destroy_empty_store(void) {
    store_destroy(store);

    /* Already destroyed: tearDown's store_destroy(NULL) does nothing. */
    store = NULL;
}

/**
 * Inserting into an empty store adds one record.
 */
static void test_store_insert_into_empty_store(void) {
    const record_t record = { .reading = { .temp_c = 21.2f } };

    TEST_ASSERT_EQUAL_INT(0, store_insert(store, &record));
    TEST_ASSERT_EQUAL_size_t(1, store_count(store));
}

/**
 * The count includes every record, whether its key is new or shared, while
 * the height counts only nodes, one per key. The 100 records span 10 keys, 10
 * records each, so every node's array grows past its starting room. The first
 * 10 arrive in increasing order, so the 10 nodes form a chain: height 10.
 */
static void test_store_count_and_height_after_1_2_and_100_inserts(void) {
    for (int i = 0; i < 100; i++) {
        /* 15.0, 15.5, ..., 19.5 °C, then around again. */
        const record_t record = {
            .reading = { .temp_c = 15.0f + (float)(i % 10) * 0.5f },
        };
        TEST_ASSERT_EQUAL_INT(0, store_insert(store, &record));

        if (i == 0) {
            TEST_ASSERT_EQUAL_size_t(1, store_count(store));
            TEST_ASSERT_EQUAL_size_t(1, store_height(store));
        }
        if (i == 1) {
            TEST_ASSERT_EQUAL_size_t(2, store_count(store));
            TEST_ASSERT_EQUAL_size_t(2, store_height(store));
        }
    }

    TEST_ASSERT_EQUAL_size_t(100, store_count(store));
    TEST_ASSERT_EQUAL_size_t(10, store_height(store));
}

/**
 * A long run at one temperature, the usual case in a replay, fills one node
 * and leaves the tree's height at 1.
 */
static void test_store_one_temperature_stays_one_node_tall(void) {
    const record_t record = { .reading = { .temp_c = 15.0f } };

    for (int i = 0; i < 100; i++) {
        TEST_ASSERT_EQUAL_INT(0, store_insert(store, &record));
    }

    TEST_ASSERT_EQUAL_size_t(100, store_count(store));
    TEST_ASSERT_EQUAL_size_t(1, store_height(store));
}

/**
 * Every record with a key is found, in the order it was inserted. The 9
 * records fall in 3 steps, 3 records each, inserted in turn: 20.0, 21.0,
 * 22.0, then 20.2, 21.2, 22.2, then 20.4, 21.4, 22.4 °C. Each record's
 * timestamp is its place in that order, 0 to 8.
 */
static void test_store_find_gets_every_duplicate_in_order(void) {
    const float steps[] = { 20.0f, 21.0f, 22.0f };

    for (int i = 0; i < 9; i++) {
        const record_t record = {
            .reading = {
                .timestamp = i,
                .temp_c = steps[i % 3] + (float)(i / 3) * 0.2f,
            },
        };
        TEST_ASSERT_EQUAL_INT(0, store_insert(store, &record));
    }

    for (int s = 0; s < 3; s++) {
        /* Any temperature in the step finds its records. */
        const record_t key = { .reading = { .temp_c = steps[s] + 0.3f } };
        const record_t *records;
        size_t count;

        TEST_ASSERT_EQUAL_INT(0, store_find(store, &key, &records, &count));
        TEST_ASSERT_EQUAL_size_t(3, count);

        for (int j = 0; j < 3; j++) {
            TEST_ASSERT_EQUAL_INT64(s + 3 * j, records[j].reading.timestamp);
        }
    }
}

/**
 * Finding a key that is not there fails and leaves the out-parameters as
 * they were: in an empty store, and on either side of a store's only key.
 */
static void test_store_find_missing_key_fails(void) {
    const record_t below = { .reading = { .temp_c = 19.0f } };
    const record_t stored = { .reading = { .temp_c = 21.0f } };
    const record_t above = { .reading = { .temp_c = 23.0f } };

    const record_t untouched = { .reading = { .temp_c = -1.0f } };
    const record_t *records = &untouched;
    size_t count = 42;

    TEST_ASSERT_EQUAL_INT(1, store_find(store, &stored, &records, &count));

    TEST_ASSERT_EQUAL_INT(0, store_insert(store, &stored));
    TEST_ASSERT_EQUAL_INT(1, store_find(store, &below, &records, &count));
    TEST_ASSERT_EQUAL_INT(1, store_find(store, &above, &records, &count));

    TEST_ASSERT_EQUAL_PTR(&untouched, records);
    TEST_ASSERT_EQUAL_size_t(42, count);
}

/* The records a walk visited, in the order it visited them. */
typedef struct {
    record_t records[16];
    size_t count;
} visited_t;

/* A store_walk callback that copies each record into a visited_t. */
static void collect(const record_t *record, void *context) {
    visited_t *visited = context;

    TEST_ASSERT_TRUE(visited->count < 16);
    visited->records[visited->count] = *record;
    visited->count++;
}

/**
 * The walk visits records in key order, and records with the same key in the
 * order they were inserted. The records go in out of order; each record's
 * timestamp is its place in the insertion order.
 */
static void test_store_walk_visits_records_in_key_order(void) {
    const float temps[] = { 22.0f, 20.0f, 21.0f, 20.2f, 23.0f, 21.4f, 20.4f };

    for (int i = 0; i < 7; i++) {
        const record_t record = {
            .reading = { .timestamp = i, .temp_c = temps[i] },
        };
        TEST_ASSERT_EQUAL_INT(0, store_insert(store, &record));
    }

    visited_t visited = { .count = 0 };
    store_walk(store, collect, &visited);

    /* 20.0, 20.2, 20.4, then 21.0, 21.4, then 22.0, then 23.0 °C. */
    const int expected[] = { 1, 3, 6, 2, 5, 0, 4 };

    TEST_ASSERT_EQUAL_size_t(7, visited.count);
    for (size_t i = 0; i < 7; i++) {
        TEST_ASSERT_EQUAL_INT64(
            expected[i], visited.records[i].reading.timestamp);
    }
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_record_compare_same_step_is_equal);
    RUN_TEST(test_record_compare_neighboring_steps_are_ordered);
    RUN_TEST(test_record_compare_boundary_starts_the_step_above);
    RUN_TEST(test_store_create_starts_empty);
    RUN_TEST(test_store_destroy_empty_store);
    RUN_TEST(test_store_insert_into_empty_store);
    RUN_TEST(test_store_count_and_height_after_1_2_and_100_inserts);
    RUN_TEST(test_store_one_temperature_stays_one_node_tall);
    RUN_TEST(test_store_find_gets_every_duplicate_in_order);
    RUN_TEST(test_store_find_missing_key_fails);
    RUN_TEST(test_store_walk_visits_records_in_key_order);

    return UNITY_END();
}
