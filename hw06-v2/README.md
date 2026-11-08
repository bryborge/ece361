# Memory-Mapped I/O Simulation

> Uses the iom361 library to explore bit manipulation techniques,
> edge-detection, and the "read-modify-write" operations typical of many
> microprocessor architectures.

### What is this?

A second build of Homework 6, done for learning. The graded submission is in
`../hw06/`. This one grew outward from a "hello world" in `main.c`, and a module
was only pulled out once the running program needed it.

The program replays a scenario file into a trace, one line per tick, where a press
of switch k toggles LED k. iom361 stands in for the hardware, so only `main.c` and
the tests set the "world" with its `_iom361_set_...` functions. `sensor.c` and
`panel.c` only read and write registers, the way they would on a real board.

| File | What it's for |
|------|---------------|
| `main.c` | The replay: one tick at a time, sets the world, runs the device, and prints a trace line. |
| `panel.h`, `panel.c` | Finds pressed and released switches, and toggles one LED by read-modify-write. |
| `sensor.h`, `sensor.c` | `sensor_read`: reads, masks, converts and rounds the temperature and humidity. |
| `scenario.h`, `scenario.c` | `scenario_load` checks a scenario file and loads it, and `scenario_at` gives the line in force at a tick. |
| `external/` | The course's I/O module, iom361. |
| `tests/test_hw06.c` | Unit tests for `panel.c`, `sensor.c` and `scenario.c`. |
| `scenarios/`, `expected/` | Two scenarios, and the trace each should give, written by hand. |
| `regress.sh` | Runs `build/replay` on each scenario and diffs the output with its expected trace. |
| `AI_USAGE.md` | Which AI tools I used, for what, and what they got wrong. |

### Build and test

Run these from `hw06-v2/`:

```sh
make        # builds build/replay
make test   # builds (if needed), then runs the unit tests and regress.sh
make clean  # removes build/
```

It's all compiled with `gcc -std=c11 -Wall -Wextra`, with no warnings, and with
`-fsanitize=address,undefined -fno-sanitize-recover=undefined`, so that undefined
behavior fails the tests instead of only printing a report. The unit tests print
`PASS` or `FAIL` per check, and `regress.sh` per scenario. Both exit nonzero on a
failure.

### Usage

```sh
build/replay scenarios/toggle.txt
```

Here are ticks 1 to 3 of that scenario. Switch 0 is pressed at tick 2 and held at
tick 3, so only tick 2 counts as a press:

```
2026-10-06 08:00:01 temp=21.5 rh=40.0 switches=0x0 pressed=0x0 leds=0x0
2026-10-06 08:00:02 temp=21.5 rh=40.0 switches=0x1 pressed=0x1 leds=0x1
2026-10-06 08:00:03 temp=21.5 rh=40.0 switches=0x1 pressed=0x0 leds=0x1
```

The time is in UTC, so a trace is the same on every computer. Errors go to stderr,
with exit status 1:

```
usage: replay scenario.txt
replay: nope.txt: No such file or directory
replay: scenarios/bad.txt:7: bad line
```

### Scenario files

```
start 1791273600                # 2026-10-06 08:00:00 UTC
# tick  temp   rh    switches
0       21.5   40    0x0
2       21.5   40    0x1        # switch 0 pressed
```

A file is a `start` line with a Unix timestamp, then one line per change, up to 1000
data lines. The values hold until the next line. The first data line must be at tick
0, and ticks must increase. Comments and blank lines are fine, and the switches are
hex, with or without `0x`.

`scenario_load` returns 0, or the number of the first wrong line, counting every line
in the file. A file with no data lines returns one past its last line.

### Where this differs from the assignment

- **No `panel_led`.** The sheet requires it and its tests. Nothing in the replay
  calls it, so I removed it, and I'll rewrite it when something does. `panel_released`
  stayed, also without a caller, because it's the mirror of `panel_pressed`.
- **The binary is `build/replay`,** not `./replay`.
- **iom361 is in `external/`,** not directly in the homework directory.

### Known gaps

**These wrong files still load:**

| In the file | What happens |
|-------------|--------------|
| `start5` | Reads as `start 5`. |
| A second `start` line | Replaces the first, for the whole trace. |
| `5.5 40 0x1` (a field missing) | Reads as tick 5, temperature 0.5, humidity 40. |
| A temperature or humidity out of range | iom361 clamps it, so `200` prints as `150.0`. |

**A line over 254 characters** gets split in two by `fgets`, even if it's a comment.
The second piece is then usually rejected, with a line number one too high.

**One rule has no test.** `sensor_read` must leave `*r` unchanged when either read
fails, but iom361 can't be made to fail only the second read. The order of the code
guarantees it, and a test doesn't.
