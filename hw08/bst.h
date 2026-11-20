#ifndef BST_H
#define BST_H

#include <stdbool.h>
#include <stddef.h>

/*
 * Every function taking a bst_t * assumes it is NULL or came from bst_create
 * and has not yet been passed to bst_destroy.
 */
typedef struct bst bst_t;

/**
 * Creates a new, empty tree. If it cannot allocate memory, it returns NULL.
 * The caller owns the tree and frees it with bst_destroy.
 *
 * @return Pointer to the new tree, or NULL.
 */
bst_t *bst_create(void);

/**
 * Frees every node and the tree itself. A NULL t does nothing. t must not be
 * used afterward.
 */
void bst_destroy(bst_t *t);

/**
 * Adds key as a new node. At each node on the way down, a smaller key goes
 * left and any other goes right (including duplicates). If t is NULL or
 * memory allocation fails, the tree is left unchanged.
 *
 * @return 0 on success, otherwise -1.
 */
int bst_insert(bst_t *t, int key);

/**
 * Returns whether or not the key exists in the given tree. If visits is
 * non-NULL, store the number of nodes compared in *visits, whether or not the
 * key was found. For a NULL tree, return false and record 0 visits if
 * requested.
 *
 * @return True if key found in the tree, False otherwise.
 */
bool bst_contains(const bst_t *t, int key, size_t *visits);

/**
 * Counts all stored keys in the given tree, including duplicates. A NULL t
 * counts as empty.
 *
 * @return The number of stored keys.
 */
size_t bst_size(const bst_t *t);

/**
 * Counts nodes along the longest path from the root down. An empty tree has a
 * height of 0 and a single node has a height of 1. A NULL t counts as empty.
 *
 * @return The node count of the longest path.
 */
size_t bst_height(const bst_t *t);

#endif
