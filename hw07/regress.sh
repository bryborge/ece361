#!/bin/sh
# regress.sh: the HW7 regression test. Runs ./balance on every file in inputs/
# and compares its stdout and exit status with the file of the same name in
# expected/, which holds the output followed by a line "exit N".
# Usage, in hw07/ after make:   sh regress.sh
# Exits with 1 if any case differs, so that make test fails.
fail=0
for input in inputs/*.txt; do
    name=$(basename "$input")
    # The status is part of the text compared, so diff's status is the
    # pipeline's verdict. stderr (a sanitizer report) goes to the terminal.
    if { ./balance < "$input"; echo "exit $?"; } | diff - "expected/$name"; then
        echo "PASS  regression $name"
    else
        echo "FAIL  regression $name"
        fail=1
    fi
done
exit $fail
