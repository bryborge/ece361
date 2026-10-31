# FSM transition table (draft)

> **Status: pending review.** The table below follows from the design
> decisions made so far, but some cells may change. It must be reviewed on
> paper and explicitly confirmed (cell by cell, with any changes recorded
> here) before any of `fsm.h` or `fsm.c` is implemented. When it is
> confirmed, it moves into `SPEC.md` and this file is deleted.

## Events

The table cannot see the temperature, the switches, or the clock, so every
condition it needs becomes an event. `fsm_event` produces exactly one event
per tick.

| Event | When |
|---|---|
| `E_NONE` | nothing happened this tick |
| `E_REMOTE` | REMOTE pressed (its bit went from 0 to 1), warm, barrier clear |
| `E_REMOTE_FROZEN` | REMOTE pressed at or below 1.0 °C, barrier clear |
| `E_REMOTE_BLOCKED` | REMOTE pressed while the barrier is on, warm |
| `E_REMOTE_FROZEN_BLOCKED` | REMOTE pressed while the barrier is on, at or below 1.0 °C |
| `E_BARRIER` | the barrier is on (a level: every tick it is on) |
| `E_TIMEOUT` | the state's timer ran out: 15 ticks in `OPENING` or `CLOSING`, 2 in `OBSTRUCTED` |

**Priority, when several happen in one tick:**

- In `CLOSING`: barrier, then timeout, then press, then none. The barrier
  must stop a closing door, even on its last tick.
- Everywhere else: timeout, then press, then barrier, then none. Opening
  finishes on time, `OBSTRUCTED` reopens after 2 ticks (rule 2), and a press
  while the barrier is on becomes one of the `_BLOCKED` events so the table
  decides.

**Timer:** `ticks_in_state` restarts only when the state changes, never on
a cell that stays put. The timer runs out when `ticks_in_state >= duration`.

## Actions

The LEDs and the RGB color come from the state, not from the table. The
actions only say what to push onto the event stack. Presses are pushed when
they are detected, whatever the table does with them, so no action records
a press.

| Action | Meaning |
|---|---|
| `A_UNDEFINED` | 0: a cell nobody filled in |
| `A_NONE` | nothing to record |
| `A_OBSTRUCT` | the barrier stopped a closing door |
| `A_REFUSE_COLD` | a press refused by the freeze guard (rule 3) |
| `A_REFUSE_BLOCKED` | a press refused because the barrier is on |

## Table

Each cell is the next state, then the action if it is not `A_NONE`. Cells
marked † can never be reached; they are filled in because every cell must be.

| State \ Event | `E_NONE` | `E_REMOTE` | `E_REMOTE_FROZEN` | `E_REMOTE_BLOCKED` | `E_REMOTE_FROZEN_BLOCKED` | `E_BARRIER` | `E_TIMEOUT` |
|---|---|---|---|---|---|---|---|
| `CLOSED`     | `CLOSED`     | `OPENING`    | `CLOSED`, `A_REFUSE_COLD` | `OPENING`                    | `CLOSED`, `A_REFUSE_COLD`    | `CLOSED`                 | `CLOSED` †  |
| `OPENING`    | `OPENING`    | `CLOSING`    | `CLOSING`                 | `OPENING`, `A_REFUSE_BLOCKED` | `OPENING`, `A_REFUSE_BLOCKED` | `OPENING`                | `OPEN`      |
| `OPEN`       | `OPEN`       | `CLOSING`    | `CLOSING`                 | `OPEN`, `A_REFUSE_BLOCKED`    | `OPEN`, `A_REFUSE_BLOCKED`    | `OPEN`                   | `OPEN` †    |
| `CLOSING`    | `CLOSING`    | `OPENING`    | `OPENING`                 | `OBSTRUCTED`, `A_OBSTRUCT` †  | `OBSTRUCTED`, `A_OBSTRUCT` †  | `OBSTRUCTED`, `A_OBSTRUCT` | `CLOSED`  |
| `OBSTRUCTED` | `OBSTRUCTED` | `OBSTRUCTED` | `OBSTRUCTED`              | `OBSTRUCTED`                 | `OBSTRUCTED`                 | `OBSTRUCTED`             | `OPENING`   |

## Why each row is what it is

- **`CLOSED`:** a press opens the door, even with the barrier on, because
  opening moves away from anything in the doorway. At or below 1.0 °C every
  press is refused (rule 3), barrier or not.
- **`OPENING`:** a press reverses the door, because that is what the user
  wants, unless the barrier is on: sending the door down onto something is
  refused. The cold does not matter here; only a closed door can be frozen
  to the floor. The barrier alone changes nothing, since opening moves away
  from it.
- **`OPEN`:** a press closes the door, unless the barrier is on. The cold
  does not matter.
- **`CLOSING`:** a press reverses the door. The barrier stops it
  (`OBSTRUCTED`, rule 2), including in the two cells that cannot be reached,
  because if either one ever were, the barrier is on and the door is coming
  down.
- **`OBSTRUCTED`:** everything is ignored until the 2 ticks are up, then
  the door opens again (rule 2). Presses are still pushed onto the stack
  when they are detected.

## Design decisions

The events, priority, timer, actions, and rows above record most of the
design. These are the rest, settled in the week 5 design session:

- **Mealy actions, Moore outputs.** Actions sit on transitions. The LEDs
  (and, from week 6, the RGB color) are read from the state through
  `uint32_t fsm_leds(state_t s)` in `fsm.c`, so the LED register and the
  status word's motor bits can never disagree with the state.
- **Event code in `fsm.c`, pure.**
  `event_t fsm_event(state_t s, uint32_t prev_switches, uint32_t switches, float temp_c, time_t ticks_in_state)`
  keeps no `static` state; `main` keeps `prev_switches` and the time the
  current state was entered, and passes `now - entered`. Taking a `float`
  instead of a `reading_t` keeps `fsm.h` independent of `sensor.h`.
- **One definition of a press.** `bool fsm_remote_pressed(uint32_t prev, uint32_t now)`
  is used by `fsm_event` and by `main`, which pushes every detected press
  onto the stack.
- **Constants.** `SW_REMOTE` (bit 0) and `SW_BARRIER` (bit 1) in `fsm.h`;
  `FREEZE_C` = 1.0, compared with `temp_c <= FREEZE_C` (exact, because
  `sensor.c` rounds to 0.1); the travel times in a private array indexed by
  state (`OPENING` and `CLOSING` 15, `OBSTRUCTED` 2, otherwise 0).
- **Carried over from HW5.** `transition_t`, `A_UNDEFINED` = 0, an `N_*`
  count at the end of each enum, `fsm_step` as one lookup with asserts on
  its arguments, `state_name`, `event_name` and `action_name` with two-sided
  bounds checks, and the startup check in `main` that no entry is
  `A_UNDEFINED`. Not carried over: `fsm_check`.
- **Tests.** Every cell, written out from this table rather than read from
  the code, plus sequences run through `fsm_event` and `fsm_step` in a
  `runs_to`-style helper. Scenario files 1, 3 and 4 are written this week;
  their expected traces wait for the replay in week 6.
- **Deferred.** The status word, including what the "freeze guard active"
  bit means, is settled in its own session.

## Known limitations

- **No position.** The five given states do not say where the door is, so a
  reversed door takes the full 15 ticks to finish, however far it had gone.
- **One event per tick.** A press on the tick a timer runs out, or in
  `CLOSING` while the barrier is on, is still pushed onto the stack but
  does not change what the door does.
