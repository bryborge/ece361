/*
 * test_aux.c:
 *
 * Tests for aux.c. Prints PASS or FAIL for each check; test_main.c runs them
 * and prints the summary. Leaks are not checked here: the tests are built
 * with AddressSanitizer, whose LeakSanitizer reports them at exit.
 */
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>

#include "aux.h"
#include "check.h"

/*
 * The test runner is linked with -Wl,--wrap=realloc, so every call to realloc
 * comes here instead. The wrapper records the call and passes it on to the
 * real realloc, which the linker names __real_realloc. This makes the growth
 * of the hidden array visible.
 */
static size_t realloc_calls;
static size_t realloc_last_size;

void *__real_realloc(void *ptr, size_t size);
void *__wrap_realloc(void *ptr, size_t size);

void *__wrap_realloc(void *ptr, size_t size) {
    realloc_calls++;
    realloc_last_size = size;

    return __real_realloc(ptr, size);
}

/**
 * Starts each check with a new, empty history and the realloc counters at 0.
 *
 * @return The history, for the check to destroy.
 */
static history_t *start(void) {
    realloc_calls = 0;
    realloc_last_size = 0;

    history_t *history = history_create();
    assert(history != NULL);
    return history;
}

/**
 * Pushes a press for each tick from first up to, but not including, last.
 *
 * @return true if every push succeeded.
 */
static bool push_presses(history_t *history, int first, int last) {
    bool all_pushed = true;

    for (int tick = first; tick < last; tick++) {
        const history_entry_t entry = { .kind = H_PRESS, .tick = tick };
        all_pushed = history_push(history, entry) == 0 && all_pushed;
    }
    return all_pushed;
}

/**
 * Pops one entry for each tick from first down to 0.
 *
 * @return true if every pop succeeded and gave a press for that tick.
 */
static bool pops_presses_down_from(history_t *history, int first) {
    for (int tick = first; tick >= 0; tick--) {
        history_entry_t entry;

        if (history_pop(history, &entry) != 0) return false;
        if (entry.kind != H_PRESS || entry.tick != tick) return false;
    }
    return true;
}

/**
 * A new history has no entries.
 */
static void check_history_create_starts_empty(void) {
    history_t *history = start();

    CHECK(history_count(history) == 0);
    history_destroy(history);
}

/**
 * A history is empty when new, not empty after a push, and empty again after
 * that entry is popped.
 */
static void check_history_is_empty_follows_entries(void) {
    history_t *history = start();

    CHECK(history_is_empty(history));

    const history_entry_t pushed = { .kind = H_PRESS, .tick = 1 };
    CHECK(history_push(history, pushed) == 0);
    CHECK(!history_is_empty(history));

    history_entry_t popped;
    CHECK(history_pop(history, &popped) == 0);
    CHECK(history_is_empty(history));
    history_destroy(history);
}

/**
 * A push adds one entry.
 */
static void check_history_push_adds_an_entry(void) {
    history_t *history = start();
    const history_entry_t entry = { .kind = H_PRESS, .tick = 5 };

    CHECK(history_push(history, entry) == 0);
    CHECK(history_count(history) == 1);
    history_destroy(history);
}

/**
 * A new history has room for 8 entries, and the room doubles each time a push
 * finds it full: to 16 on the 9th push and to 32 on the 17th.
 */
static void check_history_push_doubles_room_when_full(void) {
    history_t *history = start();

    /* Pushes 1 to 8 fit in the starting room. */
    CHECK(push_presses(history, 0, 8));
    CHECK(realloc_calls == 0);

    /* Push 9 doubles the room to 16. */
    CHECK(push_presses(history, 8, 9));
    CHECK(realloc_calls == 1);
    CHECK(realloc_last_size == 16 * sizeof(history_entry_t));

    /* Pushes 10 to 16 fit in the new room. */
    CHECK(push_presses(history, 9, 16));
    CHECK(realloc_calls == 1);

    /* Push 17 doubles the room to 32. */
    CHECK(push_presses(history, 16, 17));
    CHECK(realloc_calls == 2);
    CHECK(realloc_last_size == 32 * sizeof(history_entry_t));

    CHECK(history_count(history) == 17);
    history_destroy(history);
}

/**
 * Peeking at an empty history fails and leaves the caller's entry as it was.
 */
static void check_history_peek_fails_when_empty(void) {
    history_t *history = start();
    history_entry_t entry = { .kind = H_OBSTRUCTION, .tick = 42 };

    CHECK(history_peek(history, &entry) == 1);
    CHECK(entry.kind == H_OBSTRUCTION);
    CHECK(entry.tick == 42);
    history_destroy(history);
}

/**
 * Peeking gets the most recent entry and leaves it in the history.
 */
static void check_history_peek_gets_top_without_removing_it(void) {
    history_t *history = start();
    const history_entry_t first = { .kind = H_PRESS, .tick = 3 };
    const history_entry_t second = { .kind = H_REFUSAL, .tick = 7 };

    CHECK(history_push(history, first) == 0);
    CHECK(history_push(history, second) == 0);

    history_entry_t entry;
    CHECK(history_peek(history, &entry) == 0);
    CHECK(entry.kind == H_REFUSAL);
    CHECK(entry.tick == 7);

    CHECK(history_count(history) == 2);
    history_destroy(history);
}

/**
 * Popping an empty history fails and leaves the caller's entry as it was.
 */
static void check_history_pop_fails_when_empty(void) {
    history_t *history = start();
    history_entry_t entry = { .kind = H_OBSTRUCTION, .tick = 42 };

    CHECK(history_pop(history, &entry) == 1);
    CHECK(entry.kind == H_OBSTRUCTION);
    CHECK(entry.tick == 42);
    history_destroy(history);
}

/**
 * After the history has grown, entries still come off in the reverse of the
 * order they went on: last in, first out.
 */
static void check_history_pop_is_last_in_first_out_after_growing(void) {
    history_t *history = start();

    /* 9 pushes: one more than the starting room of 8. */
    CHECK(push_presses(history, 0, 9));
    CHECK(realloc_calls == 1);

    CHECK(pops_presses_down_from(history, 8));

    history_entry_t entry;
    CHECK(history_pop(history, &entry) == 1);
    CHECK(history_count(history) == 0);
    history_destroy(history);
}

/**
 * Destroying a history that still holds entries, after it has grown, frees
 * everything. LeakSanitizer fails the run at exit if anything is left.
 */
static void check_history_destroy_frees_a_history_with_entries(void) {
    history_t *history = start();

    CHECK(push_presses(history, 0, 9));
    history_destroy(history);
}

void run_aux_tests(void) {
    check_history_create_starts_empty();
    check_history_is_empty_follows_entries();
    check_history_push_adds_an_entry();
    check_history_push_doubles_room_when_full();
    check_history_peek_fails_when_empty();
    check_history_peek_gets_top_without_removing_it();
    check_history_pop_fails_when_empty();
    check_history_pop_is_last_in_first_out_after_growing();
    check_history_destroy_frees_a_history_with_entries();
}
