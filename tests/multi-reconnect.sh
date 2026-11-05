#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-or-later
set -eu
if [ "$#" -ne 1 ]; then
    echo "Usage: $0 EPICS_HOST_ARCH" >&2
    exit 2
fi
export EPICS_CA_AUTO_ADDR_LIST=NO
export EPICS_CA_ADDR_LIST='127.0.0.1:25140 127.0.0.1:25141'
export EPICS_CAS_INTF_ADDR_LIST=127.0.0.1
export EPICS_CA_REPEATER_PORT=25142
export EPICS_CA_SERVER_PORT=25141
log_producer=$(mktemp "${TMPDIR:-/tmp}/numeric-multi-producer.XXXXXX")
log_consumer=$(mktemp "${TMPDIR:-/tmp}/numeric-multi-consumer.XXXXXX")
producer_pid=
consumer_pid=
cleanup() {
    if [ -n "$producer_pid" ]; then kill "$producer_pid" 2>/dev/null || true; wait "$producer_pid" 2>/dev/null || true; fi
    if [ -n "$consumer_pid" ]; then kill "$consumer_pid" 2>/dev/null || true; wait "$consumer_pid" 2>/dev/null || true; fi
    rm -f "$log_producer" "$log_consumer"
}
trap cleanup EXIT HUP INT TERM
start_producer() {
    EPICS_CAS_SERVER_PORT=25140 "./bin/$1/numericExample" tests/multi-reconnect-producer.cmd --headless >"$log_producer" 2>&1 &
    producer_pid=$!
}
start_producer "$1"
EPICS_CAS_SERVER_PORT=25141 "./bin/$1/numericExample" tests/multi-reconnect-consumer.cmd --headless >"$log_consumer" 2>&1 &
consumer_pid=$!
if ! "./bin/$1/numericMultiReconnectTest" initial; then cat "$log_producer" "$log_consumer" >&2; exit 1; fi
kill "$producer_pid"
wait "$producer_pid" 2>/dev/null || true
producer_pid=
if ! "./bin/$1/numericMultiReconnectTest" down; then cat "$log_producer" "$log_consumer" >&2; exit 1; fi
start_producer "$1"
if ! "./bin/$1/numericMultiReconnectTest" recover; then cat "$log_producer" "$log_consumer" >&2; exit 1; fi
