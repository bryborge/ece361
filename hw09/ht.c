/*
 * ht.c:
 *
 * A hash table of int keys that counts how many times each key was added,
 * with separate chaining: each bucket is the head of a linked list of the
 * keys that land in it. Built from the week 9 lecture slides.
 */
#include <stdlib.h>

#include "ht.h"

/* One different key, and how many times it was added. */
struct node {
    int key;
    size_t count;
    struct node *next;      /* the next node in the same bucket */
};

struct ht {
    size_t nbuckets;
    size_t size;            /* the number of different keys */
    struct node **buckets;  /* nbuckets chain heads; NULL is an empty chain */
};

ht_t *ht_create(size_t nbuckets) {
    if (nbuckets == 0) return NULL;

    ht_t *t = malloc(sizeof *t);
    if (t == NULL) return NULL;

    /* calloc sets every head to NULL: all chains start empty. */
    t->buckets = calloc(nbuckets, sizeof *t->buckets);
    if (t->buckets == NULL) {
        free(t);
        return NULL;
    }
    t->nbuckets = nbuckets;
    t->size = 0;

    return t;
}

void ht_destroy(ht_t *t) {
    if (t == NULL) return;

    for (size_t i = 0; i < t->nbuckets; i++) {
        struct node *n = t->buckets[i];

        while (n != NULL) {
            struct node *next = n->next;  /* n->next is gone once n is freed */
            free(n);
            n = next;
        }
    }
    free(t->buckets);
    free(t);
}

int ht_add(ht_t *t, int key) {
    if (t == NULL) return -1;

    size_t i = (size_t)key % t->nbuckets;

    for (struct node *n = t->buckets[i]; n != NULL; n = n->next) {
        if (n->key == key) {
            n->count++;
            return 0;
        }
    }

    /* A new key. Nothing has changed yet, so a failed malloc needs no undo. */
    struct node *n = malloc(sizeof *n);
    if (n == NULL) return -1;
    n->key = key;
    n->count = 1;
    n->next = t->buckets[i];  /* at the front of chain i */
    t->buckets[i] = n;
    t->size++;

    return 0;
}

size_t ht_count(const ht_t *t, int key) {
    if (t == NULL) return 0;

    size_t i = (size_t)key % t->nbuckets;

    for (const struct node *n = t->buckets[i]; n != NULL; n = n->next) {
        if (n->key == key) return n->count;
    }

    return 0;
}

size_t ht_size(const ht_t *t) {
    if (t == NULL) return 0;

    return t->size;
}

size_t ht_longest_chain(const ht_t *t) {
    if (t == NULL) return 0;

    size_t longest = 0;

    for (size_t i = 0; i < t->nbuckets; i++) {
        size_t length = 0;

        for (const struct node *n = t->buckets[i]; n != NULL; n = n->next) {
            length++;
        }
        if (length > longest) longest = length;
    }

    return longest;
}
