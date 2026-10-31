#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-or-later
# Verify the compile-time minimum without needing an old GSL installation.
set -eu
if [ "$#" -ne 1 ]; then
    echo "Usage: $0 GSL_PREFIX" >&2
    exit 2
fi
test_dir=$(mktemp -d "${TMPDIR:-/tmp}/numeric-gsl-version.XXXXXX")
trap 'rm -rf "$test_dir"' EXIT HUP INT TERM
mkdir "$test_dir/gsl"
printf '#define GSL_MAJOR_VERSION 2\n#define GSL_MINOR_VERSION 7\n' > "$test_dir/gsl/gsl_version.h"
if ${CXX:-c++} -std=c++14 -I"$test_dir" -I"$1/include" -InumericApp/src -c numericApp/src/numeric.cpp -o "$test_dir/check.o" >"$test_dir/build.log" 2>&1; then
    echo 'GSL 2.7 headers were unexpectedly accepted' >&2
    exit 1
fi
if ! grep -Fq 'numeric requires GSL 2.8 or newer' "$test_dir/build.log"; then
    cat "$test_dir/build.log" >&2
    exit 1
fi
printf 'GSL 2.7 headers rejected as expected\n'
