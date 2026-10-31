# SPDX-License-Identifier: GPL-3.0-or-later
# Run from the numeric repository root after building.
dbLoadDatabase("dbd/numericExample.dbd")
numericExample_registerRecordDeviceDriver(pdbbase)
numericInitialize
dbLoadRecords("examples/mean.db", "P=demo:")
dbLoadRecords("examples/running-stats.db", "P=demo:")
dbLoadRecords("examples/acknowledged-stats.db", "P=demo:")
dbLoadRecords("examples/covariance-pairs.db", "P=demo:")
dbLoadRecords("examples/fft.db", "P=demo:")
dbLoadRecords("examples/window.db", "P=demo:")
dbLoadRecords("examples/scalar.db", "P=demo:")
dbLoadRecords("examples/polynomial.db", "P=demo:")
dbLoadRecords("examples/interpolation.db", "P=demo:")
dbLoadRecords("examples/fitting.db", "P=demo:")
dbLoadRecords("examples/array.db", "P=demo:")
iocInit
