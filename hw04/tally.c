/*
 * tally.c:
 *
 * Implementation of the tally. Tallies come from a static pool of TALLY_MAX,
 * each marked "in use" or "free".
 */
#include <string.h>

#include "tally.h"

struct entry {
    char name[TALLY_LEN + 1];  /* the characters + '\0' */
    int count;                 /* 1 or more */
};

struct tally {
    int in_use;                   /* 1 while handed out, 0 when free */
    int n;                        /* entries held: e[0..n-1] */
    struct entry e[TALLY_NAMES];  /* sorted by strcmp, each name once */
};

/* The pool of tallies. Starts all zero (free). */
static struct tally pool[TALLY_MAX];

/**
 * Finds where name is, or where it belongs, in t's sorted entries.
 *
 * @param t    The tally to search.
 * @param name The name to look for.
 * @return The index of name's entry if t holds name; otherwise the index
 *         where name belongs to keep the entries sorted.
 */
static int find_pos(const struct tally *t, const char *name) {
    int pos = 0;

    while (pos < t->n && strcmp(t->e[pos].name, name) < 0) pos++;

    return pos;
}

/**
 * Tells whether the entry at the given position holds name.
 *
 * @param t    The tally to look in.
 * @param pos  The index find_pos returned for name.
 * @param name The name to compare.
 * @return 1 if e[pos] is one of t's entries and holds name, otherwise 0.
 */
static int entry_matches(const struct tally *t, int pos, const char *name) {
    return pos < t->n && strcmp(t->e[pos].name, name) == 0;
}

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
 * @param t The tally to release; NULL does nothing.
 */
void tally_destroy(tally_t *t) {
    if (t != NULL) t->in_use = 0;
}

/**
 * Counts one more occurrence of name. A name t already holds just has its
 * count raised. A new name goes in at the index where it belongs, after the
 * entries from there to the end each move up by one.
 *
 * @param t    The tally to add to.
 * @param name The name to count; t copies it, so the caller keeps its buffer.
 * @return The new count of name, or -1, changing nothing, if t or name is
 *         NULL, name is empty or longer than TALLY_LEN, or name is new and t
 *         is full.
 */
int tally_add(tally_t *t, const char *name) {
    if (t == NULL || name == NULL) return -1;

    size_t len = strlen(name);
    if (len == 0 || len > TALLY_LEN) return -1;

    int pos = find_pos(t, name);
    if (entry_matches(t, pos, name)) return ++t->e[pos].count;

    if (t->n == TALLY_NAMES) return -1;

    /* Move e[pos..n-1] up by one, starting from the end, so that each entry
     * is copied before the one below it lands on top of it. */
    for (int k = t->n; k > pos; k--) {
        t->e[k] = t->e[k - 1];
    }

    memcpy(t->e[pos].name, name, len + 1);  /* the characters + '\0' */
    t->e[pos].count = 1;
    t->n++;

    return 1;
}

/**
 * Looks name up in t.
 *
 * @param t    The tally to look in.
 * @param name The name to look up.
 * @return How often name was added to t: 0 if it never was, or if t or name
 *         is NULL.
 */
int tally_count(const tally_t *t, const char *name) {
    if (t == NULL || name == NULL) return 0;

    int pos = find_pos(t, name);

    return entry_matches(t, pos, name) ? t->e[pos].count : 0;
}

/**
 * Reports how many different names t holds.
 *
 * @param t The tally to look at.
 * @return The number of entries held, from 0 to TALLY_NAMES; 0 if t is NULL.
 */
int tally_size(const tally_t *t) {
    return t == NULL ? 0 : t->n;
}

/**
 * Writes each entry as "name count" on its own line. The entries are kept
 * sorted, so walking them from e[0] gives strcmp order.
 *
 * @param t   The tally to print; NULL writes nothing.
 * @param out Where to write; NULL writes nothing.
 */
void tally_print(const tally_t *t, FILE *out) {
    if (t == NULL || out == NULL) return;

    for (int i = 0; i < t->n; i++) {
        fprintf(out, "%s %d\n", t->e[i].name, t->e[i].count);
    }
}
