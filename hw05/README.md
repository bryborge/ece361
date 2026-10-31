# Homework 5: Combination Lock

> In Homework 5, we learn about finite state machines written as a table.
> The project is a combination lock that opens on 3-1-4.

### What is this?

The lock has four states: `LOCKED`, `GOT1` and `GOT2` (one or two digits of
the code typed), and `OPEN`. It has four events: a right digit, a wrong digit,
the door closing, and the 5-second timer running out. Everything the lock
does is in one table, `lock_table`, indexed by state and event, which gives
the next state and the action (`NONE`, `UNLOCK`, `LOCK` or `BEEP`). So
`lock_step` is a single table lookup, with no `if` or `switch` on the state.

`lock_event` turns a typed character into an event. Which digit is right
depends on the state, so it reads the next digit of the code from the
`next_digit` array, indexed by state; `OPEN` has no next digit, so every
digit is wrong while the door is open.

| File | Responsibility |
|------|----------------|
| `lock.h`, `lock.c`   | The transition table, `lock_step`, `lock_event`, and the state, event and action names. |
| `main.c`             | Runs the lock on stdin and prints each step. |
| `data/codes.txt`     | Sample keypad input for `lock`; see [Usage](#usage). |
| `tests/test_lock.c`  | Unit tests for every function in `lock.c` and for the key sequences from the assignment. |
| `Makefile`           | Builds `lock` and the tests into `build/`. |
| `docs/PRD.md`        | The assignment's requirements and the decisions made beyond them. |
| `docs/algorithms.md` | Pseudocode for `lock_event`, `main` and the sequence tests. |
| `AI_USAGE.md`        | Which AI tools were used, for what, and what they got wrong. |
| `check.h`, `check.c` | Bonus: `fsm_check`, which finds the mistakes a transition table can hold. |
| `tests/test_check.c` | Unit tests for `fsm_check`, on broken copies of `lock_table`. |
| `BONUS.md`           | The checker's output on three broken tables, and why each is a real bug. |

### Build and test

```sh
make        # builds build/lock
make test   # builds (if needed) and runs build/tests/test_lock
make bonus  # builds (if needed) and runs build/tests/test_check
make clean  # removes build/
```

Everything is built with `gcc -std=c11 -Wall -Wextra` and compiles with no
warnings. `build/tests/test_lock` prints one line per check (`PASS` or
`FAIL`) followed by a summary, and exits nonzero if any check failed.

Everything is also built with `-fsanitize=address,undefined`, so an
out-of-bounds read of a table aborts with a report instead of passing
silently.

### Usage

```sh
build/lock < data/codes.txt
```

`lock` starts in `LOCKED` and reads its input one character at a time. For
each event it prints the state, the character, the event, the next state and
the action; after the input, it prints the final state.

- `0` to `9` are digits, `c` is the door closing, and `t` is the timer running
  out. Only lowercase `c` and `t` count.
- Whitespace (spaces, line endings, a Windows file's `\r`) is skipped.
- Any other character is skipped with a line on stderr, such as
  `lock: ignoring 'C'`, so a typo is noticed without changing what goes to
  stdout. A byte that cannot be printed is shown in hex.

The whole file runs on one lock, so each line starts in the state the line
before it left. `data/codes.txt` is ordered so that each line starts in
`LOCKED`:

| Line | Shows |
|------|-------|
| `3314` | A wrong digit starts over: the second `3` throws away the first, so `1` and `4` are wrong too. |
| `31t4` | The timer running out starts over, silently. |
| `314c` | The code opens the door, and closing it locks it again. |
| `314`  | The code opens the door, and it stays open. |
| `C`    | An uppercase `C` is not an event; it is reported on stderr. |

The output on stdout:

```
LOCKED 3 RIGHT -> GOT1 NONE
GOT1 3 WRONG -> LOCKED BEEP
LOCKED 1 WRONG -> LOCKED BEEP
LOCKED 4 WRONG -> LOCKED BEEP
LOCKED 3 RIGHT -> GOT1 NONE
GOT1 1 RIGHT -> GOT2 NONE
GOT2 t TIMEOUT -> LOCKED NONE
LOCKED 4 WRONG -> LOCKED BEEP
LOCKED 3 RIGHT -> GOT1 NONE
GOT1 1 RIGHT -> GOT2 NONE
GOT2 4 RIGHT -> OPEN UNLOCK
OPEN c CLOSE -> LOCKED LOCK
LOCKED 3 RIGHT -> GOT1 NONE
GOT1 1 RIGHT -> GOT2 NONE
GOT2 4 RIGHT -> OPEN UNLOCK
final: OPEN
```

and on stderr, `lock: ignoring 'C'`.

### State diagram

Each edge is labeled `EVENT / ACTION`; an edge with no action does `NONE`.
Only the edges that change the state or cause an action are drawn. Every
other event leaves the state as it is and does `NONE`; the transition table
below lists all 16.

```
        WRONG / BEEP
          +----+
          |    v
       +--------+  RIGHT   +------+  RIGHT   +------+ RIGHT / UNLOCK +------+
  ---->| LOCKED |--------->| GOT1 |--------->| GOT2 |--------------->| OPEN |
       +--------+          +------+          +------+                +------+
         ^  ^  ^              |                 |                       |
         |  |  | WRONG / BEEP |                 |                       |
         |  |  +-- TIMEOUT ---+                 |                       |
         |  |                                   |                       |
         |  |     WRONG / BEEP, TIMEOUT         |                       |
         |  +-----------------------------------+                       |
         |                                                              |
         |        CLOSE / LOCK, TIMEOUT / LOCK                          |
         +--------------------------------------------------------------+
```

### Transition table

Each cell is the next state and the action.

| State \ Event | `E_RIGHT` | `E_WRONG` | `E_CLOSE` | `E_TIMEOUT` |
|---------------|-----------|-----------|-----------|-------------|
| `LOCKED` | `GOT1`, `NONE`   | `LOCKED`, `BEEP` | `LOCKED`, `NONE` | `LOCKED`, `NONE` |
| `GOT1`   | `GOT2`, `NONE`   | `LOCKED`, `BEEP` | `GOT1`, `NONE`   | `LOCKED`, `NONE` |
| `GOT2`   | `OPEN`, `UNLOCK` | `LOCKED`, `BEEP` | `GOT2`, `NONE`   | `LOCKED`, `NONE` |
| `OPEN`   | `OPEN`, `NONE`   | `OPEN`, `NONE`   | `LOCKED`, `LOCK` | `LOCKED`, `LOCK` |

Two entries are not set by the assignment's description of the lock:

- **`LOCKED` on `E_TIMEOUT`**: no timer runs while the lock is locked, so a
  real lock never sees this (though typing `t` in `LOCKED` produces it), but
  every entry must be filled. It stays `LOCKED` and does `NONE`;
  `A_UNDEFINED` is kept to mean "an entry left out".
- **`OPEN` on `E_RIGHT`**: `lock_event` never produces it, since every digit
  is wrong while open, but a caller can pass it to `lock_step` directly. Like
  any digit while open, it does nothing.

### Input ranges and boundary behavior

**`state_t lock_step(state_t s, event_t e, action_t *action)`**
- Asserts that `s` and `e` are in range and that `action` is not `NULL`: a
  bad argument is a bug in the caller, so it stops the program instead of
  reading outside the table.

**`int lock_event(state_t s, char c)`**
- Asserts that `s` is in range.
- Returns `-1` for anything that is not a digit, `c` or `t`, including `C`,
  `T`, whitespace and bytes above 127.

**`state_name`, `event_name`, `action_name`**
- Return the name without its `E_` or `A_` prefix (`GOT2`, `RIGHT`,
  `UNLOCK`), or `?` for a value out of range.
