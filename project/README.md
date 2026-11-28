# Garage Door Opener

This is the final project for ECE361 - Computer System Organization at Portland State University. It is an embedded system designed to behave like a garage door opener. It uses the `iom361` library, version 4.0 (r4), built specifically for this class to imitate an I/O device attached to a microcontroller.

## Assignment and Specification

The project assignment details are captured in the assignment document found in [ASSIGNMENT.md](docs/ASSIGNMENT.md).

The project spec can be found in [SPEC.md](docs/SPEC.md).

## Getting Started

### Requirements

- gcc, with C11 support
- make

### Building

```sh
make
```

The program executable is in `build/garage`. It replays a scenario file and prints one trace line per tick, for example:

```sh
./build/garage scenarios/1_normal.txt
```

With `--report` after the scenario, it replays the scenario without printing the trace, and then prints the moving average of the temperature over 20 readings:

```sh
./build/garage scenarios/6_report.txt --report
```

`make clean` removes everything that was built.

### Testing

```sh
make test
```

This builds and runs every unit test and scenario test, printing PASS or FAIL for each check, and ends with one line giving the number that passed and failed. Everything is built with AddressSanitizer and UndefinedBehaviorSanitizer, so a leak or undefined behavior also fails the run.

Each scenario test replays `scenarios/<name>.txt` and compares what the program prints with `scenarios/<name>.expected`. Scenario 6 is replayed with `--report`, so its expected output is the report rather than the trace.
