# SPDX-License-Identifier: GPL-3.0-or-later
dbLoadDatabase("dbd/numericExample.dbd")
numericExample_registerRecordDeviceDriver(pdbbase)
dbLoadRecords("tests/multi-reconnect-producer.db")
iocInit
