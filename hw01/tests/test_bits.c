#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bits.h"

static int fails = 0;

#define CHECK(cond) do {                                              \
    if (cond) printf("PASS %s\n", #cond);                             \
    else { printf("FAIL %s (line %d)\n", #cond, __LINE__); fails++; } \
} while (0)

struct capture {
    char out[64];
    char err[64];
};

static struct capture capture_print_binary(uint32_t x, int width) {
    struct capture c = {0};
    FILE *out = fmemopen(c.out, sizeof c.out, "w");
    FILE *err = fmemopen(c.err, sizeof c.err, "w");

    if (!out || !err) {
        perror("fmemopen");
        exit(1);
    }

    FILE *saved_out = stdout;
    FILE *saved_err = stderr;
    stdout = out;
    stderr = err;

    print_binary(x, width);

    stdout = saved_out;
    stderr = saved_err;
    fclose(out);
    fclose(err);

    return c;
}

int main(void) {
    CHECK(strcmp(capture_print_binary(1, 1).out, "1\n") == 0);
    CHECK(strcmp(capture_print_binary(0x80000000, 32).out,
                 "1000 0000 0000 0000 0000 0000 0000 0000\n") == 0);
    CHECK(strcmp(capture_print_binary(0xFF, 4).out, "1111\n") == 0);
    CHECK(strcmp(capture_print_binary(0x16, 5).out, "1 0110\n") == 0);

    CHECK(capture_print_binary(0, 0).out[0] == '\0');
    CHECK(capture_print_binary(0, 0).err[0] != '\0');
    CHECK(capture_print_binary(0, 33).out[0] == '\0');
    CHECK(capture_print_binary(0, 33).err[0] != '\0');

    printf("%d failed\n", fails);

    return fails != 0;
}
