/**
 * @file store.h
 * @brief The record store.
 */
#ifndef STORE_H
#define STORE_H

#include <stddef.h>
#include <stdint.h>

#include "fsm.h"
#include "sensor.h"

typedef struct {
    reading_t reading;
    state_t state;
    uint32_t status;
} record_t;

/**
 * A binary search tree of records, ordered by key.
 */
typedef struct store store_t;

/**
 * Compare two records by key. The key is the temperature in 0.5 °C steps:
 * each step runs from a multiple of 0.5 °C up to, but not including, the next.
 * Asserts that no argument is NULL.
 *
 * @param a The first record.
 * @param b The second record.
 * @return A negative number, zero, or a positive number when a's key is less
 *         than, equal to, or greater than b's.
 */
int record_compare(const record_t *a, const record_t *b);

/**
 * Create an empty store.
 *
 * @return The store, or NULL if memory could not be allocated.
 */
store_t *store_create(void);

/**
 * Destroy a store and every record in it. Does nothing if store is NULL.
 *
 * @param store The store to destroy.
 */
void store_destroy(store_t *store);

/**
 * Count the records in a store, including every record that shares a key.
 * Asserts that store is not NULL.
 *
 * @param store The store to count.
 * @return The number of records.
 */
size_t store_count(const store_t *store);

/**
 * Insert a copy of a record. Records with the same key are kept in the order
 * they were inserted. Asserts that no argument is NULL.
 *
 * @param store  The store to insert into.
 * @param record The record to copy into the store.
 * @return 0 on success, or 1 if memory could not be allocated. The store is
 *         unchanged on failure.
 */
int store_insert(store_t *store, const record_t *record);

/**
 * Count the nodes along the longest path from the root down. Records that
 * share a key share a node, so they don't add to the height. An empty store
 * has a height of 0, and a store with one key has a height of 1. Asserts that
 * store is not NULL.
 *
 * @param store The store to measure.
 * @return The height of the tree.
 */
size_t store_height(const store_t *store);

/**
 * Find every record with the same key as a given record. The records are
 * given back as an array, in the order they were inserted. The array belongs
 * to the store and stays valid only until the next insert or destroy. Asserts
 * that no argument is NULL.
 *
 * @param store   The store to search.
 * @param key     A record with the key to find. Only its temperature is used.
 * @param records Where a pointer to the records is stored. Not written if no
 *                record has the key.
 * @param count   Where the number of records is stored. Not written if no
 *                record has the key.
 * @return 0 if records with the key were found, or 1 if none were.
 */
int store_find(
    const store_t *store, const record_t *key,
    const record_t **records, size_t *count
);

/**
 * Visit every record in key order. Records with the same key are visited in
 * the order they were inserted. The store must not be changed during the
 * walk. Asserts that store and visit are not NULL.
 *
 * @param store   The store to walk.
 * @param visit   Called once for each record, with the record and context.
 * @param context Passed to every call of visit, unchanged. May be NULL.
 */
void store_walk(
    const store_t *store,
    void (*visit)(const record_t *record, void *context),
    void *context
);

#endif
