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
test_dir=$(mktemp -d "${TMPDIR:-/tmp}/numeric-network-interruption.XXXXXX")
# Native Windows IOCs read this environment path without MSYS translation.
if command -v cygpath >/dev/null 2>&1; then test_dir=$(cygpath -m "$test_dir"); fi
export NUMERIC_TEST_PARTITION_MARKER="$test_dir/entered"
ioc_pid=
relay_pid=
client_pid=
cleanup() {
    for pid in "$client_pid" "$relay_pid" "$ioc_pid"; do
        if [ -n "$pid" ]; then kill "$pid" 2>/dev/null || true; wait "$pid" 2>/dev/null || true; fi
    done
    rm -rf "$test_dir"
}
trap cleanup EXIT HUP INT TERM
fail() {
    for log in "$test_dir"/*.log; do [ ! -f "$log" ] || cat "$log" >&2; done
    exit 1
}
start_relay() {
    "./bin/$1/numericCaRelay" >"$test_dir/relay.log" 2>&1 &
    relay_pid=$!
    count=0
    while ! grep -q '^ready[[:space:]]*$' "$test_dir/relay.log" && [ "$count" -lt 100 ]; do
        kill -0 "$relay_pid" 2>/dev/null || fail
        sleep 0.1
        count=$((count + 1))
    done
    grep -q '^ready[[:space:]]*$' "$test_dir/relay.log" || fail
}
"./bin/$1/numericNetworkIoc" >"$test_dir/ioc.log" 2>&1 &
ioc_pid=$!
start_relay "$1"
EPICS_CA_ADDR_LIST=127.0.0.1:25066 "./bin/$1/numericPartitionTest" first >"$test_dir/client.log" 2>&1 &
client_pid=$!
count=0
while [ ! -f "$NUMERIC_TEST_PARTITION_MARKER" ] && [ "$count" -lt 100 ]; do
    kill -0 "$client_pid" 2>/dev/null && kill -0 "$ioc_pid" 2>/dev/null && kill -0 "$relay_pid" 2>/dev/null || fail
    sleep 0.1
    count=$((count + 1))
done
[ -f "$NUMERIC_TEST_PARTITION_MARKER" ] || fail
# Cut only the client's relay; the IOC and its processing continue.
kill "$relay_pid"
wait "$relay_pid" 2>/dev/null || true
relay_pid=
if ! wait "$client_pid"; then client_pid=; fail; fi
client_pid=
cat "$test_dir/client.log"
kill -0 "$ioc_pid" 2>/dev/null || fail
"./bin/$1/numericPartitionTest" observe || fail
EPICS_CA_ADDR_LIST=127.0.0.1:25066 "./bin/$1/numericPartitionTest" blocked || fail
start_relay "$1"
EPICS_CA_ADDR_LIST=127.0.0.1:25066 "./bin/$1/numericPartitionTest" retry || fail
