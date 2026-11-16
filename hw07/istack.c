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
    int *data;      /* the top of the stack is data[count - 1] */
    size_t count;
    size_t cap;
};

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

void istack_destroy(istack_t *s) {
    if (s == NULL) return;

    free(s->data);
    free(s);
}

int istack_push(istack_t *s, int v) {
    if (s == NULL) return -1;

    if (s->count == s->cap) {
        /* 2 * cap * sizeof would wrap past SIZE_MAX to a size too small */
        if (s->cap > (SIZE_MAX / 2) / sizeof *s->data) return -1;

        int *bigger = realloc(s->data, 2 * s->cap * sizeof *s->data);
        if (bigger == NULL) return -1;

        s->data = bigger;
        s->cap *= 2;
    }
    s->data[s->count++] = v;
    return 0;
}

int istack_pop(istack_t *s, int *v) {
    if (istack_peek(s, v) != 0) return -1;

    s->count--;
    return 0;
}

int istack_peek(const istack_t *s, int *v) {
    if (s == NULL || s->count == 0) return -1;

    if (v != NULL) *v = s->data[s->count - 1];
    return 0;
}

size_t istack_count(const istack_t *s) {
    return s == NULL ? 0 : s->count;
}

bool istack_is_empty(const istack_t *s) {
    return istack_count(s) == 0;
}

size_t istack_capacity(const istack_t *s) {
    return s == NULL ? 0 : s->cap;
}

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
