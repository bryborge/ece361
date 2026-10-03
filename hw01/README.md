# Homework 1

A Bit-manipulation library.

## Testing `print_binary`

### Helper function to capture/validate stdout

The assignment explicitly states that the `print_binary` function should have the signature: `void print_binary(uint32_t x, int width)`. I considered changing the signature so that print_binary returns a value, so that it might be simpler to use with the proposed test template, but I opted out of that so that I could stay true to the intent behind the function's name. Below is a list of design choices I made to implement a helper function that captures stdout/stderr so that it can be tested in a nearly similar way as the other functions in this library:

- `capture_print_binary` swaps `stdout`/`stderr` for `fmemopen` streams, calls `print_binary`, and restores them.
- Direct assignment (`stdout = mem`). The usual redirect works at the operating-system level, but a memory stream exists only inside the C library, so the OS can't send output to it. Works on Linux and macOS; the C standard doesn't guarantee it.
- `_POSIX_C_SOURCE` defined in the test so that `bits.c` stays strict C11.
- Streams opened and closed per call, so no state leaks. `fclose` flushes, so no `fflush`.
- Buffers zeroed; 64 bytes fits every tested output.
- `fmemopen` NULL exits with an error. With a fixed mode and buffer, only out-of-memory (`ENOMEM`) can cause this.
- Error cases check `stderr` is non-empty

I considered moving the helper function and data struct to its own helper file, but it didn't seem necessary yet (nothing else would use it), so following YAGNI principle, I opted to hold off on that kind of clean up refactor.
