# Bonus: a table checker

`fsm_check(table, start, out)` in `check.c` writes one line per problem it
finds in a transition table and returns how many it found. It looks for:

- **An entry left out.** Its action is `A_UNDEFINED`, which is 0, so any
  entry the initializer skips is caught.
- **A next state out of range.**
- **A state that cannot be reached from `start`.** It searches breadth first
  over the table.
- **A state with no way out.** No event takes it to a different state.

Entries that are left out or out of range are reported once and are not
followed by the other two checks. A left-out entry reads as
`{ LOCKED, A_UNDEFINED }`, so following it would make `LOCKED` look
reachable, or look like a way out, when no real transition leads there.

```sh
make bonus  # builds (if needed) and runs build/tests/test_check
```

On `lock_table` itself, `fsm_check(lock_table, LOCKED, stdout)` writes
nothing and returns 0. Below are three broken copies of it. Each is the
table with one or two entries changed, and the output is what `fsm_check`
writes for each, starting from `LOCKED`. `tests/test_check.c` checks the
same output and counts.

## 1. Closing the door never throws the bolt

`[OPEN][E_CLOSE]` is left out of the initializer, as if the line were
deleted:

```
OPEN CLOSE: entry left out (A_UNDEFINED)
```

Returns 1.

**Why it is a real bug:** the left-out entry reads as `{ LOCKED, A_UNDEFINED }`.
When the door closes, the lock's state says `LOCKED`, but the action is not
`A_LOCK`, so nothing locks the door. The lock believes it is locked while
the door is free to open, and every later code is checked as if it were
locked. Nothing fails at run time, so without the checker this is found by
someone walking through a door that should have been locked.

## 2. The right code leads nowhere

`[GOT2][E_RIGHT]` has its next state mistyped as `N_STATES` (4), one past
`OPEN`, the kind of slip an `OPEN + 1` or a reordered enum makes:

```
GOT2 RIGHT: next state out of range
OPEN: not reachable from LOCKED
```

Returns 2.

**Why it is a real bug:** typing the right code is the one thing that should
open the lock, and it sends the lock to a state that does not exist. The
next `lock_step` would index past the end of `lock_table`. Here its assert
stops the program, and without the assert it would read memory that is not
part of the table. The second line shows the consequence: no other entry
leads to `OPEN`, so the door can never be opened at all.

## 3. Once open, never locked again

`[OPEN][E_CLOSE]` and `[OPEN][E_TIMEOUT]` are both written as
`{ OPEN, A_NONE }`, as if the `E_WRONG` line had been copied down and not
edited:

```
OPEN: no way out
```

Returns 1.

**Why it is a real bug:** every entry is filled in and every next state is
in range, so the first two checks have nothing to say. But no event takes
`OPEN` anywhere else. After the first correct code the lock stays open
forever: closing the door and the timer both do nothing. A lock that can be
opened but never relocked fails its one job, and the program would run
without any sign of trouble.
