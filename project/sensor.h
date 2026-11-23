/**
 * @file sensor.h
 * @brief The temperature and humidity sensor.
 */
#ifndef SENSOR_H
#define SENSOR_H

#include <stdint.h>
#include <time.h>

typedef struct {
    time_t timestamp;
    float temp_c;
    float rh_pct;
} reading_t;

/**
 * Take a sensor reading. Temperature and humidity are read from the registers
 * defined in iom361, simulating a memory-mapped AHT20 device. These sensor
 * values are rounded to 0.1. On failure, *reading is not written.
 *
 * @param base    The base address of the iom361 register block.
 * @param now     Time of this reading, UTC seconds since the Unix epoch.
 * @param reading Where the record is stored.
 * @return 0 on success, or else the iom361 return code of the register read
 *         that fails.
 */
int sensor_read(uint32_t *base, time_t now, reading_t *reading);

/**
 * Format a timestamp as "YYYY-MM-DD HH:MM:SS" in UTC. On failure, text is not
 * written.
 *
 * @param timestamp UTC seconds since the Unix epoch.
 * @param text      Where the text is stored.
 * @return 0 on success, or 1 if the timestamp is outside the range of dates
 *         that can be formatted.
 */
int sensor_format_timestamp(time_t timestamp, char text[20]);

#endif
