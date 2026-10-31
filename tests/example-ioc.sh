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
log_file=$(mktemp "${TMPDIR:-/tmp}/numeric-example-ioc.XXXXXX")
"./bin/$1/numericExample" examples/ioc/st.cmd --headless >"$log_file" 2>&1 &
ioc_pid=$!
cleanup() {
    kill "$ioc_pid" 2>/dev/null || true
    wait "$ioc_pid" 2>/dev/null || true
    rm -f "$log_file"
}
trap cleanup EXIT HUP INT TERM
if ! "./bin/$1/numericExampleTest"; then
    cat "$log_file" >&2
    exit 1
fi
if ! "./bin/$1/numericPhase3ExampleTest"; then
    cat "$log_file" >&2
    exit 1
fi
