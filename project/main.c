#include <stdio.h>
#include "iom361_r4.h"

int main(void) {
    int rc;
    uint32_t *base = iom361_initialize(2, 2, &rc);
    if (base == NULL) {
        fprintf(stderr, "iom361_initialize failed: %d\n", rc);
        return 1;
    }
    return 0;
}
