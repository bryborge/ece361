/*
 * store.c:
 *
 * The store as a sorted array of records, for now; week 8 replaces the
 * inside with a binary search tree. There is one store, static, marked
 * "in use" or "free".
 */
#include <assert.h>
#include <math.h>

#include "store.h"

/* Records the store can hold, about 17 minutes of one-second ticks. The
 * week-8 tree allocates per node, and this cap goes away with it. */
#define STORE_CAP 1024

struct store {
    int in_use;               /* 1 while handed out, 0 when free */
    int n;                    /* records held: r[0..n-1] */
    record_t r[STORE_CAP];    /* sorted by key, equal keys in insertion order */
};

/* The one store. Starts all zero (free). */
static struct store pool;

/**
 * Quantizes a temperature to its 0.5 °C step. floor, not round, so that
 * 21.0 through 21.4 share a key and 21.5 starts the next one. Multiplying a
 * float by 2 is exact, so the boundaries x.0 and x.5 land exactly.
 *
 * @param temp_c The temperature in °C.
 * @return The index of its step: floor(temp_c / 0.5).
 */
static int quantize(float temp_c) {
    return (int) floorf(temp_c * 2.0f);
}

/**
 * Compares two records by quantized temperature only. The store keeps
 * equal keys in insertion order itself; this does not tie-break.
 *
 * @param a The first record.
 * @param b The second record.
 * @return -1, 0, or 1 as a's key is below, equal to, or above b's.
 */
int record_compare(const record_t *a, const record_t *b) {
    assert(a != NULL && b != NULL);

    int ka = quantize(a->reading.temp_c);
    int kb = quantize(b->reading.temp_c);

    return (ka > kb) - (ka < kb);
}

/**
 * Hands out the store, marked in use and emptied.
 *
 * @return The store, or NULL if it is already in use.
 */
store_t *store_create(void) {
    if (pool.in_use) return NULL;

    pool.in_use = 1;
    pool.n = 0;

    return &pool;
}

/**
 * Marks store s free. Its records are left as they are; store_create empties
 * the store when it hands it out.
 *
 * @param s The store to release; NULL does nothing.
 */
void store_destroy(store_t *s) {
    if (s != NULL) s->in_use = 0;
}

/**
 * Inserts a copy of r after the last record whose key is not above r's, so
 * that equal keys stay in insertion order. The records from there to the
 * end each move up by one first.
 *
 * @param s The store to insert into.
 * @param r The record to copy in.
 * @return 0, or -1 and nothing changed if s is full.
 */
int store_insert(store_t *s, const record_t *r) {
    assert(s != NULL && r != NULL);

    if (s->n == STORE_CAP) return -1;

    int pos = 0;
    while (pos < s->n && record_compare(&s->r[pos], r) <= 0) pos++;

    /* Move r[pos..n-1] up by one, starting from the end, so that each record
     * is copied before the one below it lands on top of it. */
    for (int i = s->n; i > pos; i--) {
        s->r[i] = s->r[i - 1];
    }

    s->r[pos] = *r;
    s->n++;

    return 0;
}

/**
 * Skips the records with a lower key, then copies each record with temp_c's
 * key. They are contiguous because the array is sorted.
 *
 * @param s      The store to search.
 * @param temp_c A temperature in the step to find.
 * @param out    Where to copy the matching records.
 * @param max    How many records out has room for.
 * @return How many records match; only the first max are copied.
 */
int store_find(const store_t *s, float temp_c, record_t out[], int max) {
    assert(s != NULL && out != NULL);

    int key = quantize(temp_c);
    int i = 0;

    while (i < s->n && quantize(s->r[i].reading.temp_c) < key) i++;

    int found = 0;
    for (; i < s->n && quantize(s->r[i].reading.temp_c) == key; i++) {
        if (found < max) out[found] = s->r[i];
        found++;
    }

    return found;
}

/**
 * Copies every record. The array is kept sorted, so copying it from r[0]
 * gives key order.
 *
 * @param s   The store to walk.
 * @param out Where to copy the records.
 * @param max How many records out has room for.
 * @return How many records s holds; only the first max are copied.
 */
int store_walk(const store_t *s, record_t out[], int max) {
    assert(s != NULL && out != NULL);

    for (int i = 0; i < s->n && i < max; i++) {
        out[i] = s->r[i];
    }

    return s->n;
}

/**
 * @param s The store to look at.
 * @return The number of records held.
 */
int store_count(const store_t *s) {
    assert(s != NULL);

    return s->n;
}

/**
 * An array has no height of its own. This returns the number of distinct
 * keys, which is the height of a binary search tree with one node per key
 * built from inserts in ascending key order, so tests that insert that way
 * hold for the week-8 tree too.
 *
 * @param s The store to look at.
 * @return The number of distinct keys; 0 when empty.
 */
int store_height(const store_t *s) {
    assert(s != NULL);

    int h = 0;
    for (int i = 0; i < s->n; i++) {
        if (i == 0 || record_compare(&s->r[i - 1], &s->r[i]) != 0) h++;
    }

    return h;
}
