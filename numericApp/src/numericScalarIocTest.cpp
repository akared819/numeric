// SPDX-License-Identifier: GPL-3.0-or-later
#include <dbAccess.h>
#include <dbUnitTest.h>
#include <dbEvent.h>
#include <dbScan.h>
#include <iocsh.h>
#include <testMain.h>
#include <cstdlib>
#include <cmath>
#include <limits>
extern "C" int numericIocTest_registerRecordDeviceDriver(struct dbBase*);
static bool valueNear(const char* name, double expected) {
    DBADDR addr; double actual = 0; long count = 1;
    return !dbNameToAddr(name, &addr) && !dbGetField(&addr, DBR_DOUBLE, &actual, nullptr, &count, nullptr) &&
           count == 1 && std::fabs(actual - expected) < 1e-12;
}
MAIN(numericScalarIocTest) {
    const bool noInit = std::getenv("NUMERIC_TEST_NO_INIT") != nullptr;
    testPlan(0); testdbPrepare();
    testdbReadDatabase("dbd/numericIocTest.dbd", nullptr, nullptr);
    numericIocTest_registerRecordDeviceDriver(pdbbase);
    testdbReadDatabase("examples/scalar.db", nullptr, "P=");
    testdbReadDatabase("numericApp/src/numericScalarIocTest.db", nullptr, nullptr);
    if (!noInit) iocshCmd("numericInitialize");
    testIocInitOk();
    testMonitor* valueMonitor = testMonitorCreate("scalarMean.VAL", DBE_VALUE | DBE_ALARM, 0);
    testMonitorSync(valueMonitor); testMonitorCount(valueMonitor, 1);
    double values[] = {1,2,3,4};
    testdbPutArrFieldOk("scalarInput", DBF_DOUBLE, 4, values);
    testdbPutFieldOk("scalarMean.PROC", DBF_LONG, 1);
    if (noInit) {
        testdbGetFieldEqual("scalarMean.SEVR", DBF_STRING, "INVALID");
        testdbGetFieldEqual("scalarMean.NORD", DBF_ULONG, 0u);
        testMonitorDestroy(valueMonitor);
        testIocShutdownOk(); testdbCleanup(); return testDone();
    }
    testdbGetFieldEqual("scalarMean.VAL", DBF_DOUBLE, 2.5);
    testdbGetFieldEqual("scalarMean.NORD", DBF_ULONG, 4u);
    testdbGetFieldEqual("scalarMean.SEVR", DBF_STRING, "NO_ALARM");
    DBADDR readOnly; double replacement = 99;
    testOk(!dbNameToAddr("scalarMean.VAL", &readOnly) &&
           dbPutField(&readOnly, DBR_DOUBLE, &replacement, 1) != 0,
           "result field rejects client writes");
    testdbGetFieldEqual("scalarMean.VAL", DBF_DOUBLE, 2.5);
    testdbGetFieldEqual("scalarMean.UDF", DBF_UCHAR, 0u);
    dbCommon* scalarRecord = testdbRecordPtr("scalarMean");
    dbScanLock(scalarRecord);
    const bool hasTimestamp = scalarRecord->time.secPastEpoch != 0;
    dbScanUnlock(scalarRecord);
    testOk(hasTimestamp, "successful processing sets a timestamp");
    testMonitorSync(valueMonitor);
    testOk(testMonitorCount(valueMonitor, 1) > 0, "successful calculation posts a value monitor");
    testdbPutFieldOk("scalarVariance.PROC", DBF_LONG, 1);
    testdbGetFieldEqual("scalarVariance.NORD", DBF_ULONG, 4u);
    testOk(valueNear("scalarVariance.VAL", 5.0/3.0), "variance uses sample normalization");
    testdbPutArrFieldOk("scalarInput", DBF_DOUBLE, 1, values);
    testdbPutFieldOk("scalarVariance.PROC", DBF_LONG, 1);
    testdbGetFieldEqual("scalarVariance.NORD", DBF_ULONG, 0u);
    testdbGetFieldEqual("scalarVariance.SEVR", DBF_STRING, "INVALID");
    testOk(valueNear("scalarVariance.VAL", 5.0/3.0), "invalid input preserves previous variance");
    double invalid[] = {std::numeric_limits<double>::quiet_NaN(), 1};
    testdbPutArrFieldOk("scalarInput", DBF_DOUBLE, 2, invalid);
    testdbPutFieldOk("scalarMean.PROC", DBF_LONG, 1);
    testdbGetFieldEqual("scalarMean.SEVR", DBF_STRING, "INVALID");
    testdbGetFieldEqual("scalarMean.NORD", DBF_ULONG, 0u);
    testdbGetFieldEqual("scalarMean.VAL", DBF_DOUBLE, 2.5);
    testMonitorSync(valueMonitor);
    testOk(testMonitorCount(valueMonitor, 1) > 0, "invalid calculation posts an alarm monitor");
    testdbPutArrFieldOk("scalarInput", DBF_DOUBLE, 0, values);
    testdbPutFieldOk("scalarMean.PROC", DBF_LONG, 1);
    testdbGetFieldEqual("scalarMean.NORD", DBF_ULONG, 0u);
    testdbGetFieldEqual("scalarMean.SEVR", DBF_STRING, "INVALID");
    testdbGetFieldEqual("scalarMean.VAL", DBF_DOUBLE, 2.5);
    testdbPutArrFieldOk("scalarInput", DBF_DOUBLE, 4, values);
    testdbPutFieldOk("scalarMean.PROC", DBF_LONG, 1);
    testdbGetFieldEqual("scalarMean.SEVR", DBF_STRING, "NO_ALARM");
    testdbGetFieldEqual("scalarMean.VAL", DBF_DOUBLE, 2.5);
    double tooMany[] = {1,2,3,4,5};
    testdbPutArrFieldOk("scalarOversizedInput", DBF_DOUBLE, 5, tooMany);
    testdbPutFieldOk("scalarLimited.PROC", DBF_LONG, 1);
    testdbGetFieldEqual("scalarLimited.NORD", DBF_ULONG, 0u);
    testdbGetFieldEqual("scalarLimited.SEVR", DBF_STRING, "INVALID");
    testdbPutFieldOk("scalarForward.PROC", DBF_LONG, 1);
    testdbGetFieldEqual("scalarProcessed.VAL", DBF_DOUBLE, 1.0);
    testdbPutFieldOk("scalarWrongType.PROC", DBF_LONG, 1);
    testdbGetFieldEqual("scalarWrongType.SEVR", DBF_STRING, "INVALID");
    testdbPutFieldOk("scalarInvalidOp.PROC", DBF_LONG, 1);
    testdbGetFieldEqual("scalarInvalidOp.SEVR", DBF_STRING, "INVALID");
    testdbPutFieldOk("scalarInvalidMax.PROC", DBF_LONG, 1);
    testdbGetFieldEqual("scalarInvalidMax.SEVR", DBF_STRING, "INVALID");
    testMonitorDestroy(valueMonitor);
    testIocShutdownOk(); testdbCleanup(); return testDone();
}
