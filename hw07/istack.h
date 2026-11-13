/*
 * istack.h:
 *
 * A stack of int that grows as it fills. The values live in one heap array
 * that starts with room for ISTACK_START values and doubles when full; the
 * room never shrinks.
 */
#ifndef ISTACK_H
#define ISTACK_H

#include <stdbool.h>
#include <stddef.h>

#define ISTACK_START 8                  /* room at first; doubles when full */
typedef struct istack istack_t;         /* opaque */

/**
 * Creates a new, empty stack with room for ISTACK_START values.
 *
 * @return the stack, or NULL if memory runs out (nothing is leaked). The
 *         caller owns the stack and must free it with istack_destroy.
 */
istack_t *istack_create(void);

/**
 * Frees the stack and every value it holds. s must not be used afterwards.
 * NULL does nothing.
 */
void istack_destroy(istack_t *s);

/**
 * Pushes v on top of s. If s is full, first doubles its room.
 *
 * @return 0, or -1 if s is NULL or memory runs out; then s is unchanged.
 */
int istack_push(istack_t *s, int v);

/**
 * Removes the top value of s and stores it in *v. v may be NULL, to discard
 * the value.
 *
 * @return 0, or -1 if s is NULL or empty; then *v is unchanged.
 */
int istack_pop(istack_t *s, int *v);

/**
 * Stores the top value of s in *v without removing it. v may be NULL.
 *
 * @return 0, or -1 if s is NULL or empty; then *v is unchanged.
 */
int istack_peek(const istack_t *s, int *v);

/**
 * @return the number of values in s; 0 if s is NULL.
 */
size_t istack_count(const istack_t *s);

/**
 * @return whether s holds no values; true if s is NULL.
 */
bool istack_is_empty(const istack_t *s);

/**
 * @return how many values s has room for right now; 0 if s is NULL.
 */
size_t istack_capacity(const istack_t *s);

/**
 * Copies the values of s into a new array, bottom first, and stores their
 * number in *n. The caller owns the array and must free it with free.
 *
 * @return the array; or NULL, with *n = 0, if s is NULL or empty or memory
 *         runs out. If n is NULL, returns NULL and allocates nothing.
 */
int *istack_to_array(const istack_t *s, size_t *n);

#endif
