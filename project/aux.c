/**
 * @file aux.c
 * @brief The event history.
 *
 * The stack is a dynamic array. The bottom entry is at index 0 and the top at
 * index count - 1, so a push or a pop touches only the end of the array.
 */
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include "aux.h"

static const size_t initial_capacity = 8;

struct history {
    history_entry_t *entries;   // The array, bottom first.
    size_t count;               // The number of entries in use.
    size_t capacity;            // The number of entries there is room for.
};

history_t *history_create(void) {
    history_t *history = malloc(sizeof *history);
    if (history == NULL) return NULL;

    history->entries = malloc(initial_capacity * sizeof *history->entries);
    if (history->entries == NULL) {
        free(history);
        return NULL;
    }

    history->count = 0;
    history->capacity = initial_capacity;

    return history;
}

void history_destroy(history_t *history) {
    if (history == NULL) return;

    free(history->entries);
    free(history);
}

size_t history_count(const history_t *history) {
    assert(history != NULL);

    return history->count;
}

bool history_is_empty(const history_t *history) {
    assert(history != NULL);

    return history->count == 0;
}

int history_push(history_t *history, history_entry_t entry) {
    assert(history != NULL);

    if (history->count == history->capacity) {
        if (history->capacity > SIZE_MAX / 2 / sizeof *history->entries) {
            return 1;
        }

        const size_t capacity = history->capacity * 2;

        history_entry_t *entries =
            realloc(history->entries, capacity * sizeof *entries);
        if (entries == NULL) return 1;

        history->entries = entries;
        history->capacity = capacity;
    }

    history->entries[history->count] = entry;
    history->count++;

    return 0;
}

int history_peek(const history_t *history, history_entry_t *entry) {
    assert(history != NULL);
    assert(entry != NULL);

    if (history_is_empty(history)) return 1;

    *entry = history->entries[history->count - 1];

    return 0;
}

int history_pop(history_t *history, history_entry_t *entry) {
    assert(history != NULL);
    assert(entry != NULL);

    if (history_is_empty(history)) return 1;

    history->count--;
    *entry = history->entries[history->count];

    return 0;
}
