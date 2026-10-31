#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-or-later
set -eu
if [ "$#" -ne 1 ]; then
    echo "Usage: $0 EPICS_HOST_ARCH" >&2
    exit 2
fi
export EPICS_CA_AUTO_ADDR_LIST=NO
export EPICS_CA_ADDR_LIST=127.0.0.1
export EPICS_CAS_INTF_ADDR_LIST=127.0.0.1
export EPICS_CA_SERVER_PORT=25064
export EPICS_CAS_SERVER_PORT=25064
export EPICS_CA_REPEATER_PORT=25065
log_file=$(mktemp "${TMPDIR:-/tmp}/numeric-network-ioc.XXXXXX")
test_dir=$(mktemp -d "${TMPDIR:-/tmp}/numeric-crash-test.XXXXXX")
export NUMERIC_TEST_CRASH_MARKER="$test_dir/entered"
ioc_pid=
client_pid=
start_ioc() {
    "./bin/$1/numericNetworkIoc" >>"$log_file" 2>&1 &
    ioc_pid=$!
}
stop_ioc() {
    if [ -n "$ioc_pid" ]; then
        kill "$ioc_pid" 2>/dev/null || true
        wait "$ioc_pid" 2>/dev/null || true
        ioc_pid=
    fi
}
cleanup() {
    if [ -n "$client_pid" ]; then
        kill "$client_pid" 2>/dev/null || true
        wait "$client_pid" 2>/dev/null || true
    fi
    stop_ioc
    rm -f "$log_file"
    rm -rf "$test_dir"
}
trap cleanup EXIT HUP INT TERM
start_ioc "$1"
if ! "./bin/$1/numericAckCaTest" || ! "./bin/$1/numericNetworkTest" ||
   ! "./bin/$1/numericRestartTest" before; then
    cat "$log_file" >&2
    exit 1
fi
stop_ioc
start_ioc "$1"
if ! "./bin/$1/numericRestartTest" after; then
    cat "$log_file" >&2
    exit 1
fi
# Arm the test-only delay, then wait until the IOC enters it before killing it.
touch "$NUMERIC_TEST_CRASH_MARKER.arm"
"./bin/$1/numericCrashTest" inflight >"$test_dir/client.log" 2>&1 &
client_pid=$!
attempt=0
while [ ! -f "$NUMERIC_TEST_CRASH_MARKER" ] && [ "$attempt" -lt 100 ]; do
    if ! kill -0 "$client_pid" 2>/dev/null || ! kill -0 "$ioc_pid" 2>/dev/null; then
        break
    fi
    sleep 0.1
    attempt=$((attempt + 1))
done
if [ ! -f "$NUMERIC_TEST_CRASH_MARKER" ]; then
    cat "$test_dir/client.log" "$log_file" >&2
    echo "IOC did not enter the controlled delay" >&2
    exit 1
fi
kill -KILL "$ioc_pid"
wait "$ioc_pid" 2>/dev/null || true
ioc_pid=
if ! wait "$client_pid"; then
    client_pid=
    cat "$test_dir/client.log" "$log_file" >&2
    exit 1
fi
client_pid=
cat "$test_dir/client.log"
rm -f "$NUMERIC_TEST_CRASH_MARKER.arm"
start_ioc "$1"
if ! "./bin/$1/numericCrashTest" recovery; then
    cat "$log_file" >&2
    exit 1
fi
