/**
 * @file test_aux.c
 * @brief Unit tests for the event history module.
 */
#include <stddef.h>

#include "unity.h"

#include "aux.h"

/*
 * The test is linked with -Wl,--wrap=realloc, so every call to realloc in
 * aux.c comes here instead. The wrapper records the call and passes it on to
 * the real realloc, which the linker names __real_realloc. This makes the
 * growth of the hidden array visible.
 */
static size_t realloc_calls;
static size_t realloc_last_size;

void *__real_realloc(void *ptr, size_t size);

void *__wrap_realloc(void *ptr, size_t size) {
    realloc_calls++;
    realloc_last_size = size;

    return __real_realloc(ptr, size);
}

/*
 * Each test gets a new, empty history. tearDown destroys it, and Unity runs
 * tearDown even after a failed assertion, so a failing test does not leak.
 */
static history_t *history;

void setUp(void) {
    realloc_calls = 0;
    realloc_last_size = 0;

    history = history_create();
    TEST_ASSERT_NOT_NULL(history);
}

void tearDown(void) {
    history_destroy(history);
    history = NULL;
}

/**
 * A new history has no entries.
 */
static void test_history_create_starts_empty(void) {
    TEST_ASSERT_EQUAL_size_t(0, history_count(history));
}

/**
 * A history is empty when new, not empty after a push, and empty again after
 * that entry is popped.
 */
static void test_history_is_empty_follows_entries(void) {
    TEST_ASSERT_TRUE(history_is_empty(history));

    const history_entry_t pushed = { .kind = H_PRESS, .tick = 1 };
    TEST_ASSERT_EQUAL_INT(0, history_push(history, pushed));
    TEST_ASSERT_FALSE(history_is_empty(history));

    history_entry_t popped;
    TEST_ASSERT_EQUAL_INT(0, history_pop(history, &popped));
    TEST_ASSERT_TRUE(history_is_empty(history));
}

/**
 * A push adds one entry.
 */
static void test_history_push_adds_an_entry(void) {
    const history_entry_t entry = { .kind = H_PRESS, .tick = 5 };
    TEST_ASSERT_EQUAL_INT(0, history_push(history, entry));
    TEST_ASSERT_EQUAL_size_t(1, history_count(history));
}

/**
 * A new history has room for 8 entries, and the room doubles each time a push
 * finds it full: to 16 on the 9th push and to 32 on the 17th.
 */
static void test_history_push_doubles_room_when_full(void) {
    int tick = 0;

    /* Pushes 1 to 8 fit in the starting room. */
    for (; tick < 8; tick++) {
        const history_entry_t entry = { .kind = H_PRESS, .tick = tick };
        TEST_ASSERT_EQUAL_INT(0, history_push(history, entry));
    }
    TEST_ASSERT_EQUAL_size_t(0, realloc_calls);

    /* Push 9 doubles the room to 16. */
    const history_entry_t ninth = { .kind = H_PRESS, .tick = tick++ };
    TEST_ASSERT_EQUAL_INT(0, history_push(history, ninth));
    TEST_ASSERT_EQUAL_size_t(1, realloc_calls);
    TEST_ASSERT_EQUAL_size_t(16 * sizeof(history_entry_t), realloc_last_size);

    /* Pushes 10 to 16 fit in the new room. */
    for (; tick < 16; tick++) {
        const history_entry_t entry = { .kind = H_PRESS, .tick = tick };
        TEST_ASSERT_EQUAL_INT(0, history_push(history, entry));
    }
    TEST_ASSERT_EQUAL_size_t(1, realloc_calls);

    /* Push 17 doubles the room to 32. */
    const history_entry_t seventeenth = { .kind = H_PRESS, .tick = tick++ };
    TEST_ASSERT_EQUAL_INT(0, history_push(history, seventeenth));
    TEST_ASSERT_EQUAL_size_t(2, realloc_calls);
    TEST_ASSERT_EQUAL_size_t(32 * sizeof(history_entry_t), realloc_last_size);

    TEST_ASSERT_EQUAL_size_t(17, history_count(history));
}

/**
 * Peeking at an empty history fails and leaves the caller's entry as it was.
 */
static void test_history_peek_fails_when_empty(void) {
    history_entry_t entry = { .kind = H_OBSTRUCTION, .tick = 42 };

    TEST_ASSERT_EQUAL_INT(1, history_peek(history, &entry));
    TEST_ASSERT_EQUAL_INT(H_OBSTRUCTION, entry.kind);
    TEST_ASSERT_EQUAL_INT(42, entry.tick);
}

/**
 * Peeking gets the most recent entry and leaves it in the history.
 */
static void test_history_peek_gets_top_without_removing_it(void) {
    const history_entry_t first = { .kind = H_PRESS, .tick = 3 };
    const history_entry_t second = { .kind = H_REFUSAL, .tick = 7 };
    TEST_ASSERT_EQUAL_INT(0, history_push(history, first));
    TEST_ASSERT_EQUAL_INT(0, history_push(history, second));

    history_entry_t entry;
    TEST_ASSERT_EQUAL_INT(0, history_peek(history, &entry));
    TEST_ASSERT_EQUAL_INT(H_REFUSAL, entry.kind);
    TEST_ASSERT_EQUAL_INT(7, entry.tick);

    TEST_ASSERT_EQUAL_size_t(2, history_count(history));
}

/**
 * Popping an empty history fails and leaves the caller's entry as it was.
 */
static void test_history_pop_fails_when_empty(void) {
    history_entry_t entry = { .kind = H_OBSTRUCTION, .tick = 42 };

    TEST_ASSERT_EQUAL_INT(1, history_pop(history, &entry));
    TEST_ASSERT_EQUAL_INT(H_OBSTRUCTION, entry.kind);
    TEST_ASSERT_EQUAL_INT(42, entry.tick);
}

/**
 * After the history has grown, entries still come off in the reverse of the
 * order they went on: last in, first out.
 */
static void test_history_pop_is_last_in_first_out_after_growing(void) {
    /* 9 pushes: one more than the starting room of 8. */
    for (int tick = 0; tick < 9; tick++) {
        const history_entry_t entry = { .kind = H_PRESS, .tick = tick };
        TEST_ASSERT_EQUAL_INT(0, history_push(history, entry));
    }
    TEST_ASSERT_EQUAL_size_t(1, realloc_calls);

    for (int tick = 8; tick >= 0; tick--) {
        history_entry_t entry;
        TEST_ASSERT_EQUAL_INT(0, history_pop(history, &entry));
        TEST_ASSERT_EQUAL_INT(H_PRESS, entry.kind);
        TEST_ASSERT_EQUAL_INT(tick, entry.tick);
    }

    history_entry_t entry;
    TEST_ASSERT_EQUAL_INT(1, history_pop(history, &entry));
    TEST_ASSERT_EQUAL_size_t(0, history_count(history));
}

/**
 * Destroying a history that still holds entries, after it has grown, frees
 * everything. LeakSanitizer fails the test at exit if anything is left.
 */
static void test_history_destroy_frees_a_history_with_entries(void) {
    for (int tick = 0; tick < 9; tick++) {
        const history_entry_t entry = { .kind = H_PRESS, .tick = tick };
        TEST_ASSERT_EQUAL_INT(0, history_push(history, entry));
    }

    history_destroy(history);

    /* Already destroyed: tearDown's history_destroy(NULL) does nothing. */
    history = NULL;
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_history_create_starts_empty);
    RUN_TEST(test_history_is_empty_follows_entries);
    RUN_TEST(test_history_push_adds_an_entry);
    RUN_TEST(test_history_push_doubles_room_when_full);
    RUN_TEST(test_history_peek_fails_when_empty);
    RUN_TEST(test_history_peek_gets_top_without_removing_it);
    RUN_TEST(test_history_pop_fails_when_empty);
    RUN_TEST(test_history_pop_is_last_in_first_out_after_growing);
    RUN_TEST(test_history_destroy_frees_a_history_with_entries);

    return UNITY_END();
}
