# Garage Door Opener

This is the final project for ECE361 - Computer System Organization at Portland State University. It is an embedded system designed to behave like a garage door opener. It uses the `iom361` library, version 4.0 (r4), built specifically for this class to imitate an I/O device attached to a microcontroller.

## Assignment and Specification

The project assignment details are captured in the assignment document found in [docs/ASSIGNMENT.md](docs/ASSIGNMENT.md).

The project spec can be found in [docs/SPEC.md](docs/SPEC.md).

## Getting Started

This project is built with [Meson](https://mesonbuild.com/). It builds the `garage_opener` program and test suite.

### Requirements

- A C11 compiler
- Meson 1.7 or later
- Ninja

### Installing

```sh
meson setup build
meson compile -C build
```

The program executable is in `build/garage_opener`. It replays a scenario file, for example:

```sh
./build/garage_opener scenarios/normal.txt
```

### Testing

The [Unity](https://github.com/ThrowTheSwitch/Unity) test framework is used by Meson under the hood.

```sh
meson test -C build
```

For verbose test output, run the above command with `-v`.

#### Test Coverage

> [!IMPORTANT]
>
> Coverage counters accumulate across runs, including runs of `garage_opener` itself. To reset them before a test run:
>
> ```sh
> ninja -C build clean-gcda
> ```

[gcovr](https://gcovr.com) is required for generating code coverage reports.

1.  Enable coverage.

    ```sh
    meson configure build -Db_coverage=true
    ```

2.  Build everything.

    ```sh
    meson compile -C build
    ```

3.  Run test suite.

    ```sh
    meson test -C build
    ```

4.  Generate the coverage reports.

    ```sh
    ninja -C build coverage
    ```

Follow the terminal output and open the desired report (in `build/meson-logs/`).
