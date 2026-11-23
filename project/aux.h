/**
 * @file aux.h
 * @brief The event history.
 */
#ifndef AUX_H
#define AUX_H

#include <stdbool.h>
#include <stddef.h>

typedef enum {
    H_PRESS,                    // The remote was pressed.
    H_REFUSAL,                  // A remote press was refused.
    H_OBSTRUCTION,              // The door was obstructed.
} history_kind_t;

typedef struct {
    history_kind_t kind;
    int tick;                   // The tick it happened on.
} history_entry_t;

/**
 * The history: a stack of entries.
 */
typedef struct history history_t;

/**
 * Create an empty history with room for 8 entries. The room doubles each time
 * the history is full and another entry is pushed.
 *
 * @return The history, or NULL if memory could not be allocated.
 */
history_t *history_create(void);

/**
 * Destroy a history and every entry still in it. Does nothing if history is
 * NULL.
 *
 * @param history The history to destroy.
 */
void history_destroy(history_t *history);

/**
 * Count the entries in a history. Asserts that history is not NULL.
 *
 * @param history The history to count.
 * @return The number of entries.
 */
size_t history_count(const history_t *history);

/**
 * Check whether a history has no entries. Asserts that history is not NULL.
 *
 * @param history The history to check.
 * @return true if the history has no entries.
 */
bool history_is_empty(const history_t *history);

/**
 * Push an entry onto the top of a history. If the history is full, its room
 * doubles first. Asserts that history is not NULL.
 *
 * @param history The history to push onto.
 * @param entry   The entry to push.
 * @return 0 on success, or 1 if the history was full and more memory could
 *         not be allocated. The history is unchanged on failure.
 */
int history_push(history_t *history, history_entry_t entry);

/**
 * Get the entry on top of a history without removing it. Asserts that no
 * argument is NULL.
 *
 * @param history The history to look at.
 * @param entry   Where the entry is stored. Not written if the history is
 *                empty.
 * @return 0 on success, or 1 if the history is empty.
 */
int history_peek(const history_t *history, history_entry_t *entry);

/**
 * Remove the entry on top of a history. Asserts that no argument is NULL.
 *
 * @param history The history to remove from.
 * @param entry   Where the removed entry is stored. Not written if the
 *                history is empty.
 * @return 0 on success, or 1 if the history is empty.
 */
int history_pop(history_t *history, history_entry_t *entry);

#endif
