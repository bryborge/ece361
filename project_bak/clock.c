#include <stddef.h>

#include "clock.h"

/**
 * Formats a time in UTC.
 *
 * @param t   The time.
 * @param out Where to store the text.
 * @return 0, or -1 and changes nothing if gmtime can't split t. The C standard
 *         allows that, and strftime would crash on the NULL.
 */
int clock_format(time_t t, char out[CLOCK_TEXT_SIZE]) {
    const struct tm *utc = gmtime(&t);

    if (utc == NULL) return -1;

    strftime(out, CLOCK_TEXT_SIZE, "%Y-%m-%d %H:%M:%S", utc);
    return 0;
}
