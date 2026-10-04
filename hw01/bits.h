#ifndef BITS_H
#define BITS_H

#include <stdint.h>

void print_binary(uint32_t x, int width);
uint32_t get_field(uint32_t word, int pos, int width);
uint32_t set_field(uint32_t word, int pos, int width, uint32_t value);
int32_t sign_extend(uint32_t value, int width);

#endif
