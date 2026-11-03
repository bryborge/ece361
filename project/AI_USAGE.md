# AI Usage

## Tools

| Tool | Model | Used for |
|------|-------|----------|
| Claude Code | Sonnet 5.5 | Week 2: CLAUDE.md, module stubs, Makefile, SPEC.md skeleton |
| Claude Code | Opus 5.5 | Week 5: reviewing my draft transition table, a design interview on the fixes, moving the confirmed design into SPEC.md |

## What it got wrong

### Week 2

**Deleted files nobody asked it to delete.** I answered "b and remove the junk file" to a question about where iom361 should live. Claude took (b) to mean deleting the whole `iom361_r4/` folder, and ran `rm -rf` on it after moving out only the `.h` and `.c`. That removed the Makefile, README, demo, and test files, which I wanted to keep. It had listed the folder before deleting but never checked whether anything in it was needed, and did not confirm the wider reading of my answer first.

**Claimed the files were unrecoverable without checking git.** When I pointed out the mistake, Claude said there was no copy to restore from, since it assumed the folder was untracked, and asked me to re-download from Canvas. The folder was committed. I had to tell Claude to use git, and `git restore` brought the files back unchanged. Claude had seen the folder was missing from its first `git status` snapshot, which predated my adding the library, and never re-ran `git status` before declaring the loss permanent.

**Wrote a CLAUDE.md that was wrong about iom361.** It said the iom361 files are "copied in" to `project/`, and that rule was then overtaken by keeping them in `iom361_r4/`. Claude did not update the line, so CLAUDE.md still misdescribes the layout.

### Week 5

**Overstated a risk to win an argument.** For tracking the door's position, Claude compared deriving it from the clock with stepping a counter each tick, and argued that the counter could be stepped twice in one tick. That only happens if the increment is put in the wrong place, which a single call before the event lookups avoids. The real argument, that a counter stores elapsed time a second time beside the timestamp, came only after I asked for pseudocode.

**A false claim that nearly caused a pointless rename.** Claude said that with position tracking, `OPENING` and `CLOSING` no longer time out but "arrive", so `E_TIMEOUT` should be renamed. Because position is derived from the clock, arriving at the end of travel is exactly a timeout. We spent a round on names before it corrected itself, and `E_TIMEOUT` kept its name.

**A script that edited more than it was meant to.** To remove the `E_NONE` column from the transition table, Claude ran a script that dropped a cell from every table row starting with a state name. That also matched three rows of the position table, and deleted their "Position now" cells. It noticed only on its next read of the file, and restored them when moving the table into SPEC.md.

**A design that crashed, defended by an overstated objection.** Claude proposed looking up events in the order barrier, timeout, press. In a final consistency pass it found that this order lets the door enter `CLOSING` at position 0 with no time allowed, so the next tick computes position -1 and the bounds `assert` fires. It had earlier rejected press before timeout, calling a cold press on `CLOSING`'s last tick a hole in the freeze guard, but my own table says the cold only matters in `CLOSED`. When I asked whether barrier, press, timeout would work, it did, and it is simpler: one timeout check and at most one transition per tick.
