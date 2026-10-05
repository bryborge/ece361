#include <stdint.h>

#include "bits.h"
#include "status.h"

// Positions
#define HEAT_POS       0
#define COOL_POS       1
#define FAN_POS        2
#define FAULT_POS      3
#define MODE_POS       4
#define SETPOINT_POS   8
// Widths
#define HEAT_WIDTH     1
#define COOL_WIDTH     1
#define FAN_WIDTH      1
#define FAULT_WIDTH    1
#define MODE_WIDTH     3
#define SETPOINT_WIDTH 8

/**
 * Decodes a 16-bit thermostat status word into its fields.
 *
 * `mode` holds the raw three-bit value. The invalid codes (5..7) are
 * returned as they appear in the word; callers check `mode <= MODE_FAN_ONLY`.
 * Bit 7 is reserved and is never read, so a word with it set decodes to the
 * same fields as the same word with it clear.
 *
 * @param word Packed status word.
 * @return The decoded fields.
 */
status_t status_unpack(uint16_t word) {
    uint32_t setpoint_bits = get_field(word, SETPOINT_POS, SETPOINT_WIDTH);

    status_t status = {
        .heat     = get_field(word, HEAT_POS, HEAT_WIDTH),
        .cool     = get_field(word, COOL_POS, COOL_WIDTH),
        .fan      = get_field(word, FAN_POS, FAN_WIDTH),
        .fault    = get_field(word, FAULT_POS, FAULT_WIDTH),
        .mode     = get_field(word, MODE_POS, MODE_WIDTH),
        .setpoint = sign_extend(setpoint_bits, SETPOINT_WIDTH),
    };

    return status;
}
