# HW6 PRD: Scenario replay

Source: *Homework 6, ECE 361, Fall 2026, Revision 0* (Nov 3, 2026), cited as
**[HW]**.

Requirements (sections 2 to 6) come from the assignment and are graded.
Section 7 gives implementation recommendations: they are the suggested way to
meet the requirements, not requirements themselves. Section 8 records what the
assignment leaves open and what has been decided. Section 9 gives the algorithms
those decisions lead to.

## 1. Overview

The part of the project that is the same for every device: reading the sensor
and the switches through iom361's registers, lighting LEDs without disturbing
the others, keeping the clock, and replaying a scenario file into a trace, one
line per tick. The device here is simple: a press of switch k toggles LED k. In
the project, a state machine takes its place. [HW]

Grading is Satisfactory / Not Yet. AI tools are allowed for the code. The Shine
lasts 10 to 15 minutes, is about your own code, and allows no supporting
materials. [HW]

**Who writes what.** We settled the design together. Claude writes the code and
the documentation in small pieces, test first (red, then green, then cleanup),
and stops after each one for review. The user reviews each piece and manages
git; Claude runs git only when the user hands a specific task over.

## 2. Deliverables

All in `hw06/`. [HW]

| File | Contents |
|------|----------|
| `sensor.h`, `sensor.c` | `reading_t` and `sensor_read` (section 3). |
| `panel.h`, `panel.c` | The four panel functions (section 4). |
| `scenario.h`, `scenario.c` | `scenario_t`, `scenario_load`, and the line in force at a tick (section 5.1). |
| `main.c` | The replay (section 5.2). |
| `iom361_r4.h`, `iom361_r4.c` | The course module, unmodified. |
| `tests/test_hw06.c` | The unit tests (section 6.1). |
| `scenarios/`, `expected/` | At least two scenarios, each with its expected trace (section 6.2). |
| `regress.sh` | Compares the replay with each expected trace with `diff`, as in HW2. |
| `Makefile` | The targets in section 6.3. |
| `README.md` | How to build and test. |
| `AI_USAGE.md` | Claude drafts it with the README, and the user reviews it. |

## 3. Part 1: The sensor

```c
typedef struct {
    time_t t;
    float temp_c;
    float rh_pct;
} reading_t;

int sensor_read(uint32_t *base, time_t now, reading_t *r);
```

`sensor_read` reads `TEMP_REG` and `HUMID_REG`, keeps bits 19..0 of each,
converts them with the formulas in `iom361_r4.h`, rounds to 0.1, and stores them
with the timestamp `now` in `*r`. It returns 0, or the iom361 return code of a
failed read, and then it changes nothing. [HW]

`sensor.c` has no `printf` and no calls to the `_iom361_set_...` test functions:
on a real board they would not exist. [HW]

The formulas, from `iom361_r4.h`:

```
Temp (degrees C) = (ST  / 2**20) * 200 - 50
Rel humidity (%) = (SRH / 2**20) * 100
```

## 4. Part 2: Switches and LEDs

| Function | What it does |
|----------|--------------|
| `uint32_t panel_pressed(uint32_t now, uint32_t before)` | The switches that went from off to on, one bit per switch. |
| `uint32_t panel_released(uint32_t now, uint32_t before)` | The switches that went from on to off. |
| `int panel_led(uint32_t *base, int n, int on)` | Turns LED `n` on or off by read-modify-write, so that every other LED keeps its state; `-1` if `n` is not 0 to 31. |
| `int panel_toggle(uint32_t *base, int n)` | Toggles LED `n`, the same way. |

[HW]

## 5. Part 3: The scenario and the loop

### 5.1 `scenario.h`, `scenario.c`

Load a scenario file into a `scenario_t` that holds at most 1000 lines, with no
`malloc`. The format is comments with `#`, a `start <timestamp>` line, then
`tick temp rh switches`, with the switches in hexadecimal and increasing ticks.
`scenario_load` returns 0, or the number of the first wrong line. A function
gives the line in force at a tick: the values hold between lines. [HW]

```
start 1791273600                # 2026-10-06 08:00:00 UTC
# tick  temp   rh    switches
0       15.0   60    0x0
5       15.0   60    0x1
```

The interface decided in section 8:

```c
#define SCENARIO_MAX_LINES 1000

typedef struct {
    int tick;
    float temp_c;
    float rh_pct;
    uint32_t switches;
} scenario_line_t;

typedef struct {
    time_t start;
    int count;
    scenario_line_t lines[SCENARIO_MAX_LINES];
} scenario_t;

int scenario_load(FILE *fp, scenario_t *s);
const scenario_line_t *scenario_at(const scenario_t *s, int tick);
```

### 5.2 `main.c`

The assignment runs it as `./replay scenario.txt`; this repo builds it as
`build/replay` (section 8.1). It initializes iom361 with 4 switches and 4 LEDs
and turns the display off. For every tick from 0 to the last one:

1. The line in force sets the "world" with `_iom361_set_sensor1` and
   `_iom361_set_switches`.
2. The clock is `start + tick`.
3. It reads the sensor and the switches through the registers.
4. It toggles LED k for every switch k just pressed.
5. It prints one line:

```
2026-10-06 08:00:02 temp=21.5 rh=40.0 switches=0x1 pressed=0x1 leds=0x1
```

The time is printed in UTC with `gmtime` and `strftime`, so a trace is the same
on every computer. [HW]

## 6. Part 4: Tests and a Makefile

### 6.1 `tests/test_hw06.c`

At minimum, with PASS or FAIL per test and a nonzero exit status on failure [HW]:

| # | Case |
|---|------|
| 1 | A reading of 91.0 °C reads back as 91.0. |
| 2 | A failed read changes nothing. |
| 3 | Pressed and released for a press, a release, and a held switch. |
| 4 | `panel_led` and `panel_toggle` keep the other LEDs. |
| 5 | A load with comments and blank lines. |
| 6 | Values holding between lines. |
| 7 | A scenario without a start line. |
| 8 | A scenario with a missing field. |
| 9 | A scenario with ticks that do not increase. |

Section 8.1 adds to this list.

### 6.2 Regression test

At least two scenarios in `scenarios/` with their expected traces in `expected/`.
`make test` runs the unit tests, then compares the replay on each scenario with
its expected trace, using a `regress.sh` with `diff`, as in HW2. [HW]

### 6.3 `Makefile`

| Target | Does |
|--------|------|
| `make` | Builds `replay` from all modules and `iom361_r4.c`, with no warnings. |
| `make test` | Runs the unit tests, then `regress.sh`. |
| `make clean` | Removes everything that was built. |

Each `.o` lists the headers it includes. [HW]

The Makefile is written and verified.

## 7. Implementation recommendations

- **Parsing numbers.** Use `strtoll` for `start`, `strtol` for `tick`, `strtod`
  for `temp` and `rh`, and `strtoul` in base 16 for `switches`. Each reports where
  it stopped, which is what catches `15.0abc`, and sets `errno` to `ERANGE` when
  the value does not fit.
- **`strtoul` and a minus sign.** `strtoul` accepts `-1` and wraps it to a huge
  value, so `switches` needs an explicit check for a leading `-`.
- **Hex in `temp`.** `strtod` also reads `0x10` as 16.0. It is in range and means
  what it says, so it stays accepted.
- **Too-long lines.** With `fgets`, a line is too long when the buffer is full and
  holds no newline. A last line with no newline that fits the buffer is fine, so
  the test also has to check for end of file.
- **Shifts.** Build LED masks with `1u << n`. `1 << 31` shifts into the sign bit of
  an `int`, which is undefined, and UBSan reports it.
- **Rounding.** Compute in `double`, as `round(x * 10.0) / 10.0`, and link with
  `-lm`.

## 8. Decisions beyond the spec

### 8.1 Decided

**Interfaces**

| Question | Decision | Why |
|----------|----------|-----|
| How `scenario_load` gets its input | `int scenario_load(FILE *fp, scenario_t *s)`. `main` owns `fopen`. | The sheet's contract, "0 or a line number", has no value for a file that won't open. Separates I/O from parsing, and tests can use `tmpfile()`. |
| The line in force | `scenario_at(const scenario_t *s, int tick)` returns `const scenario_line_t *`. A negative tick asserts. Past the last line, the last line holds. | The values hold between lines. A negative tick is a bug in the caller. |
| What the limit counts | 1000 data lines. Comments and blanks do not count. | They take no storage. |
| `*s` after a failed load | Unspecified, and must not be used. | The sheet asks "changes nothing" only of `sensor_read`, and every failure path exits. Rolling back would need a second buffer that nothing uses. |
| Where the `scenario_t` lives | A local in `main`, about 16 KB. | Fine on a laptop stack. No `malloc`, as required. |

**Scenario validation**

| Question | Decision | Why |
|----------|----------|-----|
| A tick before the first line | The first data tick must be 0, or that line is wrong. | Otherwise the trace depends on values the file never stated. The sheet's example and the project's scenarios all start at 0. A scenario starting later is rejected, which is the known cost. |
| Accepted forms | Blank lines; whole-line, indented and trailing comments; CRLF; hex with or without `0x`. | Accept what can be interpreted safely. The project's scenario files use trailing comments. |
| A wrong line | A data line before `start`; a second `start`; a missing or extra field; text that does not parse; a tick that does not increase, or a negative one; data line 1001; a line too long for the buffer. | Each cannot be safely interpreted. The return value is that line's number, counting every physical line, so it matches what an editor shows. |
| Nothing to point at | An empty file, only comments, or `start` with no data returns one past the last line. An empty file returns 1. | The failure must be nonzero to be reported, and this points at where the missing thing was expected. |
| `temp`, `rh` ranges | `temp` from -50 to 150 and `rh` from 0 to 100, both inclusive. Outside that is a wrong line, as are `nan` and `inf`. | iom361 clamps silently: 200 °C reads back as 150.0, which would hide a typo. Exactly 150 and 100 read back correctly (tested). |
| `switches`, `tick` ranges | `switches` must fit 32 bits and `tick` must fit an `int`. | They are stored in those types. |
| Switch bits above 3 | Accepted. iom361 clears them, and the trace shows the masked value. | `scenario.c` does not know the switch count, and those switches do not exist on this board. |
| `start` range | From 0 to `253402300799` (9999-12-31 23:59:59 UTC), as a named constant with a comment. | A value near `LLONG_MAX` would overflow `start + tick`, which is undefined. Adding `INT_MAX` to the cap reaches only year 10068, which `gmtime` handles (tested). |

**Sensor and panel**

| Question | Decision | Why |
|----------|----------|-----|
| `-0.0` | `sensor_read` adds `0.0f` after rounding, with a comment. | `-0.04` otherwise prints `temp=-0.0` (tested), because `round` keeps the sign. Traces are compared as text, and temperatures near zero are likely in the project's freeze guard. |
| Panel failures | `n` is checked first and returns `-1`. A failed read or write returns the iom361 code. A failed read means no write. | The codes (1, 2, 3, 5) cannot collide with `-1`. A failed read returns `0xDEADBEEF`, which must never be written back as LED state. |

**Replay**

| Question | Decision | Why |
|----------|----------|-----|
| `before` at tick 0 | Read from the switch register before the loop, which gives 0. A switch on at tick 0 counts as a press. | The board did start with that switch off, so it is a real edge. The sheet's sample shows `pressed=0x1` at tick 2, so it does not settle this. |
| Stopping the loop | `if (tick == last) break` in the body, with a comment. | A tick of `INT_MAX` is accepted, so `tick <= last` could never be false, and incrementing past it is signed overflow. |
| Trace format | `"%s temp=%.1f rh=%.1f switches=0x%x pressed=0x%x leds=0x%x\n"`, with lowercase, unpadded hex. The LEDs are read after toggling. | The sheet's sample only shows `0x1`, so it does not settle case or padding. A line shows the LEDs as its tick leaves them. |
| Trace length | No cap. A tick of 2 billion prints 2 billion lines. | It is what the file asks for. |
| Where the binary is | `build/replay`, not `./replay`. `regress.sh` and the README use that path. | The user's accepted deviation from the sheet: everything the build produces stays in `build/`. |
| The `gmtime` NULL check | Stays, with a comment citing the C standard. The trace buffer fits a 5-digit year. | With the `start` cap it cannot fail on a 64-bit `time_t`, but the standard allows NULL, and passing it to `strftime` would crash. |

Failures in `main` print one line on stderr and exit with status 1. stdout holds
only the trace, and a partial trace stays if the run stops early.

| Failure | Message |
|---------|---------|
| Wrong argument count | `usage: replay scenario.txt` |
| File will not open | `replay: scenarios/x.txt: No such file or directory` |
| Load failure | `replay: scenarios/x.txt: line 7: invalid scenario line` |
| `iom361_initialize` fails, or a register or panel call fails mid-run | `replay: tick 12: register access failed (code 1)` |
| `gmtime` returns NULL | `replay: tick 12: time out of range` |

The load message cannot give a reason, because `scenario_load` returns only a line
number. That is the cost of keeping the sheet's contract.

**Tests**

| Question | Decision | Why |
|----------|----------|-----|
| The harness | hw05's `CHECK` macro, one PASS or FAIL line per check, and a summary. | The repo's convention. |
| Added sensor tests | The bit mask, by writing directly to `base[TEMP_REG / 4]`; `-0.04` reads as `0.0`, with its sign checked by `signbit`; a wrong `base` leaves a sentinel `reading_t` unchanged. | `_iom361_set_sensor1` cannot set bits 31..20, and writes to that register are ignored. `-0.0 == 0.0`, so only `signbit` can tell them apart. |
| Added panel tests | `n` of -1 and 32 returns `-1`; `n` of 31, where UBSan catches a signed shift; a wrong `base` returns its code and leaves the LEDs unchanged. | These guard the boundaries. |
| Added loader tests | Each wrong-line rule, with its exact line number; each range edge, accepted and rejected; exactly 1000 data lines pass and 1001 fail. | The line number is the contract, so a nonzero result is not enough. |
| Checking the tests | Design-guarding tests run against deliberately broken copies of the code. One-off runs, kept out of the repo. | Shows the tests catch the bug, as in hw05. |
| `main`'s failure paths | Run by hand and reported, not in `make test`. | The sheet asks only for a comparison of the trace, and `regress.sh` compares stdout. This is a known gap. |
| Regression scenarios | `toggle.txt`: press, hold and release; two switches together; a switch on at tick 0; values holding across gaps. `edges.txt`: a start at 23:59:58 crossing midnight; temperatures of -50, -0.04 and 150; RH of 0 and 100; switch bits above 3. Each about 15 ticks or fewer. | Together they reach every printed field and format decision above. |
| Expected traces | Written by hand from the sheet, before `main.c` exists. | Generating them from `replay` would only prove that the program agrees with itself. |

**Process**

| Question | Decision | Why |
|----------|----------|-----|
| Build order | PRD; `panel.c`; `sensor.c`; `scenario.c`; scenarios and traces; `main.c`; `README.md` and `AI_USAGE.md`. Tests first in each code chunk. | Each part is checked before they are joined, and `main.c` is the one step that is hard to diagnose if an earlier piece is off. `make test` stays red until `main.c` is done. |
| `docs/algorithms.md` | Deleted once this PRD absorbed it (section 9). | It would only duplicate this file. |

### 8.2 Still open

Nothing.

## 9. Algorithms

### 9.1 Load a scenario

```
FUNCTION scenario_load(input, scenario)
    SET line_number TO 0
    SET seen_start TO false
    SET count TO 0

    WHILE another line can be read from input DO
        INCREMENT line_number
        IF the line is too long for the read buffer THEN
            RETURN line_number
        END IF

        REMOVE everything from the first # to the end of the line
        IF only whitespace remains THEN
            GO TO the next line
        END IF

        IF seen_start is false THEN
            IF the line is not exactly "start" and a timestamp THEN
                RETURN line_number
            END IF
            IF the timestamp is negative or after 253402300799 THEN
                RETURN line_number
            END IF
            STORE the timestamp in scenario
            SET seen_start TO true
            GO TO the next line
        END IF

        IF the line is not exactly tick, temp, rh, switches THEN
            RETURN line_number
        END IF
        IF tick does not fit an int THEN
            RETURN line_number
        END IF
        IF temp is not from -50 to 150 OR rh is not from 0 to 100 THEN
            RETURN line_number
        END IF
        IF switches does not fit 32 bits THEN
            RETURN line_number
        END IF
        IF count is 0 AND tick is not 0 THEN
            RETURN line_number
        END IF
        IF count is above 0 AND tick is not above the previous tick THEN
            RETURN line_number
        END IF
        IF count equals the capacity of scenario THEN
            RETURN line_number
        END IF

        STORE the fields as line number count of scenario
        INCREMENT count
    END WHILE

    IF count is 0 THEN
        RETURN line_number + 1
    END IF
    STORE count in scenario
    RETURN 0
END FUNCTION
```

Three rules need no code of their own:

- A data line before `start`, and a second `start`, each fail the exact-shape
  check of the branch they reach.
- Requiring 0 first and increasing after also rejects negative ticks.
- The final `count is 0` return covers an empty file, a file of only comments,
  and `start` with no data.

Written as "from -50 to 150", the range check also rejects `nan`, which fails every
comparison, and `inf`.

### 9.2 Find the line in force at a tick

```
FUNCTION scenario_at(scenario, tick)
    STOP with an assertion IF tick is negative
    SET found TO the first line
    FOR each later line, in order DO
        IF the line's tick is above tick THEN
            RETURN found
        END IF
        SET found TO the line
    END FOR
    RETURN found
END FUNCTION
```

**Why a linear scan, not a binary search.** A scenario holds at most 1000 lines,
so a tick costs at most 1000 comparisons, and no measurement or requirement calls
for less. The scan is also half the code, with no boundary arithmetic to get
wrong. A binary search can replace it behind the same function if a measurement
ever says so. A cursor kept in `main` would be faster still, but it would not meet
the sheet's "a function gives the line in force at a tick".

A loaded scenario always starts at tick 0, so `found` always exists for a tick
that is not negative.

### 9.3 Replay a scenario

```
FUNCTION replay(scenario, base)
    SET before TO the switch register, read once before the loop
    SET last TO the tick of the scenario's last line
    SET tick TO 0

    REPEAT forever
        SET line TO the line in force at tick
        SET the world from line: temperature, humidity, switches

        SET clock TO scenario start + tick
        READ the sensor through its registers, stamped with clock
        READ the switch register into now

        SET pressed TO the switches that went from off to on
        FOR k FROM 0 TO the number of switches - 1 DO
            IF bit k of pressed is set THEN
                TOGGLE LED k
            END IF
        END FOR

        READ the LED register into leds
        PRINT one trace line from clock, reading, now, pressed, leds

        SET before TO now
        STOP the loop IF tick equals last
        INCREMENT tick
    END REPEAT
END FUNCTION
```

## 10. Acceptance criteria (Satisfactory)

- [ ] Every deliverable in section 2 is in the repository.
- [ ] `make` builds with no warnings.
- [ ] `make test` passes: the unit tests, then every regression scenario.
- [ ] `make clean` works.
- [ ] `sensor.c` has no `printf` and no `_iom361_set_...` calls.
- [ ] `scenario.c` has no `malloc`.
- [ ] The Shine is completed and meets the bar.

[HW, Deliverable, Shine, and requirements]

## 11. Out of scope

The project's next step: copy `sensor.c`, the scenario code and the loop into
`project/`, replace the toggling with the state machine, and print the device's
trace format. It isn't graded and isn't on the Shine. [HW]

Two differences to handle then:

- `project/sensor.c` does not keep only bits 19..0, so it should not be the source
  for anything here.
- The sheet names the timestamp `t`, and `project/sensor.h` names it `timestamp`.
  One side needs a rename.

## 12. Shine preparation

Sample question from the assignment [HW]:

- "Why does your `panel_led` read the register before it writes it, and what goes
  wrong if it doesn't?"

  **Why it reads first.** `LEDS_REG` is one 32-bit word that holds all the LEDs at
  once, and a write replaces the whole word. There is no way to write a single
  bit. So changing one LED takes three steps: read the word, change that one bit,
  write the word back. The other 31 bits go back with the values they already had.

  **What goes wrong without it.** Writing `1u << n` alone sets LED `n` and turns
  every other LED off. With LEDs 0 and 2 on (`0x5`), turning on LED 1 writes `0x2`
  instead of `0x7`, and LEDs 0 and 2 go dark. A trace shows it right away: `leds=`
  never has more than one bit set.

  **The failure case.** A failed read returns `0xDEADBEEF`. If `panel_led` wrote
  that back with one bit changed, it would be recording an error marker as LED
  state. So it checks the read's return code and does not write.

Questions the design invites:

- Why does `sensor_read` compute into local variables and store to `*r` last?
  (Because a failed second read must leave `*r` unchanged, even though the first
  read succeeded.)
- Why keep bits 19..0, when `_iom361_set_sensor1` can't set the others? (The header
  defines only those 20 bits as the value, and the driver should not depend on the
  rest being 0. The test has to write to the register directly to reach this.)
- Why does a reading of 91.0 need rounding? (The register holds a 20-bit integer,
  so 91.0 comes back as 90.99998, which would be below a 91.0 threshold.)
- Why add `0.0f` after rounding? (`round` keeps the sign, so -0.04 becomes -0.0 and
  prints with a minus sign. In IEEE 754, -0.0 + 0.0 is +0.0.)
- What is the expression for `panel_pressed`, and why? (`now & ~before`: on now, and
  off before. `panel_released` is `~now & before`. A held switch is on in both, so
  it is in neither.)
- Why `1u << n` and not `1 << n`? (At `n` of 31, `1 << 31` shifts into the sign bit
  of an `int`, which is undefined.)
- Why does `main` print the time with `gmtime` and not `localtime`? (A trace must be
  the same on every computer, whatever its time zone, or `diff` would fail on
  someone else's machine.)
- Why does `scenario_load` take a `FILE *`? (So its return value is only ever 0 or a
  line number, as the sheet says, and tests can feed it text through `tmpfile()`.)
- What does a line in force mean, and why must the first tick be 0? (A line's values
  hold until the next line. Before the first line nothing is in force, so the first
  tick must be 0 for the replay's tick 0 to have values.)
- Why a linear scan in `scenario_at`? (Section 9.2.)
- Why is the loop's stop test in the body? (A tick of `INT_MAX` is accepted, and
  `tick <= last` could never be false.)
- Why are the expected traces written by hand? (A trace generated from `replay` only
  proves the program agrees with itself.)
