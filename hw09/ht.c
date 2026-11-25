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
