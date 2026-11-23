#!/bin/sh
#
# Replay a scenario and compare the program's output with the expected output.
# Fails if the program fails or if the two differ, and prints the difference.
#
# usage: run_scenario.sh PROGRAM SCENARIO EXPECTED
set -eu

program=$1
scenario=$2
expected=$3

if ! actual=$("$program" "$scenario"); then
    echo "$program failed on $scenario" >&2
    exit 1
fi

printf '%s\n' "$actual" | diff -u "$expected" -
