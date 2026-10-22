/*
 * tally.c:
 *
 * Implementation of the tally. Tallies come from a static pool of TALLY_MAX,
 * each marked in use or free.
 */
#include "tally.h"

/* entry with name and count. */
struct entry {
    char name[TALLY_LEN + 1];
    int count;
};

/* A tally. */
struct tally {
    int in_use;
    int n;
    struct entry e[TALLY_NAMES];
};

/* The pool of tallies. Starts all zero (free). */
static struct tally pool[TALLY_MAX];

/**
 * Hands out the first free tally in the pool, marked in use and emptied.
 *
 * @return A new, empty tally, or NULL if every tally in the pool is in use.
 */
tally_t *tally_create(void) {
    for (int i = 0; i < TALLY_MAX; i++) {
        if (!pool[i].in_use) {
            pool[i].in_use = 1;
            pool[i].n = 0;
            return &pool[i];
        }
    }

    return NULL;
}

/**
 * Marks t free so that tally_create can hand it out again. Its entries are
 * left as they are; tally_create empties the tally when it hands it out.
 *
 * @param t The tally to give back; NULL does nothing.
 */
void tally_destroy(tally_t *t) {
    if (t != NULL) t->in_use = 0;
}
