/*
 * panel.c:
 */
#include <stdint.h>

#include "panel.h"

/**
 */
uint32_t panel_pressed(uint32_t now, uint32_t before) {
    return now & ~before;
}

/**
 */
uint32_t panel_released(uint32_t now, uint32_t before) {
    return ~now & before;
}
