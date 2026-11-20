/*
 * test_bst.c:
 *
 * Tests for bst. Prints PASS or FAIL for each check, then a summary; exits
 * nonzero if any check failed. Leaks are not checked here: make memcheck
 * runs these tests under AddressSanitizer, which reports them at exit.
 */
#include <stdbool.h>
#include <stdio.h>

#include "bst.h"

static int total;
static int fails;

#define CHECK(cond) do {                                               \
    total++;                                                           \
    if (cond) printf("(PASS) Test: %s\n", #cond);                      \
    else {                                                             \
        printf("(FAIL) Test: %s (line %d)\n", #cond, __LINE__);        \
        fails++;                                                       \
    }                                                                  \
} while (0)

int main(void) {
    printf("%d of %d checks passed\n", total - fails, total);

    return fails != 0;
}
