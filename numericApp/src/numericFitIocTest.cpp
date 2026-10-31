// SPDX-License-Identifier: GPL-3.0-or-later
#include <dbAccess.h>
#include <dbUnitTest.h>
#include <iocsh.h>
#include <testMain.h>
#include <cmath>
#include <cstdlib>
extern "C" int numericIocTest_registerRecordDeviceDriver(struct dbBase*);
bool checkFit() {
    DBADDR addr; double actual[6]; long count=6;
    if (dbNameToAddr("fitResult", &addr) || dbGetField(&addr, DBR_DOUBLE, actual, nullptr, &count, nullptr) || count != 6) return false;
    const double expected[] = {.9,.9,.245,-.105,.07,.7};
    for (int i=0;i<6;++i) if (std::fabs(actual[i]-expected[i]) > 1e-12) return false;
    return true;
}
MAIN(numericFitIocTest) {
    const bool noInit=std::getenv("NUMERIC_TEST_NO_INIT") != nullptr;
    testPlan(0); testdbPrepare(); testdbReadDatabase("dbd/numericIocTest.dbd", nullptr, nullptr);
    numericIocTest_registerRecordDeviceDriver(pdbbase);
    testdbReadDatabase("examples/fitting.db", nullptr, "P=");
    testdbReadDatabase("numericApp/src/numericFitIocTest.db", nullptr, nullptr);
    if (!noInit) iocshCmd("numericInitialize"); testIocInitOk();
    double x[] = {0,1,2,3}, y[] = {1,2,2,4};
    testdbPutArrFieldOk("fitX", DBF_DOUBLE, 4, x); testdbPutArrFieldOk("fitY", DBF_DOUBLE, 4, y);
    testdbPutFieldOk("fitLinear.PROC", DBF_LONG, 1);
    if (noInit) {
        testdbGetFieldEqual("fitLinear.VAL", DBF_LONG, -1);
        testdbGetFieldEqual("fitLinear.SEVR", DBF_STRING, "INVALID");
        testIocShutdownOk(); testdbCleanup(); return testDone();
    }
    testdbGetFieldEqual("fitLinear.VAL", DBF_LONG, 0); testOk(checkFit(), "fit results match expected");
    testdbPutFieldOk("fitSmall.PROC", DBF_LONG, 1); testdbGetFieldEqual("fitSmall.VAL", DBF_LONG, -1);
    testdbPutFieldOk("fitShortOutput.PROC", DBF_LONG, 1); testdbGetFieldEqual("fitShortOutput.VAL", DBF_LONG, -1);
    testdbPutFieldOk("fitBadType.PROC", DBF_LONG, 1); testdbGetFieldEqual("fitBadType.VAL", DBF_LONG, -1);
    testdbPutArrFieldOk("fitY", DBF_DOUBLE, 3, y); testdbPutFieldOk("fitLinear.PROC", DBF_LONG, 1);
    testdbGetFieldEqual("fitLinear.VAL", DBF_LONG, -1); testdbGetFieldEqual("fitLinear.NEVA", DBF_ULONG, 0u);
    testdbGetFieldEqual("fitLinear.SEVR", DBF_STRING, "INVALID"); testOk(checkFit(), "previous fit preserved");
    testdbPutArrFieldOk("fitY", DBF_DOUBLE, 4, y); testdbPutFieldOk("fitLinear.PROC", DBF_LONG, 1);
    testdbGetFieldEqual("fitLinear.SEVR", DBF_STRING, "NO_ALARM"); testOk(checkFit(), "fit recovered");
    testIocShutdownOk(); testdbCleanup(); return testDone();
}
