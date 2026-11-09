# AI Usage

## Tools

| Tool | Model | Used for |
|------|-------|----------|
| Claude Code | Opus 5.5 | Guiding the build order, explaining the code as we went, writing most of the modules, the tests, the Makefile and `regress.sh`, and a draft of this file and the README |

## Description

I had already finished hw06, and built it a second time to see whether a cleaner
implementation would fall out. The goal was learning, so I ranked that above the
sheet, and asked Claude to work in small steps, the way a person would.

Claude scaffolded a "hello world" that lit LED 0, and laid out a path that grew
outward from `main.c`: read the switches, detect a press, toggle an LED, add a tick
loop, the clock, the sensor, the scenario file, and the regression test. It showed
the bugs along the way before fixing them, such as the plain LED write clearing the
other LEDs, and 91.0 °C reading back as 90.99998.

I extracted `panel_pressed` and `panel_released` myself. When I got stuck, I asked
Claude to point out the fixes without making them: types repeated at the call sites,
`!` where I needed `~`, and a `bool` return that lost which switch was pressed. Claude
wrote `panel_toggle`, `sensor.c`, `scenario.c`, the tests, the scenarios and
`regress.sh`, and I read each piece and asked about what I didn't follow.

I made the design calls: keeping the code flat with iom361 in `external/`, removing
the error checks in `main` that could never fire, keeping the datasheet's names `st`
and `srh`, rounding with `lround`, requiring the first tick to be 0, and removing
`panel_led`. I also cut back and rewrote many of the comments.

Claude wrote each expected trace by hand before running `replay` on it, and ran the
tests against deliberately broken copies of the code to show they catch the bug.

## What it got wrong

**Code before it was needed.** We had agreed that a function gets extracted only when
the running program needs it. Claude wrote `panel_led` anyway, because the sheet lists
it next to `panel_toggle`. I noticed it had no caller and had it removed. Once the
replay was done, I had Claude add it back to meet the sheet.

**Tests that proved less than they looked like.** Its first `sensor_read` tests still
passed with the 20-bit mask deleted, because iom361 keeps the upper bits at 0. Its
failed-read test accepted any nonzero return, not iom361's code. The broken-copy run
exposed the mask gap, and it added a test that sets the upper bits directly and
tightened the check.

**Comments that confused me.** Its comments on the `sscanf` lines in `scenario_load`
described the rarest case, a stray fifth word, and never said what each chunk was for.
I couldn't follow them. We rewrote them as the question each chunk answers.

**A wrong number.** It told me removing `panel_led` would fail 11 checks. The count
dropped by 13.
