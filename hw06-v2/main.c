#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>

#include "iom361_r4.h"
#include "panel.h"

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

    uint32_t before = iom361_readReg(base, SWITCHES_REG, &rc);
    _iom361_set_switches(0x6);
    uint32_t now = iom361_readReg(base, SWITCHES_REG, &rc);

    uint32_t pressed = panel_pressed(now, before);
    uint32_t released = panel_released(now, before);

    printf("switches=0x%" PRIx32 " pressed=0x%" PRIx32 "\n", now, pressed);
    printf("switches=0x%" PRIx32 " released=0x%" PRIx32 "\n", now, released);

    return 0;
}
