#include "iom361_r4.h"
#include "panel.h"

uint32_t panel_pressed(uint32_t now, uint32_t before) {
    return now & ~before;
}

uint32_t panel_released(uint32_t now, uint32_t before) {
    return ~now & before;
}

int panel_led(uint32_t *base, int n, int on) {
    if (n < 0 || n > 31) return -1;

    /* Read first to avoid writing over the other LEDs' state. */
    int rc;
    uint32_t leds = iom361_readReg(base, LEDS_REG, &rc);
    if (rc != 0) return rc;

    /* Unsigned: 1 << 31 would shift into the sign bit of an int. */
    uint32_t mask = 1u << n;
    if (on) {
        leds |= mask;
    } else {
        leds &= ~mask;
    }

    iom361_writeReg(base, LEDS_REG, leds, &rc);
    return rc;
}

int panel_toggle(uint32_t *base, int n) {
    if (n < 0 || n > 31) return -1;

    /* Read first to avoid writing over the other LEDs' state. */
    int rc;
    uint32_t leds = iom361_readReg(base, LEDS_REG, &rc);
    if (rc != 0) return rc;

    iom361_writeReg(base, LEDS_REG, leds ^ (1u << n), &rc);
    return rc;
}
