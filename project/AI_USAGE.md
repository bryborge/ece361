# AI Usage

## Tools

| Tool | Model | Used for |
|------|-------|----------|
| Claude Code | Sonnet 5.5 | Week 2: CLAUDE.md, module stubs, Makefile, SPEC.md skeleton |

## What it got wrong

### Week 2

**Deleted files nobody asked it to delete.** I answered "b and remove the junk file" to a question about where iom361 should live. Claude took (b) to mean deleting the whole `iom361_r4/` folder, and ran `rm -rf` on it after moving out only the `.h` and `.c`. That removed the Makefile, README, demo, and test files, which I wanted to keep. It had listed the folder before deleting but never checked whether anything in it was needed, and did not confirm the wider reading of my answer first.

**Claimed the files were unrecoverable without checking git.** When I pointed out the mistake, Claude said there was no copy to restore from, since it assumed the folder was untracked, and asked me to re-download from Canvas. The folder was committed. I had to tell Claude to use git, and `git restore` brought the files back unchanged. Claude had seen the folder was missing from its first `git status` snapshot, which predated my adding the library, and never re-ran `git status` before declaring the loss permanent.

**Wrote a CLAUDE.md that was wrong about iom361.** It said the iom361 files are "copied in" to `project/`, and that rule was then overtaken by keeping them in `iom361_r4/`. Claude did not update the line, so CLAUDE.md still misdescribes the layout.
