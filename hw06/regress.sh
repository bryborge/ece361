#!/bin/sh
# regress.sh: the HW6 regression test. Runs build/replay on every scenario in
# scenarios/ and compares its output with the file of the same name in expected/.
# Usage, in hw06/ after make:   sh regress.sh
# Exits with 1 if any output differs, so that make test fails.
fail=0
for scenario in scenarios/*.txt; do
    name=$(basename "$scenario")
    if build/replay "$scenario" | diff - "expected/$name" > /dev/null; then
        echo "PASS  regression $name"
    else
        echo "FAIL  regression $name"
        fail=1
    fi
done
exit $fail
