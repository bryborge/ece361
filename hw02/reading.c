/*
 * reading.c: reading sensor readings from standard input.
 */
#include <stdio.h>

#include "reading.h"

#define LINE_LEN     128

/* 1 if line holds only spaces and tabs, or its first non-blank character is '#' */
static int is_blank_or_comment(const char line[]) {
    int i = 0;
    while (line[i] == ' ' || line[i] == '\t')
        i++;
    return line[i] == '\n' || line[i] == '\0' || line[i] == '#';
}

struct reading_counts reading_read(int ticks[], float temps[], float hums[], int max) {
    struct reading_counts c = {0, 0};
    char line[LINE_LEN];

    while (fgets(line, sizeof line, stdin) != NULL) {
        if (is_blank_or_comment(line))
            continue;
        if (c.count == max) {
            fprintf(stderr, "warning: more than %d readings, the rest are ignored\n", max);
            break;
        }
        if (sscanf(line, "%d %f %f", &ticks[c.count], &temps[c.count], &hums[c.count]) == 3)
            c.count++;
        else
            c.skipped++;
    }
    return c;
}
