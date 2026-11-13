/*
 * istack.c:
 *
 * The stack is two heap blocks: this struct, whose address the caller keeps
 * and which never moves, and the int array, which realloc may move when it
 * grows.
 */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "istack.h"

struct istack {
    int   *data;    /* room for cap values; the top is data[count - 1] */
    size_t count;
    size_t cap;
};

/**
 * Outside in: the struct first, then its array. If the array fails, the
 * struct is freed too, so a failed create leaks nothing.
 */
istack_t *istack_create(void) {
    istack_t *s = malloc(sizeof *s);
    if (s == NULL) return NULL;

    s->data = malloc(ISTACK_START * sizeof *s->data);
    if (s->data == NULL) {
        free(s);
        return NULL;
    }
    s->count = 0;
    s->cap = ISTACK_START;
    return s;
}

/**
 * Inside out: the struct holds the only pointer to the array, so the array
 * is freed first. Freeing s first would read s->data from a freed block.
 */
void istack_destroy(istack_t *s) {
    if (s == NULL) return;

    free(s->data);
    free(s);
}

/**
 * realloc goes into a second pointer: on failure it returns NULL and keeps
 * the old block, and s->data must still point at it. The size check comes
 * first so that 2 * cap * sizeof cannot wrap around to a small number.
 */
int istack_push(istack_t *s, int v) {
    if (s == NULL) return -1;

    if (s->count == s->cap) {
        /* 2 * cap * sizeof would wrap past SIZE_MAX to a too-small size */
        if (s->cap > (SIZE_MAX / 2) / sizeof *s->data) return -1;

        int *bigger = realloc(s->data, 2 * s->cap * sizeof *s->data);
        if (bigger == NULL) return -1;

        s->data = bigger;
        s->cap *= 2;
    }
    s->data[s->count++] = v;
    return 0;
}

/**
 * A peek, then the top is dropped: the rules for NULL, an empty stack and
 * v == NULL live in istack_peek alone. The room never shrinks.
 */
int istack_pop(istack_t *s, int *v) {
    if (istack_peek(s, v) != 0) return -1;

    s->count--;
    return 0;
}

/**
 * *v is written only on success, so a failed peek leaves it as it was.
 */
int istack_peek(const istack_t *s, int *v) {
    if (s == NULL || s->count == 0) return -1;

    if (v != NULL) *v = s->data[s->count - 1];
    return 0;
}

/**
 * A NULL stack reads as an empty one, as the spec asks.
 */
size_t istack_count(const istack_t *s) {
    return s == NULL ? 0 : s->count;
}

/**
 * Defined through istack_count, so NULL is handled in one place.
 */
bool istack_is_empty(const istack_t *s) {
    return istack_count(s) == 0;
}

/**
 * The room right now: ISTACK_START times a power of 2, never less than the
 * most values s has held.
 */
size_t istack_capacity(const istack_t *s) {
    return s == NULL ? 0 : s->cap;
}

/**
 * count <= cap, and cap values were already allocated once, so
 * count * sizeof cannot overflow here.
 */
int *istack_to_array(const istack_t *s, size_t *n) {
    if (n == NULL) return NULL;

    *n = 0;
    if (istack_is_empty(s)) return NULL;

    int *copy = malloc(s->count * sizeof *copy);
    if (copy == NULL) return NULL;

    memcpy(copy, s->data, s->count * sizeof *copy);
    *n = s->count;
    return copy;
}
