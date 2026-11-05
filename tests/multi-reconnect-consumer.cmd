# SPDX-License-Identifier: GPL-3.0-or-later
dbLoadDatabase("dbd/numericExample.dbd")
numericExample_registerRecordDeviceDriver(pdbbase)
numericInitialize
dbLoadRecords("tests/multi-reconnect-consumer.db")
iocInit
