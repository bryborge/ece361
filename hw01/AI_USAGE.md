# AI Usage

## Tools

| Tool | Model | Used for |
|------|-------|----------|
| Gemini | Pro | Answer questions while hand-implementing Part 2 |
| Claude | Sonnet 5.5 | Doc comments, README docs, and capturing architecture trade off of fmemopen vs tmpfile |
| Claude Code | Sonnet 5, Opus 5, Opus 5.5 | Part 3: planning interview, requirements doc, implementation, tests, and README |

## Description

Gemini Pro model was used for question and answers within a chat context while I implemented by hand. This made it possible to learn-by-doing while leveraging an "extra set of eyes" to validate my thinking and check my work.

Claude Sonnet 5.5 was used to assist in writing documentation in the code, in README, and to capture an architectural decision that I was on the fence about during implementation (found in `docs/test-portability/`). The decision, tradeoffs, and explanation can be found in that document.

Part 3 was implemented by Claude Code, but only after a requirements doc was generated temporarily (to guide a stronger model's implementation). Claude (Sonnet 5) built that doc by interviewing me about each design decision before any code was written: the shape of `status_t`, how an invalid mode is reported, what happens to the reserved bit, and which test words to use. I then switched to Opus 5 and later Opus 5.5 for implementation, which went one small chunk at a time, each paused for my review. I kept all git operations to myself, and edited the output directly wherever I preferred my own style.

## What it got wrong

### Part 2

In this implementation, one of the most glaring things AI got wrong was when it tried to add several extra flags to `CFLAGS` in the Makefile. When I asked it why it placed these extra flags there, it said it had to in order to catch edge cases when tests were run. When I questioned that reasoning, it said "I'll show instead of tell". When it did, it realized that it was wrong, that it wasn't necessary, and confessed that its reasoning was incorrect.

### Part 3

**The set point's type.** During planning, Claude recommended `int8_t` for `setpoint`, and I agreed, since it matches the -128 to 127 range exactly. Once the tests passed, Claude built deliberately broken copies of `status.c` to check that the tests would catch real bugs. A copy that skipped `sign_extend` entirely still passed every test. Storing a raw byte into an `int8_t` sign-extends it on gcc by itself, so the field type was quietly doing the work the code was supposed to do, and no test could tell the difference. I asked whether a different type would fix that. With `int32_t` (the type `sign_extend` returns), the broken copy fails two tests. `int8_t` had not even saved memory: padding keeps `status_t` at 12 bytes either way. So the original recommendation cost testability and bought nothing.

**The test words had a gap.** Claude proposed the list of test words for `status_unpack`, and called it complete. The same broken-copy check showed otherwise: in every test word, COOL, FAN, and FAULT were either all 0 or all 1, so a flag reading its neighbor's bit would pass. Four more words, each setting one flag alone, closed the gap.

## Other Notes

If you are seeing this version of `AI_USAGE.md`, it means that the AI-first development branch (for "Part 2" of Homework 1) was not accepted. I ended up implementing this project twice -- once with an AI-first development workflow, and once where I physically typed the implementation and leveraged AI as a rubber-ducky (this version). Both branches are in Github for posterity as closed and merged PRs respectively. I notice one very interesting thing between the versions:

The AI-first branch of work is very similar, but more memory-optimized (for example, returning a calculation without storing it in a variable first). This implementation uses named variables and is written in a style that is less optimal in terms of performance but more readable, especially for a student practicing C programming. In a world of tradeoffs, I chose this implementation because it is easier to reason about and reflects my software development "style."

**Reference:**

- [(Part 2) AI-Native Approach](https://github.com/bryborge/ece361/pull/1)
- [(Part 2) AI-Rubber Duck Approach](https://github.com/bryborge/ece361/pull/2)
- [(Part 3) AI-Native Approach](https://github.com/bryborge/ece361/pull/3)
