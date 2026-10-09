#!/usr/bin/env bash
set -u

failures=0
run_rejected() {
    "$@" >/tmp/fiorino-cli.stdout 2>/tmp/fiorino-cli.stderr
    status=$?
    if (( status == 0 )); then
        printf 'unexpected success: %q\n' "$*" >&2
        failures=$((failures + 1))
    fi
}

for value in \
    . .. ../ .../ ./../ ".\./" "%2e%2e%2f" \
    "" "/" "//" "$(printf 'x%.0s' {1..4096})" \
    "vault with spaces" "vault;touch /tmp/fiorino-cli-pwned"; do
    run_rejected ./build/fiorino-encrypt -v "$value"
    run_rejected ./build/fiorino-decrypt -v "$value"
done

run_rejected ./build/fiorino-encrypt --
run_rejected ./build/fiorino-decrypt --
run_rejected ./build/fiorino-encrypt -v
run_rejected ./build/fiorino-decrypt -v

if [[ -e /tmp/fiorino-cli-pwned ]]; then
    echo 'possible command injection detected' >&2
    failures=$((failures + 1))
    rm -f /tmp/fiorino-cli-pwned
fi

if (( failures != 0 )); then
    exit 1
fi

printf 'CLI rejection tests passed\n'
