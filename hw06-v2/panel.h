/*
 * panel.h:
 *
 * A generic device's switches and LEDs.
 */
#ifndef PANEL_H
#define PANEL_H

#include <stdint.h>

/**
 */
uint32_t panel_pressed(uint32_t now, uint32_t before);

/**
 */
uint32_t panel_released(uint32_t now, uint32_t before);

#endif
