# Homework 6: Scenario Replay

> In Homework 6, we learn about memory-mapped I/O through iom361's registers.
> The project replays a scenario file into a trace, where a press of switch k
> toggles LED k.

### Build and test

> [!Note]
> The assignment says `./replay scenario.txt`. I build the binary in `build/`
> instead, with everything else the build produces, so run `build/replay` from
> `hw06/`.

```sh
make        # builds build/replay
make test   # builds (if needed), then runs the unit tests and regress.sh
make clean  # removes build/
```

### Usage

```sh
build/replay scenarios/<scenario>.txt
```
