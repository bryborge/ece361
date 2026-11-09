#!/bin/sh
# regress.sh: the HW6 regression test. Runs build/replay on every scenario in
# scenarios/ and compares its output with the file of the same name in expected/.
# Usage, in hw06/ after make:   sh regress.sh
# Exits with 1 if replay fails or any output differs, so that make test fails.
fail=0
for scenario in scenarios/*.txt; do
    name=$(basename "$scenario")
    # To a file, not a pipe: a pipeline's status is diff's, which would hide a
    # replay that printed the right trace and then failed.
    if build/replay "$scenario" > build/trace.out && diff build/trace.out "expected/$name"; then
        echo "PASS  regression $name"
    else
        echo "FAIL  regression $name"
        fail=1
    fi
done
exit $fail
