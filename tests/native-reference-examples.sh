#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-or-later
# Build documentation examples against the already-built numeric library.
set -eu
arch=${1:?pass EPICS_HOST_ARCH}
test_dir=$(mktemp -d "${TMPDIR:-/tmp}/numeric-doc-examples.XXXXXX")
trap 'rm -rf "$test_dir"' EXIT HUP INT TERM
export LD_LIBRARY_PATH="$PWD/lib/$arch${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
for source in native-core-reference native-api-reference; do
    "${CXX:-c++}" -std=c++14 -I numericApp/src "examples/$source.cpp" \
        -L "lib/$arch" -lnumeric -o "$test_dir/$source"
    "$test_dir/$source"
done
