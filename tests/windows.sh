#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-or-later
# Run from the repository root in MSYS2 MinGW64 after building numeric.
set -eu
arch=${1:-windows-x64-mingw}
case "$arch" in
    windows-x64-mingw) ;;
    *) echo "Usage: $0 [windows-x64-mingw]" >&2; exit 2 ;;
esac
: "${EPICS_BASE:?Set EPICS_BASE to the matching MinGW EPICS Base installation}"
: "${GSL_DIR:?Set GSL_DIR to the matching MinGW GSL installation}"
export PATH="$(pwd)/bin/$arch:$EPICS_BASE/bin/$arch:$GSL_DIR/bin:$PATH"
sh tests/gsl-version-check.sh "$GSL_DIR"
for test in numericTest numericFftTest numericWindowTest numericFitTest numericIntegrationTest numericRootsTest numericHistogramTest; do
    echo "Running $test"
    "./bin/$arch/$test"
done
for test in numericFftIocTest numericWindowIocTest numericArrayIocTest numericInterpolationIocTest numericFitIocTest numericIntegrationIocTest numericRootsIocTest numericPolynomialIocTest numericScalarIocTest numericIocTest; do
    echo "Running $test (initialized and uninitialized)"
    "./bin/$arch/$test"
    NUMERIC_TEST_NO_INIT=1 "./bin/$arch/$test"
done
"./bin/$arch/numericBenchmark" 64 100 windows-validation
"./bin/$arch/numericIocBenchmark" 64 100 windows-validation
if "./bin/$arch/numericBenchmark" 0 100 windows-validation; then
    echo 'Invalid benchmark size was accepted' >&2
    exit 1
fi
sh tests/example-ioc.sh "$arch"
sh tests/network-ack.sh "$arch"
sh tests/network-interruption.sh "$arch"
