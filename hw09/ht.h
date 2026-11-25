#ifndef HT_H
#define HT_H

#include <stddef.h>

/*
 * A hash table that counts how many times each key was added. Keys that land
 * in the same bucket are kept in a linked list (separate chaining).
 *
 * Every function taking an ht_t * assumes it is NULL or came from ht_create
 * and has not yet been passed to ht_destroy. Keys are assumed never to be
 * negative.
 */
typedef struct ht ht_t;

/**
 * Creates a new, empty table with nbuckets buckets. If nbuckets is 0 or it
 * cannot allocate memory, it returns NULL and nothing is leaked. The caller
 * owns the table and frees it with ht_destroy.
 *
 * @return Pointer to the new table, or NULL.
 */
ht_t *ht_create(size_t nbuckets);

/**
 * Frees every node, the bucket array, and the table itself. A NULL t does
 * nothing. t must not be used afterward.
 */
void ht_destroy(ht_t *t);

/**
 * Counts one more of key, in bucket key % nbuckets. If key is already in that
 * bucket's chain, its count goes up by 1. Otherwise a new node with count 1
 * goes at the front of the chain. If t is NULL or memory allocation fails,
 * the table is left unchanged.
 *
 * @return 0 on success, otherwise -1.
 */
int ht_add(ht_t *t, int key);

/**
 * Returns how many times key was added. A key never added, or a NULL t,
 * gives 0.
 *
 * @return The number of times key was added.
 */
size_t ht_count(const ht_t *t, int key);

/**
 * Returns the number of different keys in the table. A NULL t counts as
 * empty.
 *
 * @return The number of different keys.
 */
size_t ht_size(const ht_t *t);

/**
 * Returns the number of nodes in the longest chain. An empty table, or a NULL
 * t, gives 0.
 *
 * @return The node count of the longest chain.
 */
size_t ht_longest_chain(const ht_t *t);

#endif
