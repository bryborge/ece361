# HW5 PRD: The combination lock

Source: *Homework 5, ECE 361, Fall 2026, Revision 0* (Oct 27, 2026), cited
as **[HW]**. Pseudocode for the algorithms is in
[algorithms.md](algorithms.md), cited as **[ALG]**.

Requirements (sections 2 to 6) come from the assignment and are graded.
Section 7 gives implementation recommendations: they are the suggested way to
meet the requirements, not requirements themselves. Section 8 records what the
assignment leaves open: what has been decided, and what is still open.

## 1. Overview

A combination lock written as a finite state machine. The states, the events
and the transitions go into a table indexed by state and event, and in C a
single lookup in that table replaces a chain of `if`s. [HW]

| Milestone | Date |
|-----------|------|
| Code pushed to `hw05/` | Sun, Nov 1, 2026, 11:59pm |
| Shine window opens | Mon, Nov 2, 2026, morning |
| Shine due | Wed, Nov 4, 2026, 11:59pm |

Grading is Satisfactory / Not Yet. AI tools are allowed for the code. The
Shine lasts 10 to 15 minutes and is about your own code. It allows no notes,
no second screen, no AI and no other person, and the webcam must be on. [HW]

**Who writes what.** We settled the design together. Claude writes the code
and the documentation, in small pieces, and stops after each one for review.
The user reviews each piece and manages git; Claude runs git only when the
user hands a specific task over.

## 2. Deliverables

All in `hw05/` at the deadline. [HW]

| File | Contents |
|------|----------|
| `lock.h` | The interface in section 3.1, with a comment per function. |
| `lock.c` | `lock_table`, `lock_step`, `lock_event` and the name functions. |
| `main.c` | The program in section 4.1. |
| `data/` | At least one input file of your own, e.g. `data/codes.txt`. |
| `tests/test_lock.c` | The tests in section 4.2. |
| `Makefile` | The targets in section 4.3. |
| `README.md` | How to build and test, the state diagram and the transition table (section 4.4). |
| `AI_USAGE.md` | Not specified in HW5. In hw04 it covered the tools used, what for, and one thing a model got wrong. Claude drafts it together with the README, and the user reviews it. |

The bonus adds `check.h`, `check.c`, `tests/test_check.c` and `BONUS.md`
(section 5).

## 3. Part 1: The table

### 3.1 `lock.h`

Use exactly these types, and add a comment per function. [HW]

```c
typedef enum { LOCKED, GOT1, GOT2, OPEN, N_STATES } state_t;
typedef enum { E_RIGHT, E_WRONG, E_CLOSE, E_TIMEOUT, N_EVENTS } event_t;
typedef enum { A_UNDEFINED, A_NONE, A_UNLOCK, A_LOCK, A_BEEP, N_ACTIONS } action_t;
typedef struct { state_t next; action_t action; } transition_t;

extern const transition_t lock_table[N_STATES][N_EVENTS];
state_t     lock_step(state_t s, event_t e, action_t *action);
int         lock_event(state_t s, char c);
const char *state_name(state_t s);   /* also event_name, action_name */
```

The header also needs an include guard. `A_UNDEFINED` is 0 on purpose: a
table entry that is left out is zero-initialized, so it reads as
`{LOCKED, A_UNDEFINED}`, and the bonus checker can detect it. [HW, Bonus]

### 3.2 The lock's behavior

- The code is **3-1-4**. Each right digit moves the lock one step on, and the
  third one opens the door (`UNLOCK`).
- A wrong digit beeps (`BEEP`) and starts over from the beginning, even in the
  middle of the code.
- While a code is being typed, a 5-second timer runs. If it runs out, the lock
  starts over silently.
- While the door is open, digits do nothing. Closing the door, or the timer
  running out, locks it again (`LOCK`).
- Closing the door while it is already locked, or while a code is being typed,
  changes nothing.

[HW]

The transition table that follows from these rules is below. Each cell gives
the next state and the action. The two cells marked \* are not settled by the
rules; they were decided separately (section 8.1). The other 14 come straight
from the rules. All 16 entries have been reviewed and confirmed.

| State \ Event | `E_RIGHT` | `E_WRONG` | `E_CLOSE` | `E_TIMEOUT` |
|---------------|-----------|-----------|-----------|-------------|
| `LOCKED` | `GOT1`, `NONE` | `LOCKED`, `BEEP` | `LOCKED`, `NONE` | `LOCKED`, `NONE` \* |
| `GOT1`   | `GOT2`, `NONE` | `LOCKED`, `BEEP` | `GOT1`, `NONE`   | `LOCKED`, `NONE` |
| `GOT2`   | `OPEN`, `UNLOCK` | `LOCKED`, `BEEP` | `GOT2`, `NONE` | `LOCKED`, `NONE` |
| `OPEN`   | `OPEN`, `NONE` \* | `OPEN`, `NONE` | `LOCKED`, `LOCK` | `LOCKED`, `LOCK` |

### 3.3 Functions

| Function | Behavior |
|----------|----------|
| `lock_table` | All 16 entries, in `lock.c`, written with designated initializers (`[GOT1] = { ... }`). [HW] |
| `lock_step` | Exactly one table lookup. No `if` or `switch` on the state. Returns the next state and stores the action through `action`. [HW] |
| `lock_event` | A digit is `E_RIGHT` if it is the next digit of the code for state `s`, and `E_WRONG` otherwise. `c` is `E_CLOSE`, `t` is `E_TIMEOUT`, and anything else is `-1`. [HW] |
| `state_name`, `event_name`, `action_name` | Read from `const char *` arrays indexed by the enum. Return `"?"` for a value out of range. [HW] |

The names are printed without their prefixes (`RIGHT`, `UNLOCK`), as in the
spec's example output (section 4.1). [HW]

## 4. Part 2: Program, tests, Makefile, README

### 4.1 `main.c`

Starts in `LOCKED` and reads standard input one character at a time. For each
event it prints one line: the state, the character, the event, the next state
and the action. After the last event it prints `final: STATE`. [HW]

```
GOT2 4 RIGHT -> OPEN UNLOCK
```

Characters that are not events are handled as decided in section 8.1.

### 4.2 `tests/test_lock.c`

At minimum [HW]:

| # | Case |
|---|------|
| 1 | Every one of the 16 table entries. |
| 2 | `lock_event` for a right digit and a wrong digit in each state, and while open. |
| 3 | `lock_event` for `c`, `t` and other characters. |
| 4 | `3 1 4` opens. |
| 5 | `3 3 1 4` does not open. |
| 6 | `3 1 t 4` does not open. |
| 7 | `3 1 4 c` locks again. |

Print PASS or FAIL for each test, then a summary. Exit with a nonzero status if
any test fails. [HW]

Section 8.1 adds tests for the name functions and fixes how the table and the
key sequences are checked.

### 4.3 `Makefile`

| Target | Does |
|--------|------|
| `make` | Builds `lock`, with no warnings. |
| `make test` | Builds and runs the tests. |
| `make clean` | Removes everything that was built. |

Each `.o` rule lists the headers that its source includes. [HW]

The Makefile is written and verified. The binaries are `build/lock` and
`build/tests/test_lock`, and the flags are hw04's (section 8.1).

### 4.4 `README.md`

How to build and test, the state diagram (in ASCII or as a photo of your
drawing) and the transition table (as a Markdown table). [HW]

## 5. Bonus: a table checker (deferred)

Worth one revision token. Write `int fsm_check(const transition_t
table[N_STATES][N_EVENTS], state_t start, FILE *out)` in `check.c` and
`check.h`. It prints one line per problem and returns the number of problems
it found. The problems are:

- an entry left out (`A_UNDEFINED`);
- a next state out of range;
- a state that can't be reached from `start` (search the table breadth first);
- a state with no way out.

Also add `tests/test_check.c` and a `make bonus` target that runs it. Then
write `BONUS.md`: the checker's output on three broken copies of your table,
and why each is a real bug for a lock. A good answer to one extra Shine
question earns the token. [HW]

**Status:** To be done after the core work in sections 3 and 4. Ask the
user about it once the core is finished.

## 6. Acceptance criteria (Satisfactory)

- [ ] Every deliverable in section 2 is in the repository at the deadline.
- [ ] `make` builds with no warnings.
- [ ] `make test` passes.
- [ ] `make clean` works.
- [ ] `lock_step` is one table lookup.
- [ ] The Shine is completed by its deadline and meets the bar.

[HW, Deliverable, Shine, and requirements]

## 7. Implementation recommendations

The algorithms are written out step by step
in [ALG]:

- **`lock_event`**: uses a per-state `NEXT_DIGIT` array and rejects
  non-digits before the lookup.
- **`main`**: the read loop, the whitespace skip, the report to stderr and the
  event line.
- **`run()`**: the helper the tests use to run a key sequence.

Other notes:

- **Name arrays.** Write them with designated initializers (`[GOT1] = "GOT1"`)
  so that each name sits next to its enum value. The table is written the same
  way.
- **Bounds checks on the enums.** gcc makes these enums unsigned, so
  `s < 0` triggers `-Wtype-limits` under `-Wextra`. A warning would fail
  "`make` with no warnings". Write `(unsigned)s >= N_STATES` instead.
- **`assert` in `lock_step`.** The asserts check that the caller kept the
  contract; they don't choose what to do based on the state. So they don't
  break the rule "no `if` or `switch` on the state".

## 8. Decisions beyond the spec

### 8.1 Decided

| Question | Decision | Why |
|----------|----------|-----|
| `LOCKED` + `E_TIMEOUT` | `{LOCKED, A_NONE}` | No timer runs while locked, but the entry must still be filled in. `A_UNDEFINED` would make the bonus checker report a deliberate entry as left out. |
| `OPEN` + `E_RIGHT` | `{OPEN, A_NONE}` | `lock_event` never produces it (see the next row), but a caller can pass it to `lock_step` directly. "While open, digits do nothing." |
| Digit while `OPEN` | Every digit is `E_WRONG`. | There is no next digit. `NEXT_DIGIT[OPEN]` holds a value that no digit can equal. |
| Where the code lives | `NEXT_DIGIT[N_STATES]`, one designated initializer per state | No branching on the state. Avoids `"314"[s]`, which only works because the enum order happens to match the number of digits typed. |
| Order of checks in `lock_event` | `c`, then `t`, then reject non-digits, then the lookup | Rejecting non-digits first means only a digit can be `RIGHT` or `WRONG`. |
| Out-of-range input to `lock_step` | `assert` that `s` and `e` are in range and that `action` is not `NULL` | Fail fast when the contract is broken. Returning a safe default would hide the bug. |
| Out-of-range state in `lock_event` | `assert` | `-1` keeps a single meaning: "not an event". |
| Upper-case `C` and `T` | Not events; they return `-1`. | The spec defines the interface as lowercase. |
| Non-events in `main` | Whitespace is skipped silently. Anything else is skipped with `lock: ignoring 'x'` on stderr. | Line endings, including CRLF's `'\r'`, are expected framing. Any other character is probably a typo and should not disappear without a word. stdout keeps exactly the spec's format. |
| Exit status of `main` | 0, even when characters were ignored | Same as hw04's `events`: the stderr line is the report, and the run itself succeeded. |
| `data/codes.txt` | Keypad input only, with no comments. One line for each of the spec's four sequences (`314`, `3314`, `31t4`, `314c`), then a line with a stray `C`. | Comment text would contain events (`3`, `c`, `t`, ...), so each line is explained in the README instead. The `C` line shows the stderr report. |
| Sanitizers in the default build | Kept in `CFLAGS`, as in hw04. | The grader's machine has run past homeworks built with these flags without trouble. |
| `AI_USAGE.md` | Claude drafts it at the same time as the README, and the user reviews it. | It records the user's own account of how AI was used. |
| Build output | `build/lock` and `build/tests/test_lock`. Flags: `-std=c11 -Wall -Wextra -g -I. -fsanitize=address,undefined`. | The repo's convention, from hw04. The README gives the path to the binary. |
| Testing the table | Write the 16 expected entries out independently and check each through `lock_step`. | Comparing `lock_table` with itself proves nothing. Checking through `lock_step` also tests the lookup. |
| Testing the sequences | Check the final state and the last action. | Catches a table that reaches `OPEN` but never emits `UNLOCK`. |
| Non-event in a test string | Record a FAIL and stop that sequence. | Letting `lock_step`'s assert fire would abort the program and lose the summary and the nonzero exit status. |
| Testing the name functions | Check the last valid value, `N_*` and `-1` for each. | This is where an off-by-one in the bounds check would show. |
| README diagram | ASCII. Draw only the edges that change the state or emit an action, plus the note "all other events: stay, NONE". | Sixteen edges would clutter the diagram. The Markdown table lists every entry. |

### 8.2 Still open

1. **`A_UNDEFINED` at run time.** Should `lock_step` also assert that the
   action it looks up is not `A_UNDEFINED`? `fsm_check` would catch a
   missing entry before the program runs, and an assert would catch one
   while it runs. This will be settled later, together with the bonus.

## 9. Out of scope

The project's next step is to turn the device's transition table into
`project/fsm.h` and `fsm.c`, with an enum for the states and the events and a
table indexed by both. It isn't graded and isn't on the Shine. [HW]

## 10. Shine preparation

Sample question from the assignment [HW]:

- "Why does 3 3 1 4 not open, and which table entry would you change so that
  it does?"

  **Trace:** `3` takes `LOCKED` to `GOT1`. The second `3` is `E_WRONG` in
  `GOT1`, so the lock goes back to `LOCKED` and beeps. Starting over throws
  that `3` away instead of counting it as a new first digit. `1` is then
  `E_WRONG` in `LOCKED` (it beeps and stays put), and so is `4`. The run ends
  in `LOCKED`, and the last action is `BEEP`.

  **The change:** `GOT1` + `E_WRONG` goes to `{GOT1, BEEP}`. Now `3 3 1 4`
  opens. But the entry can't tell which wrong digit was typed, so `3 5 1 4`
  opens too, and that's a real weakening of the lock. A correct fix needs
  `lock_event` to distinguish "wrong, but it's the code's first digit".
  That is the same idea as the failure function in string matching (KMP).

Questions the design invites:

- Why is `lock_step` a single lookup, and why is the `assert` in it not a
  branch on the state?
- Why is `A_UNDEFINED` 0?
- Why does `lock_event` reject non-digits before it reads `NEXT_DIGIT`?
- Why do the tests write out the expected table rather than read
  `lock_table`?
