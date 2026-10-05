#ifndef STATUS_H
#define STATUS_H

#include <stdbool.h>
#include <stdint.h>

#define MODE_OFF      0
#define MODE_HEAT     1
#define MODE_COOL     2
#define MODE_AUTO     3
#define MODE_FAN_ONLY 4

typedef struct {
    bool heat;
    bool cool;
    bool fan;
    bool fault;
    int  mode;        // raw 0..7; 5..7 are invalid
    int8_t setpoint;  // degrees C, two's complement (-128 to 127)
} status_t;

status_t status_unpack(uint16_t word);

#endif
