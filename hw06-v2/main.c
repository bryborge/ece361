#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

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

    iom361_set_display(false);

    /* the world, one switch reading per tick; a scenario file replaces it */
    const uint32_t switches[] = {0x0, 0x1, 0x1, 0x0, 0x1, 0x6};
    const size_t num_ticks = sizeof switches / sizeof switches[0];
    const time_t start = 1791273600;    /* 2026-10-06 08:00:00 UTC */

    uint32_t before = 0;

    for (size_t tick = 0; tick < num_ticks; tick++) {
        _iom361_set_switches(switches[tick]);
        uint32_t now = iom361_readReg(base, SWITCHES_REG, NULL);

        uint32_t pressed = panel_pressed(now, before);

        for (int k = 0; k < NUM_SWITCHES; k++) {
            if (pressed & (1u << k)) panel_toggle(base, k);
        }

        uint32_t leds = iom361_readReg(base, LEDS_REG, NULL);

        /* one second per tick; UTC, so the trace is the same in every time zone */
        time_t t = start + (time_t) tick;
        char when[32];
        strftime(when, sizeof when, "%Y-%m-%d %H:%M:%S", gmtime(&t));

        printf("%s switches=0x%" PRIx32 " pressed=0x%" PRIx32
               " leds=0x%" PRIx32 "\n", when, now, pressed, leds);

        before = now;
    }

    return 0;
}
