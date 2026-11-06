# Homework 6: Scenario Replay

> In Homework 6, we learn about memory-mapped I/O through iom361's registers.
> The project replays a scenario file into a trace, where a press of switch k
> toggles LED k.

### What is this?

The part of the final project that's the same for every device: reading the
sensor and the switches through iom361's registers, lighting LEDs without
disturbing the others, keeping the clock, and replaying a scenario file into a
trace, one line per tick. The device here is simple: a press of switch k toggles
LED k. In the project, a state machine takes its place.

iom361 stands in for the hardware, so only `main.c` and the tests set the "world"
with its `_iom361_set_...` functions. `sensor.c` and `panel.c` only read and write
registers, the way they would on a real board.

| File | What it's for |
|------|---------------|
| `sensor.h`, `sensor.c` | `sensor_read`: reads, masks, converts and rounds the temperature and humidity. |
| `panel.h`, `panel.c` | Finds pressed and released switches, and changes one LED by read-modify-write. |
| `scenario.h`, `scenario.c` | `scenario_load` checks a scenario file and loads it, and `scenario_at` gives the line in force at a tick. |
| `main.c` | The replay: one tick at a time, sets the world, runs the device, and prints a trace line. |
| `iom361_r4.h`, `iom361_r4.c` | The course's I/O module, unmodified. |
| `tests/test_hw06.c` | Unit tests for `sensor.c`, `panel.c` and `scenario.c`. |
| `scenarios/`, `expected/` | Two scenarios, and the trace each should give, written by hand. |
| `regress.sh` | Runs `build/replay` on each scenario and diffs the output with its expected trace. |
| `Makefile` | Builds everything into `build/`. |
| `docs/PRD.md` | The assignment's requirements, plus the decisions we made where it left things open. |
| `AI_USAGE.md` | Which AI tools I used, for what, and what they got wrong. |

### Build and test

> [!Note]
> The assignment says `./replay scenario.txt`. I build the binary in `build/`
> instead, with everything else the build produces, so run `build/replay` from
> `hw06/`.

```sh
make        # builds build/replay
make test   # builds (if needed), then runs the unit tests and regress.sh
make clean  # removes build/
```

It's all compiled with `gcc -std=c11 -Wall -Wextra`, with no warnings. The unit
tests print a `PASS` or `FAIL` line per check, with a summary, and exit nonzero if
anything failed. `regress.sh` prints `PASS` or `FAIL` per scenario, and does the
same.

Everything's also built with `-fsanitize=address,undefined`. I added
`-fno-sanitize-recover=undefined`, because otherwise UBSan only prints a report and
the program still exits 0, so a `1 << 31` bug would pass `make test`.

### Usage

```sh
build/replay scenarios/<scenario>.txt
```

It prints one line per tick, from tick 0 to the last line's tick. Here are ticks 1
to 3 of `scenarios/toggle.txt`. Switch 0 is pressed at tick 2 and held at tick 3, so
only tick 2 counts as a press:

```
2026-10-06 08:00:01 temp=21.5 rh=40.0 switches=0x0 pressed=0x0 leds=0x0
2026-10-06 08:00:02 temp=21.5 rh=40.0 switches=0x1 pressed=0x1 leds=0x1
2026-10-06 08:00:03 temp=21.5 rh=40.0 switches=0x1 pressed=0x0 leds=0x1
```

The time is in UTC, so a trace is the same on every computer. The hex is lowercase
and unpadded, and `leds` shows the LEDs as the tick leaves them. A switch that's on
at tick 0 counts as a press, because the board starts with every switch off.

Errors go to stderr, with exit status 1, so stdout only ever holds the trace:

```
usage: replay scenario.txt
replay: scenarios/nope.txt: No such file or directory
replay: scenarios/bad.txt: line 7: invalid scenario line
```

### Scenario files

```
start 1791273600                # 2026-10-06 08:00:00 UTC
# tick  temp   rh    switches
0       21.5   40    0x0
2       21.5   40    0x1        # switch 0 pressed
```

A file is a `start` line with a Unix timestamp, then one line per change. The values
hold until the next line. Comments, blank lines and CRLF line endings are fine, and
the switches are hex, with or without `0x`.

| Field | Accepted |
|-------|----------|
| `start` | 0 to 253402300799, the end of the year 9999, so `start + tick` can't overflow |
| `tick` | 0 on the first data line, then increasing |
| `temp` | -50 to 150. iom361 would clamp anything else without a word, which would hide a typo |
| `rh` | 0 to 100, for the same reason |
| `switches` | Any bits. iom361 clears those above switch 3, so the trace shows what the board sees |

A file can hold at most 1000 data lines of at most 255 characters each.

`scenario_load` returns 0, or the number of the first wrong line, counting every line
in the file. If nothing is wrong except that data is missing (an empty file, or
`start` with no data lines), it returns the number one past the last line.

Lines are parsed with `sscanf` and a field count, as in lecture. That leaves some
wrong lines that still load, such as `5.5 40 0x1`, which reads as tick 5 and
temperature 0.5. `docs/PRD.md` lists all of them.

### Input ranges and boundary behavior

**`int sensor_read(uint32_t *base, time_t now, reading_t *r)`**
- Returns 0, or the iom361 return code of a failed read, which leaves `*r` unchanged.
- Keeps only bits 19..0 of each register.
- Rounds to the nearest 0.1. A value that rounds to zero is `0.0`, never `-0.0`.

**`uint32_t panel_pressed(uint32_t now, uint32_t before)`, `panel_released`**
- A held switch is in neither.

**`int panel_led(uint32_t *base, int n, int on)`, `panel_toggle(base, n)`**
- Return `-1` if `n` isn't 0 to 31, checked before the registers are touched.
- Return the iom361 return code of a failed access. A failed read means no write.
- `panel_led` treats any nonzero `on` as on.

**`const scenario_line_t *scenario_at(const scenario_t *s, int tick)`**
- Asserts that `tick` isn't negative and that the scenario has lines.
- Past the last line, the last line stays in force.
