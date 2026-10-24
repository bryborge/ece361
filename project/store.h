/*
 * store.h:
 *
 * The store keeps every record the device makes, ordered by temperature
 * quantized to 0.5 °C (key K2): 21.2 and 21.4 °C have the same key. Records
 * with the same key are kept in the order they were inserted.
 *
 * Passing NULL where a store, record, or output array is expected is a bug
 * in the caller, and aborts with an assert. Every function that takes a
 * store_t assumes it came from store_create and has not been passed to
 * store_destroy since.
 */
#ifndef STORE_H
#define STORE_H

#include <stdint.h>

#include "fsm.h"
#include "sensor.h"

typedef struct {
    reading_t reading;
    state_t state;      /* the state at the moment of the reading */
    uint32_t status;    /* the status word at that moment */
} record_t;

typedef struct store store_t;

/**
 * Given two records, compares their temperatures quantized to 0.5 °C.
 * Returns a negative number, 0, or a positive number as a's key is below,
 * equal to, or above b's.
 */
int record_compare(const record_t *a, const record_t *b);

/**
 * Returns a new, empty store, or NULL if the store is already in use.
 */
store_t *store_create(void);

/**
 * Given a store s, releases it so that store_create can hand it out again.
 * Does nothing if s is NULL.
 */
void store_destroy(store_t *s);

/**
 * Given a store s and a record r, inserts a copy of r after any records
 * with the same key. Returns 0, or -1 and changes nothing if s is full.
 */
int store_insert(store_t *s, const record_t *r);

/**
 * Given a store s, a temperature, and an array out of max records, copies
 * every record whose key is the key of temp_c into out, in insertion order,
 * up to max of them. Returns how many records match, which is more than
 * were copied if out was too small.
 */
int store_find(const store_t *s, float temp_c, record_t out[], int max);

/**
 * Given a store s and an array out of max records, copies every record into
 * out, in key order and insertion order within a key, up to max of them.
 * Returns how many records s holds, which is more than were copied if out
 * was too small.
 *
 * The sheet asks for a walk with a callback. That needs function pointers,
 * which the course has not covered yet, so the walk copies out until the
 * week-8 tree, when it takes a callback instead.
 */
int store_walk(const store_t *s, record_t out[], int max);

/**
 * Given a store s, returns how many records it holds.
 */
int store_count(const store_t *s);

/**
 * Given a store s, returns its height: 0 when empty, 1 for a single key.
 */
int store_height(const store_t *s);

#endif
