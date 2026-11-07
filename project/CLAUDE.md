# CLAUDE.md

Garage door opener, the ECE 361 final project. [ASSIGNMENT.md](ASSIGNMENT.md) is the professor's spec and the source of truth for behavior, deliverables, and tests. Read it before designing anything. Do **NOT** edit it.

Code for this project lives only in `project/`. Sibling folders (`hw01`, `hw02`, ...) are weekly homework. As the term progresses, the homework folders will contain relevant material to be used here.

## Where we are

Weekly plan is in ASSIGNMENT.md section 8. Work only at the current week's stage; later modules stay stubs until their week.

**Week 5:** the transition table is confirmed and recorded in [SPEC.md](SPEC.md) (events, order, position and timer, table, interface). Implementation notes for `fsm.[ch]` that SPEC.md does not record:

- Carry over from HW5: `transition_t`, `A_UNDEFINED` = 0, an `N_*` count at the end of each enum, `fsm_step` as one lookup with asserts on its arguments, `state_name`, `event_name` and `action_name` with two-sided bounds checks, and the startup check in `main` that no entry is `A_UNDEFINED`. Not carried over: `fsm_check`.
- Tests: every cell, written out from SPEC.md rather than read from the code, plus sequences run through `fsm_tick` in a `runs_to`-style helper, including a reversal mid-travel that checks the length of the return trip, a press on each state's last tick, and a press on the tick the pause ends. Scenario files 1, 3 and 4 this week; their expected traces wait for the replay in week 6.
- `now - entered_at` gets a comment citing the POSIX assumption.

**Week 6:** the sensor, the clock, the status word, and the replay in `main.c` are done, and SPEC.md records the decisions. `sensor.c`, `scenario.[ch]` and the replay come from `hw06/`, with `fsm_tick` in place of hw06's toggling. Scenarios 1, 3, 4, 5 and 7 have hand-derived traces in `scenarios/<name>.expected`, compared by `tests/test_replay.c`, which runs `build/garage` through `system()`.

**To re-evaluate when their week comes:**

- **Week 7:** `main` ignores `fsm_tick`'s return value. It goes onto the event stack.
- **Week 8:** `main` stops with `store full` at 1024 records. The tree removes the limit, so that failure path and its test in `tests/test_replay.c` change.
- **Week 9:** `--report` is a usage error today (`main.c`, argument check). When `report.c` lands, re-evaluate this fail behavior and replace it with the real mode, along with its check in `tests/test_replay.c`.

## Ground rules

- This is graded coursework and the final exam asks the student to explain and change any line. Keep code small and plain, and explain non-obvious choices in the reply so the student can defend them.
- Decisions where the spec is silent are the student's. Propose, let them choose, then record the choice in `SPEC.md` and test it.
- Version control belongs to the student. Run no git operations.
- `AI_USAGE.md` is a deliverable (tools used, for what, and one thing a tool got wrong). When a tool gets something wrong in this project, say so, so the student can record it.

## Build

- Standard is `gcc -std=c11 -Wall -Wextra`, zero warnings, and clean under `-fsanitize=address,undefined`.
- `make` builds `build/garage`, `make test` builds and runs every test and ends with one pass/fail count line, `make clean` removes everything built.
- The Canvas download lives in `iom361_r4/` (built with `-Iiom361_r4`) and is never modified. A suspected bug goes to Agora, not into the file.

## Module boundaries

| Module | Rule |
|---|---|
| `sensor.[ch]` | Reads TEMP_REG and HUMID_REG, rounds to 0.1 °C and 0.1 %RH. No `printf`. Returns the reading through a pointer, failure through the return value. |
| `fsm.[ch]` | State, event, and action enums, transition table (an array indexed by state and event, no `if` chains), one next-state function (`fsm_step`). Also turns a tick's inputs into events (`fsm_tick`) and the state into LEDs (`fsm_leds`). No iom361 calls. |
| `store.[ch]` | Opaque `store_t`, ordered by temperature quantized to 0.5 °C via a single `record_compare`. Starts as a sorted array (week 4), becomes a BST in week 8 with no change outside `store.c`. |
| `aux.[ch]` | Opaque stack of events, starts at 8 slots, doubles when full. |
| `report.[ch]` | Moving average over W readings (W is a parameter). Reads the store through `store.h` only. |
| `status.[ch]` | Packs and unpacks the status word with `get_field` and `set_field`. |
| `bits.[ch]` | `get_field` and `set_field`, copied unchanged from `hw01`. |
| `clock.[ch]` | Turns a `time_t` into the trace's UTC text. |
| `scenario.[ch]` | Loads a scenario file, and gives the line in force at a tick. No iom361 calls. |
| `main.c` | Scenario replay and trace output. |

## iom361 gotchas

- Only `main.c` (scenario replay) and tests call `_iom361_set_sensor1` and `_iom361_set_switches`. `sensor.c` and `fsm.c` never do.
- Check `rc` on every iom361 call: a failed register read returns `0xDEADBEEF`, which looks like data.
- Readings come back within 0.0001 of what was set, either side, so a raw conversion of 91.0 can read 90.99998. Rounding in `sensor.c` is what makes threshold comparisons exact.
- Writes to `LEDS_REG` and `RGB_LED_REG` print to stdout. Replay calls `iom361_set_display(false)` first so the trace stays clean.
- There is no clock. Time is `time_t` UTC: scenario start plus tick, one second per tick, formatted with `gmtime` and `strftime` as `%Y-%m-%d %H:%M:%S`.

## Other

- The status word is one `uint32_t` packed with the student's HW1 `get_field` and `set_field` (reused from `hw01` in `bits.[ch]`, not rewritten). SPEC.md gives the layout. Unused bits are 0.
- Trace line format is fixed by ASSIGNMENT.md section 4 so tests can diff it against expected files.
- No em-dashes in any prose here (docs, comments, SPEC.md). Rewrite the sentence with a comma, colon, period, parentheses, or a conjunction.
