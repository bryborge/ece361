# Homework 5: Combination Lock

> In Homework 5, we learn about finite state machines written as a table.
> The project is a combination lock that opens on 3-1-4.

### What is this?

A combination lock, built as a state machine. It has four states: `LOCKED`,
`GOT1` and `GOT2` (you've typed one or two digits of the code so far), and
`OPEN`. It reacts to four events: a right digit, a wrong digit, the door
closing, and the 5-second timer running out.

The whole behavior of the lock lives in one table, `lock_table`. It's indexed
by state and event, and it tells you the next state and what to do about it
(`NONE`, `UNLOCK`, `LOCK` or `BEEP`). This makes it so `lock_step` is just a
table lookup.

That makes it a Mealy machine: the actions happen on the transitions, not in
the states. Going from `GOT2` to `OPEN` unlocks the door, but sitting in
`OPEN` doesn't do anything.

`main` also checks the table once at startup: if any entry was left out (its
action is still `A_UNDEFINED`), an assert stops the program before the lock
runs at all.

`lock_event` is what turns a keypress into an event. Whether a digit is
"right" depends on where you are in the code, so it looks up the expected
digit in `next_digit`, which is indexed by state. `OPEN` doesn't expect any
digit, so every digit counts as wrong once the door is open.

| File | What it's for |
|------|---------------|
| `lock.h`, `lock.c`   | The transition table, `lock_step`, `lock_event`, and the state, event and action names. |
| `main.c`             | Runs the lock on stdin and prints each step. |
| `data/codes.txt`     | Sample keypad input; see [Usage](#usage). |
| `data/codes.expected` | What `lock` should print to stdout for `data/codes.txt`; `make test` checks it. |
| `data/codes.expected.err` | What `lock` should print to stderr for `data/codes.txt`; `make test` checks it. |
| `tests/test_lock.c`  | Tests for every function in `lock.c`, plus the key sequences from the assignment. |
| `check.h`, `check.c` | Bonus: `fsm_check`, which looks for mistakes in a transition table. |
| `tests/test_check.c` | Tests for `fsm_check`, run on broken copies of `lock_table`. |
| `BONUS.md`           | What the checker says about three broken tables, and why each one is a real bug. |
| `Makefile`           | Builds everything into `build/`. |
| `docs/PRD.md`        | The assignment's requirements, plus the decisions we made where it left things open. |
| `AI_USAGE.md`        | Which AI tools I used, for what, and what they got wrong. |

### Build and test

```sh
make        # builds build/lock
make test   # builds (if needed), runs test_lock, and checks the trace
make bonus  # builds (if needed) and runs build/tests/test_check
make clean  # removes build/
```

It's all compiled with `gcc -std=c11 -Wall -Wextra`, with no warnings. Both
test programs print a `PASS` or `FAIL` line per check, with a summary, and
exit nonzero if anything failed.

`make test` also runs `build/lock < data/codes.txt` and diffs its stdout
against `data/codes.expected` and its stderr against
`data/codes.expected.err`, so `main.c` gets tested too, not just `lock.c`.
That covers the typo report and the whitespace skip, which only show up on
stderr.

Everything's also built with `-fsanitize=address,undefined`, so if something
ever reads past the end of a table, the program stops with a report instead
of quietly carrying on.

### Usage

```sh
build/lock < data/codes.txt
```

`lock` starts out `LOCKED` and reads its input one character at a time. For
each event it prints the state, the character, the event, the next state and
the action. When the input runs out, it prints where it ended up.

- `0` to `9` are digits, `c` means the door closed, and `t` means the timer
  ran out. Only lowercase `c` and `t` count.
- Whitespace (spaces, line endings, the `\r` in a Windows file) is skipped.
- Anything else gets skipped too, but with a note on stderr like
  `lock: ignoring 'C'`, so typos don't slip by unnoticed and stdout stays
  clean. Bytes that can't be printed show up in hex.

One thing to know about `data/codes.txt`: the whole file runs through a single
lock, so each line picks up wherever the line before it left off. I ordered
the lines so that each one starts from `LOCKED`:

| Line | What it shows |
|------|---------------|
| `3314` | A wrong digit starts you over. The second `3` is wrong in `GOT1`, and it doesn't count as a fresh first digit, so the `1` and `4` after it are wrong too. |
| `31t4` | The timer running out starts you over, without a beep. |
| `314c` | The code opens the door, and closing it locks it again. |
| `314`  | The code opens the door, and it stays open. |
| `C`    | Uppercase `C` isn't an event, so it gets reported on stderr. |

Here's what lands on stdout:

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

Edges are labeled `EVENT / ACTION`, and an edge with no action does `NONE`.
I only drew the edges that change the state or actually do something. Every
other event leaves the lock where it is and does nothing; the table below
has all 16.

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

Each cell is the next state, then the action.

| State \ Event | `E_RIGHT` | `E_WRONG` | `E_CLOSE` | `E_TIMEOUT` |
|---------------|-----------|-----------|-----------|-------------|
| `LOCKED` | `GOT1`, `NONE`   | `LOCKED`, `BEEP` | `LOCKED`, `NONE` | `LOCKED`, `NONE` |
| `GOT1`   | `GOT2`, `NONE`   | `LOCKED`, `BEEP` | `GOT1`, `NONE`   | `LOCKED`, `NONE` |
| `GOT2`   | `OPEN`, `UNLOCK` | `LOCKED`, `BEEP` | `GOT2`, `NONE`   | `LOCKED`, `NONE` |
| `OPEN`   | `OPEN`, `NONE`   | `OPEN`, `NONE`   | `LOCKED`, `LOCK` | `LOCKED`, `LOCK` |

The assignment's description of the lock doesn't pin down two of these, so I
had to make a call:

- **`LOCKED` on `E_TIMEOUT`**: the timer isn't running while the lock is
  locked, so a real lock would never see this. (You can still type `t` in
  `LOCKED`, though.) Every entry has to be filled in, so it just stays
  `LOCKED` and does `NONE`. I didn't use `A_UNDEFINED` here, because that's
  reserved for "somebody forgot this entry".
- **`OPEN` on `E_RIGHT`**: `lock_event` never produces this, because every
  digit is wrong once the door is open. But nothing stops a caller from
  passing it to `lock_step` directly, so it does what any digit does while
  the door is open, which is nothing.

### Input ranges and boundary behavior

**`state_t lock_step(state_t s, event_t e, action_t *action)`**
- Asserts that `s` and `e` are in range and that `action` isn't `NULL`. A bad
  argument means the caller has a bug, so it's better to stop right there
  than to read something random from outside the table.

**`int lock_event(state_t s, char c)`**
- Asserts that `s` is in range.
- Returns `-1` for anything that isn't a digit, `c` or `t`. That includes `C`,
  `T`, whitespace, and bytes above 127.

**`state_name`, `event_name`, `action_name`**
- Return the name without its `E_` or `A_` prefix (`GOT2`, `RIGHT`,
  `UNLOCK`), or `?` if the value is out of range.
