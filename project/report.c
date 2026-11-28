/**
 * @file report.c
 * @brief The temperature report.
 *
 * The store is ordered by temperature, but a moving average needs the records
 * in time order. So the report walks the store, collects a pointer to every
 * record, and sorts the pointers by timestamp.
 */
#include <assert.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "report.h"
#include "sensor.h"
#include "store.h"

typedef struct {
    const record_t **records;
    size_t count;
} collection_t;

/* Helper Prototypes -------------------------------------------------------- */

static const record_t **records_in_time_order(
    const store_t *store, size_t count
);
static void collect(const record_t *record, void *context);
static int compare_by_time(const void *a, const void *b);
static int print_averages(
    const record_t **records, size_t count, size_t window, FILE *out
);

/* Public Functions --------------------------------------------------------- */

int report_print(const store_t *store, size_t window, FILE *out) {
    assert(store != NULL);
    assert(out != NULL);
    assert(window > 0);

    /* There's no average to print, but we didn't fail either. Return 0. */
    const size_t count = store_count(store);
    if (count < window) return 0;

    /* calloc failed. Return 1. */
    const record_t **records = records_in_time_order(store, count);
    if (records == NULL) return 1;

    /* Print averages and pass the return code. */
    const int rc = print_averages(records, count, window, out);
    free(records);

    return rc;
}

/* Helper Functions --------------------------------------------------------- */

/**
 * Get a pointer to every record in a store, sorted into time order.
 */
static const record_t **records_in_time_order(
    const store_t *store, size_t count
) {
    assert(store != NULL);
    assert(count > 0);

    const record_t **records = calloc(count, sizeof *records);
    if (records == NULL) return NULL;

    collection_t collection = {.records = records, .count = 0};
    store_walk(store, collect, &collection);
    assert(collection.count == count);

    qsort(records, count, sizeof *records, compare_by_time);

    return records;
}

/**
 * A store_walk callback that puts a pointer to each record in the next slot
 * of the collection given as context.
 */
static void collect(const record_t *record, void *context) {
    collection_t *collection = context;

    collection->records[collection->count] = record;
    collection->count++;
}

/**
 * A qsort comparison that orders record pointers by timestamp, earliest
 * first.
 */
static int compare_by_time(const void *a, const void *b) {
    const record_t *record_a = *(const record_t *const *)a;
    const record_t *record_b = *(const record_t *const *)b;

    const time_t time_a = record_a->reading.timestamp;
    const time_t time_b = record_b->reading.timestamp;

    return (time_a > time_b) - (time_a < time_b);
}

/**
 * Print one line for each record that ends a full window, giving the average
 * temperature of the window. The sum is kept in whole tenths of a degree, so
 * it stays exact as records enter and leave the window.
 */
static int print_averages(
    const record_t **records, size_t count, size_t window, FILE *out
) {
    assert(records != NULL);
    assert(out != NULL);
    assert(window > 0 && window <= count);

    const long w = (long)window;
    long sum = 0;

    for (size_t i = 0; i < count; i++) {
        sum += lroundf(records[i]->reading.temp_c * 10.0f);
        if (i >= window) {
            sum -= lroundf(records[i - window]->reading.temp_c * 10.0f);
        }

        if (i + 1 < window) continue;

        /* Divide, rounding halves away from zero. */
        const long average = (sum >= 0 ? sum + w / 2 : sum - w / 2) / w;

        char text[20];
        if (sensor_format_timestamp(records[i]->reading.timestamp, text) != 0) {
            return 1;
        }
        if (fprintf(out, "%s avg=%.1f\n", text, average / 10.0) < 0) return 1;
    }

    return 0;
}
