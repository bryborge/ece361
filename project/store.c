/**
 * @file store.c
 * @brief The record store.
 */
#include <assert.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include "store.h"

/* The room a new node's array starts with. */
static const size_t node_initial_capacity = 1;

/*
 * One node per key. Every record with that key is kept in the node's array,
 * oldest first.
 */
struct node {
    record_t *records;          // The array, oldest first.
    size_t count;               // The number of records in use.
    size_t capacity;            // The number of records there is room for.
    struct node *left;          // Keys less than this node's.
    struct node *right;         // Keys greater than this node's.
};

struct store {
    struct node *root;
};

int record_compare(const record_t *a, const record_t *b) {
    assert(a != NULL);
    assert(b != NULL);

    /*
     * The key is the number of the 0.5 °C step. Doubling the temperature
     * makes each step 1 wide, and floorf rounds down.
     */
    const float key_a = floorf(a->reading.temp_c * 2.0f);
    const float key_b = floorf(b->reading.temp_c * 2.0f);

    if (key_a < key_b) return -1;
    if (key_a > key_b) return 1;

    return 0;
}

store_t *store_create(void) {
    store_t *store = malloc(sizeof *store);
    if (store == NULL) return NULL;

    store->root = NULL;

    return store;
}

/*
 * Free every node in the subtree rooted at node, along with each node's
 * records. Both subtrees are freed before the node itself. Asserts that
 * node is not NULL.
 */
static void free_subtree(struct node *node) {
    assert(node != NULL);

    if (node->left != NULL) free_subtree(node->left);
    if (node->right != NULL) free_subtree(node->right);

    free(node->records);
    free(node);
}

void store_destroy(store_t *store) {
    if (store == NULL) return;
    if (store->root != NULL) free_subtree(store->root);

    free(store);
}

/*
 * Count the records in the subtree rooted at node: every record in every
 * node, not the number of nodes. An empty subtree (NULL) counts as 0.
 */
static size_t count_subtree(const struct node *node) {
    if (node == NULL) return 0;

    return node->count
         + count_subtree(node->left)
         + count_subtree(node->right);
}

size_t store_count(const store_t *store) {
    assert(store != NULL);

    return count_subtree(store->root);
}

/*
 * Insert a copy of record into the subtree rooted at node, and return the
 * subtree's root. Adds a new node if the subtree was empty. Sets *rc to 0
 * on success, or to 1 if memory could not be allocated, in which case the
 * subtree is unchanged.
 */
static struct node *insert_subtree(
    struct node *node, const record_t *record, int *rc
) {
    /* The record's key is new, so it gets its own node. */
    if (node == NULL) {
        struct node *created = malloc(sizeof *created);
        if (created == NULL) {
            *rc = 1;
            return NULL;
        }

        created->records =
            malloc(node_initial_capacity * sizeof *created->records);
        if (created->records == NULL) {
            free(created);
            *rc = 1;
            return NULL;
        }

        created->records[0] = *record;
        created->count = 1;
        created->capacity = node_initial_capacity;
        created->left = NULL;
        created->right = NULL;

        *rc = 0;
        return created;
    }

    const int order = record_compare(record, &node->records[0]);

    if (order < 0) {
        node->left = insert_subtree(node->left, record, rc);
        return node;
    }
    if (order > 0) {
        node->right = insert_subtree(node->right, record, rc);
        return node;
    }

    /*
     * The same key: append to this node's records, doubling the room first
     * if it is full.
     */
    if (node->count == node->capacity) {
        if (node->capacity > SIZE_MAX / 2 / sizeof *node->records) {
            *rc = 1;
            return node;
        }

        const size_t capacity = node->capacity * 2;

        record_t *records = realloc(node->records, capacity * sizeof *records);
        if (records == NULL) {
            *rc = 1;
            return node;
        }

        node->records = records;
        node->capacity = capacity;
    }

    node->records[node->count] = *record;
    node->count++;

    *rc = 0;
    return node;
}

int store_insert(store_t *store, const record_t *record) {
    assert(store != NULL);
    assert(record != NULL);

    int rc;
    store->root = insert_subtree(store->root, record, &rc);

    return rc;
}

/*
 * Count the nodes along the longest path down from node. An empty subtree
 * (NULL) has a height of 0.
 */
static size_t height_subtree(const struct node *node) {
    if (node == NULL) return 0;

    const size_t left = height_subtree(node->left);
    const size_t right = height_subtree(node->right);

    return 1 + (left > right ? left : right);
}

size_t store_height(const store_t *store) {
    assert(store != NULL);

    return height_subtree(store->root);
}

int store_find(
    const store_t *store, const record_t *key,
    const record_t **records, size_t *count
) {
    assert(store != NULL);
    assert(key != NULL);
    assert(records != NULL);
    assert(count != NULL);

    const struct node *node = store->root;

    while (node != NULL) {
        const int order = record_compare(key, &node->records[0]);

        if (order < 0) {
            node = node->left;
        } else if (order > 0) {
            node = node->right;
        } else {
            *records = node->records;
            *count = node->count;
            return 0;
        }
    }

    return 1;
}

/* Visit every record in the subtree rooted at node, in key order. */
static void walk_subtree(
    const struct node *node,
    void (*visit)(const record_t *record, void *context),
    void *context
) {
    if (node == NULL) return;

    walk_subtree(node->left, visit, context);

    for (size_t i = 0; i < node->count; i++) {
        visit(&node->records[i], context);
    }

    walk_subtree(node->right, visit, context);
}

void store_walk(
    const store_t *store,
    void (*visit)(const record_t *record, void *context),
    void *context
) {
    assert(store != NULL);
    assert(visit != NULL);

    walk_subtree(store->root, visit, context);
}
