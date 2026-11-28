/**
 * @file report.h
 * @brief The temperature report.
 */
#ifndef REPORT_H
#define REPORT_H

#include <stddef.h>
#include <stdio.h>

#include "store.h"

/**
 * Print the moving average of the temperature, with records taken in time
 * order. Asserts that store and out are not NULL and that window is not 0.
 *
 * @param store  The store to read.
 * @param window The number of readings to average over.
 * @param out    Where the report is printed.
 * @return 0 on success, or 1 if memory could not be allocated or the report
 *         could not be printed.
 */
int report_print(const store_t *store, size_t window, FILE *out);

#endif
