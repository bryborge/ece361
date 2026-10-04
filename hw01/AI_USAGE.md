# AI Usage

## Tools

| Tool | Model | Used for |
|------|-------|----------|
| Gemini | Pro | Answer questions while hand-implementing Part 2 |
| Claude | Sonnet 5.5 | Doc comments, README docs, and capturing architecture trade off of fmemopen vs tmpfile |

## Description

Gemini Pro model was used for question and answers within a chat context while I implemented by hand. This made it possible to learn-by-doing while leveraging an "extra set of eyes" to validate my thinking and check my work.

Claude Sonnet 5.5 was used to assist in writing documentation in the code, in README, and to capture an architectural decision that I was on the fence about during implementation (found in `docs/test-portability/`). The decision, tradeoffs, and explanation can be found in that document.

## What it got wrong

In this implementation, one of the most glaring things AI got wrong was when it tried to add several extra flags to CFLAGS in the Makefile. When I asked it why it placed these extra flags there, it said it had to in order to catch edge cases when tests were run. When I questioned that reasoning, it said "I'll show instead of tell". When it did, it realized that it was wrong, that it wasn't necessary, and confessed that its reasoning was incorrect.

## Other Notes

If you are seeing this version of `AI_USAGE.md`, it means that the AI-first development branch was not accepted. I ended up implementing this project twice once with an AI-first development workflow, and once where I physically typed the implementation and leveraged AI as a rubber-ducky (this version). Both branches are in Github for posterity as closed and merged PRs respectively. I notice one very interesting thing between the versions:

The AI-first branch of work is very similar, but more memory-optimized (for example, returning a calculation without storing it in a variable first). This implementation uses named variables and is written in a style that is less optimal in terms of performance but more readable, especially for a student practicing C programming. In a world of tradeoffs, I chose this implementation because it is easier to reason about and reflects my software development "style."
