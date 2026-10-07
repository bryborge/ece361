# CLAUDE.md

Garage door opener, the ECE 361 final project (project 7, device type C, key K2, second structure S1, report Q5). [ASSIGNMENT.md](ASSIGNMENT.md) is the professor's spec and the source of truth for behavior, deliverables, and tests. Read it before designing anything. Do not edit it.

Code for this project lives only in `project/`. Sibling folders (`hw01`, `hw02`, ...) are weekly homework.

## Where we are

Weekly plan is in ASSIGNMENT.md section 8. Work only at the current week's stage; later modules stay stubs until their week. Week 2: five module stubs, a `Makefile` linking them with iom361, and a first `SPEC.md`.

## Ground rules

- This is graded coursework and the final exam asks the student to explain and change any line. Keep code small and plain, and explain non-obvious choices in the reply so the student can defend them.
- Decisions where the spec is silent are the student's. Propose, let them choose, then record the choice in `SPEC.md` and test it.
- Version control belongs to the student. Run no git operations.
- `AI_USAGE.md` is a deliverable (tools used, for what, and one thing a tool got wrong). When a tool gets something wrong in this project, say so, so the student can record it.

## Build

- Standard is `gcc -std=c11 -Wall -Wextra`, zero warnings, and clean under `-fsanitize=address,undefined`.
- `make` builds `garage`, `make test` builds and runs every test and ends with one pass/fail count line, `make clean` removes everything built.
- The Canvas download lives in `iom361_r4/` (built with `-Iiom361_r4`) and is never modified. A suspected bug goes to Agora, not into the file.

## Module boundaries

| Module | Rule |
|---|---|
| `sensor.[ch]` | Reads TEMP_REG and HUMID_REG, rounds to 0.1 °C and 0.1 %RH. No `printf`. Returns the reading through a pointer, failure through the return value. |
| `fsm.[ch]` | State and event enums, transition table (an array indexed by state and event, no `if` chains), one next-state function. |
| `store.[ch]` | Opaque `store_t`, ordered by temperature quantized to 0.5 °C via a single `record_compare`. Starts as a sorted array (week 4), becomes a BST in week 8 with no change outside `store.c`. |
| `aux.[ch]` | Opaque stack of events, starts at 8 slots, doubles when full. |
| `report.[ch]` | Moving average over W readings (W is a parameter). Reads the store through `store.h` only. |
| `main.c` | Scenario replay and trace output. |

## iom361 gotchas

- Only `main.c` (scenario replay) and tests call `_iom361_set_sensor1` and `_iom361_set_switches`. `sensor.c` and `fsm.c` never do.
- Check `rc` on every iom361 call: a failed register read returns `0xDEADBEEF`, which looks like data.
- Readings come back within 0.0001 of what was set, either side, so a raw conversion of 91.0 can read 90.99998. Rounding in `sensor.c` is what makes threshold comparisons exact.
- Writes to `LEDS_REG` and `RGB_LED_REG` print to stdout. Replay calls `iom361_set_display(false)` first so the trace stays clean.
- There is no clock. Time is `time_t` UTC: scenario start plus tick, one second per tick, formatted with `gmtime` and `strftime` as `%Y-%m-%d %H:%M:%S`.

## Other

- The status word is one `uint32_t` packed with the student's HW1 `get_field` and `set_field` (reuse from `hw01`, do not rewrite). Unused bits are 0.
- Trace line format is fixed by ASSIGNMENT.md section 4 so tests can diff it against expected files.
- No em-dashes in any prose here (docs, comments, SPEC.md). Rewrite the sentence with a comma, colon, period, parentheses, or a conjunction.
