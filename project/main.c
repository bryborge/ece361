/**
 * @file main.c
 * @brief The garage door opener entrypoint.
 */
#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "iom361_r4.h"

#include "aux.h"
#include "fsm.h"
#include "report.h"
#include "sensor.h"
#include "store.h"

/* Switch register bits. */
#define SW_REMOTE        (1u << 0)
#define SW_LIGHT_BARRIER (1u << 1)

/* LED register bits. */
#define LED_MOTOR_UP     (1u << 0)
#define LED_MOTOR_DOWN   (1u << 1)

/* The number of history entries printed on an obstruction. */
#define OBSTRUCTION_HISTORY 3

/* The number of readings the report averages over. */
#define REPORT_WINDOW 20

/* History kinds, as printed. */
static const char *const history_kind_names[] = {
    [H_PRESS]       = "PRESS",
    [H_REFUSAL]     = "REFUSAL",
    [H_OBSTRUCTION] = "OBSTRUCTION",
};

/* Scenario file ------------------------------------------------------------ */

#define LINE_SIZE 256

/* A scenario file that is being read. */
typedef struct {
    const char *path;
    FILE *file;
    int line_number;
} scenario_t;

/* A change to the world. From its tick on, the values hold until the next. */
typedef struct {
    int tick;
    float temp_c;
    float rh_pct;
    uint32_t switches;
} world_t;

/**
 * Report a problem with the line of the scenario that was read last.
 */
static void scenario_error(const scenario_t *scenario, const char *message) {
    fprintf(stderr, "%s:%d: %s\n",
        scenario->path, scenario->line_number, message);
}

/**
 * Check that only whitespace is left in a string.
 */
static bool is_blank(const char *text) {
    while (isspace((unsigned char)*text)) text++;

    return *text == '\0';
}

/**
 * Read the next line of a scenario that holds content. A comment runs from
 * '#' to the end of its line and is removed. Lines left blank are skipped.
 *
 * @return 1 if a line was read, 0 at the end of the file, or -1 after
 *         reporting an error.
 */
static int scenario_read_line(scenario_t *scenario, char line[LINE_SIZE]) {
    while (fgets(line, LINE_SIZE, scenario->file) != NULL) {
        scenario->line_number++;

        if (strchr(line, '\n') == NULL && !feof(scenario->file)) {
            scenario_error(scenario, "the line is too long");
            return -1;
        }

        char *comment = strchr(line, '#');
        if (comment != NULL) *comment = '\0';

        if (!is_blank(line)) return 1;
    }

    if (ferror(scenario->file)) {
        scenario_error(scenario, "the file could not be read");
        return -1;
    }

    return 0;
}

/**
 * Read the start line of a scenario: the word "start" and a Unix timestamp.
 *
 * @return true on success, or false after reporting an error.
 */
static bool scenario_read_start(scenario_t *scenario, time_t *start) {
    char line[LINE_SIZE];
    if (scenario_read_line(scenario, line) != 1) {
        if (!ferror(scenario->file)) {
            scenario_error(scenario, "expected \"start\" and a timestamp");
        }
        return false;
    }

    const char *text = line;
    while (isspace((unsigned char)*text)) text++;

    char *end;
    errno = 0;
    const long long seconds =
        strncmp(text, "start", 5) == 0 && isspace((unsigned char)text[5])
            ? strtoll(text + 5, &end, 10)
            : -1;

    if (seconds < 0 || errno != 0 || end == text + 5 || !is_blank(end)) {
        scenario_error(scenario, "expected \"start\" and a timestamp");
        return false;
    }

    *start = (time_t)seconds;
    return true;
}

/**
 * Read the next change of a scenario: a tick, a temperature, a humidity, and
 * the switch register in hexadecimal.
 *
 * @return 1 if a change was read, 0 at the end of the file, or -1 after
 *         reporting an error.
 */
static int scenario_read_change(scenario_t *scenario, world_t *change) {
    char line[LINE_SIZE];
    const int rc = scenario_read_line(scenario, line);
    if (rc != 1) return rc;

    char *end;
    errno = 0;

    const long tick = strtol(line, &end, 10);
    bool valid = end != line && tick >= 0 && tick <= INT_MAX;

    const char *text = end;
    const float temp_c = strtof(text, &end);
    valid = valid && end != text && isfinite(temp_c);

    text = end;
    const float rh_pct = strtof(text, &end);
    valid = valid && end != text && isfinite(rh_pct);

    text = end;
    const unsigned long switches = strtoul(text, &end, 16);
    valid = valid && end != text && switches <= UINT32_MAX;

    if (!valid || errno != 0 || !is_blank(end)) {
        scenario_error(scenario,
            "expected a tick, a temperature, a humidity, and the switches");
        return -1;
    }

    *change = (world_t){
        .tick     = (int)tick,
        .temp_c   = temp_c,
        .rh_pct   = rh_pct,
        .switches = (uint32_t)switches,
    };
    return 1;
}

/* Replay ------------------------------------------------------------------- */

/**
 * Run the device for one tick: read the inputs, run the state machine, write
 * the outputs, store a record, record what happened in the history, and, with
 * trace true, print a trace line.
 *
 * @return 0 on success, or 1 after reporting an error.
 */
static int run_tick(
    uint32_t *base, fsm_t *fsm, store_t *store, history_t *history,
    time_t start, int tick, bool trace
) {
    const time_t now = start + tick;

    /* Read the inputs. */
    reading_t reading;
    int rc = sensor_read(base, now, &reading);
    if (rc != 0) {
        fprintf(stderr, "sensor_read failed: %d\n", rc);
        return 1;
    }

    const uint32_t switches = iom361_readReg(base, SWITCHES_REG, &rc);
    if (rc != 0) {
        fprintf(stderr, "reading the switches failed: %d\n", rc);
        return 1;
    }

    /* Run the state machine. */
    const fsm_inputs_t inputs = {
        .remote_on       = (switches & SW_REMOTE) != 0,
        .barrier_blocked = (switches & SW_LIGHT_BARRIER) != 0,
        .temp_c          = reading.temp_c,
    };
    fsm_report_t report;
    fsm_tick(fsm, &inputs, &report);

    /* Write the outputs. */
    const fsm_outputs_t outputs = fsm_outputs(fsm->state);
    const uint32_t leds = (outputs.motor_up ? LED_MOTOR_UP : 0u)
                        | (outputs.motor_down ? LED_MOTOR_DOWN : 0u);

    iom361_writeReg(base, LEDS_REG, leds, &rc);
    if (rc != 0) {
        fprintf(stderr, "writing the LEDs failed: %d\n", rc);
        return 1;
    }

    iom361_writeReg(base, RGB_LED_REG,
        build_rgb_reg(1, outputs.red, outputs.green, outputs.blue), &rc);
    if (rc != 0) {
        fprintf(stderr, "writing the RGB LED failed: %d\n", rc);
        return 1;
    }

    /* Store a record of the tick. */
    const uint32_t status = fsm_status_pack((fsm_status_t){
        .state               = fsm->state,
        .motor_up            = outputs.motor_up,
        .motor_down          = outputs.motor_down,
        .freeze_guard_active = report.freeze_guard_active,
    });

    const record_t record = {
        .reading = reading,
        .state   = fsm->state,
        .status  = status,
    };
    if (store_insert(store, &record) != 0) {
        fprintf(stderr, "storing the record failed: out of memory\n");
        return 1;
    }

    /* Record what happened in the order fsm_tick handled it. */
    if (report.obstruction_detected) {
        const history_entry_t entry = { .kind = H_OBSTRUCTION, .tick = tick };
        if (history_push(history, entry) != 0) {
            fprintf(stderr,
                "recording the obstruction failed: out of memory\n");
            return 1;
        }
    }

    if (report.remote_pressed) {
        const history_entry_t entry = { .kind = H_PRESS, .tick = tick };
        if (history_push(history, entry) != 0) {
            fprintf(stderr, "recording the press failed: out of memory\n");
            return 1;
        }
    }

    if (report.press_refused) {
        const history_entry_t entry = { .kind = H_REFUSAL, .tick = tick };
        if (history_push(history, entry) != 0) {
            fprintf(stderr, "recording the refusal failed: out of memory\n");
            return 1;
        }
    }

    /* Everything from here on only prints. */
    if (!trace) return 0;

    /* Print the trace line. */
    char timestamp[20];
    rc = sensor_format_timestamp(reading.timestamp, timestamp);
    if (rc != 0) {
        fprintf(stderr, "formatting the timestamp failed: %d\n", rc);
        return 1;
    }

    printf("%s state=%s temp=%.1f rh=%.1f leds=0x%" PRIX32
        " status=0x%08" PRIX32 "\n",
        timestamp, fsm_state_name(fsm->state), reading.temp_c,
        reading.rh_pct, leds, status);

    /*
     * On an obstruction, print the most recent entries, top first. Popping is
     * the only way to see below the top, so they are pushed back afterwards
     * in reverse order, leaving the history as it was.
     */
    if (report.obstruction_detected) {
        history_entry_t last[OBSTRUCTION_HISTORY];
        size_t popped = 0;

        while (popped < OBSTRUCTION_HISTORY
               && history_pop(history, &last[popped]) == 0) {
            popped++;
        }

        for (size_t i = 0; i < popped; i++) {
            printf("  history tick=%d event=%s\n",
                last[i].tick, history_kind_names[last[i].kind]);
        }

        while (popped > 0) {
            popped--;
            if (history_push(history, last[popped]) != 0) {
                fprintf(stderr,
                    "restoring the history failed: out of memory\n");
                return 1;
            }
        }
    }

    return 0;
}

/**
 * Replay a scenario from its start line to the tick of its last change. With
 * trace false, nothing is printed: the replay only fills the store and the
 * history.
 *
 * @return 0 on success, or 1 after reporting an error.
 */
static int replay(
    scenario_t *scenario, uint32_t *base, store_t *store, history_t *history,
    bool trace
) {
    time_t start;
    if (!scenario_read_start(scenario, &start)) return 1;

    world_t next;
    int more = scenario_read_change(scenario, &next);
    fsm_t fsm = fsm_init();

    for (int tick = 0; more == 1; tick++) {
        /* Play the world. */
        if (next.tick == tick) {
            _iom361_set_sensor1(next.temp_c, next.rh_pct);
            _iom361_set_switches(next.switches);

            more = scenario_read_change(scenario, &next);
            if (more == -1) return 1;

            if (more == 1 && next.tick <= tick) {
                scenario_error(scenario, "the tick must be after the last");
                return 1;
            }
        }

        if (run_tick(base, &fsm, store, history, start, tick, trace) != 0) {
            return 1;
        }
    }

    return more == 0 ? 0 : 1;
}

/**
 * Program entry point.
 */
int main(int argc, char *argv[]) {
    const bool report_mode = argc == 3 && strcmp(argv[2], "--report") == 0;
    if (argc != 2 && !report_mode) {
        fprintf(stderr, "usage: %s SCENARIO [--report]\n", argv[0]);
        return 1;
    }

    int rc;
    uint32_t *base = iom361_initialize(2, 2, &rc);

    if (base == NULL) {
        fprintf(stderr, "iom361_initialize failed: %d\n", rc);
        return 1;
    }

    /* Keep the model's LED display out of the trace. */
    iom361_set_display(false);

    scenario_t scenario = {.path = argv[1], .file = fopen(argv[1], "r")};
    if (scenario.file == NULL) {
        fprintf(stderr, "%s: %s\n", scenario.path, strerror(errno));
        return 1;
    }

    history_t *history = history_create();
    if (history == NULL) {
        fprintf(stderr, "creating the history failed: out of memory\n");
        fclose(scenario.file);
        return 1;
    }

    store_t *store = store_create();
    if (store == NULL) {
        fprintf(stderr, "creating the store failed: out of memory\n");
        history_destroy(history);
        fclose(scenario.file);
        return 1;
    }

    rc = replay(&scenario, base, store, history, !report_mode);

    if (rc == 0 && report_mode) {
        if (report_print(store, REPORT_WINDOW, stdout) != 0) {
            fprintf(stderr, "printing the report failed\n");
            rc = 1;
        }
    }

    store_destroy(store);
    history_destroy(history);
    fclose(scenario.file);

    return rc;
}
