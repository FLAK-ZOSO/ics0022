#!/usr/bin/env bash
set -u

encrypt=./build/fiorino-encrypt
decrypt=./build/fiorino-decrypt
delete=./build/fiorino-delete

failures=0
assert_failure() {
    "$@" >/tmp/fiorino-cli.stdout 2>/tmp/fiorino-cli.stderr
    status=$?
    if (( status == 0 )); then
        printf 'expected failure: %q\n' "$*" >&2
        failures=$((failures + 1))
    fi
}

[[ -x "$encrypt" ]] || { echo "missing $encrypt" >&2; exit 1; }
[[ -x "$decrypt" ]] || { echo "missing $decrypt" >&2; exit 1; }
[[ -x "$delete" ]] || { echo "missing $delete" >&2; exit 1; }

assert_failure "$encrypt"
assert_failure "$decrypt"
assert_failure "$delete"
assert_failure "$encrypt" -f
assert_failure "$decrypt" -f
assert_failure "$delete" -f
assert_failure "$encrypt" -x
assert_failure "$decrypt" -x
assert_failure "$delete" -x

if (( failures != 0 )); then
    exit 1
fi

printf 'CLI usage tests passed\n'
