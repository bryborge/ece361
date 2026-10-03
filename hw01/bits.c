#include <inttypes.h>
#include <limits.h>
#include <stdio.h>
#include "bits.h"

void print_binary(uint32_t x, int width) {
    const int max_width = (int)(sizeof x * CHAR_BIT);

    if (width < 1 || width > max_width) {
        fprintf(
            stderr,
            "print_binary: width of %d out of range (expected 1-%d).\n",
            width,
            max_width
        );
        return;
    }

    for (int i = width - 1; i >= 0; i--) {
        printf("%" PRIu32, (x >> i) & 1);

        if (i % 4 == 0 && i != 0) {
            printf(" ");
        }
    }

    printf("\n");
}

// uint32_t get_field(uint32_t word, int pos, int width) {
//     /* Implement ... */
//     return 0;
// }

// uint32_t set_field(uint32_t word, int pos, int width, uint32_t value) {
//     /* Implement ... */
//     return 0;
// }

// int32_t sign_extend(uint32_t value, int width) {
//     /* Implement ... */
//     return 0;
// }
