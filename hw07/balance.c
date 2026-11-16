/*
 * balance.c:
 *
 * Checks that the brackets on standard input match: ( with ), [ with ],
 * { with }. Every bracket counts, also in comments and strings. Stops at the
 * first error and prints exactly one line on stdout.
 *
 * Usage: build/balance < file
 * Exit status: 0 balanced, 1 not balanced, 2 if balance itself failed (out of
 * memory or a read error; reported on stderr).
 */
#include <stdio.h>

#include "istack.h"

/**
 * @return the closer that matches opener c, or 0 if c is not an opener.
 */
static int closer_of(int c) {
    switch (c) {
    case '(': return ')';
    case '[': return ']';
    case '{': return '}';
    default:  return 0;
    }
}

static int is_opener(int c) {
    return c == '(' || c == '[' || c == '{';
}

static int is_closer(int c) {
    return c == ')' || c == ']' || c == '}';
}

/**
 * Reads stdin to the end or to the first error, pushing each opener's
 * character code onto s and popping it at each closer. Prints the result line.
 * Every return goes back to main, which destroys s once for all of them.
 *
 * @return the exit status: 0, 1 or 2, as in the comment at the top.
 */
static int check_brackets(istack_t *s) {
    size_t line = 1;
    size_t col = 0;
    size_t pairs = 0;
    size_t deepest = 0;
    int c;

    /* Main loop. */
    while ((c = getchar()) != EOF) {
        col++;

        /* The current character is an opening bracket of some sort */
        if (is_opener(c)) {
            /* Is there room on the stack? */
            if (istack_push(s, c) != 0) {
                fprintf(stderr, "balance: out of memory\n");
                return 2;
            }

            /* Is this the most open at once so far? If so, record it. */
            if (istack_count(s) > deepest) deepest = istack_count(s);
        /* The current character is a closing bracket of some sort */
        } else if (is_closer(c)) {
            /* Is anything open for this closer? */
            int opener;
            if (istack_pop(s, &opener) != 0) {
                printf("line %zu, column %zu: found '%c', nothing open\n",
                       line, col, c);
                return 1;
            }

            /* Does it close the innermost opener? */
            if (closer_of(opener) != c) {
                printf("line %zu, column %zu: found '%c', expected '%c'\n",
                       line, col, c, closer_of(opener));
                return 1;
            }

            pairs++;
        /* The current character is a new line */
        } else if (c == '\n') {
            line++;
            col = 0;
        }
    }

    if (ferror(stdin)) {
        fprintf(stderr, "balance: read error\n");
        return 2;
    }

    /* Is anything still open? */
    int innermost;
    if (istack_peek(s, &innermost) == 0) {
        printf("end of input: %zu still open, innermost '%c'\n",
               istack_count(s), innermost);
        return 1;
    }

    printf("balanced: %zu pairs, deepest %zu\n", pairs, deepest);
    return 0;
}

int main(void) {
    istack_t *s = istack_create();
    if (s == NULL) {
        fprintf(stderr, "balance: out of memory\n");
        return 2;
    }

    int status = check_brackets(s);
    istack_destroy(s);

    return status;
}
