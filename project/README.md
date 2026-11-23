# Garage Door Opener

This is the final project for ECE361 - Computer System Organization at Portland State University. It is an embedded system designed to behave like a garage door opener. It uses the `iom361` library, version 4.0 (r4), built specifically for this class to imitate an I/O device attached to a microcontroller.

## Assignment and Specification

The project assignment details are captured in the assignment document found in [ASSIGNMENT.md](ASSIGNMENT.md).

The project spec can be found in [SPEC.md](SPEC.md).

## Getting Started

### Requirements

- gcc, with C11 support
- make

### Building

```sh
make
```

The program executable is in `build/garage`. It replays a scenario file, for example:

```sh
./build/garage scenarios/1_normal.txt
```

`make clean` removes everything that was built.

### Testing

```sh
make test
```

This builds and runs every unit test and scenario test, printing PASS or FAIL for each check, and ends with one line giving the number that passed and failed. Everything is built with AddressSanitizer and UndefinedBehaviorSanitizer, so a leak or undefined behavior also fails the run.

Each scenario test replays `scenarios/<name>.txt` and compares what the program prints with `scenarios/<name>.expected`.
