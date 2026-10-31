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
ioc_pid=
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
    stop_ioc
    rm -f "$log_file"
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
