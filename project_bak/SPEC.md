# SPEC

## Events and transition table

### Events

The table cannot see the temperature, the switches, or the clock, so every
condition it needs becomes an event. Only `E_TIMEOUT` depends on the state;
which press event a press becomes depends only on the temperature and the
barrier.

| Event | When |
|---|---|
| `E_REMOTE` | REMOTE pressed (its bit went from 0 to 1), above `FREEZE_C`, barrier clear |
| `E_REMOTE_FROZEN` | REMOTE pressed at or below `FREEZE_C`, barrier clear |
| `E_REMOTE_BARRIER` | REMOTE pressed above `FREEZE_C`, barrier on |
| `E_REMOTE_FROZEN_BARRIER` | REMOTE pressed at or below `FREEZE_C`, barrier on |
| `E_BARRIER` | the barrier is on (a level: every tick it is on) |
| `E_TIMEOUT` | the current state's time is up (see Position and timer) |

**Order.** Every event that happens in a tick goes through the table, one
lookup each, always in this order: barrier, then press, then timeout. A
tick with no events makes no lookup, so there is no "nothing happened"
event. Each lookup starts from the state the previous one left, and the
timeout is checked against that state's timer.

- Barrier first: the barrier must stop a closing door, even on its last
  tick (the sample covers the interval in which the door was still above
  the floor). In every other state its cell is a self-loop, so going first
  costs nothing.
- Press before timeout: a press on a state's last tick is looked up in
  that state, while it still has travel left to reverse. So no state is
  ever entered with no time allowed: a reversal on `OPENING`'s last tick
  starts `CLOSING` from 15, on `CLOSING`'s last tick starts `OPENING` from
  0, and `OBSTRUCTED` ignores the press before its timeout reopens the
  door.

**One transition per tick.** Whichever lookup changes the state, the
lookups after it cannot: the barrier's only transition leads to
`OBSTRUCTED`, where a press does nothing and the pause has just begun, and
a press's transition starts a state with time allowed. So the trace shows
every state the door is in.

### Position and timer

The door's position runs from 0 (down) to `TRAVEL` = 15 (up). It is not
stored per tick: the door keeps the timestamp of its last state change,
`entered_at`, and its position at that moment, `pos_at_entry`, and derives
the rest from the clock, so elapsed time has one source of truth.

| State | Position now | Time allowed, fixed on entry |
|---|---|---|
| `OPENING` | `pos_at_entry + elapsed` | `TRAVEL - pos_at_entry` |
| `CLOSING` | `pos_at_entry - elapsed` | `pos_at_entry` |
| `OBSTRUCTED` | `pos_at_entry` | `PAUSE` = 2 |
| `CLOSED`, `OPEN` | `pos_at_entry` (0 or 15) | none: no timer |

`elapsed` is `now - entered_at`. The timer runs out when the state has a
time allowed and `elapsed` is at least that time. A full open or close takes
15 ticks (rule 1); a reversed or obstructed door takes as long as it has
left to travel. `entered_at` and `pos_at_entry` change only when the state
changes, never on a cell that stays put, and the position snapshot is taken
before the state changes, because the position is computed from the old
state.

### Actions

The LEDs and the RGB color come from the state, not from the table. The
actions only say what to push onto the event stack. Presses are pushed when
they are detected, whatever the table does with them, so no action records
a press.

| Action | Meaning |
|---|---|
| `A_UNDEFINED` | 0: a cell nobody filled in |
| `A_NONE` | nothing to record |
| `A_OBSTRUCT` | the barrier stopped a closing door |
| `A_REFUSE_FROZEN` | a press refused by the freeze guard (rule 3) |
| `A_REFUSE_BARRIER` | a press refused because the barrier is on |

### Table

Each cell is the next state, then the action if it is not `A_NONE`. Cells
marked † can never be reached; they are filled in because every cell must be.

| State \ Event | `E_REMOTE` | `E_REMOTE_FROZEN` | `E_REMOTE_BARRIER` | `E_REMOTE_FROZEN_BARRIER` | `E_BARRIER` | `E_TIMEOUT` |
|---|---|---|---|---|---|---|
| `CLOSED`     | `OPENING`    | `CLOSED`, `A_REFUSE_FROZEN` | `OPENING`                     | `CLOSED`, `A_REFUSE_FROZEN`   | `CLOSED`                   | `CLOSED` †  |
| `OPENING`    | `CLOSING`    | `CLOSING`                   | `OPENING`, `A_REFUSE_BARRIER` | `OPENING`, `A_REFUSE_BARRIER` | `OPENING`                  | `OPEN`      |
| `OPEN`       | `CLOSING`    | `CLOSING`                   | `OPEN`, `A_REFUSE_BARRIER`    | `OPEN`, `A_REFUSE_BARRIER`    | `OPEN`                     | `OPEN` †    |
| `CLOSING`    | `OPENING`    | `OPENING`                   | `OBSTRUCTED`, `A_OBSTRUCT` †  | `OBSTRUCTED`, `A_OBSTRUCT` †  | `OBSTRUCTED`, `A_OBSTRUCT` | `CLOSED`    |
| `OBSTRUCTED` | `OBSTRUCTED` | `OBSTRUCTED`                | `OBSTRUCTED`                  | `OBSTRUCTED`                  | `OBSTRUCTED`               | `OPENING`   |

The † cells: `CLOSED` and `OPEN` have no timer, so they never time out.
`CLOSING` never sees a press with the barrier on, because the barrier is
looked up first and has already moved the door to `OBSTRUCTED`.

### Where events are made, and the interface

Events are made in `fsm.c`, not `main.c`. What counts as a press, the
freeze threshold, the order of lookups, and the timer are all decisions
about behavior, so they sit beside the table that uses them; `main` only
moves data between iom361, the state machine, and the stack. It also means
the whole state machine is tested without iom361.

- **Mealy actions, Moore outputs.** Actions sit on transitions. The LEDs
  are read from the state through `uint32_t fsm_leds(state_t s)`, so the
  LED register and the status word's motor bits can never disagree with
  the state. The RGB color is read from the state the same way, through
  `rgb_t fsm_rgb(state_t s)`. `fsm.c` makes no iom361 calls, so `main` passes
  the color to `build_rgb_reg`.
- **The door and one tick.** `fsm.h` declares
  `typedef struct { state_t state; time_t entered_at; int pos_at_entry; } door_t;`
  and
  `action_t fsm_tick(door_t *d, time_t now, uint32_t prev_switches, uint32_t switches, float temp_c)`.
  `fsm_tick` runs the tick's events through the table in order, updates
  the door, and returns the tick's action, or `A_NONE`. One tick has at
  most one action: the only action the barrier gives is `A_OBSTRUCT`, which
  leaves the door in `OBSTRUCTED`, where no press is refused, and the
  timeout gives none. An `assert` catches a second action if the table
  ever changes. `fsm_step` stays a single table lookup. `main` keeps
  `prev_switches` and pushes the action onto the stack, so `fsm.c` does
  not depend on `aux.h`. `door_t` is plain data with no `static` state
  behind it, so a test builds one by hand. Taking a `float` instead of a
  `reading_t` keeps `fsm.h` independent of `sensor.h`. Both `entered_at` and `pos_at_entry` are needed: in
  `OBSTRUCTED` the door has two independent quantities, where it stopped
  and how long it has been stopped.
- **The door starts** as `{CLOSED, scenario start, 0}`.
- **One definition of a press.** `bool fsm_remote_pressed(uint32_t prev, uint32_t now)`
  is used by `fsm_tick` and by `main`, which pushes every detected press
  onto the stack.
- **Constants.** `SW_REMOTE` (bit 0) and `SW_BARRIER` (bit 1) are in
  `fsm.h`. `FREEZE_C` = 1.0, `TRAVEL` = 15 and `PAUSE` = 2 are private to
  `fsm.c`. `FREEZE_C` is compared with `temp_c <= FREEZE_C`, which is
  exact because `sensor.c` rounds to 0.1.
- **Durations.** `elapsed` is the plain subtraction `now - entered_at`,
  which assumes `time_t` counts whole seconds. POSIX guarantees that and
  C11 does not; the assignment already depends on it (`gmtime`, the 2038
  question).
- **Position is checked.** The position function asserts that its result
  is between 0 and `TRAVEL`.

## State machine drawing

Each arrow is labeled with its events, then `/` and the action when it is
not `A_NONE`. Events are written without the `E_` prefix and actions without
the `A_` prefix.

```text
        +--------+  REMOTE, REMOTE_BARRIER  +---------+  TIMEOUT  +------+
start-->| CLOSED |------------------------->| OPENING |---------->| OPEN |--+
        +--------+                          +---------+           +------+  |
            ^                                 |   ^  ^                      |
            |                  REMOTE,        |   |  |                      |
            |                  REMOTE_FROZEN  |   |  |  TIMEOUT             |
            |                  (both ways)    |   |  +------------+         |
            |                                 v   |               |         |
            |       TIMEOUT               +---------+       +------------+  |
            +-----------------------------| CLOSING |------>| OBSTRUCTED |  |
                                          +---------+       +------------+  |
                                            ^         BARRIER / OBSTRUCT    |
                                            |                               |
                                            +-------------------------------+
                                                  REMOTE, REMOTE_FROZEN
```

Not drawn: entries that leave the state where it is. These six have an
action:

| State | Events | Action |
|---|---|---|
| `CLOSED` | `REMOTE_FROZEN`, `REMOTE_FROZEN_BARRIER` | `REFUSE_FROZEN` |
| `OPENING` | `REMOTE_BARRIER`, `REMOTE_FROZEN_BARRIER` | `REFUSE_BARRIER` |
| `OPEN` | `REMOTE_BARRIER`, `REMOTE_FROZEN_BARRIER` | `REFUSE_BARRIER` |

Also not drawn are the four unreachable entries (†), and every other entry,
which stays where it is with no action.

## Decisions where a rule was silent

### Each row of the table

- **`CLOSED`:** a press opens the door, even with the barrier on, because
  opening moves away from anything in the doorway. At or below 1.0 °C every
  press is refused (rule 3), barrier or not.
- **`OPENING`:** a press reverses the door, because that is what the user
  wants, unless the barrier is on: sending the door down onto something is
  refused. The cold does not matter here; only a closed door can be frozen
  to the floor. The barrier alone changes nothing, since opening moves away
  from it.
- **`OPEN`:** a press closes the door, unless the barrier is on. The cold
  does not matter. The barrier alone changes nothing: the door is up and
  not moving.
- **`CLOSING`:** a press reverses the door. The barrier stops it
  (`OBSTRUCTED`, rule 2), including in the two cells that cannot be reached,
  because if either one ever were, the barrier is on and the door is coming
  down.
- **`OBSTRUCTED`:** presses are ignored for the whole pause, including the
  tick it ends on, then the door opens again (rule 2). Presses are still
  pushed onto the stack when they are detected.

### Timing

- **Reversal.** Rule 1 gives the time of a full open or close but not of a
  reversed one. A reversed or obstructed door travels only as far as it has
  gone (see Position and timer), because a door reversed after 1 tick that
  took 15 ticks to come back would not match the physics.
- **The barrier on the last tick of `CLOSING`** stops the door, even as it
  reaches the floor: the sample covers the tick in which the door was still
  coming down, and the cautious answer is the one that cannot crush
  anything.
- **A press on the last tick of `OPENING` or `CLOSING`** reverses the door
  from the end it just reached (press before timeout). On `CLOSING`'s last
  tick this holds even at or below 1.0 °C: the door is still `CLOSING` and
  has only just touched down, so it cannot be frozen to the floor, and
  rule 3 applies to `CLOSED`.

### Humidity

- Humidity is read and recorded but gates nothing: no rule gives it a
  threshold, so the only threshold in the rules is `FREEZE_C`.

### Scenario files

The sheet gives the format and leaves these open. `scenario.[ch]` is a module of
its own, so the loader is tested without iom361. `scenario_load` takes a
`FILE *`, so that it returns only 0 or the number of the first wrong line, and
`main` handles a file that won't open.

- **The first data tick must be 0.** Before the first line nothing is in force,
  and using iom361's reset values would make the trace depend on values the file
  never stated.
- **Accepted:** blank lines; whole-line, indented and trailing comments; CRLF; hex
  with or without `0x`.
- **A wrong line:** a data line before `start`; a second `start`; a missing or
  extra field; text that does not parse; a tick that does not increase; data line
  1001 (a file holds at most 1000 data lines); a line over 255 characters. The
  number returned counts every line in the file, as an editor does.
- **Nothing to point at:** an empty file, a file of only comments, or `start` with
  no data returns the number one past the last line, so the failure is still
  nonzero.
- **Ranges:** `temp` from -50 to 150 and `rh` from 0 to 100, both inclusive.
  iom361 clamps anything else without a word, which would hide a typo. `start` from
  0 to 253402300799 (the end of the year 9999), so `start + tick` can't overflow.
- **Switch bits above switch 1** are accepted, and iom361 clears them, so the trace
  shows what the board sees. `scenario.c` does not know the switch count.
- **Lines are parsed with `sscanf` and a field count,** as in lecture. That leaves
  these gaps: `5.5 40 0x1` loads as tick 5 and temperature 0.5, because `%d` stops
  at the `.`; `5 45 40 -1` loads with every switch bit set, because `%x` accepts a
  minus sign; a tick or switch value too large for its type also loads, because an
  out-of-range conversion is undefined in the C standard. Scanning into strings and
  converting with `strto*` would close all of them.
- **The line in force** is found by a linear scan: at most 1000 comparisons per
  tick, and no measurement or requirement calls for less.

### The replay

- **The first tick.** `prev_switches` starts at 0, so a switch already on at tick 0
  counts as a press: the board starts with every switch off, so that is a real
  edge.
- **The trace shows the tick's result:** the state, LEDs and status after the state
  machine has run. The hex is lowercase; `leds` is unpadded and `status` is 8 digits,
  as in the sheet's sample line.
- **The stop test is in the loop's body.** A last tick of `INT_MAX` is accepted, so
  `tick <= last` could never be false, and `tick++` would overflow.
- **Failures** print one line on stderr and exit with status 1. stdout holds only
  the trace, and a partial trace stays if the run stops early.
- **A full store stops the run** (`garage: tick 1024: store full`). A missing record
  is lost data, and the report depends on the records. The week-8 tree removes the
  limit.
- **`--report` is a usage error until week 9,** when `report.c` exists. That choice
  gets re-evaluated then.
- **Not covered by `make test`:** `main`'s RGB write, because neither the trace nor
  the display (off during a replay) shows it. `fsm_rgb` is tested, and the write was
  checked by hand once, with the display on.

### Known limitation

- **Sampled input.** The switches are read once per tick, so a press and
  release within one tick is not seen. This comes from the scenario format
  (one switch value per tick line), not from the state machine.

## record_t, quantization, and duplicate keys

```c
typedef struct {
    time_t timestamp;   /* UTC seconds since the Unix epoch */
    float temp_c;       /* rounded to 0.1 °C */
    float rh_pct;       /* rounded to 0.1 %RH */
} reading_t;

typedef struct {
    reading_t reading;
    state_t state;      /* the state at the moment of the reading */
    uint32_t status;    /* the status word at that moment */
} record_t;
```

**Quantization.** The key of a record is `floor(temp_c * 2)`, the index of its 0.5 °C step. `floor`, not `round`: 21.2 and 21.4 °C must share a key, and `round` would put them at 42 and 43. A step includes its lower edge, so 21.5 starts the step 21.5 to 21.9. `floor` also keeps negative temperatures in the right step (-0.2 °C is below 0.0). Multiplying a float by 2 is exact, so the edges x.0 and x.5 land exactly. `record_compare` compares keys only and does not tie-break.

**Duplicate keys: a list per key.** Each key holds every record with that key, in insertion order, which is time order. In the week-4 sorted array, that means a new record goes after the last record with the same key. In the week-8 tree, each node holds the list for its key. A count per key was rejected because it discards the timestamp, state, and status of all but one reading, and the report needs them. A tie-break on the timestamp was rejected because records arrive in time order, so all the records of one key would form a chain down the right side, and a garage stays in one 0.5 °C step for hundreds of ticks. With a list per key, the height depends only on the number of distinct keys.

**Height.** `store_height` returns the number of distinct keys while the store is an array. That equals the height of a tree with one node per key built from inserts in ascending key order, so the height tests hold before and after the tree.

**Capacity.** Until week 8 the store is one static array of 1024 records (`STORE_CAP`, private to `store.c`), and `store_insert` returns -1 when it is full. The tree removes the cap.

**NULL.** A NULL store, record, or output array is a bug in the caller and aborts with an `assert`, except `store_destroy(NULL)`, which does nothing (like `free(NULL)`).

## Stack growth

TBD (week 7)

## Report definition

TBD (week 9)

## Status word bit layout

One `uint32_t`, packed and unpacked in `status.c` with hw01's `get_field` and
`set_field` (`bits.[ch]`, copied unchanged). Unused bits are 0.

| Bits | Field | Values |
|---|---|---|
| 2..0 | state | `CLOSED` 0, `OPENING` 1, `OPEN` 2, `CLOSING` 3, `OBSTRUCTED` 4 |
| 3 | `MOTOR_UP` | 1 while `OPENING` |
| 4 | `MOTOR_DOWN` | 1 while `CLOSING` |
| 5 | freeze guard active | 1 while `CLOSED` at or below 1.0 °C |
| 31..6 | unused | 0 |

So the words are `CLOSED` `0x00` (`0x20` with the guard active), `OPENING` `0x09`,
`OPEN` `0x02`, `CLOSING` `0x13`, and `OBSTRUCTED` `0x04`. The fields are contiguous:
the sheet's sample `0x00000011` comes from a different layout, and it says the layout
is ours to choose.

**Freeze guard active** means a press on this tick would have been refused. Rule 3
needs both `CLOSED` and the cold, so the bit needs both too. It stays off while an
open door closes in the cold, where the cold does not matter, and it is computed
after the tick's state change, so it comes on the tick a cold door reaches `CLOSED`.
`bool fsm_freeze_active(state_t s, float temp_c)` is in `fsm.c`, because `FREEZE_C` is
private there.

`status_of(state, temp_c)` builds the status from `fsm_leds` and `fsm_freeze_active`,
so the motor bits can't disagree with the LEDs.

## RGB color of each state

The RGB LED is always enabled, and its color comes from the state.

| State | Color | Red, green, blue |
|---|---|---|
| `CLOSED` | green | 0, 255, 0 |
| `OPENING` | cyan | 0, 255, 255 |
| `OPEN` | blue | 0, 0, 255 |
| `CLOSING` | yellow | 255, 255, 0 |
| `OBSTRUCTED` | red | 255, 0, 0 |

Closed and safe is green, the door coming down is the caution color, and a door
stopped by the barrier is red.

## 2038

A 32-bit `time_t` runs out at 2038-01-19 03:14:07 UTC. Adding a tick past that is
signed overflow, which C leaves undefined; in practice it wraps to 1901-12-13.

What that does depends on the state the door is in:

- **`OPENING`, `CLOSING` or `OBSTRUCTED`:** these have a timer, so every tick calls
  `elapsed`. `now` has fallen below `entered_at`, so its `assert` stops the program
  mid-travel, with the motor LED still lit.
- **`CLOSED` or `OPEN`:** these have no timer and never call `elapsed`, so the door
  keeps working. The next press stamps a new `entered_at`, and from then on the
  arithmetic is consistent again.

Either way, the trace dates and record timestamps jump back to 1901.

The `assert` is what makes the failure loud. Without it, `now - entered_at` also
overflows, and if it wraps the way the clock did, it gives the right small number, so
the door would finish its travel. That would be luck: it is still undefined behavior.

The cap on `start` wouldn't help either, because it assumes a `time_t` wider than 32
bits. A real fix is a 64-bit `time_t`, which is what this project runs on.

## The given states: defend or criticize

TBD (week 10)

## Stack compared with an alternative

TBD (week 7)
