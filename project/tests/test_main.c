/*
 * test_main.c:
 *
 * Runs every module's tests, then prints one line with the number passed and
 * failed, as `make test` must end with. Exits nonzero if any check failed.
 */
#include <stdio.h>

#include "check.h"

int checks_total;
int checks_failed;

int main(void) {
    run_store_tests();
    run_fsm_tests();
    run_sensor_tests();
    run_status_tests();
    run_clock_tests();

    printf("\n%d passed, %d failed\n", checks_total - checks_failed, checks_failed);
    return checks_failed != 0;
}
