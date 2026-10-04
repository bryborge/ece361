# fmemopen vs tmpfile in test_bits.c

`tests/test_bits.c` uses `fmemopen` to capture `print_binary`'s stdout output
into an in-memory buffer for comparison in `prints_binary`. This is POSIX-only
and requires `_POSIX_C_SOURCE 200809L` defined before any headers are
included, which is why that define sits at the top of the file.

## Why fmemopen needs the POSIX define

Compiling with `-std=c11` restricts glibc's headers to strict ISO C11
declarations, hiding POSIX-only functions. Defining `_POSIX_C_SOURCE 200809L`
(POSIX.1-2008) tells glibc's `<stdio.h>` to also expose those POSIX
extensions, including `fmemopen`.

## Portability limitation

`fmemopen` does not exist on platforms without a POSIX-compatible libc, most
notably plain Windows built with MSVC. The `_POSIX_C_SOURCE` define itself is
harmless there (unrecognized feature-test macros are simply ignored), but the
call to `fmemopen` would fail to compile. glibc, musl, and macOS's libc are
all POSIX-ish and work as-is; Windows under WSL or MinGW also provides a
POSIX layer with `fmemopen`.

## Alternative: tmpfile()

`tmpfile()` is standard C (available since C89) and works on every platform,
including MSVC. Swapping to it would mean:

```c
FILE *mem = tmpfile();
if (mem == NULL) return false;

FILE *saved_stdout = stdout;
stdout = mem;
print_binary(x, width);
fflush(mem);
stdout = saved_stdout;

rewind(mem);
size_t n = fread(buf, 1, sizeof buf - 1, mem);
buf[n] = '\0';

bool matches = strcmp(buf, expected) == 0;
fclose(mem);
```

This also removes the need for the `_POSIX_C_SOURCE` define entirely.

## Tradeoff

`tmpfile()` buys portability at the cost of going through the filesystem
instead of memory:

- Each call creates and deletes a real (though usually auto-cleaned) file,
  so there's more I/O overhead per call than an in-memory buffer swap. For a
  handful of test assertions like these, this cost is negligible; it would
  only matter in a hot loop run many thousands of times.
- `tmpfile()` can fail if the environment can't create a file in the
  default temp location (e.g. certain restrictive sandboxes), a failure mode
  `fmemopen` doesn't have since it never touches the filesystem.

## Decision

Current environment (Linux/WSL) already supports `fmemopen` without issue, so
no change has been made. Revisit this if the test suite needs to build under
plain MSVC.
