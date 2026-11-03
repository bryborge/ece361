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
  the state. The RGB color will be read from the state the same way.
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

TBD (week 5)

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

TBD (week 3)

## RGB color of each state

TBD (week 6)

## 2038

TBD

## The given states: defend or criticize

TBD (week 10)

## Stack compared with an alternative

TBD (week 7)
