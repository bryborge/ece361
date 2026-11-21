/*
 * treestats.c:
 *
 * ./treestats < file reads integers until the end of the input, inserts them
 * into a bst in that order, and prints the tree's size and height:
 *
 *   n 7, height 3                    exit status 0
 *   empty tree                       exit status 0
 *   not a number after 4 values      exit status 1
 *
 * A number is a whole whitespace-separated token in int range; "12abc" and
 * "99999999999" are not numbers.
 */
#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bst.h"

/*
 * Holds any int with room to spare. scanf stops a longer token at 63
 * characters, so a token that fills the buffer is rejected as too long
 * rather than read as two numbers.
 */
#define TOKEN_SIZE 64
#define TOKEN_FORMAT "%63s"

/* Parses tok as a whole decimal int into *v; false if it is anything else. */
static bool parse_int(const char *tok, int *v) {
    char *end;

    errno = 0;
    long n = strtol(tok, &end, 10);
    if (end == tok || *end != '\0') return false;
    if (errno == ERANGE || n < INT_MIN || n > INT_MAX) return false;
    *v = (int)n;

    return true;
}

int main(void) {
    bst_t *t = bst_create();
    char tok[TOKEN_SIZE];
    size_t count = 0;
    int v;

    if (t == NULL) {
        fprintf(stderr, "treestats: out of memory\n");
        return 1;
    }

    while (scanf(TOKEN_FORMAT, tok) == 1) {
        if (strlen(tok) == TOKEN_SIZE - 1 || !parse_int(tok, &v)) {
            printf("not a number after %zu values\n", count);
            bst_destroy(t);
            return 1;
        }
        if (bst_insert(t, v) != 0) {
            fprintf(stderr, "treestats: out of memory after %zu values\n",
                    count);
            bst_destroy(t);
            return 1;
        }
        count++;
    }
    if (ferror(stdin)) {
        fprintf(stderr, "treestats: error reading input\n");
        bst_destroy(t);
        return 1;
    }

    if (bst_size(t) == 0) {
        printf("empty tree\n");
    } else {
        printf("n %zu, height %zu\n", bst_size(t), bst_height(t));
    }
    bst_destroy(t);

    return 0;
}
