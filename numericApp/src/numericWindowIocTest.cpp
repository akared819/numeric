// SPDX-License-Identifier: GPL-3.0-or-later
#include <dbAccess.h>
#include <dbUnitTest.h>
#include <iocsh.h>
#include <testMain.h>
#include <cmath>
#include <cstdlib>
#include <limits>
extern "C" int numericIocTest_registerRecordDeviceDriver(struct dbBase*);
static bool matches(const char* name, const double* expected) {
    DBADDR addr; double values[5] = {}; long count = 5;
    if (dbNameToAddr(name, &addr) || dbGetField(&addr, DBR_DOUBLE, values, nullptr, &count, nullptr) || count != 5) return false;
    for (int i=0; i<5; ++i) if (!std::isfinite(values[i]) || std::fabs(values[i]-expected[i]) > 1e-12) return false;
    return true;
}
MAIN(numericWindowIocTest) {
    const bool noInit = std::getenv("NUMERIC_TEST_NO_INIT") != nullptr;
    testPlan(0); testdbPrepare();
    testdbReadDatabase("dbd/numericIocTest.dbd", nullptr, nullptr);
    numericIocTest_registerRecordDeviceDriver(pdbbase);
    testdbReadDatabase("examples/window.db", nullptr, "P=");
    testdbReadDatabase("numericApp/src/numericWindowIocTest.db", nullptr, nullptr);
    if (!noInit) iocshCmd("numericInitialize");
    testIocInitOk();
    double values[] = {1,2,3,4,5}, expected[] = {4.0/3,2,3,4,14.0/3};
    testdbPutArrFieldOk("windowInput", DBF_DOUBLE, 5, values);
    testdbPutFieldOk("movingMean.PROC", DBF_LONG, 1);
    if (noInit) {
        testdbGetFieldEqual("movingMean.VAL", DBF_LONG, -1);
        testdbGetFieldEqual("movingMean.NEVA", DBF_ULONG, 0u);
        testdbGetFieldEqual("movingMean.SEVR", DBF_STRING, "INVALID");
        testdbPutFieldOk("movingMedian.PROC", DBF_LONG, 1);
        testdbGetFieldEqual("movingMedian.VAL", DBF_LONG, -1);
        testdbGetFieldEqual("movingMedian.SEVR", DBF_STRING, "INVALID");
        testIocShutdownOk(); testdbCleanup(); return testDone();
    }
    testdbGetFieldEqual("movingMean.VAL", DBF_LONG, 0);
    testdbGetFieldEqual("movingMean.NEVA", DBF_ULONG, 5u);
    testOk(matches("movingMeanOutput", expected), "moving mean and edge padding");
    double medianInput[] = {9,1,3,2,5}, medianExpected[] = {9,3,2,3,5};
    testdbPutArrFieldOk("windowInput", DBF_DOUBLE, 5, medianInput);
    testdbPutFieldOk("movingMedian.PROC", DBF_LONG, 1);
    testdbGetFieldEqual("movingMedian.VAL", DBF_LONG, 0);
    testdbGetFieldEqual("movingMedian.NEVA", DBF_ULONG, 5u);
    testOk(matches("movingMedianOutput", medianExpected), "median uses centered value-padded windows");
    testdbPutArrFieldOk("windowInput", DBF_DOUBLE, 5, values);
    const double wider[] = {1.6,2.2,3,3.8,4.4};
    testdbPutFieldOk("windowLength", DBF_DOUBLE, 5.0);
    testdbPutFieldOk("movingMean.PROC", DBF_LONG, 1);
    testOk(matches("movingMeanOutput", wider), "new window size replaces workspace");
    testdbPutFieldOk("windowLength", DBF_DOUBLE, 3.0);
    testdbPutFieldOk("movingMean.PROC", DBF_LONG, 1);
    testOk(matches("movingMeanOutput", expected), "original window size works again");
    testdbPutFieldOk("smallWindowOutput.PROC", DBF_LONG, 1);
    testdbGetFieldEqual("smallWindowOutput.VAL", DBF_LONG, -1);
    testdbPutFieldOk("smallMedianOutput.PROC", DBF_LONG, 1);
    testdbGetFieldEqual("smallMedianOutput.VAL", DBF_LONG, -1);
    testdbGetFieldEqual("smallMedianOutput.SEVR", DBF_STRING, "INVALID");
    testdbPutFieldOk("smallWindowLink.PROC", DBF_LONG, 1);
    testdbGetFieldEqual("smallWindowLink.VAL", DBF_LONG, -1);
    testdbPutFieldOk("smallWindowInput.PROC", DBF_LONG, 1);
    testdbGetFieldEqual("smallWindowInput.VAL", DBF_LONG, -1);
    testdbPutFieldOk("windowLength", DBF_DOUBLE, 4.0);
    testdbPutFieldOk("movingMean.PROC", DBF_LONG, 1);
    testdbGetFieldEqual("movingMean.VAL", DBF_LONG, -1);
    testdbGetFieldEqual("movingMean.NEVA", DBF_ULONG, 0u);
    testdbGetFieldEqual("movingMean.SEVR", DBF_STRING, "INVALID");
    testOk(matches("movingMeanOutput", expected), "invalid window preserves output");
    testdbPutFieldOk("movingMedian.PROC", DBF_LONG, 1);
    testdbGetFieldEqual("movingMedian.VAL", DBF_LONG, -1);
    testdbGetFieldEqual("movingMedian.NEVA", DBF_ULONG, 0u);
    testdbGetFieldEqual("movingMedian.SEVR", DBF_STRING, "INVALID");
    testOk(matches("movingMedianOutput", medianExpected), "invalid window preserves median output");
    testdbPutFieldOk("windowLength", DBF_DOUBLE, 3.0);
    double extreme[] = {std::numeric_limits<double>::max(), -std::numeric_limits<double>::max(),
                        std::numeric_limits<double>::max(), 0, 0};
    testdbPutArrFieldOk("windowInput", DBF_DOUBLE, 5, extreme);
    testdbPutFieldOk("movingMean.PROC", DBF_LONG, 1);
    testdbGetFieldEqual("movingMean.VAL", DBF_LONG, -1);
    testOk(matches("movingMeanOutput", expected), "numerical failure preserves output");
    double pulse[] = {0,0,1,0,0}, smooth[] = {0,.25,.5,.25,0};
    testdbPutArrFieldOk("windowInput", DBF_DOUBLE, 5, pulse);
    testdbPutFieldOk("gaussianSmooth.PROC", DBF_LONG, 1);
    testdbGetFieldEqual("gaussianSmooth.VAL", DBF_LONG, 0);
    testOk(matches("gaussianOutput", smooth), "Gaussian impulse response");
    testdbPutFieldOk("gaussianAlpha", DBF_DOUBLE, 0.0);
    testdbPutFieldOk("gaussianSmooth.PROC", DBF_LONG, 1);
    testdbGetFieldEqual("gaussianSmooth.VAL", DBF_LONG, -1);
    testOk(matches("gaussianOutput", smooth), "invalid alpha preserves output");
    pulse[2] = std::numeric_limits<double>::quiet_NaN();
    testdbPutFieldOk("gaussianAlpha", DBF_DOUBLE, 1.0);
    testdbPutArrFieldOk("windowInput", DBF_DOUBLE, 5, pulse);
    testdbPutFieldOk("gaussianSmooth.PROC", DBF_LONG, 1);
    testdbGetFieldEqual("gaussianSmooth.VAL", DBF_LONG, -1);
    testOk(matches("gaussianOutput", smooth), "non-finite input preserves output");
    testdbPutFieldOk("movingMedian.PROC", DBF_LONG, 1);
    testdbGetFieldEqual("movingMedian.VAL", DBF_LONG, -1);
    testOk(matches("movingMedianOutput", medianExpected), "non-finite input preserves median output");
    testdbPutArrFieldOk("windowInput", DBF_DOUBLE, 5, values);
    testdbPutFieldOk("movingMean.PROC", DBF_LONG, 1);
    testdbGetFieldEqual("movingMean.VAL", DBF_LONG, 0);
    testdbGetFieldEqual("movingMean.SEVR", DBF_STRING, "NO_ALARM");
    testOk(matches("movingMeanOutput", expected), "valid data recovers");
    testdbPutFieldOk("movingMedian.PROC", DBF_LONG, 1);
    testdbGetFieldEqual("movingMedian.VAL", DBF_LONG, 0);
    testdbGetFieldEqual("movingMedian.SEVR", DBF_STRING, "NO_ALARM");
    testOk(matches("movingMedianOutput", values), "median recovers with ordered input");
    testIocShutdownOk(); testdbCleanup(); return testDone();
}
