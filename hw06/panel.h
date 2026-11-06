#ifndef PANEL_H
#define PANEL_H

#include <stdint.h>

uint32_t panel_pressed(uint32_t now, uint32_t before);
uint32_t panel_released(uint32_t now, uint32_t before);
int panel_led(uint32_t *base, int n, int on);
int panel_toggle(uint32_t *base, int n);

#endif
