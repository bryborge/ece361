# AI usage: HW1

## Tools

| Tool | Model | Used for |
|------|-------|----------|
| Claude | Sonnet 5.5 | Planning Part 2 |
| Claude Code (VS Code extension) | Opus 5.5 | Implementing Part 2 from the plan |

## Planning (Claude Sonnet 5.5)

A planning session produced `docs/hw01-part2-plan.md`: the function contracts, the design decisions, the test cases (Section 6), the Makefile, and the order of work. Two items outside the course slides were accepted there: `strcmp` in the tests and `-I.` in the Makefile.

## Implementation (Claude Opus 5.5)

Claude Code followed the plan step by step and wrote `README.md`, `bits.h`, `bits.c`, `Makefile`, `.gitignore` and `tests/test_bits.c`. For each function it added the tests first (cases copied from Section 6 of the plan), ran them against a deliberately wrong stub to see them fail, and then wrote the function. It stopped at every checkpoint without staging or committing; I made every commit.

It also suggested one test that was not in the plan: `format_binary` with a buffer of exactly the needed size must not write past it. I asked for it to be added, and it confirmed the check catches a version that writes one byte too many.

## What a model got wrong

The plan (Sonnet 5.5) said `sign_extend` should end with an explicit `(int32_t)` cast, and listed "no cast" as a rejected alternative, as if the cast mattered. It does not. A `return` statement already converts its value to the function's return type, so `return v;` and `return (int32_t)v;` perform the same implementation-defined conversion.

How it was found: Opus 5.5 wrote the cast as the plan said, then pointed out at the checkpoint that the cast changed no behavior. I asked whether it did anything at all. It compiled `bits.c` both ways and the assembly was identical at `-O0` and `-O2`, with no warnings under `-Wall -Wextra`. The cast only silences `-Wsign-conversion`, which this Makefile does not enable. The cast was dropped and the plan was updated to match.
