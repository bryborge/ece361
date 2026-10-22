/*
 * tally.h:
 *
 * A tally counts how often each name occurs, such as the events a device
 * sees.
 *
 * Every function that takes a tally_t assumes it came from tally_create and
 * has not been given back with tally_destroy since.
 */
#ifndef TALLY_H
#define TALLY_H

#include <stdio.h>

#define TALLY_MAX    4  /* tallies that can exist at the same time */
#define TALLY_NAMES 16  /* different names one tally can hold */
#define TALLY_LEN   15  /* longest name, without the '\0' */

typedef struct tally tally_t;  /* opaque */

/**
 * Returns a new, empty tally, or NULL if TALLY_MAX tallies are already in use.
 * The tally stays valid until it is passed to tally_destroy.
 */
tally_t *tally_create(void);

/**
 * Given a tally t, gives it back so that tally_create can hand it out again.
 * Returns nothing. Does nothing if t is NULL. Assumes t is not used again
 * afterwards.
 */
void tally_destroy(tally_t *t);

/**
 * Given a tally t and a name, counts one more occurrence of name and returns
 * its new count (1 the first time). The tally keeps its own copy of name, so
 * the caller may reuse its buffer afterwards. Returns -1 and changes nothing
 * if t or name is NULL, name is empty or longer than TALLY_LEN, or name is
 * new and the tally already holds TALLY_NAMES names. Assumes name, if not
 * NULL, is a '\0'-terminated string.
 */
int tally_add(tally_t *t, const char *name);

/**
 * Given a tally t and a name, returns how often name was added to t: 0 if it
 * never was, or if t or name is NULL. Changes nothing. Assumes name, if not
 * NULL, is a '\0'-terminated string.
 */
int tally_count(const tally_t *t, const char *name);

/**
 * Given a tally t, returns how many different names it holds, from 0 to
 * TALLY_NAMES: 0 if t is NULL. Changes nothing.
 */
int tally_size(const tally_t *t);

/**
 * Given a tally t and an output stream out, writes one line per name, the
 * name and its count separated by one space ("OPEN 3"), in alphabetical
 * (strcmp) order. An empty tally writes nothing. Returns nothing. Does
 * nothing if t or out is NULL. Assumes out is open for writing.
 */
void tally_print(const tally_t *t, FILE *out);

#endif
