/*
 * bits.h:
 *
 * Reads and writes bit fields in a word. Copied unchanged from hw01, as the
 * sheet asks.
 */
#ifndef BITS_H
#define BITS_H

#include <stdint.h>

uint32_t get_field(uint32_t word, int pos, int width);
uint32_t set_field(uint32_t word, int pos, int width, uint32_t value);

#endif
