# How It Was Used

Claude (Opus 5.5) was used to guide implementation, but was instructed to allow me to _do_ the implementing. What transpired was more like a paired programming session between me (learner) and it (C programming expert). When I was going in an unfruitful direction, it would ask questions related to bit manipulation techniques (ANDing, ORing, shifting, masking, etc) that we've explored in lecture.

## Prompt

> ... It's important to me to learn these implementation steps deeply so that I can speak to these decisions.  And if my solution is wrong, that's okay!  I still want to learn why it's wrong.

While not rigid, this prompt segment was sufficient in providing appropriate guidance for me to drive the implement by hand. This "guidance" approach was intentional, because I come from a tradition where understanding something deeply means physically engaging with it. As my subject matter expertise grows, my need to implement by hand will decrease, and the pragmatic choice then will be to let AI implement while I verify.

See: [The Pragmatic Programmer (book)](https://pragprog.com/titles/tpp20/the-pragmatic-programmer-20th-anniversary-edition/)

## What It Got Wrong

When I was attempting to figure out how I wanted to handle erroneous `width` values, the model strongly recommended that I change the function signature for `print_binary` to return an `int`. That likely would have been my choice as well, but I decided to interpret the Homework assignment exactly, and to keep `void` and return nothing for that function, which consequently pushes the responsibility of error handling down to the consumer.

So, it wasn't "wrong" per se, but its suggestions went against my interpretation of the assignment's stated constraints.
