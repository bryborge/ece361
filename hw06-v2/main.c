#include <stdint.h>
#include <stdio.h>

#include "iom361_r4.h"

#define NUM_SWITCHES 4
#define NUM_LEDS 4

int main(void) {
    int rc;
    uint32_t *base = iom361_initialize(NUM_SWITCHES, NUM_LEDS, &rc);

    if (base == NULL) {
        fprintf(stderr, "replay: iom361_initialize failed (rc %d)\n", rc);
        return 1;
    }

    iom361_writeReg(base, LEDS_REG, 0x1, &rc);

    if (rc != 0) {
        fprintf(stderr, "replay: writing LEDS_REG failed (rc %d)\n", rc);
        return 1;
    }

    return 0;
}
