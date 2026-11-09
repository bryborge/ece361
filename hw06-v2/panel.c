/*
 * panel.c:
 *
 * Edge detection on switch readings, and single-LED changes by
 * read-modify-write of LEDS_REG.
 */
#include <stdint.h>

#include "iom361_r4.h"
#include "panel.h"

/**
 * A switch was pressed if it is on now and was off before.
 */
uint32_t panel_pressed(uint32_t now, uint32_t before) {
    return now & ~before;
}

/**
 * The mirror of panel_pressed: on before, off now.
 */
uint32_t panel_released(uint32_t now, uint32_t before) {
    return ~now & before;
}

/**
 * Read-modify-write: the LED register holds every LED, so writing 1u << n
 * alone would turn all the others off. XOR flips bit n and leaves the others
 * as they are.
 */
int panel_toggle(uint32_t *base, int n) {
    if (n < 0 || n > 31) return -1;

    int rc;
    uint32_t leds = iom361_readReg(base, LEDS_REG, &rc);
    if (rc != 0) return rc;

    leds ^= 1u << n;

    iom361_writeReg(base, LEDS_REG, leds, &rc);
    return rc;
}

/**
 * The same read-modify-write as panel_toggle. OR sets bit n, and AND with the
 * inverted mask clears it.
 */
int panel_led(uint32_t *base, int n, int on) {
    if (n < 0 || n > 31) return -1;

    int rc;
    uint32_t leds = iom361_readReg(base, LEDS_REG, &rc);
    if (rc != 0) return rc;

    if (on) leds |= 1u << n;
    else    leds &= ~(1u << n);

    iom361_writeReg(base, LEDS_REG, leds, &rc);
    return rc;
}
