/*
 * clock.h:
 *
 * The device's clock is a time_t: seconds since the Unix epoch. The scenario
 * gives the start, and each tick adds a second. This turns a time into text.
 */
#ifndef CLOCK_H
#define CLOCK_H

#include <time.h>

/* Room for the text, and for the longest year gmtime can give. */
#define CLOCK_TEXT_SIZE 32

/**
 * Given a time `t`, stores it in `out` as `2026-10-06 08:00:46`, in UTC, so a
 * trace is the same on every computer.
 *
 * @return 0, or -1 and changes nothing if `t` can't be split into a date.
 */
int clock_format(time_t t, char out[CLOCK_TEXT_SIZE]);

#endif
