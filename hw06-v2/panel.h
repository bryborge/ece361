/*
 * panel.h:
 *
 * A generic device's switches and LEDs.
 */
#ifndef PANEL_H
#define PANEL_H

#include <stdint.h>

/**
 * Given two readings of the switch register, returns the switches that went
 * from off to on, one bit per switch.
 */
uint32_t panel_pressed(uint32_t now, uint32_t before);

/**
 * Given two readings of the switch register, returns the switches that went
 * from on to off, one bit per switch.
 */
uint32_t panel_released(uint32_t now, uint32_t before);

/**
 * Toggles LED n. Every other LED keeps its state.
 *
 * @return 0, -1 if n is not 0 to 31, or the iom361 return code of a failed
 *         register access.
 */
int panel_toggle(uint32_t *base, int n);

#endif
