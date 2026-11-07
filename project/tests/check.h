/*
 * check.h:
 *
 * The CHECK macro shared by every test file. tests/test_main.c defines the
 * counters and prints the one summary line.
 */
#ifndef CHECK_H
#define CHECK_H

#include <stdio.h>  // IWYU pragma: keep (CHECK expands to printf)

extern int checks_total;
extern int checks_failed;

#define CHECK(cond) do {                                               \
    checks_total++;                                                    \
    if (cond) printf("(PASS) Test: %s\n", #cond);                      \
    else {                                                             \
        printf("(FAIL) Test: %s (line %d)\n", #cond, __LINE__);        \
        checks_failed++;                                               \
    }                                                                  \
} while (0)

void run_store_tests(void);
void run_fsm_tests(void);
void run_sensor_tests(void);
void run_status_tests(void);
void run_clock_tests(void);
void run_scenario_tests(void);
void run_replay_tests(void);

#endif
