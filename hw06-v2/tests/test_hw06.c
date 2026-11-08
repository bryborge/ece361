/*
 * test_hw06.c:
 *
 * Tests for hw06. Prints PASS or FAIL for each check, then a summary; exits
 * nonzero if any check failed.
 */
#include <stdint.h>
#include <stdio.h>

#include "iom361_r4.h"

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

static void check_iom361_initializes(void) {
    int rc;
    uint32_t *base = iom361_initialize(4, 4, &rc);

    CHECK(base != NULL);
    CHECK(rc == 0);
}

int main(void) {
    check_iom361_initializes();

    printf("%d of %d checks passed\n", total - fails, total);

    return fails != 0;
}
