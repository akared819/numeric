// SPDX-License-Identifier: GPL-3.0-or-later
#include <dbAccess.h>
#include <dbUnitTest.h>
#include <dbEvent.h>
#include <dbScan.h>
#include <iocsh.h>
#include <testMain.h>
#include <cmath>
#include <cstdlib>
#include <limits>
extern "C" int numericIocTest_registerRecordDeviceDriver(struct dbBase*);
static bool matches(const char* name, const double* expected, long length) {
    DBADDR addr; double actual[16] = {}; long count = 16;
    if (dbNameToAddr(name, &addr) || dbGetField(&addr, DBR_DOUBLE, actual, nullptr, &count, nullptr) || count != length) return false;
    for (long i = 0; i < count; ++i) if (!std::isfinite(actual[i]) || std::fabs(actual[i] - expected[i]) > 1e-12) return false;
    return true;
}
MAIN(numericArrayIocTest) {
    const bool noInit = std::getenv("NUMERIC_TEST_NO_INIT") != nullptr;
    testPlan(0); testdbPrepare();
    testdbReadDatabase("dbd/numericIocTest.dbd", nullptr, nullptr);
    numericIocTest_registerRecordDeviceDriver(pdbbase);
    testdbReadDatabase("examples/array.db", nullptr, "P=");
    testdbReadDatabase("numericApp/src/numericArrayIocTest.db", nullptr, nullptr);
    if (!noInit) iocshCmd("numericInitialize");
    testIocInitOk();
    testMonitor* monitor = testMonitorCreate("arrayMean.VAL", DBE_VALUE | DBE_ALARM, 0);
    testMonitorSync(monitor); testMonitorCount(monitor, 1);
    double data[] = {1,2,3,4,5}, expected[] = {4.0/3,2,3,4,14.0/3};
    testdbPutArrFieldOk("arrayInput", DBF_DOUBLE, 5, data);
    testdbPutFieldOk("arrayMean.PROC", DBF_LONG, 1);
    if (noInit) {
        testdbGetFieldEqual("arrayMean.SEVR", DBF_STRING, "INVALID");
        testdbGetFieldEqual("arrayMean.NORD", DBF_ULONG, 0u);
        testMonitorDestroy(monitor); testIocShutdownOk(); testdbCleanup(); return testDone();
    }
    testdbGetFieldEqual("arrayMean.NORD", DBF_ULONG, 5u);
    testdbGetFieldEqual("arrayMean.NIN", DBF_ULONG, 5u);
    testdbGetFieldEqual("arrayMean.SEVR", DBF_STRING, "NO_ALARM");
    testOk(matches("arrayMean.VAL", expected, 5), "moving mean is published as a DOUBLE array");
    DBADDR addr; double overwrite[] = {9,9,9,9,9};
    testOk(!dbNameToAddr("arrayMean.VAL", &addr) && dbPutField(&addr, DBR_DOUBLE, overwrite, 5) != 0,
           "array result rejects client writes");
    testOk(matches("arrayMean.VAL", expected, 5), "rejected write preserves output");
    testMonitorSync(monitor); testOk(testMonitorCount(monitor, 1) > 0, "successful calculation posts a value monitor");
    dbCommon* record = testdbRecordPtr("arrayMean"); dbScanLock(record);
    const bool timestamp = record->time.secPastEpoch != 0; dbScanUnlock(record);
    testOk(timestamp, "successful processing sets a timestamp");
    testdbPutFieldOk("arrayMean.WIN", DBF_DOUBLE, 4.0);
    testdbPutFieldOk("arrayMean.PROC", DBF_LONG, 1);
    testdbGetFieldEqual("arrayMean.NIN", DBF_ULONG, 0u);
    testdbGetFieldEqual("arrayMean.NORD", DBF_ULONG, 5u);
    testdbGetFieldEqual("arrayMean.SEVR", DBF_STRING, "INVALID");
    testOk(matches("arrayMean.VAL", expected, 5), "invalid window preserves previous output and count");
    testMonitorSync(monitor); testOk(testMonitorCount(monitor, 1) > 0, "failure posts an alarm monitor");
    testdbPutFieldOk("arrayMean.WIN", DBF_DOUBLE, 3.0);
    testdbPutFieldOk("arrayMean.PROC", DBF_LONG, 1);
    testdbGetFieldEqual("arrayMean.SEVR", DBF_STRING, "NO_ALARM");
    testOk(matches("arrayMean.VAL", expected, 5), "record recovers after invalid parameters");
    double spikes[] = {1,1,99,1,1}, filtered[] = {1,1,1,1,1};
    testdbPutArrFieldOk("arrayInput", DBF_DOUBLE, 5, spikes);
    testdbPutFieldOk("arrayMedian.PROC", DBF_LONG, 1);
    testOk(matches("arrayMedian.VAL", filtered, 5), "moving median rejects an isolated spike");
    double pulse[] = {0,0,1,0,0}, smooth[] = {0,0.25,0.5,0.25,0};
    testdbPutArrFieldOk("arrayInput", DBF_DOUBLE, 5, pulse);
    testdbPutFieldOk("arrayGaussian.PROC", DBF_LONG, 1);
    testOk(matches("arrayGaussian.VAL", smooth, 5), "Gaussian smoothing uses configured alpha");
    testdbPutFieldOk("arrayGaussian.ALPH", DBF_DOUBLE, 0.0);
    testdbPutFieldOk("arrayGaussian.PROC", DBF_LONG, 1);
    testdbGetFieldEqual("arrayGaussian.SEVR", DBF_STRING, "INVALID");
    testOk(matches("arrayGaussian.VAL", smooth, 5), "invalid alpha preserves Gaussian output");
    testdbPutArrFieldOk("arrayInput", DBF_DOUBLE, 5, data);
    testdbPutFieldOk("arraySmallInput.PROC", DBF_LONG, 1);
    testdbGetFieldEqual("arraySmallInput.SEVR", DBF_STRING, "INVALID");
    testdbPutFieldOk("arraySmallOutput.PROC", DBF_LONG, 1);
    testdbGetFieldEqual("arraySmallOutput.SEVR", DBF_STRING, "INVALID");
    testdbPutFieldOk("arrayWrongType.PROC", DBF_LONG, 1);
    testdbGetFieldEqual("arrayWrongType.SEVR", DBF_STRING, "INVALID");
    testdbPutFieldOk("arrayInvalidOp.PROC", DBF_LONG, 1);
    testdbGetFieldEqual("arrayInvalidOp.SEVR", DBF_STRING, "INVALID");
    testdbPutFieldOk("arrayForward.PROC", DBF_LONG, 1);
    testdbGetFieldEqual("arrayProcessed.VAL", DBF_DOUBLE, 1.0);
    double invalid[] = {1,std::numeric_limits<double>::quiet_NaN(),3};
    testdbPutArrFieldOk("arrayInput", DBF_DOUBLE, 3, invalid);
    testdbPutFieldOk("arrayMean.PROC", DBF_LONG, 1);
    testdbGetFieldEqual("arrayMean.SEVR", DBF_STRING, "INVALID");
    testOk(matches("arrayMean.VAL", expected, 5), "non-finite input preserves array output");
    testdbPutArrFieldOk("arrayInput", DBF_DOUBLE, 0, data);
    testdbPutFieldOk("arrayMean.PROC", DBF_LONG, 1);
    testdbGetFieldEqual("arrayMean.SEVR", DBF_STRING, "INVALID");
    testOk(matches("arrayMean.VAL", expected, 5), "empty input preserves array output");
    double extreme[] = {std::numeric_limits<double>::max(), -std::numeric_limits<double>::max(),
                        std::numeric_limits<double>::max(), 0, 0};
    testdbPutArrFieldOk("arrayInput", DBF_DOUBLE, 5, extreme);
    testdbPutFieldOk("arrayMean.PROC", DBF_LONG, 1);
    testdbGetFieldEqual("arrayMean.SEVR", DBF_STRING, "INVALID");
    testOk(matches("arrayMean.VAL", expected, 5), "numerical failure preserves array output");
    double impulse[] = {1,0,0,0}, spectrum[] = {1,0,1,0,1,0,1,0}, inverse[] = {1,0,0,0,0,0,0,0};
    testdbPutArrFieldOk("arrayInput", DBF_DOUBLE, 4, impulse);
    testdbPutFieldOk("arrayRealFft.PROC", DBF_LONG, 1);
    testdbGetFieldEqual("arrayRealFft.NORD", DBF_ULONG, 8u);
    testOk(matches("arrayRealFft.VAL", spectrum, 8), "real FFT publishes full complex spectrum");
    testdbPutFieldOk("arrayInverse.PROC", DBF_LONG, 1);
    testdbGetFieldEqual("arrayInverse.NORD", DBF_ULONG, 8u);
    testOk(matches("arrayInverse.VAL", inverse, 8), "complex inverse reconstructs the input");
    double full[] = {1,0,0,0,0,0,0,0};
    double fullSpectrum[] = {1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0};
    testdbPutArrFieldOk("arrayInput", DBF_DOUBLE, 8, full);
    testdbPutFieldOk("arrayComplexFft.PROC", DBF_LONG, 1);
    testdbGetFieldEqual("arrayComplexFft.NORD", DBF_ULONG, 8u);
    testOk(matches("arrayComplexFft.VAL", spectrum, 8), "complex FFT publishes the expected spectrum");
    testdbPutFieldOk("arrayRealFft.PROC", DBF_LONG, 1);
    testdbGetFieldEqual("arrayRealFft.NORD", DBF_ULONG, 16u);
    testOk(matches("arrayRealFft.VAL", fullSpectrum, 16), "full-capacity input is accepted without truncation");
    testMonitorDestroy(monitor); testIocShutdownOk(); testdbCleanup(); return testDone();
}
